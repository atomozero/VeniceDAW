/*
 * VeniceAudioInputNode.cpp - Cortex Media Kit Consumer Implementation
 *
 * Live audio input routing from Cortex to VeniceDAW tracks
 */

#include "VeniceAudioInputNode.h"
#include "SimpleHaikuEngine.h"
#include <media/MediaRoster.h>
#include <media/TimeSource.h>
#include <media/Buffer.h>
#include <support/Autolock.h>
#include <string.h>
#include <stdio.h>

namespace HaikuDAW {

// ====================================
// Constructor / Destructor
// ====================================

VeniceAudioInputNode::VeniceAudioInputNode(SimpleTrack* track, const char* name)
    : BMediaNode(name)
    , BBufferConsumer(B_MEDIA_RAW_AUDIO)
    , BMediaEventLooper()
    , fTrack(track)
    , fLock("VeniceInput Lock")
    , fConnected(false)
    , fMonitoring(true)
    , fBuffersReceived(0)
    , fDroppedBuffers(0)
    , fAverageLatency(0)
    , fLastBufferTime(0)
    , fConversionBuffer(nullptr)
    , fConversionBufferSize(0)
{
    printf("VeniceAudioInputNode: Created for track '%s'\n",
           track ? track->GetName() : "NULL");

    // Initialize input descriptor
    fInput.destination.port = ControlPort();
    fInput.destination.id = 0;
    fInput.node = Node();
    fInput.source = media_source::null;
    fInput.format.type = B_MEDIA_RAW_AUDIO;
    fInput.name[0] = '\0';
    strcpy(fInput.name, name);

    // Default format (will be negotiated with producer)
    fFormat.type = B_MEDIA_RAW_AUDIO;
    fFormat.u.raw_audio = media_raw_audio_format::wildcard;
    fFormat.u.raw_audio.frame_rate = 44100.0f;
    fFormat.u.raw_audio.channel_count = 2;
    fFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
    fFormat.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
    fFormat.u.raw_audio.buffer_size = 2048 * sizeof(float) * 2;  // Stereo
}

VeniceAudioInputNode::~VeniceAudioInputNode()
{
    printf("VeniceAudioInputNode: Destroying node\n");

    // Ensure we're stopped
    if (fConnected) {
        BMediaRoster* roster = BMediaRoster::Roster();
        if (roster && fInput.source != media_source::null) {
            media_output output;
            output.source = fInput.source;
            roster->Disconnect(output, fInput);
        }
    }

    FreeBuffers();
}

// ====================================
// BMediaNode Interface
// ====================================

BMediaAddOn* VeniceAudioInputNode::AddOn(int32* internal_id) const
{
    // We're not loaded from an add-on
    return nullptr;
}

void VeniceAudioInputNode::NodeRegistered()
{
    printf("VeniceAudioInputNode: Node registered with Media Roster\n");

    // Set run mode to real-time for low latency
    SetRunMode(B_RECORDING);

    // Start event loop
    Run();
}

void VeniceAudioInputNode::SetRunMode(run_mode mode)
{
    BMediaNode::SetRunMode(mode);
    printf("VeniceAudioInputNode: Run mode set to %d\n", mode);
}

void VeniceAudioInputNode::Preroll()
{
    // Nothing to preroll for live input
    BMediaEventLooper::Preroll();
}

void VeniceAudioInputNode::SetTimeSource(BTimeSource* time_source)
{
    BMediaNode::SetTimeSource(time_source);
}

// ====================================
// BBufferConsumer Interface
// ====================================

status_t VeniceAudioInputNode::AcceptFormat(const media_destination& dest,
                                             media_format* format)
{
    if (format->type != B_MEDIA_RAW_AUDIO) {
        printf("VeniceAudioInputNode: Rejecting non-audio format\n");
        return B_MEDIA_BAD_FORMAT;
    }

    // We accept float audio at various sample rates
    media_raw_audio_format& audio = format->u.raw_audio;

    // Prefer 44100 Hz but accept others
    if (audio.frame_rate == media_raw_audio_format::wildcard.frame_rate) {
        audio.frame_rate = 44100.0f;
    }

    // Prefer stereo but accept mono
    if (audio.channel_count == media_raw_audio_format::wildcard.channel_count) {
        audio.channel_count = 2;
    }

    // Only accept float format
    if (audio.format != media_raw_audio_format::B_AUDIO_FLOAT) {
        printf("VeniceAudioInputNode: Rejecting non-float audio format\n");
        return B_MEDIA_BAD_FORMAT;
    }

    audio.byte_order = B_MEDIA_HOST_ENDIAN;

    printf("VeniceAudioInputNode: Accepted format %.0f Hz, %d channels\n",
           audio.frame_rate, audio.channel_count);

    return B_OK;
}

status_t VeniceAudioInputNode::GetNextInput(int32* cookie,
                                             media_input* out_input)
{
    if (*cookie != 0) {
        // We only have one input
        return B_BAD_INDEX;
    }

    *out_input = fInput;
    *cookie = 1;

    return B_OK;
}

void VeniceAudioInputNode::DisposeInputCookie(int32 cookie)
{
    // Nothing to dispose
}

status_t VeniceAudioInputNode::Connected(const media_source& producer,
                                         const media_destination& where,
                                         const media_format& with_format,
                                         media_input* out_input)
{
    BAutolock lock(fLock);

    printf("VeniceAudioInputNode: Connected to producer\n");

    fInput.source = producer;
    fInput.format = with_format;
    fFormat = with_format;
    fConnected = true;

    *out_input = fInput;

    // Allocate conversion buffer if needed
    AllocateBuffers();

    return B_OK;
}

void VeniceAudioInputNode::Disconnected(const media_source& producer,
                                        const media_destination& where)
{
    BAutolock lock(fLock);

    printf("VeniceAudioInputNode: Disconnected from producer\n");

    fInput.source = media_source::null;
    fConnected = false;

    FreeBuffers();
}

status_t VeniceAudioInputNode::FormatChanged(const media_source& producer,
                                              const media_destination& consumer,
                                              int32 change_tag,
                                              const media_format& format)
{
    BAutolock lock(fLock);

    printf("VeniceAudioInputNode: Format changed\n");

    fFormat = format;
    fInput.format = format;

    // Reallocate buffers for new format
    FreeBuffers();
    AllocateBuffers();

    return B_OK;
}

// ====================================
// CRITICAL: Buffer Reception
// ====================================

void VeniceAudioInputNode::BufferReceived(BBuffer* buffer)
{
    if (!buffer) return;

    BAutolock lock(fLock);

    fBuffersReceived++;

    if (!fTrack || !fMonitoring) {
        // Track not available or monitoring disabled - drop buffer
        buffer->Recycle();
        fDroppedBuffers++;
        return;
    }

    // Extract audio data
    const void* data = buffer->Data();
    size_t size = buffer->SizeUsed();
    bigtime_t performance_time = buffer->Header()->start_time;

    // Calculate latency
    bigtime_t now = TimeSource()->Now();
    bigtime_t latency = now - performance_time;
    fAverageLatency = (fAverageLatency + latency) / 2;

    // Process the buffer
    status_t result = ProcessBuffer(data, size, performance_time);

    if (result != B_OK) {
        fDroppedBuffers++;
        printf("VeniceAudioInputNode: Dropped buffer (processing failed)\n");
    }

    // Recycle buffer back to producer
    buffer->Recycle();

    fLastBufferTime = now;
}

status_t VeniceAudioInputNode::GetLatencyFor(const media_destination& for_whom,
                                              bigtime_t* out_latency,
                                              media_node_id* out_timesource)
{
    *out_latency = fAverageLatency;
    *out_timesource = TimeSource()->ID();
    return B_OK;
}

// ====================================
// BMediaEventLooper Interface
// ====================================

void VeniceAudioInputNode::HandleEvent(const media_timed_event* event,
                                        bigtime_t lateness,
                                        bool realTimeEvent)
{
    // Handle timed events (if needed in future)
    switch (event->type) {
        case BTimedEventQueue::B_START:
            printf("VeniceAudioInputNode: Start event\n");
            break;

        case BTimedEventQueue::B_STOP:
            printf("VeniceAudioInputNode: Stop event\n");
            break;

        default:
            break;
    }
}

void VeniceAudioInputNode::NodeRegistered(BMediaNode* node)
{
    // Called when node is registered (BMediaEventLooper override)
}

// ====================================
// VeniceDAW Specific Methods
// ====================================

void VeniceAudioInputNode::SetMonitoring(bool enabled)
{
    BAutolock lock(fLock);
    fMonitoring = enabled;
    printf("VeniceAudioInputNode: Monitoring %s\n", enabled ? "enabled" : "disabled");
}

// ====================================
// Internal Helpers
// ====================================

void VeniceAudioInputNode::AllocateBuffers()
{
    // Allocate conversion buffer (if format conversion needed)
    size_t frames = 2048;  // Max expected frames
    size_t channels = fFormat.u.raw_audio.channel_count;

    fConversionBufferSize = frames * channels * sizeof(float);
    fConversionBuffer = new float[frames * channels];

    printf("VeniceAudioInputNode: Allocated conversion buffer (%zu bytes)\n",
           fConversionBufferSize);
}

void VeniceAudioInputNode::FreeBuffers()
{
    if (fConversionBuffer) {
        delete[] fConversionBuffer;
        fConversionBuffer = nullptr;
        fConversionBufferSize = 0;
    }
}

status_t VeniceAudioInputNode::ProcessBuffer(const void* data, size_t size,
                                              bigtime_t performance_time)
{
    if (!data || !fTrack) return B_ERROR;

    // Extract format info
    uint32 channels = fFormat.u.raw_audio.channel_count;
    float sampleRate = fFormat.u.raw_audio.frame_rate;
    uint32 frameCount = size / (channels * sizeof(float));

    // Route audio to track's live input buffer
    const float* audioData = (const float*)data;
    fTrack->ProcessLiveInput(audioData, frameCount, channels);

    // Log every 100 buffers
    if (fBuffersReceived % 100 == 0) {
        printf("VeniceAudioInputNode: Routed buffer #%u to track (%u frames, %u channels, %.0f Hz)\n",
               fBuffersReceived, frameCount, channels, sampleRate);
    }

    return B_OK;
}

} // namespace HaikuDAW

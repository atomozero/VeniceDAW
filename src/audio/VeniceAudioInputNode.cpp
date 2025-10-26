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

VeniceAudioInputNode::VeniceAudioInputNode(SimpleTrack** tracks, int32 trackCount, const char* name)
    : BMediaNode(name)
    , BBufferConsumer(B_MEDIA_RAW_AUDIO)
    , BMediaEventLooper()
    , fTracks(tracks)
    , fTrackCount(trackCount)
    , fLock("VeniceInput Lock")
    , fMonitoring(true)
    , fBuffersReceived(0)
    , fDroppedBuffers(0)
    , fAverageLatency(0)
    , fLastBufferTime(0)
    , fConversionBuffer(nullptr)
    , fConversionBufferSize(0)
{
    printf("VeniceAudioInputNode: Created with %d inputs\n", (int)trackCount);

    // Initialize all inputs (up to MAX_INPUTS)
    int32 numInputs = (trackCount < MAX_INPUTS) ? trackCount : MAX_INPUTS;

    for (int32 i = 0; i < numInputs; i++) {
        fInputs[i].destination.port = ControlPort();
        fInputs[i].destination.id = i;
        fInputs[i].node = Node();
        fInputs[i].source = media_source::null;
        fInputs[i].format.type = B_MEDIA_RAW_AUDIO;

        // Name each input after its track
        if (fTracks && fTracks[i]) {
            snprintf(fInputs[i].name, B_MEDIA_NAME_LENGTH, "%s", fTracks[i]->GetName());
        } else {
            snprintf(fInputs[i].name, B_MEDIA_NAME_LENGTH, "Input %d", (int)(i + 1));
        }

        fInputConnected[i] = false;
    }

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

    // Disconnect all connected inputs
    BMediaRoster* roster = BMediaRoster::Roster();
    for (int32 i = 0; i < MAX_INPUTS; i++) {
        if (fInputConnected[i] && roster && fInputs[i].source != media_source::null) {
            media_output output;
            output.source = fInputs[i].source;
            roster->Disconnect(output, fInputs[i]);
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
    int32 numInputs = (fTrackCount < MAX_INPUTS) ? fTrackCount : MAX_INPUTS;

    if (*cookie >= numInputs) {
        // No more inputs
        return B_BAD_INDEX;
    }

    *out_input = fInputs[*cookie];
    (*cookie)++;

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

    // Find which input is being connected
    int32 inputIndex = FindInputIndex(where);
    if (inputIndex < 0) {
        printf("VeniceAudioInputNode: ERROR - Unknown destination in Connected()\n");
        return B_MEDIA_BAD_DESTINATION;
    }

    printf("VeniceAudioInputNode: Connected input %d (%s) to producer\n",
           (int)inputIndex, fInputs[inputIndex].name);

    fInputs[inputIndex].source = producer;
    fInputs[inputIndex].format = with_format;
    fInputConnected[inputIndex] = true;
    fFormat = with_format;  // Store format globally

    *out_input = fInputs[inputIndex];

    // Allocate conversion buffer if needed
    AllocateBuffers();

    return B_OK;
}

void VeniceAudioInputNode::Disconnected(const media_source& producer,
                                        const media_destination& where)
{
    BAutolock lock(fLock);

    // Find which input is being disconnected
    int32 inputIndex = FindInputIndex(where);
    if (inputIndex < 0) {
        printf("VeniceAudioInputNode: ERROR - Unknown destination in Disconnected()\n");
        return;
    }

    printf("VeniceAudioInputNode: Disconnected input %d (%s) from producer\n",
           (int)inputIndex, fInputs[inputIndex].name);

    fInputs[inputIndex].source = media_source::null;
    fInputConnected[inputIndex] = false;

    // Only free buffers if ALL inputs are disconnected
    bool anyConnected = false;
    for (int32 i = 0; i < MAX_INPUTS; i++) {
        if (fInputConnected[i]) {
            anyConnected = true;
            break;
        }
    }

    if (!anyConnected) {
        FreeBuffers();
    }
}

status_t VeniceAudioInputNode::FormatChanged(const media_source& producer,
                                              const media_destination& consumer,
                                              int32 change_tag,
                                              const media_format& format)
{
    BAutolock lock(fLock);

    // Find which input is changing format
    int32 inputIndex = FindInputIndex(consumer);
    if (inputIndex < 0) {
        printf("VeniceAudioInputNode: ERROR - Unknown destination in FormatChanged()\n");
        return B_MEDIA_BAD_DESTINATION;
    }

    printf("VeniceAudioInputNode: Format changed for input %d\n", (int)inputIndex);

    fFormat = format;
    fInputs[inputIndex].format = format;

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

    if (!fMonitoring) {
        // Monitoring disabled - drop buffer
        buffer->Recycle();
        fDroppedBuffers++;
        return;
    }

    // Find which input this buffer is for
    // Buffer header contains destination ID (int32)
    int32 destId = buffer->Header()->destination;

    // Find input by matching destination ID
    int32 inputIndex = -1;
    for (int32 i = 0; i < MAX_INPUTS; i++) {
        if (fInputs[i].destination.id == destId) {
            inputIndex = i;
            break;
        }
    }

    if (inputIndex < 0 || inputIndex >= fTrackCount || !fTracks[inputIndex]) {
        // Unknown input or track not available - drop buffer
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

    // Process the buffer for the specific track
    status_t result = ProcessBuffer(inputIndex, data, size, performance_time);

    if (result != B_OK) {
        fDroppedBuffers++;
        printf("VeniceAudioInputNode: Dropped buffer on input %d (processing failed)\n", (int)inputIndex);
    }

    // Recycle buffer back to producer
    buffer->Recycle();

    fLastBufferTime = now;
}

void VeniceAudioInputNode::ProducerDataStatus(const media_destination& for_whom,
                                               int32 status,
                                               bigtime_t at_performance_time)
{
    // Producer is telling us about data status (running/stopped/etc)
    // Nothing to do here for now
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

status_t VeniceAudioInputNode::ProcessBuffer(int32 inputIndex, const void* data, size_t size,
                                              bigtime_t performance_time)
{
    if (!data || inputIndex < 0 || inputIndex >= fTrackCount || !fTracks[inputIndex]) {
        return B_ERROR;
    }

    SimpleTrack* track = fTracks[inputIndex];

    // Extract format info
    uint32 channels = fFormat.u.raw_audio.channel_count;
    float sampleRate = fFormat.u.raw_audio.frame_rate;
    uint32 frameCount = size / (channels * sizeof(float));

    // Route audio to track's live input buffer
    const float* audioData = (const float*)data;
    track->ProcessLiveInput(audioData, frameCount, channels);

    // Log every 100 buffers
    if (fBuffersReceived % 100 == 0) {
        printf("VeniceAudioInputNode: Routed buffer #%u to %s (%u frames, %u channels, %.0f Hz)\n",
               fBuffersReceived, track->GetName(), frameCount, channels, sampleRate);
    }

    return B_OK;
}

int32 VeniceAudioInputNode::FindInputIndex(const media_destination& dest)
{
    // Find input by matching destination ID
    for (int32 i = 0; i < MAX_INPUTS; i++) {
        if (fInputs[i].destination.port == dest.port &&
            fInputs[i].destination.id == dest.id) {
            return i;
        }
    }
    return -1;
}

bool VeniceAudioInputNode::IsConnected(int32 inputIndex) const
{
    if (inputIndex < 0 || inputIndex >= MAX_INPUTS) {
        return false;
    }
    return fInputConnected[inputIndex];
}

bool VeniceAudioInputNode::IsConnected() const
{
    // Return true if ANY input is connected
    for (int32 i = 0; i < MAX_INPUTS; i++) {
        if (fInputConnected[i]) {
            return true;
        }
    }
    return false;
}

} // namespace HaikuDAW

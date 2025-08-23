/*
 * HaikuAudioEngine.cpp - 100% Native Haiku audio engine implementation
 * 
 * Real-time audio processing using BMediaKit
 */

#include "HaikuAudioEngine.h"
#include <media/MediaRoster.h>
#include <media/TimeSource.h>
#include <media/Buffer.h>
#include <media/MediaFile.h>
#include <media/MediaTrack.h>
#include <support/Debug.h>
#include <support/Autolock.h>
#include <app/Message.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <algorithm>

namespace HaikuDAW {

// Constants
const int32 HaikuAudioEngine::kMaxTracks;
const int32 HaikuAudioEngine::kDefaultBufferSize;
const float HaikuAudioEngine::kDefaultSampleRate;
const int32 HaikuAudioEngine::kDefaultChannels;

// ========================================
// HaikuAudioEngine Implementation
// ========================================

HaikuAudioEngine::HaikuAudioEngine(const char* name)
    : BMediaNode(name)
    , BMediaEventLooper()
    , BBufferProducer(B_MEDIA_RAW_AUDIO)
    , fMediaRoster(nullptr)
    , fTimeSource(nullptr)
    , fBufferGroup(nullptr)
    , fSoundPlayer(nullptr)
    , fOutputEnabled(false)
    , fEngineLock("AudioEngine")
    , fEngineRunning(false)
    , fMasterVolume(1.0f)
    , fSampleRate(kDefaultSampleRate)
    , fBufferSize(kDefaultBufferSize)
    , fChannelCount(kDefaultChannels)
    , fTrackLock("TrackList")
    , fNextTrackId(1)
    , fCPUUsage(0.0f)
    , fLatency(0)
    , fLastProcessTime(0)
    , fDroppedFrames(0)
    , fMeterRunner(nullptr)
{
    printf("HaikuAudioEngine: Initializing native Haiku audio engine '%s'\n", name);
    
    // Initialize media format
    fFormat.type = B_MEDIA_RAW_AUDIO;
    fFormat.u.raw_audio = media_raw_audio_format::wildcard;
    fFormat.u.raw_audio.frame_rate = fSampleRate;
    fFormat.u.raw_audio.channel_count = fChannelCount;
    fFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
    fFormat.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
    fFormat.u.raw_audio.buffer_size = fBufferSize * sizeof(float) * fChannelCount;
    
    // Initialize media output
    fOutput.source.port = ControlPort();
    fOutput.source.id = 0;
    fOutput.destination = media_destination::null;
    fOutput.node = Node();
    fOutput.format = fFormat;
    strcpy(fOutput.name, "Audio Output");
    
    // Get media roster
    fMediaRoster = BMediaRoster::Roster();
    if (!fMediaRoster) {
        printf("HaikuAudioEngine: ERROR - Could not get BMediaRoster\n");
        return;
    }
    
    // Setup will continue in NodeRegistered()
    printf("HaikuAudioEngine: Basic initialization complete\n");
}

HaikuAudioEngine::~HaikuAudioEngine()
{
    printf("HaikuAudioEngine: Shutting down\n");
    
    StopEngine();
    
    // Cleanup
    delete fMeterRunner;
    delete fSoundPlayer;
    DeleteBufferGroup();
    
    printf("HaikuAudioEngine: Shutdown complete\n");
}

void HaikuAudioEngine::NodeRegistered()
{
    printf("HaikuAudioEngine: Node registered with media roster\n");
    
    // Get time source
    fTimeSource = fMediaRoster->MakeTimeSourceFor(Node());
    if (!fTimeSource) {
        printf("HaikuAudioEngine: Using system time source\n");
    }
    
    // Create buffer group
    status_t status = CreateBufferGroup();
    if (status != B_OK) {
        printf("HaikuAudioEngine: WARNING - Buffer group creation failed: %s\n", strerror(status));
    }
    
    // Set thread priority for real-time audio
    SetPriority(B_URGENT_DISPLAY_PRIORITY);
    
    printf("HaikuAudioEngine: Node registration complete\n");
}

status_t HaikuAudioEngine::CreateBufferGroup()
{
    if (fBufferGroup) {
        DeleteBufferGroup();
    }
    
    size_t bufferSize = fFormat.u.raw_audio.buffer_size;
    fBufferGroup = new BBufferGroup(bufferSize, 8); // 8 buffers
    
    status_t status = fBufferGroup->InitCheck();
    if (status != B_OK) {
        delete fBufferGroup;
        fBufferGroup = nullptr;
        printf("HaikuAudioEngine: Buffer group init failed: %s\n", strerror(status));
        return status;
    }
    
    printf("HaikuAudioEngine: Buffer group created (size: %zu bytes, count: 8)\n", bufferSize);
    return B_OK;
}

void HaikuAudioEngine::DeleteBufferGroup()
{
    delete fBufferGroup;
    fBufferGroup = nullptr;
}

status_t HaikuAudioEngine::StartEngine()
{
    BAutolock lock(fEngineLock);
    
    if (fEngineRunning) {
        printf("HaikuAudioEngine: Already running\n");
        return B_OK;
    }
    
    printf("HaikuAudioEngine: Starting native Haiku audio engine...\n");
    
    // Start the event looper
    Run();
    
    // Create BSoundPlayer for output
    if (!fSoundPlayer) {
        media_raw_audio_format playFormat = fFormat.u.raw_audio;
        
        fSoundPlayer = new BSoundPlayer(&playFormat, "HaikuDAW Output", 
            [](void* cookie, void* buffer, size_t size, const media_raw_audio_format& format) {
                // Static callback - delegate to instance
                HaikuAudioEngine* engine = static_cast<HaikuAudioEngine*>(cookie);
                engine->ProcessAudioCallback(buffer, size, format);
            }, 
            nullptr, this);
        
        status_t status = fSoundPlayer->InitCheck();
        if (status != B_OK) {
            printf("HaikuAudioEngine: BSoundPlayer init failed: %s\n", strerror(status));
            delete fSoundPlayer;
            fSoundPlayer = nullptr;
            return status;
        }
    }
    
    // Start sound player
    status_t status = fSoundPlayer->Start();
    if (status != B_OK) {
        printf("HaikuAudioEngine: Failed to start BSoundPlayer: %s\n", strerror(status));
        return status;
    }
    
    fEngineRunning = true;
    
    // Start meter updates (commented out for now - not essential for demo)
    // BMessage meterMsg(MSG_UPDATE_METERS);
    // fMeterRunner = new BMessageRunner(BMessenger(this), &meterMsg, 100000); // 10Hz
    fMeterRunner = nullptr;
    
    printf("HaikuAudioEngine: Native engine started successfully!\n");
    printf("  Sample Rate: %.0f Hz\n", fSampleRate);
    printf("  Buffer Size: %d frames\n", fBufferSize);
    printf("  Channels: %d\n", fChannelCount);
    printf("  Latency: %ld µs\n", GetLatency());
    
    return B_OK;
}

status_t HaikuAudioEngine::StopEngine()
{
    BAutolock lock(fEngineLock);
    
    if (!fEngineRunning) {
        return B_OK;
    }
    
    printf("HaikuAudioEngine: Stopping engine...\n");
    
    fEngineRunning = false;
    
    // Stop meter updates
    delete fMeterRunner;
    fMeterRunner = nullptr;
    
    // Stop sound player
    if (fSoundPlayer) {
        fSoundPlayer->Stop();
    }
    
    // Stop event looper
    Quit();
    
    printf("HaikuAudioEngine: Engine stopped\n");
    return B_OK;
}

bool HaikuAudioEngine::IsRunning() const
{
    // No autolock needed for simple boolean read
    return fEngineRunning;
}

void HaikuAudioEngine::ProcessAudioCallback(void* buffer, size_t size, 
                                           const media_raw_audio_format& format)
{
    bigtime_t startTime = system_time();
    
    // Clear output buffer
    memset(buffer, 0, size);
    
    if (!fEngineRunning) {
        return;
    }
    
    // Calculate frame count
    int32 frameCount = size / (format.channel_count * sizeof(float));
    float* outputBuffer = static_cast<float*>(buffer);
    
    // Process tracks
    if (format.channel_count == 2) {
        float* leftChannel = outputBuffer;
        float* rightChannel = outputBuffer + 1;
        
        // Mix tracks into output
        MixTracks(leftChannel, rightChannel, frameCount);
        
        // Apply master volume
        float masterVol = fMasterVolume;
        for (int32 i = 0; i < frameCount; i++) {
            leftChannel[i * 2] *= masterVol;
            rightChannel[i * 2] *= masterVol;
        }
    }
    
    // Update performance metrics
    fLastProcessTime = system_time() - startTime;
    
    // Calculate CPU usage (simple estimation)
    bigtime_t bufferDuration = (bigtime_t)((frameCount / fSampleRate) * 1000000);
    fCPUUsage = (float)fLastProcessTime / (float)bufferDuration;
}

void HaikuAudioEngine::MixTracks(float* outputLeft, float* outputRight, int32 frameCount)
{
    BAutolock lock(fTrackLock);
    
    // Simple mixing for demo - in real implementation would be more sophisticated
    for (auto& track : fTracks) {
        if (track && !track->IsMuted()) {
            // For demo: generate a test tone for each track
            static float phase = 0.0f;
            float frequency = 440.0f + (track->GetId() * 110.0f); // Different frequency per track
            float phaseIncrement = (2.0f * M_PI * frequency) / fSampleRate;
            float volume = track->GetVolume();
            
            for (int32 i = 0; i < frameCount; i++) {
                float sample = sin(phase) * volume * 0.1f; // Low volume
                
                // Apply 3D positioning (simplified)
                float x, y, z;
                track->GetPosition(x, y, z);
                float distance = sqrt(x*x + y*y + z*z) + 1.0f;
                sample /= distance; // Distance attenuation
                
                // Pan based on X position
                float pan = x / 10.0f; // Normalize to -1..1
                pan = std::max(-1.0f, std::min(1.0f, pan));
                
                float leftGain = (1.0f - pan) * 0.5f;
                float rightGain = (1.0f + pan) * 0.5f;
                
                outputLeft[i * 2] += sample * leftGain;
                outputRight[i * 2] += sample * rightGain;
                
                phase += phaseIncrement;
                if (phase > 2.0f * M_PI) {
                    phase -= 2.0f * M_PI;
                }
            }
        }
    }
}

// === Track Management ===

status_t HaikuAudioEngine::AddTrack(std::shared_ptr<HaikuAudioTrack> track)
{
    if (!track) {
        return B_BAD_VALUE;
    }
    
    BAutolock lock(fTrackLock);
    
    if (fTracks.size() >= kMaxTracks) {
        printf("HaikuAudioEngine: Maximum track count reached (%d)\n", kMaxTracks);
        return B_NO_MEMORY;
    }
    
    fTracks.push_back(track);
    printf("HaikuAudioEngine: Added track '%s' (ID: %d)\n", 
           track->GetName(), track->GetId());
    
    return B_OK;
}

status_t HaikuAudioEngine::RemoveTrack(int32 trackId)
{
    BAutolock lock(fTrackLock);
    
    auto it = std::remove_if(fTracks.begin(), fTracks.end(),
        [trackId](const std::shared_ptr<HaikuAudioTrack>& track) {
            return track->GetId() == trackId;
        });
    
    if (it != fTracks.end()) {
        fTracks.erase(it, fTracks.end());
        printf("HaikuAudioEngine: Removed track ID %d\n", trackId);
        return B_OK;
    }
    
    return B_ENTRY_NOT_FOUND;
}

HaikuAudioTrack* HaikuAudioEngine::GetTrack(int32 trackId)
{
    BAutolock lock(fTrackLock);
    
    for (auto& track : fTracks) {
        if (track->GetId() == trackId) {
            return track.get();
        }
    }
    
    return nullptr;
}

int32 HaikuAudioEngine::GetTrackCount() const
{
    BAutolock lock(fTrackLock);
    return fTracks.size();
}

// === Audio Parameters ===

void HaikuAudioEngine::SetMasterVolume(float volume)
{
    fMasterVolume = std::max(0.0f, std::min(1.0f, volume));
    printf("HaikuAudioEngine: Master volume = %.2f\n", fMasterVolume);
}

float HaikuAudioEngine::GetMasterVolume() const
{
    return fMasterVolume;
}

void HaikuAudioEngine::SetSampleRate(float rate)
{
    fSampleRate = rate;
    fFormat.u.raw_audio.frame_rate = rate;
    printf("HaikuAudioEngine: Sample rate = %.0f Hz\n", fSampleRate);
}

float HaikuAudioEngine::GetSampleRate() const
{
    return fSampleRate;
}

void HaikuAudioEngine::SetBufferSize(int32 frames)
{
    fBufferSize = frames;
    fFormat.u.raw_audio.buffer_size = frames * sizeof(float) * fChannelCount;
    printf("HaikuAudioEngine: Buffer size = %d frames\n", fBufferSize);
}

int32 HaikuAudioEngine::GetBufferSize() const
{
    return fBufferSize;
}

// === Performance Monitoring ===

float HaikuAudioEngine::GetCPUUsage() const
{
    return fCPUUsage * 100.0f; // Return as percentage
}

bigtime_t HaikuAudioEngine::GetLatency() const
{
    if (fSoundPlayer) {
        return fSoundPlayer->Latency();
    }
    return fLatency;
}

bigtime_t HaikuAudioEngine::GetProcessingTime() const
{
    return fLastProcessTime;
}

int32 HaikuAudioEngine::GetDroppedFrames() const
{
    return fDroppedFrames;
}

BString HaikuAudioEngine::GetStatusString() const
{
    BString status;
    
    if (fEngineRunning) {
        status.SetToFormat("Engine: RUNNING | Tracks: %d | CPU: %.1f%% | Rate: %.0fHz | Latency: %ldµs",
                          GetTrackCount(), GetCPUUsage(), fSampleRate, GetLatency());
    } else {
        status.SetTo("Native Haiku Engine: STOPPED - Ready to start");
    }
    
    return status;
}

// === BMediaNode Interface (simplified implementations) ===

BMediaAddOn* HaikuAudioEngine::AddOn(int32* internalID) const
{
    // Not part of an add-on
    return nullptr;
}

status_t HaikuAudioEngine::HandleMessage(int32 message, const void* data, size_t size)
{
    switch (message) {
        case MSG_UPDATE_METERS:
            // Update level meters
            break;
            
        default:
            return BMediaNode::HandleMessage(message, data, size);
    }
    
    return B_OK;
}

void HaikuAudioEngine::HandleEvent(const media_timed_event* event, bigtime_t lateness, bool realTimeEvent)
{
    switch (event->type) {
        case BTimedEventQueue::B_START:
            printf("HaikuAudioEngine: Media START event\n");
            break;
            
        case BTimedEventQueue::B_STOP:
            printf("HaikuAudioEngine: Media STOP event\n");
            break;
            
        case BTimedEventQueue::B_DATA_STATUS:
            // Process audio data event
            break;
            
        default:
            break;
    }
}

status_t HaikuAudioEngine::RequestCompleted(const media_request_info& info)
{
    return BMediaEventLooper::RequestCompleted(info);
}

// === BBufferProducer Interface (minimal implementations for now) ===

status_t HaikuAudioEngine::FormatSuggestionRequested(media_type type, int32 quality, media_format* format)
{
    if (type == B_MEDIA_RAW_AUDIO) {
        *format = fFormat;
        return B_OK;
    }
    return B_MEDIA_BAD_FORMAT;
}

status_t HaikuAudioEngine::FormatProposal(const media_source& output, media_format* format)
{
    if (output != fOutput.source) {
        return B_MEDIA_BAD_SOURCE;
    }
    
    *format = fFormat;
    return B_OK;
}

status_t HaikuAudioEngine::FormatChangeRequested(const media_source& source, const media_destination& destination,
                                                media_format* io_format, int32* _deprecated_)
{
    return B_ERROR; // Format changes not supported for now
}

status_t HaikuAudioEngine::GetNextOutput(int32* cookie, media_output* out_output)
{
    if (*cookie == 0) {
        *out_output = fOutput;
        *cookie = 1;
        return B_OK;
    }
    return B_BAD_INDEX;
}

status_t HaikuAudioEngine::DisposeOutputCookie(int32 cookie)
{
    return B_OK;
}

status_t HaikuAudioEngine::SetBufferGroup(const media_source& for_source, BBufferGroup* group)
{
    if (for_source != fOutput.source) {
        return B_MEDIA_BAD_SOURCE;
    }
    
    // Use provided buffer group or create our own
    if (group) {
        DeleteBufferGroup();
        fBufferGroup = group;
    } else {
        CreateBufferGroup();
    }
    
    return B_OK;
}

status_t HaikuAudioEngine::PrepareToConnect(const media_source& what, const media_destination& where,
                                           media_format* format, media_source* out_source, char* out_name)
{
    if (what != fOutput.source) {
        return B_MEDIA_BAD_SOURCE;
    }
    
    *format = fFormat;
    *out_source = fOutput.source;
    strcpy(out_name, fOutput.name);
    
    return B_OK;
}

void HaikuAudioEngine::Connect(status_t error, const media_source& source,
                              const media_destination& destination, const media_format& format, char* io_name)
{
    if (error == B_OK && source == fOutput.source) {
        fOutput.destination = destination;
        fOutput.format = format;
        strcpy(fOutput.name, io_name);
        printf("HaikuAudioEngine: Connected to %s\n", io_name);
    }
}

void HaikuAudioEngine::Disconnect(const media_source& what, const media_destination& where)
{
    if (what == fOutput.source) {
        fOutput.destination = media_destination::null;
        printf("HaikuAudioEngine: Disconnected\n");
    }
}

void HaikuAudioEngine::LateNoticeReceived(const media_source& what, bigtime_t how_much, bigtime_t performance_time)
{
    printf("HaikuAudioEngine: Late notice: %ld µs\n", how_much);
    fDroppedFrames++;
}

void HaikuAudioEngine::EnableOutput(const media_source& what, bool enabled, int32* _deprecated_)
{
    if (what == fOutput.source) {
        fOutputEnabled = enabled;
        printf("HaikuAudioEngine: Output %s\n", enabled ? "ENABLED" : "DISABLED");
    }
}

status_t HaikuAudioEngine::GetLatency(bigtime_t* out_latency)
{
    *out_latency = fLatency;
    return B_OK;
}

} // namespace HaikuDAW
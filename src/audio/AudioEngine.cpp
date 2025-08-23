/*
 * AudioEngine.cpp - Core audio engine implementation
 * 
 * Minimal but working implementation for demo
 * We'll add features incrementally
 */

#include "AudioEngine.h"
#include <media/MediaRoster.h>
#include <media/TimeSource.h>
#include <media/Buffer.h>
#include <support/Autolock.h>
#include <support/String.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

namespace HaikuDAW {

// Constants
const int32 AudioEngine::kMaxTracks = 64;
const int32 AudioEngine::kDefaultBufferSize = 512;
const float AudioEngine::kDefaultSampleRate = 44100.0f;  // CD quality for demo

// Constructor
AudioEngine::AudioEngine(const char* name)
    : BMediaNode(name)
    , BMediaEventLooper()
    , fMediaRoster(nullptr)
    , fTimeSource(nullptr)
    , fBufferGroup(nullptr)
    , fIsRunning(false)
    , fMasterVolume(1.0f)
    , fSampleRate(kDefaultSampleRate)
    , fBufferSize(kDefaultBufferSize)
    , fCPUUsage(0.0f)
    , fLatency(0)
    , fLastProcessTime(0)
    , fMeterRunner(nullptr)
{
    printf("AudioEngine: Initializing '%s'\n", name);
    
    // Initialize MediaKit
    status_t status = InitializeMediaKit();
    if (status != B_OK) {
        printf("AudioEngine: Failed to initialize MediaKit: %s\n", strerror(status));
    } else {
        printf("AudioEngine: MediaKit initialized successfully\n");
    }
}

// Destructor
AudioEngine::~AudioEngine()
{
    printf("AudioEngine: Shutting down\n");
    
    Stop();
    
    // Cleanup
    delete fMeterRunner;
    delete fBufferGroup;
    
    printf("AudioEngine: Shutdown complete\n");
}

// Initialize MediaKit
status_t AudioEngine::InitializeMediaKit()
{
    printf("AudioEngine: Setting up MediaKit...\n");
    
    // Get media roster
    fMediaRoster = BMediaRoster::Roster();
    if (!fMediaRoster) {
        printf("AudioEngine: ERROR - Could not get media roster\n");
        return B_ERROR;
    }
    
    // Get time source
    fTimeSource = fMediaRoster->MakeTimeSourceFor(TimeSource());
    if (!fTimeSource) {
        printf("AudioEngine: WARNING - Using system time source\n");
    }
    
    // Setup buffer group (simplified for demo)
    media_format format;
    format.type = B_MEDIA_RAW_AUDIO;
    format.u.raw_audio.frame_rate = fSampleRate;
    format.u.raw_audio.channel_count = 2;  // Stereo
    format.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
    format.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
    format.u.raw_audio.buffer_size = fBufferSize * sizeof(float) * 2;
    
    // Create buffer group
    fBufferGroup = new BBufferGroup(format.u.raw_audio.buffer_size, 4);
    if (fBufferGroup->InitCheck() != B_OK) {
        printf("AudioEngine: WARNING - Buffer group initialization failed\n");
        delete fBufferGroup;
        fBufferGroup = nullptr;
    }
    
    printf("AudioEngine: MediaKit setup complete\n");
    printf("  Sample Rate: %.0f Hz\n", fSampleRate);
    printf("  Buffer Size: %d frames\n", fBufferSize);
    printf("  Channels: 2 (stereo)\n");
    
    return B_OK;
}

// Start audio engine
status_t AudioEngine::Start()
{
    if (fIsRunning) {
        printf("AudioEngine: Already running\n");
        return B_OK;
    }
    
    printf("AudioEngine: Starting...\n");
    
    fIsRunning = true;
    
    // Start event looper
    Run();
    
    // Create meter update runner (10Hz updates)
    BMessage meterMsg('metr');
    fMeterRunner = new BMessageRunner(BMessenger(this), &meterMsg, 100000);
    
    printf("AudioEngine: Started successfully\n");
    return B_OK;
}

// Stop audio engine
status_t AudioEngine::Stop()
{
    if (!fIsRunning) {
        return B_OK;
    }
    
    printf("AudioEngine: Stopping...\n");
    
    fIsRunning = false;
    
    // Stop meter updates
    delete fMeterRunner;
    fMeterRunner = nullptr;
    
    // Stop event looper
    Quit();
    
    printf("AudioEngine: Stopped\n");
    return B_OK;
}

// Check if running
bool AudioEngine::IsRunning() const
{
    return fIsRunning;
}

// Add track
status_t AudioEngine::AddTrack(std::shared_ptr<AudioTrack> track)
{
    if (!track) {
        return B_BAD_VALUE;
    }
    
    BAutolock lock(fTrackLock);
    
    if (fTracks.size() >= kMaxTracks) {
        printf("AudioEngine: Maximum track count reached (%d)\n", kMaxTracks);
        return B_NO_MEMORY;
    }
    
    fTracks.push_back(track);
    printf("AudioEngine: Added track '%s' (ID: %d)\n", 
           track->GetName(), track->GetId());
    
    return B_OK;
}

// Remove track
status_t AudioEngine::RemoveTrack(int32 trackId)
{
    BAutolock lock(fTrackLock);
    
    auto it = std::remove_if(fTracks.begin(), fTracks.end(),
        [trackId](const std::shared_ptr<AudioTrack>& track) {
            return track->GetId() == trackId;
        });
    
    if (it != fTracks.end()) {
        fTracks.erase(it, fTracks.end());
        printf("AudioEngine: Removed track ID %d\n", trackId);
        return B_OK;
    }
    
    return B_ENTRY_NOT_FOUND;
}

// Get track
AudioTrack* AudioEngine::GetTrack(int32 trackId)
{
    BAutolock lock(fTrackLock);
    
    for (auto& track : fTracks) {
        if (track->GetId() == trackId) {
            return track.get();
        }
    }
    
    return nullptr;
}

// Get track count
int32 AudioEngine::GetTrackCount() const
{
    BAutolock lock(fTrackLock);
    return fTracks.size();
}

// Set master volume
void AudioEngine::SetMasterVolume(float volume)
{
    fMasterVolume = std::max(0.0f, std::min(1.0f, volume));
    printf("AudioEngine: Master volume = %.2f\n", fMasterVolume.load());
}

// Get master volume
float AudioEngine::GetMasterVolume() const
{
    return fMasterVolume;
}

// Handle media events
void AudioEngine::HandleEvent(const media_timed_event* event,
                              bigtime_t lateness,
                              bool realTimeEvent)
{
    switch (event->type) {
        case BTimedEventQueue::B_START:
            printf("AudioEngine: START event\n");
            break;
            
        case BTimedEventQueue::B_STOP:
            printf("AudioEngine: STOP event\n");
            break;
            
        case BTimedEventQueue::B_DATA:
            // Process audio data
            ProcessAudioBuffer();
            break;
            
        default:
            break;
    }
}

// Process audio buffer (simplified for demo)
void AudioEngine::ProcessAudioBuffer()
{
    bigtime_t startTime = system_time();
    
    // For demo: generate a test tone
    static float phase = 0.0f;
    const float frequency = 440.0f; // A4 note
    const float phaseIncrement = (2.0f * M_PI * frequency) / fSampleRate;
    
    // Simple sine wave generation for testing
    for (int32 i = 0; i < fBufferSize; i++) {
        float sample = sin(phase) * fMasterVolume * 0.25f; // 25% volume
        phase += phaseIncrement;
        
        // Keep phase in range
        if (phase > 2.0f * M_PI) {
            phase -= 2.0f * M_PI;
        }
    }
    
    // Update performance metrics
    fLastProcessTime = system_time() - startTime;
    
    // Simple CPU usage estimation
    float bufferDuration = (fBufferSize / fSampleRate) * 1000000.0f; // in microseconds
    fCPUUsage = (float)fLastProcessTime / bufferDuration;
}

// BMediaNode interface
BMediaAddOn* AudioEngine::AddOn(int32* internalID) const
{
    // Not part of an add-on
    return nullptr;
}

status_t AudioEngine::HandleMessage(int32 message, const void* data, size_t size)
{
    // Handle custom messages if needed
    return B_OK;
}

// Performance monitoring
float AudioEngine::GetCPUUsage() const
{
    return fCPUUsage * 100.0f; // Return as percentage
}

int32 AudioEngine::GetLatency() const
{
    return fLatency;
}

bigtime_t AudioEngine::GetProcessingTime() const
{
    return fLastProcessTime;
}

// =====================================
// AudioTrack Implementation
// =====================================

AudioTrack::AudioTrack(int32 id, const char* name)
    : fId(id)
    , fName(name)
    , fVolume(1.0f)
    , fPan(0.0f)
    , fMuted(false)
    , fSolo(false)
    , fPosX(0.0f)
    , fPosY(0.0f)
    , fPosZ(0.0f)
{
    printf("AudioTrack: Created '%s' (ID: %d)\n", name, id);
}

AudioTrack::~AudioTrack()
{
    printf("AudioTrack: Destroyed '%s'\n", fName.String());
}

void AudioTrack::SetName(const char* name)
{
    fName = name;
}

void AudioTrack::SetVolume(float volume)
{
    fVolume = std::max(0.0f, std::min(1.0f, volume));
}

float AudioTrack::GetVolume() const
{
    return fVolume;
}

void AudioTrack::SetPan(float pan)
{
    fPan = std::max(-1.0f, std::min(1.0f, pan));
}

float AudioTrack::GetPan() const
{
    return fPan;
}

void AudioTrack::SetMute(bool mute)
{
    fMuted = mute;
}

bool AudioTrack::IsMuted() const
{
    return fMuted;
}

void AudioTrack::SetSolo(bool solo)
{
    fSolo = solo;
}

bool AudioTrack::IsSolo() const
{
    return fSolo;
}

void AudioTrack::SetPosition(float x, float y, float z)
{
    fPosX = x;
    fPosY = y;
    fPosZ = z;
}

void AudioTrack::GetPosition(float& x, float& y, float& z) const
{
    x = fPosX;
    y = fPosY;
    z = fPosZ;
}

status_t AudioTrack::ProcessBuffer(AudioBuffer* input, AudioBuffer* output)
{
    if (!output) {
        return B_BAD_VALUE;
    }
    
    // Simple pass-through with volume for now
    if (input && !fMuted) {
        output->MixFrom(input, fVolume);
    }
    
    return B_OK;
}

// =====================================
// AudioBuffer Implementation
// =====================================

AudioBuffer::AudioBuffer(int32 channels, int32 frames)
    : fChannels(channels)
    , fFrames(frames)
{
    // Allocate channel pointers
    fData = new float*[channels];
    
    // Allocate data for each channel
    for (int32 i = 0; i < channels; i++) {
        fData[i] = new float[frames];
        Clear();
    }
}

AudioBuffer::~AudioBuffer()
{
    // Free channel data
    for (int32 i = 0; i < fChannels; i++) {
        delete[] fData[i];
    }
    
    // Free channel pointers
    delete[] fData;
}

float* AudioBuffer::GetChannel(int32 channel)
{
    if (channel >= 0 && channel < fChannels) {
        return fData[channel];
    }
    return nullptr;
}

const float* AudioBuffer::GetChannel(int32 channel) const
{
    if (channel >= 0 && channel < fChannels) {
        return fData[channel];
    }
    return nullptr;
}

void AudioBuffer::Clear()
{
    for (int32 i = 0; i < fChannels; i++) {
        memset(fData[i], 0, fFrames * sizeof(float));
    }
}

void AudioBuffer::CopyFrom(const AudioBuffer* source)
{
    if (!source) return;
    
    int32 channelsToCopy = std::min(fChannels, source->GetChannelCount());
    int32 framesToCopy = std::min(fFrames, source->GetFrameCount());
    
    for (int32 ch = 0; ch < channelsToCopy; ch++) {
        const float* src = source->GetChannel(ch);
        float* dst = GetChannel(ch);
        
        if (src && dst) {
            memcpy(dst, src, framesToCopy * sizeof(float));
        }
    }
}

void AudioBuffer::MixFrom(const AudioBuffer* source, float gain)
{
    if (!source) return;
    
    int32 channelsToCopy = std::min(fChannels, source->GetChannelCount());
    int32 framesToCopy = std::min(fFrames, source->GetFrameCount());
    
    for (int32 ch = 0; ch < channelsToCopy; ch++) {
        const float* src = source->GetChannel(ch);
        float* dst = GetChannel(ch);
        
        if (src && dst) {
            for (int32 i = 0; i < framesToCopy; i++) {
                dst[i] += src[i] * gain;
            }
        }
    }
}

} // namespace HaikuDAW
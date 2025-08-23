/*
 * AudioEngineSimple.cpp - Simplified audio engine implementation
 * 
 * Cross-platform implementation for testing core logic
 * Will be easily portable to Haiku BMediaKit
 */

#include "AudioEngineSimple.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace HaikuDAW {

AudioEngineSimple::AudioEngineSimple(const char* name)
    : fName(name ? name : "AudioEngine")
    , fRunning(false)
    , fMasterVolume(1.0f)
    , fSampleRate(kDefaultSampleRate)
    , fBufferSize(kDefaultBufferSize)
    , fNextTrackId(1)
    , fCPUUsage(0.0f)
    , fLatency(0)
    , fLastProcessTime(0)
{
    std::cout << "AudioEngineSimple: Initializing '" << fName << "'" << std::endl;
    std::cout << "  Sample Rate: " << fSampleRate << " Hz" << std::endl;
    std::cout << "  Buffer Size: " << fBufferSize << " frames" << std::endl;
}

AudioEngineSimple::~AudioEngineSimple()
{
    std::cout << "AudioEngineSimple: Shutting down" << std::endl;
    Stop();
}

bool AudioEngineSimple::Start()
{
    if (fRunning) {
        std::cout << "AudioEngineSimple: Already running" << std::endl;
        return true;
    }
    
    std::cout << "AudioEngineSimple: Starting..." << std::endl;
    
    fRunning = true;
    
    // Start processing thread
    fProcessingThread = std::make_unique<std::thread>(&AudioEngineSimple::ProcessingThread, this);
    
    std::cout << "AudioEngineSimple: Started successfully" << std::endl;
    return true;
}

bool AudioEngineSimple::Stop()
{
    if (!fRunning) {
        return true;
    }
    
    std::cout << "AudioEngineSimple: Stopping..." << std::endl;
    
    fRunning = false;
    
    // Wait for processing thread to finish
    if (fProcessingThread && fProcessingThread->joinable()) {
        fProcessingThread->join();
    }
    fProcessingThread.reset();
    
    std::cout << "AudioEngineSimple: Stopped" << std::endl;
    return true;
}

bool AudioEngineSimple::IsRunning() const
{
    return fRunning;
}

int32_t AudioEngineSimple::AddTrack(const std::string& name)
{
    std::lock_guard<std::mutex> lock(fTrackMutex);
    
    if (fTracks.size() >= kMaxTracks) {
        std::cout << "AudioEngineSimple: Maximum track count reached (" << kMaxTracks << ")" << std::endl;
        return -1;
    }
    
    auto track = std::make_unique<Track>(fNextTrackId, name);
    int32_t trackId = fNextTrackId++;
    
    fTracks.push_back(std::move(track));
    
    std::cout << "AudioEngineSimple: Added track '" << name << "' (ID: " << trackId << ")" << std::endl;
    return trackId;
}

bool AudioEngineSimple::RemoveTrack(int32_t trackId)
{
    std::lock_guard<std::mutex> lock(fTrackMutex);
    
    auto it = std::find_if(fTracks.begin(), fTracks.end(),
        [trackId](const std::unique_ptr<Track>& track) {
            return track->id == trackId;
        });
    
    if (it != fTracks.end()) {
        std::cout << "AudioEngineSimple: Removed track ID " << trackId << std::endl;
        fTracks.erase(it);
        return true;
    }
    
    return false;
}

int32_t AudioEngineSimple::GetTrackCount() const
{
    std::lock_guard<std::mutex> lock(fTrackMutex);
    return fTracks.size();
}

void AudioEngineSimple::SetMasterVolume(float volume)
{
    fMasterVolume = std::max(0.0f, std::min(1.0f, volume));
    std::cout << "AudioEngineSimple: Master volume = " << fMasterVolume.load() << std::endl;
}

float AudioEngineSimple::GetMasterVolume() const
{
    return fMasterVolume;
}

void AudioEngineSimple::SetSampleRate(float rate)
{
    fSampleRate = rate;
    std::cout << "AudioEngineSimple: Sample rate = " << fSampleRate << " Hz" << std::endl;
}

float AudioEngineSimple::GetSampleRate() const
{
    return fSampleRate;
}

void AudioEngineSimple::SetBufferSize(int32_t frames)
{
    fBufferSize = frames;
    std::cout << "AudioEngineSimple: Buffer size = " << fBufferSize << " frames" << std::endl;
}

int32_t AudioEngineSimple::GetBufferSize() const
{
    return fBufferSize;
}

float AudioEngineSimple::GetCPUUsage() const
{
    return fCPUUsage * 100.0f; // Return as percentage
}

int32_t AudioEngineSimple::GetLatency() const
{
    return fLatency;
}

int64_t AudioEngineSimple::GetProcessingTime() const
{
    return fLastProcessTime;
}

bool AudioEngineSimple::SetTrackVolume(int32_t trackId, float volume)
{
    std::lock_guard<std::mutex> lock(fTrackMutex);
    
    for (auto& track : fTracks) {
        if (track->id == trackId) {
            track->volume = std::max(0.0f, std::min(1.0f, volume));
            std::cout << "AudioEngineSimple: Track " << trackId << " volume = " << track->volume << std::endl;
            return true;
        }
    }
    return false;
}

bool AudioEngineSimple::SetTrackMute(int32_t trackId, bool muted)
{
    std::lock_guard<std::mutex> lock(fTrackMutex);
    
    for (auto& track : fTracks) {
        if (track->id == trackId) {
            track->muted = muted;
            std::cout << "AudioEngineSimple: Track " << trackId << (muted ? " MUTED" : " UNMUTED") << std::endl;
            return true;
        }
    }
    return false;
}

bool AudioEngineSimple::SetTrackSolo(int32_t trackId, bool solo)
{
    std::lock_guard<std::mutex> lock(fTrackMutex);
    
    for (auto& track : fTracks) {
        if (track->id == trackId) {
            track->solo = solo;
            std::cout << "AudioEngineSimple: Track " << trackId << (solo ? " SOLO" : " UNSOLO") << std::endl;
            return true;
        }
    }
    return false;
}

bool AudioEngineSimple::SetTrackPosition(int32_t trackId, float x, float y, float z)
{
    std::lock_guard<std::mutex> lock(fTrackMutex);
    
    for (auto& track : fTracks) {
        if (track->id == trackId) {
            track->posX = x;
            track->posY = y;
            track->posZ = z;
            std::cout << "AudioEngineSimple: Track " << trackId << " position = (" 
                     << x << ", " << y << ", " << z << ")" << std::endl;
            return true;
        }
    }
    return false;
}

std::string AudioEngineSimple::GetStatusString() const
{
    std::stringstream ss;
    
    if (fRunning) {
        ss << "Engine: RUNNING | Tracks: " << GetTrackCount()
           << " | CPU: " << std::fixed << std::setprecision(1) << GetCPUUsage() << "%"
           << " | Rate: " << fSampleRate << "Hz";
    } else {
        ss << "Engine: STOPPED - Ready to start";
    }
    
    return ss.str();
}

void AudioEngineSimple::ProcessingThread()
{
    std::cout << "AudioEngineSimple: Processing thread started" << std::endl;
    
    // Calculate sleep time for buffer rate
    auto bufferDuration = std::chrono::microseconds(
        static_cast<int64_t>((fBufferSize / fSampleRate) * 1000000));
    
    while (fRunning) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        ProcessAudioBuffer();
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto processingTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        
        fLastProcessTime = processingTime.count();
        
        // Calculate CPU usage
        float bufferDurationUs = static_cast<float>(bufferDuration.count());
        fCPUUsage = static_cast<float>(processingTime.count()) / bufferDurationUs;
        
        // Sleep for remaining buffer time
        if (processingTime < bufferDuration) {
            std::this_thread::sleep_for(bufferDuration - processingTime);
        }
    }
    
    std::cout << "AudioEngineSimple: Processing thread stopped" << std::endl;
}

void AudioEngineSimple::ProcessAudioBuffer()
{
    // For demo: simulate audio processing
    static float phase = 0.0f;
    const float frequency = 440.0f; // A4 note
    const float phaseIncrement = (2.0f * M_PI * frequency) / fSampleRate;
    
    // Simulate processing all tracks
    {
        std::lock_guard<std::mutex> lock(fTrackMutex);
        
        for (auto& track : fTracks) {
            if (!track->muted) {
                // Simulate track processing
                // In real implementation, this would process actual audio data
            }
        }
    }
    
    // Simulate master output processing
    for (int32_t i = 0; i < fBufferSize; i++) {
        float sample = sin(phase) * fMasterVolume * 0.1f; // Low volume test tone
        phase += phaseIncrement;
        
        if (phase > 2.0f * M_PI) {
            phase -= 2.0f * M_PI;
        }
    }
}

// ========================================
// AudioBufferSimple Implementation
// ========================================

AudioBufferSimple::AudioBufferSimple(int32_t channels, int32_t frames)
    : fChannels(channels)
    , fFrames(frames)
{
    fData.resize(channels);
    for (int32_t ch = 0; ch < channels; ch++) {
        fData[ch].resize(frames, 0.0f);
    }
}

AudioBufferSimple::~AudioBufferSimple()
{
}

float* AudioBufferSimple::GetChannel(int32_t channel)
{
    if (channel >= 0 && channel < fChannels) {
        return fData[channel].data();
    }
    return nullptr;
}

const float* AudioBufferSimple::GetChannel(int32_t channel) const
{
    if (channel >= 0 && channel < fChannels) {
        return fData[channel].data();
    }
    return nullptr;
}

void AudioBufferSimple::Clear()
{
    for (auto& channel : fData) {
        std::fill(channel.begin(), channel.end(), 0.0f);
    }
}

void AudioBufferSimple::GenerateTestTone(float frequency, float phase)
{
    const float phaseIncrement = (2.0f * M_PI * frequency) / 44100.0f; // Assume 44.1kHz
    
    for (int32_t ch = 0; ch < fChannels; ch++) {
        float currentPhase = phase;
        
        for (int32_t i = 0; i < fFrames; i++) {
            fData[ch][i] = sin(currentPhase) * 0.25f; // 25% volume
            currentPhase += phaseIncrement;
            
            if (currentPhase > 2.0f * M_PI) {
                currentPhase -= 2.0f * M_PI;
            }
        }
    }
}

} // namespace HaikuDAW
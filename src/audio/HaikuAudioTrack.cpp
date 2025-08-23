/*
 * HaikuAudioTrack.cpp - Native Haiku audio track implementation
 * Simplified version for demo - focuses on core functionality
 */

#include "HaikuAudioEngine.h"
#include <support/Autolock.h>
#include <media/MediaFile.h>
#include <media/MediaTrack.h>
#include <storage/Entry.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <algorithm>

namespace HaikuDAW {

// Helper function declaration
const char* format_to_string(uint32 format);

HaikuAudioTrack::HaikuAudioTrack(int32 id, const char* name)
    : fId(id)
    , fName(name)
    , fControlLock("TrackControl")
    , fVolume(1.0f)
    , fPan(0.0f)
    , fMuted(false)
    , fSolo(false)
    , fPosX(0.0f)
    , fPosY(0.0f)
    , fPosZ(0.0f)
    , fDistance(1.0f)
    , fPeakLevel(0.0f)
    , fRMSLevel(0.0f)
    , fLastLevelUpdate(0)
    , fMediaFile(nullptr)
    , fMediaTrack(nullptr)
    , fPlaybackPosition(0)
    , fIsPlaying(false)
    , fProcessingBuffer(nullptr)
    , fBufferSize(0)
{
    printf("HaikuAudioTrack: Created '%s' (ID: %d)\n", name, id);
    
    // Initialize file format
    fFileFormat.type = B_MEDIA_RAW_AUDIO;
    fFileFormat.u.raw_audio = media_raw_audio_format::wildcard;
}

HaikuAudioTrack::~HaikuAudioTrack()
{
    printf("HaikuAudioTrack: Destroyed '%s'\n", fName.String());
    
    // Clean up media file (proper Haiku way)
    if (fMediaTrack && fMediaFile) {
        fMediaFile->ReleaseTrack(fMediaTrack);
        fMediaTrack = nullptr;
    }
    
    if (fMediaFile) {
        delete fMediaFile;
        fMediaFile = nullptr;
    }
    
    // Clean up processing buffer
    delete[] fProcessingBuffer;
}

// === Track Properties ===

void HaikuAudioTrack::SetName(const char* name)
{
    BAutolock lock(fControlLock);
    fName = name;
}

// === Audio Control ===

void HaikuAudioTrack::SetVolume(float volume)
{
    BAutolock lock(fControlLock);
    fVolume = std::max(0.0f, std::min(2.0f, volume)); // Allow up to 200% for boost
}

float HaikuAudioTrack::GetVolume() const
{
    // Simple read - no autolock needed for demo
    return fVolume;
}

void HaikuAudioTrack::SetPan(float pan)
{
    BAutolock lock(fControlLock);
    fPan = std::max(-1.0f, std::min(1.0f, pan));
}

float HaikuAudioTrack::GetPan() const
{
    return fPan;
}

void HaikuAudioTrack::SetMute(bool mute)
{
    BAutolock lock(fControlLock);
    fMuted = mute;
}

bool HaikuAudioTrack::IsMuted() const
{
    return fMuted;
}

void HaikuAudioTrack::SetSolo(bool solo)
{
    BAutolock lock(fControlLock);
    fSolo = solo;
}

bool HaikuAudioTrack::IsSolo() const
{
    return fSolo;
}

// === 3D Spatial Audio ===

void HaikuAudioTrack::SetPosition(float x, float y, float z)
{
    BAutolock lock(fControlLock);
    fPosX = x;
    fPosY = y;
    fPosZ = z;
    fDistance = sqrt(x*x + y*y + z*z);
}

void HaikuAudioTrack::GetPosition(float& x, float& y, float& z) const
{
    x = fPosX;
    y = fPosY;
    z = fPosZ;
}

void HaikuAudioTrack::SetDistance(float distance)
{
    BAutolock lock(fControlLock);
    fDistance = std::max(0.1f, distance); // Minimum distance to avoid division by zero
}

float HaikuAudioTrack::GetDistance() const
{
    return fDistance;
}

// === Level Monitoring ===

float HaikuAudioTrack::GetPeakLevel() const
{
    return fPeakLevel;
}

float HaikuAudioTrack::GetRMSLevel() const
{
    return fRMSLevel;
}

void HaikuAudioTrack::UpdateLevels(const float* samples, int32 frameCount)
{
    if (!samples || frameCount <= 0) {
        return;
    }
    
    BAutolock lock(fControlLock);
    
    float peak = 0.0f;
    float rms = 0.0f;
    
    // Calculate peak and RMS levels
    for (int32 i = 0; i < frameCount; i++) {
        float sample = fabs(samples[i]);
        if (sample > peak) {
            peak = sample;
        }
        rms += sample * sample;
    }
    
    fPeakLevel = peak;
    fRMSLevel = sqrt(rms / frameCount);
    fLastLevelUpdate = system_time();
}

// === Audio Processing ===

status_t HaikuAudioTrack::ProcessAudio(float* inputLeft, float* inputRight,
                                      float* outputLeft, float* outputRight,
                                      int32 frameCount)
{
    BAutolock lock(fControlLock);
    
    if (fMuted || frameCount <= 0) {
        return B_OK;
    }
    
    // For demo: generate a test tone if no input
    if (!inputLeft || !inputRight) {
        // Generate different frequencies for different tracks
        static float phase = 0.0f;
        float frequency = 220.0f + (fId * 55.0f); // Different frequency per track
        float phaseIncrement = (2.0f * M_PI * frequency) / 44100.0f;
        
        for (int32 i = 0; i < frameCount; i++) {
            float sample = sin(phase) * 0.1f; // Low volume test tone
            
            // Store in temporary arrays (for this demo)
            if (!fProcessingBuffer || fBufferSize < frameCount * 2) {
                delete[] fProcessingBuffer;
                fBufferSize = frameCount * 2;
                fProcessingBuffer = new float[fBufferSize];
            }
            
            fProcessingBuffer[i * 2] = sample;     // Left
            fProcessingBuffer[i * 2 + 1] = sample; // Right
            
            phase += phaseIncrement;
            if (phase > 2.0f * M_PI) {
                phase -= 2.0f * M_PI;
            }
        }
        
        inputLeft = fProcessingBuffer;
        inputRight = fProcessingBuffer + 1;
    }
    
    // Apply volume and pan
    ApplyVolumeAndPan(const_cast<float*>(inputLeft), const_cast<float*>(inputRight), frameCount);
    
    // Apply 3D positioning
    Apply3DPositioning(const_cast<float*>(inputLeft), const_cast<float*>(inputRight), frameCount);
    
    // Mix into output
    for (int32 i = 0; i < frameCount; i++) {
        outputLeft[i] += inputLeft[i * 2];
        outputRight[i] += inputRight[i * 2];
    }
    
    // Update level meters
    UpdateLevels(inputLeft, frameCount);
    
    return B_OK;
}

void HaikuAudioTrack::Apply3DPositioning(float* left, float* right, int32 frameCount)
{
    // Simple 3D audio positioning
    float distance = fDistance + 1.0f; // Add 1 to avoid division by zero
    float attenuation = 1.0f / distance;
    
    // Pan based on X position
    float panAmount = fPosX / 10.0f; // Normalize to approximately -1..1
    panAmount = std::max(-1.0f, std::min(1.0f, panAmount));
    
    float leftGain = (1.0f - panAmount) * 0.5f * attenuation;
    float rightGain = (1.0f + panAmount) * 0.5f * attenuation;
    
    // Apply height (Y) as a simple filter (demo)
    if (fPosY > 0) {
        // Higher position = brighter (simple high-pass effect simulation)
        float brightness = std::min(1.2f, 1.0f + fPosY * 0.1f);
        leftGain *= brightness;
        rightGain *= brightness;
    }
    
    for (int32 i = 0; i < frameCount; i++) {
        left[i * 2] *= leftGain;
        right[i * 2] *= rightGain;
    }
}

void HaikuAudioTrack::ApplyVolumeAndPan(float* left, float* right, int32 frameCount)
{
    float volume = fVolume;
    float pan = fPan;
    
    // Calculate left/right gains from pan
    float leftGain = (1.0f - pan) * volume;
    float rightGain = (1.0f + pan) * volume;
    
    for (int32 i = 0; i < frameCount; i++) {
        left[i * 2] *= leftGain;
        right[i * 2] *= rightGain;
    }
}

// === File Playback (Simplified for Demo) ===

status_t HaikuAudioTrack::LoadAudioFile(const char* path)
{
    printf("HaikuAudioTrack: LoadAudioFile '%s' (demo - not implemented)\n", path);
    return B_OK; // Simplified for demo
}

void HaikuAudioTrack::SetPlaybackPosition(bigtime_t position)
{
    fPlaybackPosition = position;
}

bigtime_t HaikuAudioTrack::GetPlaybackPosition() const
{
    return fPlaybackPosition;
}

bool HaikuAudioTrack::IsPlaying() const
{
    return fIsPlaying;
}

void HaikuAudioTrack::StartPlayback()
{
    fIsPlaying = true;
    printf("HaikuAudioTrack: Started playback of '%s'\n", fName.String());
}

void HaikuAudioTrack::StopPlayback()
{
    fIsPlaying = false;
    printf("HaikuAudioTrack: Stopped playback of '%s'\n", fName.String());
}

// Helper function to convert audio format to string
const char* format_to_string(uint32 format)
{
    switch (format) {
        case media_raw_audio_format::B_AUDIO_FLOAT: return "Float";
        case media_raw_audio_format::B_AUDIO_SHORT: return "16-bit";
        case media_raw_audio_format::B_AUDIO_INT: return "32-bit int";
        case media_raw_audio_format::B_AUDIO_UCHAR: return "8-bit unsigned";
        case media_raw_audio_format::B_AUDIO_CHAR: return "8-bit signed";
        default: return "Unknown";
    }
}

} // namespace HaikuDAW
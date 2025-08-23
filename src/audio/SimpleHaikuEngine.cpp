/*
 * SimpleHaikuEngine.cpp - Engine VERAMENTE semplice per Haiku
 */

#include "SimpleHaikuEngine.h"
#include "AudioConfig.h"
#include <stdio.h>
#include <math.h>

namespace HaikuDAW {

// === SimpleTrack ===

SimpleTrack::SimpleTrack(int id, const char* name)
    : fId(id), fName(name), fVolume(1.0f), fPan(0.0f), fX(0), fY(0), fZ(0), fMuted(false), fSolo(false),
      fPeakLevel(0.0f), fRMSLevel(0.0f), fPhase(0.0f)
{
    printf("SimpleTrack: Created '%s'\n", name);
}

// === SimpleHaikuEngine ===

SimpleHaikuEngine::SimpleHaikuEngine()
    : fSoundPlayer(nullptr), fRunning(false), fMasterVolume(1.0f), fSoloTrack(-1),
      fMasterPeakLeft(0.0f), fMasterPeakRight(0.0f), fMasterRMSLeft(0.0f), fMasterRMSRight(0.0f)
{
    printf("SimpleHaikuEngine: Created\n");
}

SimpleHaikuEngine::~SimpleHaikuEngine()
{
    Stop();
    delete fSoundPlayer;
    
    // Cleanup tracks
    for (auto track : fTracks) {
        delete track;
    }
    printf("SimpleHaikuEngine: Destroyed\n");
}

status_t SimpleHaikuEngine::Start()
{
    if (fRunning) {
        return B_OK;
    }
    
    printf("SimpleHaikuEngine: Starting...\n");
    
    // Configure audio format with smaller buffer for lower latency
    media_raw_audio_format format;
    format.frame_rate = 44100.0;
    format.channel_count = 2;  // Stereo
    format.format = media_raw_audio_format::B_AUDIO_FLOAT;
    format.byte_order = B_MEDIA_LITTLE_ENDIAN;
    
    // Use optimized buffer size for low latency
    int bufferFrames = DEFAULT_BUFFER_SIZE;  // 256 frames for ~5.8ms
    format.buffer_size = bufferFrames * sizeof(float) * 2;  // frames * 4 bytes * 2 channels
    
    float latencyMs = CalculateLatencyMs(bufferFrames, DEFAULT_SAMPLE_RATE);
    printf("SimpleHaikuEngine: Low-latency configuration:\n");
    printf("  Buffer: %d frames (%ld bytes)\n", bufferFrames, format.buffer_size);
    printf("  Latency: %.2f ms (target < 10ms)\n", latencyMs);
    if (latencyMs < 10.0f) {
        printf("  ✓ Real-time latency achieved!\n");
    }
    
    fSoundPlayer = new BSoundPlayer(&format, "HaikuDAW LowLatency", AudioCallback, nullptr, this);
    
    status_t status = fSoundPlayer->InitCheck();
    if (status != B_OK) {
        printf("SimpleHaikuEngine: BSoundPlayer init failed: %s (0x%x)\n", strerror(status), (int)status);
        printf("  -> Check if media_server is running\n");
        printf("  -> Check audio settings in Media preferences\n");
        return status;
    }
    
    status = fSoundPlayer->Start();
    if (status != B_OK) {
        printf("SimpleHaikuEngine: BSoundPlayer start failed\n");
        return status;
    }
    
    fRunning = true;
    printf("SimpleHaikuEngine: Started successfully!\n");
    return B_OK;
}

status_t SimpleHaikuEngine::Stop()
{
    if (!fRunning) {
        return B_OK;
    }
    
    printf("SimpleHaikuEngine: Stopping...\n");
    
    if (fSoundPlayer) {
        fSoundPlayer->Stop();
    }
    
    fRunning = false;
    printf("SimpleHaikuEngine: Stopped\n");
    return B_OK;
}

status_t SimpleHaikuEngine::AddTrack(SimpleTrack* track)
{
    if (!track) return B_BAD_VALUE;
    
    fTracks.push_back(track);
    printf("SimpleHaikuEngine: Added track '%s'\n", track->GetName());
    return B_OK;
}

status_t SimpleHaikuEngine::RemoveTrack(int index)
{
    if (index < 0 || (size_t)index >= fTracks.size()) {
        return B_BAD_INDEX;
    }
    
    SimpleTrack* track = fTracks[index];
    printf("SimpleHaikuEngine: Removing track '%s'\n", track->GetName());
    
    // Remove from vector
    fTracks.erase(fTracks.begin() + index);
    
    // Reset solo if this was the solo track
    if (fSoloTrack == index) {
        fSoloTrack = -1;
    } else if (fSoloTrack > index) {
        // Adjust solo track index if it comes after the removed track
        fSoloTrack--;
    }
    
    // Clean up the track
    delete track;
    
    printf("SimpleHaikuEngine: Track removed (total: %d)\n", (int)fTracks.size());
    return B_OK;
}

BString SimpleHaikuEngine::GetStatus() const
{
    BString status;
    if (fRunning) {
        status.SetToFormat("RUNNING | Tracks: %d | Volume: %.1f", 
                          (int)fTracks.size(), fMasterVolume);
    } else {
        status.SetTo("STOPPED");
    }
    return status;
}

void SimpleHaikuEngine::AudioCallback(void* cookie, void* buffer, size_t size, const media_raw_audio_format& format)
{
    SimpleHaikuEngine* engine = static_cast<SimpleHaikuEngine*>(cookie);
    
    // Clear buffer
    memset(buffer, 0, size);
    
    if (!engine->fRunning) {
        return;
    }
    
    size_t frameCount = size / (format.channel_count * sizeof(float));
    engine->ProcessAudio(static_cast<float*>(buffer), frameCount);
}

void SimpleHaikuEngine::ProcessAudio(float* buffer, size_t frameCount)
{
    // Master level calculation variables
    float masterPeakLeft = 0.0f;
    float masterPeakRight = 0.0f;
    float masterRMSLeft = 0.0f;
    float masterRMSRight = 0.0f;
    
    // Debug output disabled in audio callback for real-time performance
    #ifdef DEBUG_AUDIO_VERBOSE
    static int debugCounter = 0;
    if (debugCounter++ % 44100 == 0) {  // Only once per second if debugging
        printf("ProcessAudio: %d tracks active\n", (int)fTracks.size());
    }
    #endif
    
    // Generate simple test tones for each track
    for (size_t trackIndex = 0; trackIndex < fTracks.size(); trackIndex++) {
        SimpleTrack* track = fTracks[trackIndex];
        
        // Solo logic: if any track is solo, only play solo tracks (unless muted)
        // If no solo, play all non-muted tracks
        bool shouldPlay = false;
        if (fSoloTrack >= 0) {
            // Solo mode: only play if this track is solo AND not muted
            shouldPlay = track->IsSolo() && !track->IsMuted();
        } else {
            // Normal mode: play if not muted
            shouldPlay = !track->IsMuted();
        }
        
        if (!shouldPlay) continue;
        
        // Different frequency for each track
        float frequency = 220.0f + (trackIndex * 110.0f);
        float phaseIncrement = (2.0f * M_PI * frequency) / 44100.0f;
        
        float volume = track->GetVolume() * fMasterVolume * 0.1f; // Low volume
        
        // Use track pan setting (-1 = left, 0 = center, +1 = right)
        float pan = track->GetPan();
        
        float leftGain = (1.0f - pan) * 0.5f * volume;
        float rightGain = (1.0f + pan) * 0.5f * volume;
        
        // Generate and mix audio + calculate levels
        float peakLevel = 0.0f;
        float rmsSum = 0.0f;
        
        for (size_t i = 0; i < frameCount; i++) {
            float sample = sinf(track->GetPhase()) * 0.5f;
            
            buffer[i * 2] += sample * leftGain;      // Left
            buffer[i * 2 + 1] += sample * rightGain; // Right
            
            // Calculate levels for VU meter display
            // Use track volume only (not master or 0.1 factor) for visual feedback
            float displayLevel = fabsf(sample) * track->GetVolume();
            peakLevel = fmaxf(peakLevel, displayLevel);
            rmsSum += displayLevel * displayLevel;
            
            track->GetPhase() += phaseIncrement;
            if (track->GetPhase() > 2.0f * M_PI) {
                track->GetPhase() -= 2.0f * M_PI;
            }
        }
        
        // Update track levels (smooth decay)
        float newPeak = peakLevel;
        float newRMS = sqrtf(rmsSum / frameCount);
        
        float smoothPeak = fmaxf(newPeak, track->GetPeakLevel() * 0.95f);  // Decay
        float smoothRMS = track->GetRMSLevel() * 0.9f + newRMS * 0.1f;    // Smooth
        
        track->UpdateLevels(smoothPeak, smoothRMS);
    }
    
    // Calculate master levels from final buffer
    // Normalize by the 0.1 factor used in audio processing to get visual levels
    const float displayGain = 10.0f;  // Compensate for the 0.1 factor in volume calculation
    
    for (size_t i = 0; i < frameCount; i++) {
        float leftSample = fabsf(buffer[i * 2]) * displayGain;     // Left channel
        float rightSample = fabsf(buffer[i * 2 + 1]) * displayGain; // Right channel
        
        // Clamp to avoid overly high values
        leftSample = fminf(leftSample, 2.0f);   // Max 2.0 for 200% display
        rightSample = fminf(rightSample, 2.0f);
        
        masterPeakLeft = fmaxf(masterPeakLeft, leftSample);
        masterPeakRight = fmaxf(masterPeakRight, rightSample);
        
        masterRMSLeft += leftSample * leftSample;
        masterRMSRight += rightSample * rightSample;
    }
    
    // Update master levels with smoothing
    masterRMSLeft = sqrtf(masterRMSLeft / frameCount);
    masterRMSRight = sqrtf(masterRMSRight / frameCount);
    
    fMasterPeakLeft = fmaxf(masterPeakLeft, fMasterPeakLeft * 0.95f);  // Decay
    fMasterPeakRight = fmaxf(masterPeakRight, fMasterPeakRight * 0.95f);
    fMasterRMSLeft = fMasterRMSLeft * 0.9f + masterRMSLeft * 0.1f;     // Smooth
    fMasterRMSRight = fMasterRMSRight * 0.9f + masterRMSRight * 0.1f;
}

void SimpleHaikuEngine::SetTrackSolo(int trackIndex, bool solo)
{
    if (trackIndex < 0 || (size_t)trackIndex >= fTracks.size()) {
        return;
    }
    
    SimpleTrack* targetTrack = fTracks[trackIndex];
    
    if (solo) {
        // Turn off all other solo states first
        for (size_t i = 0; i < fTracks.size(); i++) {
            fTracks[i]->SetSolo(false);
        }
        
        // Set this track as solo
        targetTrack->SetSolo(true);
        fSoloTrack = trackIndex;
        
        printf("SimpleHaikuEngine: Track %d ('%s') is now SOLO\n", 
               trackIndex, targetTrack->GetName());
    } else {
        // Turn off solo for this track
        targetTrack->SetSolo(false);
        
        // Check if any other track is still solo
        fSoloTrack = -1;
        for (size_t i = 0; i < fTracks.size(); i++) {
            if (fTracks[i]->IsSolo()) {
                fSoloTrack = i;
                break;
            }
        }
        
        printf("SimpleHaikuEngine: Track %d ('%s') solo OFF. Current solo: %d\n", 
               trackIndex, targetTrack->GetName(), fSoloTrack);
    }
}

} // namespace HaikuDAW
/*
 * TrackChannel.h - Audio channel processor for individual tracks
 *
 * Encapsulates all per-track audio processing:
 * - Volume and panning
 * - Biquad filtering (EQ)
 * - Spatial positioning (3D)
 * - Reverb processing
 * - Mute/Solo state
 */

#ifndef TRACK_CHANNEL_H
#define TRACK_CHANNEL_H

#include "BiquadFilter.h"
#include "SpatialReverb.h"
#include <String.h>

namespace VeniceDAW {

// Forward declarations
class Track3DMix;
struct AudioSampleCache;

/**
 * TrackChannel - Complete audio processing chain for a single track
 *
 * Manages:
 * - Audio sample playback with resampling
 * - EQ (via BiquadFilter)
 * - Spatial audio (3D positioning, ITD)
 * - Reverb send level
 * - Volume/Pan controls
 * - Mute/Solo state
 */
class TrackChannel {
public:
    TrackChannel();
    ~TrackChannel();

    // Configuration
    void SetTrack(Track3DMix* track);
    void SetAudioCache(const AudioSampleCache* cache);
    void SetSampleRate(float sampleRate);

    // Playback control
    void SetMuted(bool muted) { fMuted = muted; }
    void SetSolo(bool solo) { fSolo = solo; }
    bool IsMuted() const { return fMuted; }
    bool IsSolo() const { return fSolo; }

    // Volume and panning
    void SetVolume(float volume);      // 0.0 to 1.0
    void SetPan(float pan);            // -1.0 (left) to +1.0 (right)
    float GetVolume() const { return fVolume; }
    float GetPan() const { return fPan; }

    // EQ filter control
    void SetFilterEnabled(bool enabled) { fFilterEnabled = enabled; }
    void SetFilterType(HaikuDAW::BiquadFilter::FilterMode type);
    void SetFilterFrequency(float frequencyHz);
    void SetFilterQ(float q);
    void SetFilterGain(float gainDb);
    bool IsFilterEnabled() const { return fFilterEnabled; }

    // 3D spatial audio
    void SetPosition3D(float x, float y, float z);
    void GetPosition3D(float* x, float* y, float* z) const;

    // Reverb control
    void SetReverbLevel(float level);   // 0.0 to 1.0 (send amount)
    float GetReverbLevel() const { return fReverbLevel; }

    // Audio processing
    /**
     * Process audio for this track and mix into output buffer
     *
     * @param outputBuffer Stereo interleaved output buffer (L,R,L,R,...)
     * @param frameCount Number of stereo frames to process
     * @param currentTime Current playback time in seconds
     * @param sampleRate Output sample rate
     * @param listenerPos Listener position for 3D audio (x, y, z)
     */
    void ProcessAndMix(float* outputBuffer, int frameCount, float currentTime,
                       float sampleRate, const float* listenerPos);

    // Level metering (post-processing)
    float GetCurrentLevel() const { return fCurrentLevel; }

    // Reset state
    void Reset();

private:
    // Track data
    Track3DMix* fTrack;
    const AudioSampleCache* fAudioCache;
    float fSampleRate;

    // Playback state
    bool fMuted;
    bool fSolo;
    float fVolume;
    float fPan;

    // 3D positioning
    float fPosition3D[3];  // x, y, z

    // Audio processing
    bool fFilterEnabled;
    HaikuDAW::BiquadFilter fFilter;
    float fReverbLevel;
    SpatialReverb fReverb;

    // Level metering
    float fCurrentLevel;

    // Internal helpers
    void CalculateStereoGains(const float* listenerPos, float* leftGain, float* rightGain);
    float CalculateDistanceAttenuation(float distance);
    void ApplyITD(float azimuth, float* leftDelay, float* rightDelay);
};

} // namespace VeniceDAW

#endif // TRACK_CHANNEL_H

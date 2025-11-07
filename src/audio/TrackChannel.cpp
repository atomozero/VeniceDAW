/*
 * TrackChannel.cpp - Implementation of audio channel processor
 */

#include "TrackChannel.h"
#include "Track3DMix.h"
#include "AudioSampleCache.h"
#include <cmath>
#include <cstring>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace VeniceDAW {

// Constants for 3D audio
static const float SPEED_OF_SOUND = 343.0f;  // m/s at 20°C
static const float HEAD_RADIUS = 0.0875f;    // 8.75 cm average human head
static const float MIN_DISTANCE = 0.5f;      // Minimum distance for attenuation (prevents division by zero)

TrackChannel::TrackChannel()
    : fTrack(nullptr)
    , fAudioCache(nullptr)
    , fSampleRate(44100.0f)
    , fMuted(false)
    , fSolo(false)
    , fVolume(1.0f)
    , fPan(0.0f)
    , fFilterEnabled(false)
    , fReverbLevel(0.0f)
    , fCurrentLevel(0.0f)
{
    fPosition3D[0] = 0.0f;
    fPosition3D[1] = 0.0f;
    fPosition3D[2] = 0.0f;

    // Initialize filter with neutral settings
    fFilter.SetSampleRate(fSampleRate);
    fFilter.SetMode(HaikuDAW::BiquadFilter::LOW_PASS);
    fFilter.SetFrequency(20000.0f);  // Passthrough by default
    fFilter.SetQ(0.707f);
}

TrackChannel::~TrackChannel()
{
}

void TrackChannel::SetTrack(Track3DMix* track)
{
    fTrack = track;
}

void TrackChannel::SetAudioCache(const AudioSampleCache* cache)
{
    fAudioCache = cache;
}

void TrackChannel::SetSampleRate(float sampleRate)
{
    if (fSampleRate != sampleRate) {
        fSampleRate = sampleRate;
        fFilter.SetSampleRate(sampleRate);
    }
}

void TrackChannel::SetVolume(float volume)
{
    fVolume = std::max(0.0f, std::min(2.0f, volume));
}

void TrackChannel::SetPan(float pan)
{
    fPan = std::max(-1.0f, std::min(1.0f, pan));
}

void TrackChannel::SetFilterType(HaikuDAW::BiquadFilter::FilterMode type)
{
    fFilter.SetMode(type);
}

void TrackChannel::SetFilterFrequency(float frequencyHz)
{
    fFilter.SetFrequency(frequencyHz);
}

void TrackChannel::SetFilterQ(float q)
{
    fFilter.SetQ(q);
}

void TrackChannel::SetFilterGain(float gainDb)
{
    fFilter.SetGain(gainDb);
}

void TrackChannel::SetPosition3D(float x, float y, float z)
{
    fPosition3D[0] = x;
    fPosition3D[1] = y;
    fPosition3D[2] = z;
}

void TrackChannel::GetPosition3D(float* x, float* y, float* z) const
{
    if (x) *x = fPosition3D[0];
    if (y) *y = fPosition3D[1];
    if (z) *z = fPosition3D[2];
}

void TrackChannel::SetReverbLevel(float level)
{
    fReverbLevel = std::max(0.0f, std::min(1.0f, level));
}

void TrackChannel::Reset()
{
    fFilter.Reset();
    fCurrentLevel = 0.0f;
}

void TrackChannel::ProcessAndMix(float* outputBuffer, int frameCount, float currentTime,
                                   float sampleRate, const float* listenerPos)
{
    // Early exit conditions
    if (!fTrack || !fAudioCache || !fAudioCache->isValid || fAudioCache->samples.empty()) {
        fCurrentLevel = 0.0f;
        return;
    }

    if (fMuted) {
        fCurrentLevel = 0.0f;
        return;
    }

    // Calculate track start time
    float trackStartTime = fTrack->StartPosition() / fAudioCache->sampleRate;
    float relativeTime = currentTime - trackStartTime;

    if (relativeTime < 0.0f) {
        fCurrentLevel = 0.0f;
        return;  // Track hasn't started yet
    }

    // Calculate stereo gains based on 3D position and pan
    float leftGain, rightGain;
    CalculateStereoGains(listenerPos, &leftGain, &rightGain);

    // Apply volume
    leftGain *= fVolume;
    rightGain *= fVolume;

    // Calculate sample position in audio file
    float samplePosition = relativeTime * fAudioCache->sampleRate;
    int sampleIndex = (int)samplePosition;

    // Handle end of track
    int totalSamples = fAudioCache->samples.size() / 2;  // Stereo samples
    if (sampleIndex >= totalSamples) {
        fCurrentLevel = 0.0f;
        return;
    }

    // Temporary buffer for processing
    float tempBuffer[2048];  // Stereo interleaved
    int maxFrames = std::min(frameCount, 1024);

    float peakLevel = 0.0f;

    // Process audio in chunks
    for (int frame = 0; frame < frameCount && sampleIndex < totalSamples; frame++) {
        // Linear interpolation for resampling
        float frac = samplePosition - (float)sampleIndex;
        int nextIndex = std::min(sampleIndex + 1, totalSamples - 1);

        // Get stereo samples (cache is interleaved L,R,L,R...)
        int cacheIdx = sampleIndex * 2;
        int nextCacheIdx = nextIndex * 2;

        float leftSample = fAudioCache->samples[cacheIdx] * (1.0f - frac) +
                          fAudioCache->samples[nextCacheIdx] * frac;
        float rightSample = fAudioCache->samples[cacheIdx + 1] * (1.0f - frac) +
                           fAudioCache->samples[nextCacheIdx + 1] * frac;

        // Apply EQ filter if enabled
        if (fFilterEnabled) {
            leftSample = fFilter.Process(leftSample);
            rightSample = fFilter.Process(rightSample);
        }

        // Apply stereo gains (includes 3D positioning and pan)
        float outputLeft = leftSample * leftGain;
        float outputRight = rightSample * rightGain;

        // Mix into output buffer (stereo interleaved)
        int outputIdx = frame * 2;
        outputBuffer[outputIdx] += outputLeft;
        outputBuffer[outputIdx + 1] += outputRight;

        // Track peak level for metering
        float level = std::max(std::abs(outputLeft), std::abs(outputRight));
        if (level > peakLevel) peakLevel = level;

        // Advance sample position
        samplePosition += fAudioCache->sampleRate / sampleRate;
        sampleIndex = (int)samplePosition;
    }

    fCurrentLevel = peakLevel;
}

void TrackChannel::CalculateStereoGains(const float* listenerPos, float* leftGain, float* rightGain)
{
    // Calculate vector from listener to sound source
    float dx = fPosition3D[0] - (listenerPos ? listenerPos[0] : 0.0f);
    float dy = fPosition3D[1] - (listenerPos ? listenerPos[1] : 0.0f);
    float dz = fPosition3D[2] - (listenerPos ? listenerPos[2] : 0.0f);

    // Calculate distance
    float distance = sqrtf(dx * dx + dy * dy + dz * dz);

    // Apply distance attenuation
    float attenuation = CalculateDistanceAttenuation(distance);

    // Calculate azimuth angle (horizontal angle in x-y plane)
    // atan2(x, y) gives angle where: 0 = front, PI/2 = right, -PI/2 = left
    float azimuth = atan2f(dx, dy);

    // Simple pan law based on azimuth
    // -PI/2 (left) -> leftGain=1.0, rightGain=0.0
    //  0    (front) -> leftGain=0.707, rightGain=0.707 (-3dB pan law)
    // +PI/2 (right) -> leftGain=0.0, rightGain=1.0
    float panAngle = azimuth / (M_PI / 2.0f);  // Normalize to -1...+1

    // Apply constant power pan law
    float angle = (panAngle + 1.0f) * M_PI / 4.0f;  // 0...PI/2
    *leftGain = cosf(angle) * attenuation;
    *rightGain = sinf(angle) * attenuation;

    // Apply manual pan control
    if (fPan < 0.0f) {
        // Pan left: reduce right channel
        *rightGain *= (1.0f + fPan);
    } else if (fPan > 0.0f) {
        // Pan right: reduce left channel
        *leftGain *= (1.0f - fPan);
    }
}

float TrackChannel::CalculateDistanceAttenuation(float distance)
{
    // Prevent division by zero with minimum distance
    float effectiveDistance = std::max(distance, MIN_DISTANCE);

    // Inverse square law with some limiting
    float attenuation = 1.0f / (effectiveDistance * effectiveDistance);

    // Limit maximum gain to prevent extreme values at very short distances
    return std::min(attenuation, 4.0f);
}

void TrackChannel::ApplyITD(float azimuth, float* leftDelay, float* rightDelay)
{
    // Interaural Time Difference calculation
    // Maximum delay occurs at ±90 degrees (sound directly from side)
    float maxDelay = HEAD_RADIUS / SPEED_OF_SOUND;  // ~0.255 ms
    float delay = maxDelay * sinf(azimuth);

    if (delay > 0.0f) {
        // Sound from right: delay left ear
        *leftDelay = delay;
        *rightDelay = 0.0f;
    } else {
        // Sound from left: delay right ear
        *leftDelay = 0.0f;
        *rightDelay = -delay;
    }
}

} // namespace VeniceDAW

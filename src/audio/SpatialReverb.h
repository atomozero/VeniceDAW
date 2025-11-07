/*
 * SpatialReverb.h - Distance-based spatial reverb processor
 *
 * Implements algorithmic reverb with 3D distance modulation:
 * - Early reflections (discrete delays)
 * - Late reverberation (diffuse tail)
 * - Distance-based wet/dry mix
 * - High-frequency damping
 */

#ifndef SPATIAL_REVERB_H
#define SPATIAL_REVERB_H

#include <cstddef>

namespace VeniceDAW {

/**
 * SpatialReverb - Distance-aware reverb processor
 *
 * Features:
 * - Schroeder reverberator algorithm (4 comb filters + 2 allpass filters)
 * - Distance-based reverb amount (farther = more reverb)
 * - Damping filter for natural sound
 * - Stereo output from mono/stereo input
 *
 * Usage:
 *   SpatialReverb reverb;
 *   reverb.SetSampleRate(44100.0f);
 *   reverb.SetRoomSize(0.7f);    // 0.0 to 1.0
 *   reverb.SetDamping(0.5f);     // 0.0 to 1.0
 *
 *   float distance = 5.0f;  // meters
 *   float wetAmount = reverb.CalculateWetAmount(distance);
 *   reverb.ProcessStereo(leftIn, rightIn, leftOut, rightOut, frameCount, wetAmount);
 */
class SpatialReverb {
public:
    SpatialReverb();
    ~SpatialReverb();

    // Configuration
    void SetSampleRate(float sampleRate);
    void SetRoomSize(float size);      // 0.0 (small) to 1.0 (large)
    void SetDamping(float damping);    // 0.0 (bright) to 1.0 (dark)
    void SetWidth(float width);        // 0.0 (mono) to 1.0 (wide stereo)

    // Distance calculation
    float CalculateWetAmount(float distance);  // Returns 0.0 to 1.0 based on distance

    // Audio processing
    void ProcessMono(const float* input, float* leftOut, float* rightOut,
                     int frameCount, float wetAmount);
    void ProcessStereo(const float* leftIn, const float* rightIn,
                       float* leftOut, float* rightOut,
                       int frameCount, float wetAmount);

    // State management
    void Reset();  // Clear all delay buffers

    // Parameter getters
    float GetRoomSize() const { return fRoomSize; }
    float GetDamping() const { return fDamping; }
    float GetWidth() const { return fWidth; }

private:
    // Parameters
    float fSampleRate;
    float fRoomSize;
    float fDamping;
    float fWidth;

    // Delay line (circular buffer)
    class DelayLine {
    public:
        DelayLine();
        ~DelayLine();
        void SetSize(int samples);
        void Write(float sample);
        float Read(int delaySamples);
        void Clear();

    private:
        float* fBuffer;
        int fSize;
        int fWritePos;
    };

    // Comb filter (for dense reverb tail)
    class CombFilter {
    public:
        CombFilter();
        void SetBufferSize(int size);
        void SetDamping(float damping);
        void SetFeedback(float feedback);
        float Process(float input);
        void Clear();

    private:
        DelayLine fDelay;
        float fFeedback;
        float fDamping;
        float fFilterStore;
        int fBufferSize;
    };

    // Allpass filter (for diffusion)
    class AllpassFilter {
    public:
        AllpassFilter();
        void SetBufferSize(int size);
        void SetFeedback(float feedback);
        float Process(float input);
        void Clear();

    private:
        DelayLine fDelay;
        float fFeedback;
        int fBufferSize;
    };

    // Filter instances (Schroeder topology)
    static const int NUM_COMBS = 8;      // 4 per channel
    static const int NUM_ALLPASSES = 4;  // 2 per channel

    CombFilter fCombL[NUM_COMBS / 2];
    CombFilter fCombR[NUM_COMBS / 2];
    AllpassFilter fAllpassL[NUM_ALLPASSES / 2];
    AllpassFilter fAllpassR[NUM_ALLPASSES / 2];

    // Distance parameters
    float fMinDistance;  // Below this, no reverb
    float fMaxDistance;  // Above this, maximum reverb

    // Internal methods
    void UpdateFilters();
    void UpdateDamping();
};

} // namespace VeniceDAW

#endif // SPATIAL_REVERB_H

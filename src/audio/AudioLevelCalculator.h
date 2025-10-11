/*
 * AudioLevelCalculator.h - High-performance audio level calculation
 * SIMD-optimized peak and RMS detection for real-time audio
 */

#ifndef AUDIO_LEVEL_CALCULATOR_H
#define AUDIO_LEVEL_CALCULATOR_H

#include <stdint.h>
#include <stddef.h>
#include <OS.h>  // For Haiku types like uint32

namespace VeniceDAW {

/*
 * Audio level measurement results
 */
struct AudioLevels {
    float peak;          // Peak level (0.0 - 1.0+)
    float rms;           // RMS level (0.0 - 1.0+)
    float peakLeft;      // Left channel peak (for stereo)
    float peakRight;     // Right channel peak (for stereo)
    float rmsLeft;       // Left channel RMS (for stereo)
    float rmsRight;      // Right channel RMS (for stereo)
    bool clipping;       // True if any sample >= 1.0

    AudioLevels() : peak(0.0f), rms(0.0f), peakLeft(0.0f), peakRight(0.0f),
                   rmsLeft(0.0f), rmsRight(0.0f), clipping(false) {}
};

/*
 * High-performance audio level calculator
 * Uses SIMD instructions when available for maximum speed
 */
class AudioLevelCalculator {
public:
    AudioLevelCalculator();
    ~AudioLevelCalculator();

    // Calculate levels for different formats
    AudioLevels CalculateLevels(const float* samples, size_t sampleCount, uint32 channels);
    AudioLevels CalculateMonoLevels(const float* samples, size_t sampleCount);
    AudioLevels CalculateStereoLevels(const float* samples, size_t frameCount);

    // Optimized implementations
    AudioLevels CalculateLevelsSSE2(const float* samples, size_t sampleCount, uint32 channels);
    AudioLevels CalculateLevelsAVX(const float* samples, size_t sampleCount, uint32 channels);
    AudioLevels CalculateLevelsScalar(const float* samples, size_t sampleCount, uint32 channels);

    // Feature detection
    static bool HasSSE2Support();
    static bool HasAVXSupport();
    static const char* GetOptimizationLevel();

    // Utility functions
    static float ConvertToDecibels(float linear);
    static float ConvertFromDecibels(float db);
    static bool IsClipping(float sample, float threshold = 0.99f);

private:
    enum OptimizationLevel {
        OPT_SCALAR,
        OPT_SSE2,
        OPT_AVX
    };

    OptimizationLevel fOptLevel;
    bool fSSE2Available;
    bool fAVXAvailable;

    // SIMD-specific implementations
    AudioLevels CalculateMonoSSE2(const float* samples, size_t sampleCount);
    AudioLevels CalculateStereoSSE2(const float* samples, size_t frameCount);
    AudioLevels CalculateMonoAVX(const float* samples, size_t sampleCount);
    AudioLevels CalculateStereoAVX(const float* samples, size_t frameCount);

    // Fallback scalar implementations
    AudioLevels CalculateMonoScalar(const float* samples, size_t sampleCount);
    AudioLevels CalculateStereoScalar(const float* samples, size_t frameCount);

    // CPU feature detection
    void DetectCPUFeatures();
    static bool CheckSSE2();
    static bool CheckAVX();
};

/*
 * Fast inline level calculations for simple cases
 */
namespace FastAudio {

    // Ultra-fast peak detection (no RMS, no SIMD detection overhead)
    inline float FastPeak(const float* samples, size_t count) {
        float peak = 0.0f;
        for (size_t i = 0; i < count; i++) {
            float abs_sample = samples[i] >= 0.0f ? samples[i] : -samples[i];
            if (abs_sample > peak) {
                peak = abs_sample;
            }
        }
        return peak;
    }

    // Fast stereo peak detection
    inline void FastStereoPeak(const float* samples, size_t frameCount, float& peakLeft, float& peakRight) {
        peakLeft = peakRight = 0.0f;
        for (size_t i = 0; i < frameCount; i++) {
            float leftAbs = samples[i * 2] >= 0.0f ? samples[i * 2] : -samples[i * 2];
            float rightAbs = samples[i * 2 + 1] >= 0.0f ? samples[i * 2 + 1] : -samples[i * 2 + 1];

            if (leftAbs > peakLeft) peakLeft = leftAbs;
            if (rightAbs > peakRight) peakRight = rightAbs;
        }
    }

    // Fast clipping detection
    inline bool FastClippingCheck(const float* samples, size_t count, float threshold = 0.99f) {
        for (size_t i = 0; i < count; i++) {
            float abs_sample = samples[i] >= 0.0f ? samples[i] : -samples[i];
            if (abs_sample >= threshold) {
                return true;
            }
        }
        return false;
    }

} // namespace FastAudio

/*
 * Global level calculator instance for performance
 */
extern AudioLevelCalculator* g_levelCalculator;

// Convenience functions
AudioLevels CalculateAudioLevels(const float* samples, size_t sampleCount, uint32 channels);
float CalculateAudioPeak(const float* samples, size_t sampleCount);
AudioLevels CalculateStereoAudioLevels(const float* samples, size_t frameCount);

} // namespace VeniceDAW

#endif
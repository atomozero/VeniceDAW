/*
 * AudioLevelCalculator.cpp - SIMD-optimized audio level calculation
 */

#include "AudioLevelCalculator.h"
#include "AudioLogging.h"
#include <math.h>
#include <string.h>
#include <algorithm>

// Include SIMD headers if available on Haiku x86/x64
#if defined(__i386__) || defined(__x86_64__)
    #include <xmmintrin.h>  // SSE
    #include <emmintrin.h>  // SSE2
    #ifdef __AVX__
        #include <immintrin.h>  // AVX
    #endif
#endif

namespace VeniceDAW {

// Global instance
AudioLevelCalculator* g_levelCalculator = nullptr;

AudioLevelCalculator::AudioLevelCalculator()
    : fOptLevel(OPT_SCALAR)
    , fSSE2Available(false)
    , fAVXAvailable(false)
{
    DetectCPUFeatures();

    // Log detected optimization level
    AUDIO_LOG_INFO("AudioLevelCalculator", "Initialized with %s optimization", GetOptimizationLevel());
}

AudioLevelCalculator::~AudioLevelCalculator()
{
}

AudioLevels AudioLevelCalculator::CalculateLevels(const float* samples, size_t sampleCount, uint32 channels)
{
    if (!samples || sampleCount == 0) {
        return AudioLevels();
    }

    // Use the best available optimization
    switch (fOptLevel) {
        case OPT_AVX:
            return CalculateLevelsAVX(samples, sampleCount, channels);
        case OPT_SSE2:
            return CalculateLevelsSSE2(samples, sampleCount, channels);
        default:
            return CalculateLevelsScalar(samples, sampleCount, channels);
    }
}

AudioLevels AudioLevelCalculator::CalculateMonoLevels(const float* samples, size_t sampleCount)
{
    switch (fOptLevel) {
        case OPT_AVX:
            return CalculateMonoAVX(samples, sampleCount);
        case OPT_SSE2:
            return CalculateMonoSSE2(samples, sampleCount);
        default:
            return CalculateMonoScalar(samples, sampleCount);
    }
}

AudioLevels AudioLevelCalculator::CalculateStereoLevels(const float* samples, size_t frameCount)
{
    switch (fOptLevel) {
        case OPT_AVX:
            return CalculateStereoAVX(samples, frameCount);
        case OPT_SSE2:
            return CalculateStereoSSE2(samples, frameCount);
        default:
            return CalculateStereoScalar(samples, frameCount);
    }
}

// =====================================
// SSE2 Optimized Implementations
// =====================================

AudioLevels AudioLevelCalculator::CalculateMonoSSE2(const float* samples, size_t sampleCount)
{
#if defined(__i386__) || defined(__x86_64__)
    AudioLevels levels;

    if (sampleCount < 4) {
        return CalculateMonoScalar(samples, sampleCount);
    }

    __m128 peak_vec = _mm_setzero_ps();
    __m128 sum_vec = _mm_setzero_ps();
    const __m128 abs_mask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
    const __m128 clipping_threshold = _mm_set1_ps(0.99f);
    __m128 clipping_vec = _mm_setzero_ps();

    size_t simd_count = sampleCount & ~3;  // Process in groups of 4

    for (size_t i = 0; i < simd_count; i += 4) {
        __m128 samples_vec = _mm_loadu_ps(&samples[i]);
        __m128 abs_samples = _mm_and_ps(samples_vec, abs_mask);

        // Update peak
        peak_vec = _mm_max_ps(peak_vec, abs_samples);

        // Update RMS sum
        __m128 squared = _mm_mul_ps(samples_vec, samples_vec);
        sum_vec = _mm_add_ps(sum_vec, squared);

        // Check clipping
        __m128 clipping_mask = _mm_cmpge_ps(abs_samples, clipping_threshold);
        clipping_vec = _mm_or_ps(clipping_vec, clipping_mask);
    }

    // Extract peak from SIMD register
    float peak_array[4];
    _mm_storeu_ps(peak_array, peak_vec);
    levels.peak = std::max({peak_array[0], peak_array[1], peak_array[2], peak_array[3]});

    // Extract RMS sum
    float sum_array[4];
    _mm_storeu_ps(sum_array, sum_vec);
    float total_sum = sum_array[0] + sum_array[1] + sum_array[2] + sum_array[3];

    // Check clipping
    float clipping_array[4];
    _mm_storeu_ps(clipping_array, clipping_vec);
    levels.clipping = (clipping_array[0] != 0.0f || clipping_array[1] != 0.0f ||
                      clipping_array[2] != 0.0f || clipping_array[3] != 0.0f);

    // Process remaining samples
    for (size_t i = simd_count; i < sampleCount; i++) {
        float abs_sample = fabsf(samples[i]);
        levels.peak = std::max(levels.peak, abs_sample);
        total_sum += samples[i] * samples[i];
        if (abs_sample >= 0.99f) levels.clipping = true;
    }

    // Calculate RMS
    levels.rms = sqrtf(total_sum / sampleCount);

    return levels;
#else
    return CalculateMonoScalar(samples, sampleCount);
#endif
}

AudioLevels AudioLevelCalculator::CalculateStereoSSE2(const float* samples, size_t frameCount)
{
#if defined(__i386__) || defined(__x86_64__)
    AudioLevels levels;

    if (frameCount < 2) {
        return CalculateStereoScalar(samples, frameCount);
    }

    __m128 peak_left_vec = _mm_setzero_ps();
    __m128 peak_right_vec = _mm_setzero_ps();
    __m128 sum_left_vec = _mm_setzero_ps();
    __m128 sum_right_vec = _mm_setzero_ps();
    const __m128 abs_mask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
    const __m128 clipping_threshold = _mm_set1_ps(0.99f);
    __m128 clipping_vec = _mm_setzero_ps();

    size_t simd_frames = frameCount & ~1;  // Process in groups of 2 frames (4 samples)

    for (size_t i = 0; i < simd_frames; i += 2) {
        // Load 4 samples: L1, R1, L2, R2
        __m128 samples_vec = _mm_loadu_ps(&samples[i * 2]);
        __m128 abs_samples = _mm_and_ps(samples_vec, abs_mask);

        // Separate left and right channels
        __m128 left_samples = _mm_shuffle_ps(samples_vec, samples_vec, _MM_SHUFFLE(2, 0, 2, 0));  // L1, L1, L2, L2
        __m128 right_samples = _mm_shuffle_ps(samples_vec, samples_vec, _MM_SHUFFLE(3, 1, 3, 1)); // R1, R1, R2, R2
        __m128 abs_left = _mm_shuffle_ps(abs_samples, abs_samples, _MM_SHUFFLE(2, 0, 2, 0));
        __m128 abs_right = _mm_shuffle_ps(abs_samples, abs_samples, _MM_SHUFFLE(3, 1, 3, 1));

        // Update peaks
        peak_left_vec = _mm_max_ps(peak_left_vec, abs_left);
        peak_right_vec = _mm_max_ps(peak_right_vec, abs_right);

        // Update RMS sums
        sum_left_vec = _mm_add_ps(sum_left_vec, _mm_mul_ps(left_samples, left_samples));
        sum_right_vec = _mm_add_ps(sum_right_vec, _mm_mul_ps(right_samples, right_samples));

        // Check clipping
        __m128 clipping_mask = _mm_cmpge_ps(abs_samples, clipping_threshold);
        clipping_vec = _mm_or_ps(clipping_vec, clipping_mask);
    }

    // Extract results
    float peak_left_array[4], peak_right_array[4];
    _mm_storeu_ps(peak_left_array, peak_left_vec);
    _mm_storeu_ps(peak_right_array, peak_right_vec);

    levels.peakLeft = std::max({peak_left_array[0], peak_left_array[1], peak_left_array[2], peak_left_array[3]});
    levels.peakRight = std::max({peak_right_array[0], peak_right_array[1], peak_right_array[2], peak_right_array[3]});

    float sum_left_array[4], sum_right_array[4];
    _mm_storeu_ps(sum_left_array, sum_left_vec);
    _mm_storeu_ps(sum_right_array, sum_right_vec);

    float total_sum_left = sum_left_array[0] + sum_left_array[1] + sum_left_array[2] + sum_left_array[3];
    float total_sum_right = sum_right_array[0] + sum_right_array[1] + sum_right_array[2] + sum_right_array[3];

    // Check clipping
    float clipping_array[4];
    _mm_storeu_ps(clipping_array, clipping_vec);
    levels.clipping = (clipping_array[0] != 0.0f || clipping_array[1] != 0.0f ||
                      clipping_array[2] != 0.0f || clipping_array[3] != 0.0f);

    // Process remaining frames
    for (size_t i = simd_frames; i < frameCount; i++) {
        float leftAbs = fabsf(samples[i * 2]);
        float rightAbs = fabsf(samples[i * 2 + 1]);

        levels.peakLeft = std::max(levels.peakLeft, leftAbs);
        levels.peakRight = std::max(levels.peakRight, rightAbs);

        total_sum_left += samples[i * 2] * samples[i * 2];
        total_sum_right += samples[i * 2 + 1] * samples[i * 2 + 1];

        if (leftAbs >= 0.99f || rightAbs >= 0.99f) levels.clipping = true;
    }

    // Calculate final values
    levels.rmsLeft = sqrtf(total_sum_left / frameCount);
    levels.rmsRight = sqrtf(total_sum_right / frameCount);
    levels.peak = std::max(levels.peakLeft, levels.peakRight);
    levels.rms = std::max(levels.rmsLeft, levels.rmsRight);

    return levels;
#else
    return CalculateStereoScalar(samples, frameCount);
#endif
}

// =====================================
// Scalar Fallback Implementations
// =====================================

AudioLevels AudioLevelCalculator::CalculateMonoScalar(const float* samples, size_t sampleCount)
{
    AudioLevels levels;

    if (!samples || sampleCount == 0) {
        return levels;
    }

    double sum_squares = 0.0;

    for (size_t i = 0; i < sampleCount; i++) {
        float abs_sample = fabsf(samples[i]);

        // Update peak
        if (abs_sample > levels.peak) {
            levels.peak = abs_sample;
        }

        // Update RMS sum
        sum_squares += samples[i] * samples[i];

        // Check clipping
        if (abs_sample >= 0.99f) {
            levels.clipping = true;
        }
    }

    // Calculate RMS
    levels.rms = sqrtf(sum_squares / sampleCount);

    return levels;
}

AudioLevels AudioLevelCalculator::CalculateStereoScalar(const float* samples, size_t frameCount)
{
    AudioLevels levels;

    if (!samples || frameCount == 0) {
        return levels;
    }

    double sum_left = 0.0, sum_right = 0.0;

    for (size_t i = 0; i < frameCount; i++) {
        float left = samples[i * 2];
        float right = samples[i * 2 + 1];
        float leftAbs = fabsf(left);
        float rightAbs = fabsf(right);

        // Update peaks
        if (leftAbs > levels.peakLeft) levels.peakLeft = leftAbs;
        if (rightAbs > levels.peakRight) levels.peakRight = rightAbs;

        // Update RMS sums
        sum_left += left * left;
        sum_right += right * right;

        // Check clipping
        if (leftAbs >= 0.99f || rightAbs >= 0.99f) {
            levels.clipping = true;
        }
    }

    // Calculate final values
    levels.rmsLeft = sqrtf(sum_left / frameCount);
    levels.rmsRight = sqrtf(sum_right / frameCount);
    levels.peak = std::max(levels.peakLeft, levels.peakRight);
    levels.rms = std::max(levels.rmsLeft, levels.rmsRight);

    return levels;
}

// =====================================
// AVX Implementations (placeholder)
// =====================================

AudioLevels AudioLevelCalculator::CalculateMonoAVX(const float* samples, size_t sampleCount)
{
    // AVX implementation would be similar to SSE2 but with 8-wide vectors
    // For now, fall back to SSE2
    return CalculateMonoSSE2(samples, sampleCount);
}

AudioLevels AudioLevelCalculator::CalculateStereoAVX(const float* samples, size_t frameCount)
{
    // AVX implementation would be similar to SSE2 but with 8-wide vectors
    // For now, fall back to SSE2
    return CalculateStereoSSE2(samples, frameCount);
}

AudioLevels AudioLevelCalculator::CalculateLevelsSSE2(const float* samples, size_t sampleCount, uint32 channels)
{
    if (channels == 1) {
        return CalculateMonoSSE2(samples, sampleCount);
    } else if (channels == 2) {
        return CalculateStereoSSE2(samples, sampleCount / 2);
    } else {
        return CalculateLevelsScalar(samples, sampleCount, channels);
    }
}

AudioLevels AudioLevelCalculator::CalculateLevelsAVX(const float* samples, size_t sampleCount, uint32 channels)
{
    if (channels == 1) {
        return CalculateMonoAVX(samples, sampleCount);
    } else if (channels == 2) {
        return CalculateStereoAVX(samples, sampleCount / 2);
    } else {
        return CalculateLevelsScalar(samples, sampleCount, channels);
    }
}

AudioLevels AudioLevelCalculator::CalculateLevelsScalar(const float* samples, size_t sampleCount, uint32 channels)
{
    if (channels == 1) {
        return CalculateMonoScalar(samples, sampleCount);
    } else if (channels == 2) {
        return CalculateStereoScalar(samples, sampleCount / 2);
    } else {
        // Generic multi-channel fallback
        return CalculateMonoScalar(samples, sampleCount);
    }
}

// =====================================
// CPU Feature Detection
// =====================================

void AudioLevelCalculator::DetectCPUFeatures()
{
    fSSE2Available = CheckSSE2();
    fAVXAvailable = CheckAVX();

    if (fAVXAvailable) {
        fOptLevel = OPT_AVX;
    } else if (fSSE2Available) {
        fOptLevel = OPT_SSE2;
    } else {
        fOptLevel = OPT_SCALAR;
    }
}

bool AudioLevelCalculator::CheckSSE2()
{
#if defined(__i386__) || defined(__x86_64__)
    // SSE2 is standard on most x86/x64 systems
    return true;
#else
    return false;
#endif
}

bool AudioLevelCalculator::CheckAVX()
{
#ifdef __AVX__
    return true;
#else
    return false;
#endif
}

bool AudioLevelCalculator::HasSSE2Support()
{
    return CheckSSE2();
}

bool AudioLevelCalculator::HasAVXSupport()
{
    return CheckAVX();
}

const char* AudioLevelCalculator::GetOptimizationLevel()
{
    static AudioLevelCalculator temp;
    switch (temp.fOptLevel) {
        case OPT_AVX: return "AVX";
        case OPT_SSE2: return "SSE2";
        default: return "Scalar";
    }
}

// =====================================
// Utility Functions
// =====================================

float AudioLevelCalculator::ConvertToDecibels(float linear)
{
    if (linear <= 0.0f) {
        return -100.0f;  // -100dB as "silence"
    }
    return 20.0f * log10f(linear);
}

float AudioLevelCalculator::ConvertFromDecibels(float db)
{
    return powf(10.0f, db / 20.0f);
}

bool AudioLevelCalculator::IsClipping(float sample, float threshold)
{
    return fabsf(sample) >= threshold;
}

// =====================================
// Global Functions
// =====================================

AudioLevels CalculateAudioLevels(const float* samples, size_t sampleCount, uint32 channels)
{
    if (!g_levelCalculator) {
        g_levelCalculator = new AudioLevelCalculator();
    }
    return g_levelCalculator->CalculateLevels(samples, sampleCount, channels);
}

float CalculateAudioPeak(const float* samples, size_t sampleCount)
{
    return FastAudio::FastPeak(samples, sampleCount);
}

AudioLevels CalculateStereoAudioLevels(const float* samples, size_t frameCount)
{
    if (!g_levelCalculator) {
        g_levelCalculator = new AudioLevelCalculator();
    }
    return g_levelCalculator->CalculateStereoLevels(samples, frameCount);
}

} // namespace VeniceDAW
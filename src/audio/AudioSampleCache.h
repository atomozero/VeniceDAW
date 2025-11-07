/*
 * AudioSampleCache.h - Audio sample cache with editing operations
 *
 * Features:
 * - Stereo int16 sample storage
 * - Time-based waveform sampling (for visualization)
 * - Audio editing operations:
 *   - Resize: Change audio length (stretch/compress)
 *   - TimeStretch: Change tempo without pitch change
 *   - PitchShift: Change pitch without tempo change
 *   - Normalize: Adjust gain to target level
 *   - Fade: Fade in/out effects
 */

#ifndef AUDIO_SAMPLE_CACHE_H
#define AUDIO_SAMPLE_CACHE_H

#include <vector>
#include <cstdint>

namespace VeniceDAW {

/**
 * AudioSampleCache - Container for stereo audio samples with editing operations
 *
 * Storage format: Interleaved stereo int16 (L,R,L,R,...)
 *
 * Editing operations:
 * - Resize: Linear resampling to new length
 * - TimeStretch: WSOLA algorithm (waveform similarity overlap-add)
 * - PitchShift: Resampling with time correction
 * - Normalize: Peak or RMS normalization
 * - Fade: Linear or exponential fade curves
 */
struct AudioSampleCache {
    std::vector<int16_t> samples;    // Stereo int16 samples (L,R,L,R,...)
    float sampleRate;                // Sample rate (e.g., 44100)
    int channels;                    // Number of channels (always 2)
    bool isValid;

    AudioSampleCache();

    // Waveform visualization (R6-style on-the-fly calculation)
    void GetSample(float time, float duration, float* outMin, float* outMax) const;

    // Duration helpers
    float GetDuration() const;       // Total duration in seconds
    int GetFrameCount() const;       // Total stereo frames

    // Editing operations
    /**
     * Resize - Change audio length using linear resampling
     *
     * @param newLengthSeconds New length in seconds
     * @return true on success
     */
    bool Resize(float newLengthSeconds);

    /**
     * TimeStretch - Change tempo without pitch (WSOLA algorithm)
     *
     * @param ratio Time stretch ratio (0.5 = half speed, 2.0 = double speed)
     * @return true on success
     */
    bool TimeStretch(float ratio);

    /**
     * PitchShift - Change pitch without tempo
     *
     * @param semitones Pitch shift in semitones (-12 to +12)
     * @return true on success
     */
    bool PitchShift(float semitones);

    /**
     * Normalize - Adjust gain to target level
     *
     * @param targetLevel Target peak level (0.0 to 1.0, default 0.9)
     * @param useRMS Use RMS normalization instead of peak
     * @return true on success
     */
    bool Normalize(float targetLevel = 0.9f, bool useRMS = false);

    /**
     * FadeIn - Apply fade in effect
     *
     * @param durationSeconds Fade duration in seconds
     * @param exponential Use exponential curve (default linear)
     * @return true on success
     */
    bool FadeIn(float durationSeconds, bool exponential = false);

    /**
     * FadeOut - Apply fade out effect
     *
     * @param durationSeconds Fade duration in seconds
     * @param exponential Use exponential curve (default linear)
     * @return true on success
     */
    bool FadeOut(float durationSeconds, bool exponential = false);

    /**
     * Reverse - Reverse audio samples
     *
     * @return true on success
     */
    bool Reverse();

private:
    // Internal helpers
    void ConvertToFloat(std::vector<float>& output) const;
    void ConvertFromFloat(const std::vector<float>& input);

    // WSOLA helper for time stretching
    int FindBestMatch(const std::vector<float>& buffer, int pos, int searchStart,
                      int searchEnd, int templateSize);
};

} // namespace VeniceDAW

#endif // AUDIO_SAMPLE_CACHE_H

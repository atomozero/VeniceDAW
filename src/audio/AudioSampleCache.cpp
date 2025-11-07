/*
 * AudioSampleCache.cpp - Implementation of audio sample cache operations
 */

#include "AudioSampleCache.h"
#include <cmath>
#include <algorithm>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace VeniceDAW {

// Constants for DSP operations
static const int16_t INT16_MAX_VALUE = 32767;
static const int16_t INT16_MIN_VALUE = -32768;
static const float INT16_TO_FLOAT = 1.0f / 32768.0f;
static const float FLOAT_TO_INT16 = 32767.0f;

AudioSampleCache::AudioSampleCache()
    : sampleRate(0.0f)
    , channels(2)
    , isValid(false)
{
}

void AudioSampleCache::GetSample(float time, float duration, float* outMin, float* outMax) const
{
    *outMin = 0.0f;
    *outMax = 0.0f;

    if (!isValid || samples.empty() || time < 0) {
        return;
    }

    // Calculate sample range for the time window
    int startSample = (int)(time * sampleRate) * channels;
    int endSample = (int)((time + duration) * sampleRate) * channels;

    if (startSample >= (int)samples.size()) {
        return;
    }

    endSample = std::min(endSample, (int)samples.size());

    // Find min/max in range (average L+R channels for stereo)
    float minVal = 0.0f;
    float maxVal = 0.0f;

    for (int i = startSample; i < endSample; i += channels) {
        // Average stereo channels
        float avgSample = (samples[i] + samples[i + 1]) * 0.5f * INT16_TO_FLOAT;
        minVal = std::min(minVal, avgSample);
        maxVal = std::max(maxVal, avgSample);
    }

    *outMin = minVal;
    *outMax = maxVal;
}

float AudioSampleCache::GetDuration() const
{
    if (!isValid || sampleRate <= 0.0f || samples.empty()) {
        return 0.0f;
    }
    return (float)(samples.size() / channels) / sampleRate;
}

int AudioSampleCache::GetFrameCount() const
{
    return samples.size() / channels;
}

void AudioSampleCache::ConvertToFloat(std::vector<float>& output) const
{
    output.resize(samples.size());
    for (size_t i = 0; i < samples.size(); i++) {
        output[i] = samples[i] * INT16_TO_FLOAT;
    }
}

void AudioSampleCache::ConvertFromFloat(const std::vector<float>& input)
{
    samples.resize(input.size());
    for (size_t i = 0; i < input.size(); i++) {
        float sample = input[i] * FLOAT_TO_INT16;
        sample = std::max(-32768.0f, std::min(32767.0f, sample));
        samples[i] = (int16_t)sample;
    }
}

bool AudioSampleCache::Resize(float newLengthSeconds)
{
    if (!isValid || newLengthSeconds <= 0.0f) {
        return false;
    }

    int newFrameCount = (int)(newLengthSeconds * sampleRate);
    int oldFrameCount = GetFrameCount();

    if (newFrameCount == oldFrameCount) {
        return true;  // No change needed
    }

    // Convert to float for resampling
    std::vector<float> floatSamples;
    ConvertToFloat(floatSamples);

    // Create new buffer
    std::vector<float> resized(newFrameCount * channels);

    // Linear interpolation resampling
    float ratio = (float)oldFrameCount / (float)newFrameCount;

    for (int frame = 0; frame < newFrameCount; frame++) {
        float srcPos = frame * ratio;
        int srcFrame = (int)srcPos;
        float frac = srcPos - srcFrame;

        // Boundary check
        if (srcFrame >= oldFrameCount - 1) {
            srcFrame = oldFrameCount - 2;
            frac = 1.0f;
        }

        // Interpolate both channels
        for (int ch = 0; ch < channels; ch++) {
            int idx1 = srcFrame * channels + ch;
            int idx2 = (srcFrame + 1) * channels + ch;
            resized[frame * channels + ch] = floatSamples[idx1] * (1.0f - frac) +
                                              floatSamples[idx2] * frac;
        }
    }

    // Convert back to int16
    ConvertFromFloat(resized);
    return true;
}

bool AudioSampleCache::TimeStretch(float ratio)
{
    if (!isValid || ratio <= 0.0f || ratio > 4.0f) {
        return false;
    }

    if (std::abs(ratio - 1.0f) < 0.001f) {
        return true;  // No change needed
    }

    // WSOLA parameters
    const int templateSize = (int)(sampleRate * 0.02f);  // 20ms template
    const int searchWindow = (int)(sampleRate * 0.01f);  // 10ms search

    std::vector<float> floatSamples;
    ConvertToFloat(floatSamples);

    int inputFrames = GetFrameCount();
    int outputFrames = (int)(inputFrames / ratio);

    std::vector<float> output;
    output.reserve(outputFrames * channels);

    int inputPos = 0;
    float outputPos = 0.0f;

    while (inputPos < inputFrames - templateSize && output.size() < (size_t)(outputFrames * channels)) {
        // Find best match in search window
        int searchStart = std::max(0, inputPos - searchWindow);
        int searchEnd = std::min(inputFrames - templateSize, inputPos + searchWindow);

        int bestMatch = FindBestMatch(floatSamples, inputPos, searchStart, searchEnd, templateSize);

        // Copy template at best match position
        for (int i = 0; i < templateSize; i++) {
            for (int ch = 0; ch < channels; ch++) {
                output.push_back(floatSamples[(bestMatch + i) * channels + ch]);
            }
        }

        // Advance positions
        outputPos += templateSize * ratio;
        inputPos = (int)outputPos;
    }

    // Resize output to exact length
    output.resize(outputFrames * channels);

    // Convert back to int16
    ConvertFromFloat(output);
    return true;
}

int AudioSampleCache::FindBestMatch(const std::vector<float>& buffer, int pos,
                                    int searchStart, int searchEnd, int templateSize)
{
    float bestCorrelation = -1.0f;
    int bestPos = pos;

    for (int testPos = searchStart; testPos <= searchEnd; testPos++) {
        float correlation = 0.0f;

        // Calculate cross-correlation
        for (int i = 0; i < templateSize; i++) {
            for (int ch = 0; ch < channels; ch++) {
                int idx1 = (pos + i) * channels + ch;
                int idx2 = (testPos + i) * channels + ch;

                if (idx1 < (int)buffer.size() && idx2 < (int)buffer.size()) {
                    correlation += buffer[idx1] * buffer[idx2];
                }
            }
        }

        if (correlation > bestCorrelation) {
            bestCorrelation = correlation;
            bestPos = testPos;
        }
    }

    return bestPos;
}

bool AudioSampleCache::PitchShift(float semitones)
{
    if (!isValid || std::abs(semitones) > 24.0f) {
        return false;
    }

    if (std::abs(semitones) < 0.01f) {
        return true;  // No change needed
    }

    // Pitch shift ratio: 2^(semitones/12)
    float pitchRatio = powf(2.0f, semitones / 12.0f);

    // Step 1: Resample by pitch ratio
    std::vector<float> floatSamples;
    ConvertToFloat(floatSamples);

    int inputFrames = GetFrameCount();
    int resampledFrames = (int)(inputFrames * pitchRatio);

    std::vector<float> resampled(resampledFrames * channels);

    for (int frame = 0; frame < resampledFrames; frame++) {
        float srcPos = frame / pitchRatio;
        int srcFrame = (int)srcPos;
        float frac = srcPos - srcFrame;

        if (srcFrame >= inputFrames - 1) {
            srcFrame = inputFrames - 2;
            frac = 1.0f;
        }

        for (int ch = 0; ch < channels; ch++) {
            int idx1 = srcFrame * channels + ch;
            int idx2 = (srcFrame + 1) * channels + ch;
            resampled[frame * channels + ch] = floatSamples[idx1] * (1.0f - frac) +
                                                floatSamples[idx2] * frac;
        }
    }

    // Step 2: Time stretch back to original duration
    // (This maintains the original duration while changing pitch)
    samples.resize(inputFrames * channels);

    for (int frame = 0; frame < inputFrames; frame++) {
        float srcPos = frame * pitchRatio;
        int srcFrame = (int)srcPos;
        float frac = srcPos - srcFrame;

        if (srcFrame >= resampledFrames - 1) {
            srcFrame = resampledFrames - 2;
            frac = 1.0f;
        }

        for (int ch = 0; ch < channels; ch++) {
            int idx1 = srcFrame * channels + ch;
            int idx2 = (srcFrame + 1) * channels + ch;
            float value = resampled[idx1] * (1.0f - frac) + resampled[idx2] * frac;
            value *= FLOAT_TO_INT16;
            value = std::max(-32768.0f, std::min(32767.0f, value));
            samples[frame * channels + ch] = (int16_t)value;
        }
    }

    return true;
}

bool AudioSampleCache::Normalize(float targetLevel, bool useRMS)
{
    if (!isValid || targetLevel <= 0.0f || targetLevel > 1.0f) {
        return false;
    }

    std::vector<float> floatSamples;
    ConvertToFloat(floatSamples);

    float currentLevel = 0.0f;

    if (useRMS) {
        // RMS normalization
        float sumSquares = 0.0f;
        for (size_t i = 0; i < floatSamples.size(); i++) {
            sumSquares += floatSamples[i] * floatSamples[i];
        }
        currentLevel = sqrtf(sumSquares / floatSamples.size());
    } else {
        // Peak normalization
        for (size_t i = 0; i < floatSamples.size(); i++) {
            float abs = std::abs(floatSamples[i]);
            if (abs > currentLevel) {
                currentLevel = abs;
            }
        }
    }

    if (currentLevel < 0.0001f) {
        return false;  // Silence, no normalization needed
    }

    // Calculate gain factor
    float gain = targetLevel / currentLevel;

    // Apply gain
    for (size_t i = 0; i < floatSamples.size(); i++) {
        floatSamples[i] *= gain;
    }

    ConvertFromFloat(floatSamples);
    return true;
}

bool AudioSampleCache::FadeIn(float durationSeconds, bool exponential)
{
    if (!isValid || durationSeconds <= 0.0f) {
        return false;
    }

    int fadeFrames = (int)(durationSeconds * sampleRate);
    int totalFrames = GetFrameCount();
    fadeFrames = std::min(fadeFrames, totalFrames);

    for (int frame = 0; frame < fadeFrames; frame++) {
        float t = (float)frame / (float)fadeFrames;
        float gain = exponential ? (t * t) : t;

        for (int ch = 0; ch < channels; ch++) {
            int idx = frame * channels + ch;
            float sample = samples[idx] * gain;
            samples[idx] = (int16_t)sample;
        }
    }

    return true;
}

bool AudioSampleCache::FadeOut(float durationSeconds, bool exponential)
{
    if (!isValid || durationSeconds <= 0.0f) {
        return false;
    }

    int fadeFrames = (int)(durationSeconds * sampleRate);
    int totalFrames = GetFrameCount();
    fadeFrames = std::min(fadeFrames, totalFrames);

    int fadeStart = totalFrames - fadeFrames;

    for (int frame = fadeStart; frame < totalFrames; frame++) {
        float t = (float)(frame - fadeStart) / (float)fadeFrames;
        float gain = exponential ? ((1.0f - t) * (1.0f - t)) : (1.0f - t);

        for (int ch = 0; ch < channels; ch++) {
            int idx = frame * channels + ch;
            float sample = samples[idx] * gain;
            samples[idx] = (int16_t)sample;
        }
    }

    return true;
}

bool AudioSampleCache::Reverse()
{
    if (!isValid) {
        return false;
    }

    int frameCount = GetFrameCount();

    for (int i = 0; i < frameCount / 2; i++) {
        int j = frameCount - 1 - i;

        for (int ch = 0; ch < channels; ch++) {
            int idx1 = i * channels + ch;
            int idx2 = j * channels + ch;
            std::swap(samples[idx1], samples[idx2]);
        }
    }

    return true;
}

} // namespace VeniceDAW

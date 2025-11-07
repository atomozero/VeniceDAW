/*
 * SpatialReverb.cpp - Implementation of distance-based reverb
 *
 * Based on Schroeder reverberator algorithm (1962)
 * - Parallel comb filters for dense reverb tail
 * - Series allpass filters for diffusion
 * - One-pole low-pass for damping
 */

#include "SpatialReverb.h"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace VeniceDAW {

// Tuning parameters (in samples at 44.1kHz, scaled for other rates)
// These create natural-sounding reverb without metallic artifacts
static const int COMB_TUNING_L1 = 1116;
static const int COMB_TUNING_L2 = 1188;
static const int COMB_TUNING_L3 = 1277;
static const int COMB_TUNING_L4 = 1356;
static const int COMB_TUNING_R1 = 1116 + 8;   // Slight offset for stereo width
static const int COMB_TUNING_R2 = 1188 + 8;
static const int COMB_TUNING_R3 = 1277 + 8;
static const int COMB_TUNING_R4 = 1356 + 8;

static const int ALLPASS_TUNING_L1 = 556;
static const int ALLPASS_TUNING_L2 = 441;
static const int ALLPASS_TUNING_R1 = 556 + 8;
static const int ALLPASS_TUNING_R2 = 441 + 8;

static const float SCALING_WET = 3.0f;
static const float SCALING_DRY = 2.0f;
static const float SCALING_DAMP = 0.4f;
static const float SCALING_ROOM = 0.28f;
static const float OFFSET_ROOM = 0.7f;

// ----------------------------------------------
// DelayLine Implementation
// ----------------------------------------------

SpatialReverb::DelayLine::DelayLine()
    : fBuffer(nullptr)
    , fSize(0)
    , fWritePos(0)
{
}

SpatialReverb::DelayLine::~DelayLine()
{
    delete[] fBuffer;
}

void SpatialReverb::DelayLine::SetSize(int samples)
{
    if (fBuffer) {
        delete[] fBuffer;
    }

    fSize = samples;
    fBuffer = new float[fSize];
    Clear();
}

void SpatialReverb::DelayLine::Write(float sample)
{
    if (!fBuffer) return;

    fBuffer[fWritePos] = sample;
    fWritePos = (fWritePos + 1) % fSize;
}

float SpatialReverb::DelayLine::Read(int delaySamples)
{
    if (!fBuffer || fSize == 0) return 0.0f;

    int readPos = (fWritePos - delaySamples + fSize) % fSize;
    return fBuffer[readPos];
}

void SpatialReverb::DelayLine::Clear()
{
    if (fBuffer) {
        memset(fBuffer, 0, fSize * sizeof(float));
    }
    fWritePos = 0;
}

// ----------------------------------------------
// CombFilter Implementation
// ----------------------------------------------

SpatialReverb::CombFilter::CombFilter()
    : fFeedback(0.5f)
    , fDamping(0.5f)
    , fFilterStore(0.0f)
    , fBufferSize(0)
{
}

void SpatialReverb::CombFilter::SetBufferSize(int size)
{
    fBufferSize = size;
    fDelay.SetSize(size);
}

void SpatialReverb::CombFilter::SetDamping(float damping)
{
    fDamping = damping;
}

void SpatialReverb::CombFilter::SetFeedback(float feedback)
{
    fFeedback = feedback;
}

float SpatialReverb::CombFilter::Process(float input)
{
    if (fBufferSize == 0) return input;

    // Read delayed sample
    float output = fDelay.Read(fBufferSize - 1);

    // One-pole low-pass filter for damping (simulates high-frequency absorption)
    fFilterStore = (output * (1.0f - fDamping)) + (fFilterStore * fDamping);

    // Write input + filtered feedback
    fDelay.Write(input + (fFilterStore * fFeedback));

    return output;
}

void SpatialReverb::CombFilter::Clear()
{
    fDelay.Clear();
    fFilterStore = 0.0f;
}

// ----------------------------------------------
// AllpassFilter Implementation
// ----------------------------------------------

SpatialReverb::AllpassFilter::AllpassFilter()
    : fFeedback(0.5f)
    , fBufferSize(0)
{
}

void SpatialReverb::AllpassFilter::SetBufferSize(int size)
{
    fBufferSize = size;
    fDelay.SetSize(size);
}

void SpatialReverb::AllpassFilter::SetFeedback(float feedback)
{
    fFeedback = feedback;
}

float SpatialReverb::AllpassFilter::Process(float input)
{
    if (fBufferSize == 0) return input;

    float delayed = fDelay.Read(fBufferSize - 1);
    float output = -input + delayed;

    fDelay.Write(input + (delayed * fFeedback));

    return output;
}

void SpatialReverb::AllpassFilter::Clear()
{
    fDelay.Clear();
}

// ----------------------------------------------
// SpatialReverb Implementation
// ----------------------------------------------

SpatialReverb::SpatialReverb()
    : fSampleRate(44100.0f)
    , fRoomSize(0.7f)
    , fDamping(0.5f)
    , fWidth(1.0f)
    , fMinDistance(1.0f)
    , fMaxDistance(20.0f)
{
    // Initialize with default sample rate
    SetSampleRate(fSampleRate);
}

SpatialReverb::~SpatialReverb()
{
}

void SpatialReverb::SetSampleRate(float sampleRate)
{
    fSampleRate = sampleRate;

    // Scale tuning parameters for sample rate
    float scale = sampleRate / 44100.0f;

    // Initialize comb filters (left channel)
    fCombL[0].SetBufferSize((int)(COMB_TUNING_L1 * scale));
    fCombL[1].SetBufferSize((int)(COMB_TUNING_L2 * scale));
    fCombL[2].SetBufferSize((int)(COMB_TUNING_L3 * scale));
    fCombL[3].SetBufferSize((int)(COMB_TUNING_L4 * scale));

    // Initialize comb filters (right channel)
    fCombR[0].SetBufferSize((int)(COMB_TUNING_R1 * scale));
    fCombR[1].SetBufferSize((int)(COMB_TUNING_R2 * scale));
    fCombR[2].SetBufferSize((int)(COMB_TUNING_R3 * scale));
    fCombR[3].SetBufferSize((int)(COMB_TUNING_R4 * scale));

    // Initialize allpass filters (left channel)
    fAllpassL[0].SetBufferSize((int)(ALLPASS_TUNING_L1 * scale));
    fAllpassL[1].SetBufferSize((int)(ALLPASS_TUNING_L2 * scale));
    fAllpassL[0].SetFeedback(0.5f);
    fAllpassL[1].SetFeedback(0.5f);

    // Initialize allpass filters (right channel)
    fAllpassR[0].SetBufferSize((int)(ALLPASS_TUNING_R1 * scale));
    fAllpassR[1].SetBufferSize((int)(ALLPASS_TUNING_R2 * scale));
    fAllpassR[0].SetFeedback(0.5f);
    fAllpassR[1].SetFeedback(0.5f);

    UpdateFilters();
}

void SpatialReverb::SetRoomSize(float size)
{
    fRoomSize = std::max(0.0f, std::min(1.0f, size));
    UpdateFilters();
}

void SpatialReverb::SetDamping(float damping)
{
    fDamping = std::max(0.0f, std::min(1.0f, damping));
    UpdateDamping();
}

void SpatialReverb::SetWidth(float width)
{
    fWidth = std::max(0.0f, std::min(1.0f, width));
}

float SpatialReverb::CalculateWetAmount(float distance)
{
    // No reverb for very close sounds
    if (distance < fMinDistance) {
        return 0.0f;
    }

    // Maximum reverb for distant sounds
    if (distance > fMaxDistance) {
        return 1.0f;
    }

    // Linear interpolation between min and max distance
    float normalized = (distance - fMinDistance) / (fMaxDistance - fMinDistance);
    return normalized;
}

void SpatialReverb::ProcessMono(const float* input, float* leftOut, float* rightOut,
                                 int frameCount, float wetAmount)
{
    float dryAmount = 1.0f - wetAmount;

    for (int i = 0; i < frameCount; i++) {
        float inputSample = input[i];

        // Process through comb filters (parallel)
        float combOutL = 0.0f;
        for (int c = 0; c < NUM_COMBS / 2; c++) {
            combOutL += fCombL[c].Process(inputSample);
        }

        float combOutR = 0.0f;
        for (int c = 0; c < NUM_COMBS / 2; c++) {
            combOutR += fCombR[c].Process(inputSample);
        }

        // Process through allpass filters (series)
        float outputL = combOutL;
        for (int a = 0; a < NUM_ALLPASSES / 2; a++) {
            outputL = fAllpassL[a].Process(outputL);
        }

        float outputR = combOutR;
        for (int a = 0; a < NUM_ALLPASSES / 2; a++) {
            outputR = fAllpassR[a].Process(outputR);
        }

        // Mix wet/dry
        leftOut[i] += (inputSample * dryAmount + outputL * wetAmount * SCALING_WET);
        rightOut[i] += (inputSample * dryAmount + outputR * wetAmount * SCALING_WET);
    }
}

void SpatialReverb::ProcessStereo(const float* leftIn, const float* rightIn,
                                   float* leftOut, float* rightOut,
                                   int frameCount, float wetAmount)
{
    float dryAmount = 1.0f - wetAmount;

    for (int i = 0; i < frameCount; i++) {
        // Mix stereo input to mono for reverb input
        float monoInput = (leftIn[i] + rightIn[i]) * 0.5f;

        // Process through comb filters
        float combOutL = 0.0f;
        for (int c = 0; c < NUM_COMBS / 2; c++) {
            combOutL += fCombL[c].Process(monoInput);
        }

        float combOutR = 0.0f;
        for (int c = 0; c < NUM_COMBS / 2; c++) {
            combOutR += fCombR[c].Process(monoInput);
        }

        // Process through allpass filters
        float reverbL = combOutL;
        for (int a = 0; a < NUM_ALLPASSES / 2; a++) {
            reverbL = fAllpassL[a].Process(reverbL);
        }

        float reverbR = combOutR;
        for (int a = 0; a < NUM_ALLPASSES / 2; a++) {
            reverbR = fAllpassR[a].Process(reverbR);
        }

        // Mix wet/dry
        leftOut[i] += (leftIn[i] * dryAmount + reverbL * wetAmount * SCALING_WET);
        rightOut[i] += (rightIn[i] * dryAmount + reverbR * wetAmount * SCALING_WET);
    }
}

void SpatialReverb::Reset()
{
    for (int i = 0; i < NUM_COMBS / 2; i++) {
        fCombL[i].Clear();
        fCombR[i].Clear();
    }

    for (int i = 0; i < NUM_ALLPASSES / 2; i++) {
        fAllpassL[i].Clear();
        fAllpassR[i].Clear();
    }
}

void SpatialReverb::UpdateFilters()
{
    float feedback = OFFSET_ROOM + (fRoomSize * SCALING_ROOM);

    for (int i = 0; i < NUM_COMBS / 2; i++) {
        fCombL[i].SetFeedback(feedback);
        fCombR[i].SetFeedback(feedback);
    }
}

void SpatialReverb::UpdateDamping()
{
    float damping = fDamping * SCALING_DAMP;

    for (int i = 0; i < NUM_COMBS / 2; i++) {
        fCombL[i].SetDamping(damping);
        fCombR[i].SetDamping(damping);
    }
}

} // namespace VeniceDAW

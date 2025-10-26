/*
 * BiquadFilter.cpp - Implementation of biquad filter with coefficient caching
 */

#include "BiquadFilter.h"
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace HaikuDAW {

BiquadFilter::BiquadFilter()
    : fMode(LOW_PASS)
    , fSampleRate(44100.0f)
    , fFrequency(1000.0f)
    , fBandwidth(1.0f)
    , fGainDB(0.0f)
    , fA(1.0f)
    , fOmega(0.0f)
    , fSin(0.0f)
    , fCos(1.0f)
    , fAlpha(0.0f)
    , fBeta(0.0f)
    , fa0(1.0f), fa1(0.0f), fa2(0.0f)
    , fb0(1.0f), fb1(0.0f), fb2(0.0f)
    , fZ1(0.0f), fZ2(0.0f)
    , fDirty(true)
{
    // Coefficients will be calculated on first Process() call
}

void BiquadFilter::SetMode(FilterMode mode)
{
    if (fMode != mode) {
        fMode = mode;
        fDirty = true;
    }
}

void BiquadFilter::SetSampleRate(float sampleRate)
{
    if (fSampleRate != sampleRate) {
        fSampleRate = sampleRate;
        fDirty = true;
    }
}

void BiquadFilter::SetFrequency(float frequency)
{
    if (fFrequency != frequency) {
        fFrequency = frequency;
        fDirty = true;
    }
}

void BiquadFilter::SetBandwidth(float bandwidth)
{
    if (fBandwidth != bandwidth) {
        fBandwidth = bandwidth;
        fDirty = true;
    }
}

void BiquadFilter::SetQ(float q)
{
    // Convert Q to bandwidth: BW = 2 * asinh(1/(2*Q)) / ln(2)
    float bandwidth = 2.0f * asinhf(1.0f / (2.0f * q)) / logf(2.0f);
    SetBandwidth(bandwidth);
}

void BiquadFilter::SetGain(float gainDB)
{
    if (fGainDB != gainDB) {
        fGainDB = gainDB;
        fDirty = true;
    }
}

float BiquadFilter::Process(float input)
{
    // Recalculate coefficients if parameters changed
    if (fDirty) {
        _RecalculateCoefficients();
        fDirty = false;
    }

    // Direct Form I biquad
    float output = input * fb0 + fZ1;
    fZ1 = input * fb1 - output * fa1 + fZ2;
    fZ2 = input * fb2 - output * fa2;

    return output;
}

void BiquadFilter::Process(float* buffer, size_t frameCount)
{
    // Recalculate once if needed
    if (fDirty) {
        _RecalculateCoefficients();
        fDirty = false;
    }

    // Process all samples using cached coefficients
    for (size_t i = 0; i < frameCount; i++) {
        float input = buffer[i];
        float output = input * fb0 + fZ1;
        fZ1 = input * fb1 - output * fa1 + fZ2;
        fZ2 = input * fb2 - output * fa2;
        buffer[i] = output;
    }
}

void BiquadFilter::ProcessStereo(float* left, float* right, size_t frameCount)
{
    // Recalculate once if needed
    if (fDirty) {
        _RecalculateCoefficients();
        fDirty = false;
    }

    // Process both channels (shared coefficients, separate state)
    float z1_left = fZ1, z2_left = fZ2;
    float z1_right = 0.0f, z2_right = 0.0f;

    for (size_t i = 0; i < frameCount; i++) {
        // Left channel
        float input_left = left[i];
        float output_left = input_left * fb0 + z1_left;
        z1_left = input_left * fb1 - output_left * fa1 + z2_left;
        z2_left = input_left * fb2 - output_left * fa2;
        left[i] = output_left;

        // Right channel
        float input_right = right[i];
        float output_right = input_right * fb0 + z1_right;
        z1_right = input_right * fb1 - output_right * fa1 + z2_right;
        z2_right = input_right * fb2 - output_right * fa2;
        right[i] = output_right;
    }

    // Save left channel state
    fZ1 = z1_left;
    fZ2 = z2_left;
}

void BiquadFilter::Reset()
{
    fZ1 = 0.0f;
    fZ2 = 0.0f;
}

// Private methods

void BiquadFilter::_RecalculateCoefficients()
{
    // Pre-calculate intermediate values
    fOmega = 2.0f * M_PI * fFrequency / fSampleRate;
    fCos = cosf(fOmega);
    fSin = sinf(fOmega);
    fAlpha = fSin * sinhf(logf(2.0f) / 2.0f * fBandwidth * fOmega / fSin);
    fA = powf(10.0f, fGainDB / 40.0f);
    fBeta = sqrtf((fA * fA + 1.0f) / 32.0f - (fA - 1.0f) * (fA - 1.0f));

    // Calculate coefficients based on filter mode
    switch (fMode) {
        case LOW_PASS:    _CalculateLowPass();    break;
        case HIGH_PASS:   _CalculateHighPass();   break;
        case BAND_PASS:   _CalculateBandPass();   break;
        case NOTCH:       _CalculateNotch();      break;
        case PEAKING:     _CalculatePeaking();    break;
        case LOW_SHELF:   _CalculateLowShelf();   break;
        case HIGH_SHELF:  _CalculateHighShelf();  break;
    }
}

void BiquadFilter::_CalculateLowPass()
{
    // H(s) = 1 / (s^2 + s/Q + 1)
    fb0 = (1.0f - fCos) / 2.0f;
    fb1 = 1.0f - fCos;
    fb2 = (1.0f - fCos) / 2.0f;
    fa0 = 1.0f + fAlpha;
    fa1 = -2.0f * fCos;
    fa2 = 1.0f - fAlpha;

    // Normalize by a0
    fb0 /= fa0;
    fb1 /= fa0;
    fb2 /= fa0;
    fa1 /= fa0;
    fa2 /= fa0;
}

void BiquadFilter::_CalculateHighPass()
{
    // H(s) = s^2 / (s^2 + s/Q + 1)
    fb0 = (1.0f + fCos) / 2.0f;
    fb1 = -(1.0f + fCos);
    fb2 = (1.0f + fCos) / 2.0f;
    fa0 = 1.0f + fAlpha;
    fa1 = -2.0f * fCos;
    fa2 = 1.0f - fAlpha;

    // Normalize
    fb0 /= fa0;
    fb1 /= fa0;
    fb2 /= fa0;
    fa1 /= fa0;
    fa2 /= fa0;
}

void BiquadFilter::_CalculateBandPass()
{
    // H(s) = (s/Q) / (s^2 + s/Q + 1)
    fb0 = fAlpha;
    fb1 = 0.0f;
    fb2 = -fAlpha;
    fa0 = 1.0f + fAlpha;
    fa1 = -2.0f * fCos;
    fa2 = 1.0f - fAlpha;

    // Normalize
    fb0 /= fa0;
    fb1 /= fa0;
    fb2 /= fa0;
    fa1 /= fa0;
    fa2 /= fa0;
}

void BiquadFilter::_CalculateNotch()
{
    // H(s) = (s^2 + 1) / (s^2 + s/Q + 1)
    fb0 = 1.0f;
    fb1 = -2.0f * fCos;
    fb2 = 1.0f;
    fa0 = 1.0f + fAlpha;
    fa1 = -2.0f * fCos;
    fa2 = 1.0f - fAlpha;

    // Normalize
    fb0 /= fa0;
    fb1 /= fa0;
    fb2 /= fa0;
    fa1 /= fa0;
    fa2 /= fa0;
}

void BiquadFilter::_CalculatePeaking()
{
    // Peaking EQ (bell curve)
    fb0 = 1.0f + fAlpha * fA;
    fb1 = -2.0f * fCos;
    fb2 = 1.0f - fAlpha * fA;
    fa0 = 1.0f + fAlpha / fA;
    fa1 = -2.0f * fCos;
    fa2 = 1.0f - fAlpha / fA;

    // Normalize
    fb0 /= fa0;
    fb1 /= fa0;
    fb2 /= fa0;
    fa1 /= fa0;
    fa2 /= fa0;
}

void BiquadFilter::_CalculateLowShelf()
{
    // Low shelf filter
    float sqrtA = sqrtf(fA);
    fb0 = fA * ((fA + 1.0f) - (fA - 1.0f) * fCos + fBeta * fSin);
    fb1 = 2.0f * fA * ((fA - 1.0f) - (fA + 1.0f) * fCos);
    fb2 = fA * ((fA + 1.0f) - (fA - 1.0f) * fCos - fBeta * fSin);
    fa0 = (fA + 1.0f) + (fA - 1.0f) * fCos + fBeta * fSin;
    fa1 = -2.0f * ((fA - 1.0f) + (fA + 1.0f) * fCos);
    fa2 = (fA + 1.0f) + (fA - 1.0f) * fCos - fBeta * fSin;

    // Normalize
    fb0 /= fa0;
    fb1 /= fa0;
    fb2 /= fa0;
    fa1 /= fa0;
    fa2 /= fa0;
}

void BiquadFilter::_CalculateHighShelf()
{
    // High shelf filter
    float sqrtA = sqrtf(fA);
    fb0 = fA * ((fA + 1.0f) + (fA - 1.0f) * fCos + fBeta * fSin);
    fb1 = -2.0f * fA * ((fA - 1.0f) + (fA + 1.0f) * fCos);
    fb2 = fA * ((fA + 1.0f) + (fA - 1.0f) * fCos - fBeta * fSin);
    fa0 = (fA + 1.0f) - (fA - 1.0f) * fCos + fBeta * fSin;
    fa1 = 2.0f * ((fA - 1.0f) - (fA + 1.0f) * fCos);
    fa2 = (fA + 1.0f) - (fA - 1.0f) * fCos - fBeta * fSin;

    // Normalize
    fb0 /= fa0;
    fb1 /= fa0;
    fb2 /= fa0;
    fa1 /= fa0;
    fa2 /= fa0;
}

} // namespace HaikuDAW

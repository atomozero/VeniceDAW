/*
 * BiquadFilter.h - Biquad filter with coefficient caching
 * Optimized for real-time audio processing
 */

#ifndef BIQUAD_FILTER_H
#define BIQUAD_FILTER_H

#include <stdint.h>
#include <cmath>

namespace HaikuDAW {

/*
 * BiquadFilter - Digital biquad filter with lazy coefficient recalculation
 *
 * Purpose:
 * - Implements standard biquad IIR filter (Direct Form I)
 * - Caches filter coefficients to avoid redundant calculations
 * - Only recalculates when parameters change (frequency, Q, gain)
 * - Suitable for EQ, filters, dynamics processing
 *
 * Filter modes:
 * - Low-pass, High-pass, Band-pass
 * - Notch (band-reject)
 * - Peaking EQ (bell curve)
 * - Low-shelf, High-shelf
 *
 * Usage pattern:
 *   BiquadFilter lpf;
 *   lpf.SetMode(BiquadFilter::LOW_PASS);
 *   lpf.SetSampleRate(44100.0f);
 *   lpf.SetFrequency(1000.0f);
 *   lpf.SetBandwidth(1.0f);  // Q factor
 *
 *   // Process audio (coefficients calculated only on first call)
 *   for (int i = 0; i < frameCount; i++) {
 *       buffer[i] = lpf.Process(buffer[i]);
 *   }
 *
 *   // Change frequency - coefficients will be recalculated once
 *   lpf.SetFrequency(2000.0f);
 */
class BiquadFilter {
public:
    // Filter modes
    enum FilterMode {
        LOW_PASS = 0,
        HIGH_PASS,
        BAND_PASS,
        NOTCH,
        PEAKING,
        LOW_SHELF,
        HIGH_SHELF
    };

    BiquadFilter();
    ~BiquadFilter() = default;

    // Parameter setters (mark coefficients as dirty)
    void SetMode(FilterMode mode);
    void SetSampleRate(float sampleRate);
    void SetFrequency(float frequency);     // Hz
    void SetBandwidth(float bandwidth);     // Octaves (0.3 - 3.0)
    void SetQ(float q);                     // Alternative to bandwidth
    void SetGain(float gainDB);             // dB (for peaking/shelf filters)

    // Audio processing
    float Process(float input);
    void Process(float* buffer, size_t frameCount);
    void ProcessStereo(float* left, float* right, size_t frameCount);

    // State management
    void Reset();  // Clear delay samples (z1, z2)

    // Parameter getters
    FilterMode GetMode() const { return fMode; }
    float GetSampleRate() const { return fSampleRate; }
    float GetFrequency() const { return fFrequency; }
    float GetBandwidth() const { return fBandwidth; }
    float GetGain() const { return fGainDB; }

private:
    // Filter parameters
    FilterMode fMode;
    float fSampleRate;  // Hz
    float fFrequency;   // Hz
    float fBandwidth;   // Octaves at -3dB
    float fGainDB;      // dB

    // Intermediate values (cached)
    float fA;           // 10^(dBgain/40)
    float fOmega;       // 2*PI*frequency/sampleRate
    float fSin;         // sin(omega)
    float fCos;         // cos(omega)
    float fAlpha;       // sn*sinh[ln(2)/2 * bandwidth * omega/sn]
    float fBeta;        // sqrt[(A^2 + 1)/S - (A-1)^2]

    // Biquad coefficients (cached)
    float fa0, fa1, fa2;  // Numerator coefficients
    float fb0, fb1, fb2;  // Denominator coefficients

    // Delay samples (state)
    float fZ1, fZ2;

    // Optimization flag
    bool fDirty;  // True when coefficients need recalculation

    // Internal methods
    void _RecalculateCoefficients();
    void _CalculateLowPass();
    void _CalculateHighPass();
    void _CalculateBandPass();
    void _CalculateNotch();
    void _CalculatePeaking();
    void _CalculateLowShelf();
    void _CalculateHighShelf();
};

} // namespace HaikuDAW

#endif // BIQUAD_FILTER_H

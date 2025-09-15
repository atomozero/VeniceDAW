#ifndef DSP_ALGORITHMS_H
#define DSP_ALGORITHMS_H

#include <cstddef>
#include <cmath>

namespace VeniceDAW {
namespace DSP {

class BiquadFilter {
public:
    enum FilterType {
        LowPass,
        HighPass,
        BandPass,
        Notch,
        Peak,
        LowShelf,
        HighShelf,
        AllPass
    };

    BiquadFilter();
    ~BiquadFilter() = default;

    void SetCoefficients(float a0, float a1, float a2, float b0, float b1, float b2);
    
    void CalculateCoefficients(FilterType type, float sampleRate, 
                               float frequency, float Q, float gainDB);
    
    float ProcessSample(float input);
    
    void ProcessBlock(const float* input, float* output, size_t numSamples);
    
    void Reset();
    
    void GetCoefficients(float& a0, float& a1, float& a2, 
                        float& b0, float& b1, float& b2) const;

    float GetFrequencyResponse(float frequency, float sampleRate) const;
    float GetMagnitudeResponse(float frequency, float sampleRate) const;
    float GetPhaseResponse(float frequency, float sampleRate) const;

private:
    float m_a0, m_a1, m_a2;
    float m_b0, m_b1, m_b2;
    
    float m_x1, m_x2;
    float m_y1, m_y2;
    
    void NormalizeCoefficients();
};

class EnvelopeFollower {
public:
    EnvelopeFollower(float sampleRate);
    ~EnvelopeFollower() = default;
    
    void SetAttack(float attackMs);
    void SetRelease(float releaseMs);
    void SetMode(bool rms);
    
    float ProcessSample(float input);
    void ProcessBlock(const float* input, float* envelope, size_t numSamples);
    
    void Reset();
    
private:
    float m_sampleRate;
    float m_attackCoeff;
    float m_releaseCoeff;
    float m_envelope;
    bool m_rmsMode;
    
    void UpdateCoefficients();
};

class SoftClipper {
public:
    enum ClipType {
        HardClip,
        SoftClip,
        Tanh,
        Sigmoid,
        Polynomial
    };
    
    SoftClipper(ClipType type = SoftClip);
    ~SoftClipper() = default;
    
    void SetThreshold(float threshold);
    void SetKnee(float knee);
    void SetType(ClipType type);
    
    float ProcessSample(float input);
    void ProcessBlock(const float* input, float* output, size_t numSamples);
    
private:
    ClipType m_type;
    float m_threshold;
    float m_knee;
    
    float ApplyClipping(float input) const;
};

class AllPassFilter {
public:
    AllPassFilter(size_t maxDelay);
    ~AllPassFilter();
    
    void SetDelay(float delaySamples);
    void SetFeedback(float feedback);
    
    float ProcessSample(float input);
    void ProcessBlock(const float* input, float* output, size_t numSamples);
    
    void Reset();
    
private:
    float* m_buffer;
    size_t m_bufferSize;
    size_t m_writeIndex;
    float m_delay;
    float m_feedback;
    
    float InterpolatedRead(float delaySamples) const;
};

class DCBlocker {
public:
    DCBlocker(float cutoffFrequency = 20.0f, float sampleRate = 44100.0f);
    ~DCBlocker() = default;
    
    void SetCutoff(float frequency, float sampleRate);
    
    float ProcessSample(float input);
    void ProcessBlock(const float* input, float* output, size_t numSamples);
    
    void Reset();
    
private:
    float m_x1;
    float m_y1;
    float m_R;
};

// Spatial Audio Processing Components
class DelayLine {
public:
    DelayLine(size_t maxDelaySamples);
    ~DelayLine();
    
    void SetDelay(float delaySamples);
    float ProcessSample(float input);
    void ProcessBlock(const float* input, float* output, size_t numSamples);
    void Reset();
    
private:
    float* m_buffer;
    size_t m_bufferSize;
    size_t m_writeIndex;
    float m_delay;
    
    float InterpolatedRead(float delaySamples) const;
};

class ConvolutionEngine {
public:
    ConvolutionEngine(size_t maxImpulseLength);
    ~ConvolutionEngine();
    
    void SetImpulseResponse(const float* impulse, size_t length);
    float ProcessSample(float input);
    void ProcessBlock(const float* input, float* output, size_t numSamples);
    void Reset();
    
private:
    float* m_impulseResponse;
    float* m_delayLine;
    size_t m_impulseLength;
    size_t m_bufferSize;
    size_t m_writeIndex;
};

struct Vector3D {
    float x, y, z;
    
    Vector3D(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}
    
    float Distance(const Vector3D& other) const;
    Vector3D Normalize() const;
    float Magnitude() const;
    Vector3D operator-(const Vector3D& other) const;
    Vector3D operator+(const Vector3D& other) const;
    Vector3D operator*(float scalar) const;
    float Dot(const Vector3D& other) const;
    Vector3D Cross(const Vector3D& other) const;
};

struct SphericalCoordinate {
    float azimuth;    // Horizontal angle in radians (-π to π)
    float elevation;  // Vertical angle in radians (-π/2 to π/2)
    float distance;   // Distance in meters
    
    SphericalCoordinate(float az = 0.0f, float el = 0.0f, float dist = 1.0f)
        : azimuth(az), elevation(el), distance(dist) {}
    
    static SphericalCoordinate FromCartesian(const Vector3D& cartesian);
    Vector3D ToCartesian() const;
};

class SpatialAudioMath {
public:
    // Distance-based attenuation calculations
    static float CalculateDistanceAttenuation(float distance, float referenceDistance = 1.0f);
    
    // Air absorption for high frequencies
    static float CalculateAirAbsorption(float distance, float frequency, float humidity = 50.0f);
    
    // Doppler effect calculation
    static float CalculateDopplerShift(const Vector3D& sourceVelocity, 
                                      const Vector3D& listenerVelocity,
                                      const Vector3D& sourceToListener,
                                      float speedOfSound = 343.0f);
    
    // HRTF angle calculations
    static SphericalCoordinate CalculateRelativePosition(const Vector3D& source,
                                                        const Vector3D& listener,
                                                        const Vector3D& listenerForward,
                                                        const Vector3D& listenerUp);
    
    // Interaural calculations
    static float CalculateInterauralTimeDifference(float azimuth, float headRadius = 0.0875f);
    static float CalculateInterauralLevelDifference(float azimuth, float elevation);
};

inline float dBToLinear(float dB) {
    return std::pow(10.0f, dB / 20.0f);
}

inline float linearTodB(float linear) {
    return 20.0f * std::log10(std::max(1e-10f, linear));
}

inline float fastTanh(float x) {
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

inline float cubicInterpolate(float y0, float y1, float y2, float y3, float frac) {
    float a0 = y3 - y2 - y0 + y1;
    float a1 = y0 - y1 - a0;
    float a2 = y2 - y0;
    float a3 = y1;
    
    float frac2 = frac * frac;
    return a0 * frac * frac2 + a1 * frac2 + a2 * frac + a3;
}

}
}

#endif
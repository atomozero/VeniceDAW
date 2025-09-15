#include "DSPAlgorithms.h"
#include <cstring>
#include <algorithm>

namespace VeniceDAW {
namespace DSP {

static constexpr float M_PI_F = 3.14159265358979323846f;

BiquadFilter::BiquadFilter()
    : m_a0(1.0f), m_a1(0.0f), m_a2(0.0f)
    , m_b0(1.0f), m_b1(0.0f), m_b2(0.0f)
    , m_x1(0.0f), m_x2(0.0f)
    , m_y1(0.0f), m_y2(0.0f) {
}

void BiquadFilter::SetCoefficients(float a0, float a1, float a2, 
                                   float b0, float b1, float b2) {
    m_a0 = a0;
    m_a1 = a1;
    m_a2 = a2;
    m_b0 = b0;
    m_b1 = b1;
    m_b2 = b2;
    NormalizeCoefficients();
}

void BiquadFilter::CalculateCoefficients(FilterType type, float sampleRate,
                                         float frequency, float Q, float gainDB) {
    const float omega = 2.0f * M_PI_F * frequency / sampleRate;
    const float sinOmega = std::sin(omega);
    const float cosOmega = std::cos(omega);
    const float alpha = sinOmega / (2.0f * Q);
    const float A = std::pow(10.0f, gainDB / 40.0f);
    
    float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    
    switch (type) {
        case LowPass:
            b0 = (1.0f - cosOmega) / 2.0f;
            b1 = 1.0f - cosOmega;
            b2 = (1.0f - cosOmega) / 2.0f;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cosOmega;
            a2 = 1.0f - alpha;
            break;
            
        case HighPass:
            b0 = (1.0f + cosOmega) / 2.0f;
            b1 = -(1.0f + cosOmega);
            b2 = (1.0f + cosOmega) / 2.0f;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cosOmega;
            a2 = 1.0f - alpha;
            break;
            
        case BandPass:
            b0 = alpha;
            b1 = 0.0f;
            b2 = -alpha;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cosOmega;
            a2 = 1.0f - alpha;
            break;
            
        case Notch:
            b0 = 1.0f;
            b1 = -2.0f * cosOmega;
            b2 = 1.0f;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cosOmega;
            a2 = 1.0f - alpha;
            break;
            
        case Peak:
            b0 = 1.0f + alpha * A;
            b1 = -2.0f * cosOmega;
            b2 = 1.0f - alpha * A;
            a0 = 1.0f + alpha / A;
            a1 = -2.0f * cosOmega;
            a2 = 1.0f - alpha / A;
            break;
            
        case LowShelf: {
            const float sqrtA = std::sqrt(A);
            const float sqrtA2Alpha = 2.0f * sqrtA * alpha;
            
            b0 = A * ((A + 1.0f) - (A - 1.0f) * cosOmega + sqrtA2Alpha);
            b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosOmega);
            b2 = A * ((A + 1.0f) - (A - 1.0f) * cosOmega - sqrtA2Alpha);
            a0 = (A + 1.0f) + (A - 1.0f) * cosOmega + sqrtA2Alpha;
            a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosOmega);
            a2 = (A + 1.0f) + (A - 1.0f) * cosOmega - sqrtA2Alpha;
            break;
        }
            
        case HighShelf: {
            const float sqrtA = std::sqrt(A);
            const float sqrtA2Alpha = 2.0f * sqrtA * alpha;
            
            b0 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega + sqrtA2Alpha);
            b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosOmega);
            b2 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega - sqrtA2Alpha);
            a0 = (A + 1.0f) - (A - 1.0f) * cosOmega + sqrtA2Alpha;
            a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosOmega);
            a2 = (A + 1.0f) - (A - 1.0f) * cosOmega - sqrtA2Alpha;
            break;
        }
            
        case AllPass:
            b0 = 1.0f - alpha;
            b1 = -2.0f * cosOmega;
            b2 = 1.0f + alpha;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cosOmega;
            a2 = 1.0f - alpha;
            break;
    }
    
    SetCoefficients(a0, a1, a2, b0, b1, b2);
}

void BiquadFilter::NormalizeCoefficients() {
    if (std::abs(m_a0) > 1e-10f) {
        const float invA0 = 1.0f / m_a0;
        m_b0 *= invA0;
        m_b1 *= invA0;
        m_b2 *= invA0;
        m_a1 *= invA0;
        m_a2 *= invA0;
        m_a0 = 1.0f;
    }
}

float BiquadFilter::ProcessSample(float input) {
    const float output = m_b0 * input + m_b1 * m_x1 + m_b2 * m_x2
                        - m_a1 * m_y1 - m_a2 * m_y2;
    
    m_x2 = m_x1;
    m_x1 = input;
    m_y2 = m_y1;
    m_y1 = output;
    
    return output;
}

void BiquadFilter::ProcessBlock(const float* input, float* output, size_t numSamples) {
    for (size_t i = 0; i < numSamples; ++i) {
        output[i] = ProcessSample(input[i]);
    }
}

void BiquadFilter::Reset() {
    m_x1 = m_x2 = 0.0f;
    m_y1 = m_y2 = 0.0f;
}

void BiquadFilter::GetCoefficients(float& a0, float& a1, float& a2,
                                   float& b0, float& b1, float& b2) const {
    a0 = m_a0;
    a1 = m_a1;
    a2 = m_a2;
    b0 = m_b0;
    b1 = m_b1;
    b2 = m_b2;
}

float BiquadFilter::GetFrequencyResponse(float frequency, float sampleRate) const {
    const float omega = 2.0f * M_PI_F * frequency / sampleRate;
    const float cosOmega = std::cos(omega);
    const float sinOmega = std::sin(omega);
    const float cos2Omega = std::cos(2.0f * omega);
    const float sin2Omega = std::sin(2.0f * omega);
    
    const float realNum = m_b0 + m_b1 * cosOmega + m_b2 * cos2Omega;
    const float imagNum = m_b1 * sinOmega + m_b2 * sin2Omega;
    
    const float realDen = 1.0f + m_a1 * cosOmega + m_a2 * cos2Omega;
    const float imagDen = m_a1 * sinOmega + m_a2 * sin2Omega;
    
    const float denMagSq = realDen * realDen + imagDen * imagDen;
    
    if (denMagSq > 1e-10f) {
        const float real = (realNum * realDen + imagNum * imagDen) / denMagSq;
        const float imag = (imagNum * realDen - realNum * imagDen) / denMagSq;
        return std::sqrt(real * real + imag * imag);
    }
    
    return 0.0f;
}

float BiquadFilter::GetMagnitudeResponse(float frequency, float sampleRate) const {
    return GetFrequencyResponse(frequency, sampleRate);
}

float BiquadFilter::GetPhaseResponse(float frequency, float sampleRate) const {
    const float omega = 2.0f * M_PI_F * frequency / sampleRate;
    const float cosOmega = std::cos(omega);
    const float sinOmega = std::sin(omega);
    const float cos2Omega = std::cos(2.0f * omega);
    const float sin2Omega = std::sin(2.0f * omega);
    
    const float realNum = m_b0 + m_b1 * cosOmega + m_b2 * cos2Omega;
    const float imagNum = m_b1 * sinOmega + m_b2 * sin2Omega;
    
    const float realDen = 1.0f + m_a1 * cosOmega + m_a2 * cos2Omega;
    const float imagDen = m_a1 * sinOmega + m_a2 * sin2Omega;
    
    const float numPhase = std::atan2(imagNum, realNum);
    const float denPhase = std::atan2(imagDen, realDen);
    
    return numPhase - denPhase;
}

EnvelopeFollower::EnvelopeFollower(float sampleRate)
    : m_sampleRate(sampleRate)
    , m_attackCoeff(0.0f)
    , m_releaseCoeff(0.0f)
    , m_envelope(0.0f)
    , m_rmsMode(false) {
    SetAttack(10.0f);
    SetRelease(100.0f);
}

void EnvelopeFollower::SetAttack(float attackMs) {
    const float attackSamples = (attackMs * 0.001f) * m_sampleRate;
    m_attackCoeff = 1.0f - std::exp(-1.0f / attackSamples);
}

void EnvelopeFollower::SetRelease(float releaseMs) {
    const float releaseSamples = (releaseMs * 0.001f) * m_sampleRate;
    m_releaseCoeff = 1.0f - std::exp(-1.0f / releaseSamples);
}

void EnvelopeFollower::SetMode(bool rms) {
    m_rmsMode = rms;
}

float EnvelopeFollower::ProcessSample(float input) {
    float rectified = m_rmsMode ? (input * input) : std::abs(input);
    
    if (rectified > m_envelope) {
        m_envelope += m_attackCoeff * (rectified - m_envelope);
    } else {
        m_envelope += m_releaseCoeff * (rectified - m_envelope);
    }
    
    return m_rmsMode ? std::sqrt(m_envelope) : m_envelope;
}

void EnvelopeFollower::ProcessBlock(const float* input, float* envelope, size_t numSamples) {
    for (size_t i = 0; i < numSamples; ++i) {
        envelope[i] = ProcessSample(input[i]);
    }
}

void EnvelopeFollower::Reset() {
    m_envelope = 0.0f;
}

SoftClipper::SoftClipper(ClipType type)
    : m_type(type)
    , m_threshold(0.9f)
    , m_knee(0.1f) {
}

void SoftClipper::SetThreshold(float threshold) {
    m_threshold = std::max(0.01f, std::min(1.0f, threshold));
}

void SoftClipper::SetKnee(float knee) {
    m_knee = std::max(0.0f, std::min(0.5f, knee));
}

void SoftClipper::SetType(ClipType type) {
    m_type = type;
}

float SoftClipper::ProcessSample(float input) {
    return ApplyClipping(input);
}

void SoftClipper::ProcessBlock(const float* input, float* output, size_t numSamples) {
    for (size_t i = 0; i < numSamples; ++i) {
        output[i] = ApplyClipping(input[i]);
    }
}

float SoftClipper::ApplyClipping(float input) const {
    const float absInput = std::abs(input);
    const float sign = input >= 0.0f ? 1.0f : -1.0f;
    
    switch (m_type) {
        case HardClip:
            return sign * std::min(absInput, m_threshold);
            
        case SoftClip: {
            if (absInput <= m_threshold - m_knee) {
                return input;
            } else if (absInput >= m_threshold + m_knee) {
                return sign * m_threshold;
            } else {
                const float x = (absInput - m_threshold + m_knee) / (2.0f * m_knee);
                const float y = 1.0f - (x - 1.0f) * (x - 1.0f);
                return sign * (m_threshold - m_knee + 2.0f * m_knee * y);
            }
        }
            
        case Tanh:
            return m_threshold * std::tanh(input / m_threshold);
            
        case Sigmoid:
            return m_threshold * (2.0f / (1.0f + std::exp(-2.0f * input / m_threshold)) - 1.0f);
            
        case Polynomial: {
            if (absInput <= m_threshold) {
                return input;
            } else {
                const float x = absInput / m_threshold;
                const float y = 1.5f * x - 0.5f * x * x * x;
                return sign * m_threshold * std::min(1.0f, y);
            }
        }
            
        default:
            return input;
    }
}

AllPassFilter::AllPassFilter(size_t maxDelay)
    : m_bufferSize(maxDelay)
    , m_writeIndex(0)
    , m_delay(0.0f)
    , m_feedback(0.5f) {
    m_buffer = new float[m_bufferSize];
    std::memset(m_buffer, 0, m_bufferSize * sizeof(float));
}

AllPassFilter::~AllPassFilter() {
    delete[] m_buffer;
}

void AllPassFilter::SetDelay(float delaySamples) {
    m_delay = std::max(0.0f, std::min(static_cast<float>(m_bufferSize - 1), delaySamples));
}

void AllPassFilter::SetFeedback(float feedback) {
    m_feedback = std::max(-0.99f, std::min(0.99f, feedback));
}

float AllPassFilter::ProcessSample(float input) {
    const float delayed = InterpolatedRead(m_delay);
    const float feedforward = input + delayed * m_feedback;
    
    m_buffer[m_writeIndex] = feedforward;
    m_writeIndex = (m_writeIndex + 1) % m_bufferSize;
    
    return delayed - feedforward * m_feedback;
}

void AllPassFilter::ProcessBlock(const float* input, float* output, size_t numSamples) {
    for (size_t i = 0; i < numSamples; ++i) {
        output[i] = ProcessSample(input[i]);
    }
}

void AllPassFilter::Reset() {
    std::memset(m_buffer, 0, m_bufferSize * sizeof(float));
    m_writeIndex = 0;
}

float AllPassFilter::InterpolatedRead(float delaySamples) const {
    const float readPos = static_cast<float>(m_writeIndex) - delaySamples - 1.0f;
    
    float readIndex = readPos;
    while (readIndex < 0.0f) {
        readIndex += static_cast<float>(m_bufferSize);
    }
    
    const size_t index0 = static_cast<size_t>(readIndex) % m_bufferSize;
    const size_t index1 = (index0 + 1) % m_bufferSize;
    const float frac = readIndex - std::floor(readIndex);
    
    return m_buffer[index0] * (1.0f - frac) + m_buffer[index1] * frac;
}

DCBlocker::DCBlocker(float cutoffFrequency, float sampleRate)
    : m_x1(0.0f)
    , m_y1(0.0f)
    , m_R(0.0f) {
    SetCutoff(cutoffFrequency, sampleRate);
}

void DCBlocker::SetCutoff(float frequency, float sampleRate) {
    m_R = 1.0f - (M_PI_F * frequency / sampleRate);
    m_R = std::max(0.0f, std::min(0.9999f, m_R));
}

float DCBlocker::ProcessSample(float input) {
    const float output = input - m_x1 + m_R * m_y1;
    m_x1 = input;
    m_y1 = output;
    return output;
}

void DCBlocker::ProcessBlock(const float* input, float* output, size_t numSamples) {
    for (size_t i = 0; i < numSamples; ++i) {
        output[i] = ProcessSample(input[i]);
    }
}

void DCBlocker::Reset() {
    m_x1 = m_y1 = 0.0f;
}

// Spatial Audio Processing Implementation

// DelayLine implementation
DelayLine::DelayLine(size_t maxDelaySamples) 
    : m_bufferSize(maxDelaySamples + 1), m_writeIndex(0), m_delay(0.0f) {
    m_buffer = new float[m_bufferSize];
    Reset();
}

DelayLine::~DelayLine() {
    delete[] m_buffer;
}

void DelayLine::SetDelay(float delaySamples) {
    m_delay = std::max(0.0f, std::min(delaySamples, static_cast<float>(m_bufferSize - 1)));
}

float DelayLine::ProcessSample(float input) {
    m_buffer[m_writeIndex] = input;
    float output = InterpolatedRead(m_delay);
    m_writeIndex = (m_writeIndex + 1) % m_bufferSize;
    return output;
}

void DelayLine::ProcessBlock(const float* input, float* output, size_t numSamples) {
    for (size_t i = 0; i < numSamples; ++i) {
        output[i] = ProcessSample(input[i]);
    }
}

void DelayLine::Reset() {
    for (size_t i = 0; i < m_bufferSize; ++i) {
        m_buffer[i] = 0.0f;
    }
    m_writeIndex = 0;
}

float DelayLine::InterpolatedRead(float delaySamples) const {
    float readPos = static_cast<float>(m_writeIndex) - delaySamples;
    if (readPos < 0.0f) {
        readPos += static_cast<float>(m_bufferSize);
    }
    
    size_t index1 = static_cast<size_t>(readPos) % m_bufferSize;
    size_t index2 = (index1 + 1) % m_bufferSize;
    float frac = readPos - std::floor(readPos);
    
    return m_buffer[index1] * (1.0f - frac) + m_buffer[index2] * frac;
}

// ConvolutionEngine implementation
ConvolutionEngine::ConvolutionEngine(size_t maxImpulseLength) 
    : m_impulseLength(0), m_bufferSize(maxImpulseLength), m_writeIndex(0) {
    m_impulseResponse = new float[maxImpulseLength];
    m_delayLine = new float[maxImpulseLength];
    Reset();
}

ConvolutionEngine::~ConvolutionEngine() {
    delete[] m_impulseResponse;
    delete[] m_delayLine;
}

void ConvolutionEngine::SetImpulseResponse(const float* impulse, size_t length) {
    m_impulseLength = std::min(length, m_bufferSize);
    for (size_t i = 0; i < m_impulseLength; ++i) {
        m_impulseResponse[i] = impulse[i];
    }
    for (size_t i = m_impulseLength; i < m_bufferSize; ++i) {
        m_impulseResponse[i] = 0.0f;
    }
}

float ConvolutionEngine::ProcessSample(float input) {
    m_delayLine[m_writeIndex] = input;
    
    float output = 0.0f;
    for (size_t i = 0; i < m_impulseLength; ++i) {
        size_t delayIndex = (m_writeIndex + m_bufferSize - i) % m_bufferSize;
        output += m_delayLine[delayIndex] * m_impulseResponse[i];
    }
    
    m_writeIndex = (m_writeIndex + 1) % m_bufferSize;
    return output;
}

void ConvolutionEngine::ProcessBlock(const float* input, float* output, size_t numSamples) {
    for (size_t i = 0; i < numSamples; ++i) {
        output[i] = ProcessSample(input[i]);
    }
}

void ConvolutionEngine::Reset() {
    for (size_t i = 0; i < m_bufferSize; ++i) {
        m_delayLine[i] = 0.0f;
    }
    m_writeIndex = 0;
}

// Vector3D implementation
float Vector3D::Distance(const Vector3D& other) const {
    return (*this - other).Magnitude();
}

Vector3D Vector3D::Normalize() const {
    float mag = Magnitude();
    if (mag > 1e-10f) {
        return *this * (1.0f / mag);
    }
    return Vector3D(0.0f, 0.0f, 1.0f); // Default forward
}

float Vector3D::Magnitude() const {
    return std::sqrt(x * x + y * y + z * z);
}

Vector3D Vector3D::operator-(const Vector3D& other) const {
    return Vector3D(x - other.x, y - other.y, z - other.z);
}

Vector3D Vector3D::operator+(const Vector3D& other) const {
    return Vector3D(x + other.x, y + other.y, z + other.z);
}

Vector3D Vector3D::operator*(float scalar) const {
    return Vector3D(x * scalar, y * scalar, z * scalar);
}

float Vector3D::Dot(const Vector3D& other) const {
    return x * other.x + y * other.y + z * other.z;
}

Vector3D Vector3D::Cross(const Vector3D& other) const {
    return Vector3D(
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x
    );
}

// SphericalCoordinate implementation
SphericalCoordinate SphericalCoordinate::FromCartesian(const Vector3D& cartesian) {
    float distance = cartesian.Magnitude();
    float azimuth = std::atan2(cartesian.y, cartesian.x);
    float elevation = std::asin(cartesian.z / std::max(distance, 1e-10f));
    return SphericalCoordinate(azimuth, elevation, distance);
}

Vector3D SphericalCoordinate::ToCartesian() const {
    float cosElevation = std::cos(elevation);
    return Vector3D(
        distance * cosElevation * std::cos(azimuth),
        distance * cosElevation * std::sin(azimuth),
        distance * std::sin(elevation)
    );
}

// SpatialAudioMath implementation
float SpatialAudioMath::CalculateDistanceAttenuation(float distance, float referenceDistance) {
    if (distance <= referenceDistance) {
        return 1.0f;
    }
    // Inverse square law with minimum attenuation
    return std::max(0.001f, referenceDistance / distance);
}

float SpatialAudioMath::CalculateAirAbsorption(float distance, float frequency, float humidity) {
    // Simplified air absorption model
    float absorptionCoeff = 0.0f;
    
    if (frequency > 1000.0f) {
        // High frequency absorption increases with distance and decreases with humidity
        absorptionCoeff = (frequency - 1000.0f) * 0.00001f * distance * (100.0f - humidity) * 0.01f;
    }
    
    return std::max(0.1f, 1.0f - absorptionCoeff);
}

float SpatialAudioMath::CalculateDopplerShift(const Vector3D& sourceVelocity, 
                                             const Vector3D& listenerVelocity,
                                             const Vector3D& sourceToListener,
                                             float speedOfSound) {
    if (sourceToListener.Magnitude() < 0.01f) return 1.0f; // Evita divisione per zero
    
    Vector3D direction = sourceToListener.Normalize();
    Vector3D relativeVelocity = sourceVelocity - listenerVelocity;
    float velocityTowardsListener = relativeVelocity.Dot(direction);
    
    // Doppler frequency shift factor
    // Velocità positiva = allontanamento (frequenza più bassa)
    // Velocità negativa = avvicinamento (frequenza più alta)
    float dopplerFactor = (speedOfSound + velocityTowardsListener) / speedOfSound;
    
    // Clamp per evitare valori estremi
    return std::max(0.5f, std::min(2.0f, dopplerFactor));
}

SphericalCoordinate SpatialAudioMath::CalculateRelativePosition(const Vector3D& source,
                                                               const Vector3D& listener,
                                                               const Vector3D& listenerForward,
                                                               const Vector3D& listenerUp) {
    Vector3D sourceToListener = source - listener;
    
    // Create listener coordinate system
    Vector3D forward = listenerForward.Normalize();
    Vector3D up = listenerUp.Normalize();
    Vector3D right = forward.Cross(up).Normalize();
    
    // Transform to listener coordinate system
    Vector3D relative(
        sourceToListener.Dot(right),
        sourceToListener.Dot(forward),
        sourceToListener.Dot(up)
    );
    
    return SphericalCoordinate::FromCartesian(relative);
}

float SpatialAudioMath::CalculateInterauralTimeDifference(float azimuth, float headRadius) {
    // Woodworth-Schlosberg formula for ITD
    float sampleRate = 44100.0f; // Default sample rate for conversion
    float soundSpeed = 343.0f;   // Speed of sound in m/s
    
    float itdSeconds = (headRadius / soundSpeed) * (azimuth + std::sin(azimuth));
    return itdSeconds * sampleRate; // Return in samples
}

float SpatialAudioMath::CalculateInterauralLevelDifference(float azimuth, float elevation) {
    // Simplified ILD model based on head shadowing
    float absAzimuth = std::abs(azimuth);
    
    // Head shadowing is most prominent for side sources
    float shadowing = std::sin(absAzimuth) * std::cos(elevation);
    
    // Convert to dB difference (typically 0-20 dB for extreme positions)
    return shadowing * 15.0f; // Maximum 15 dB ILD
}

}
}
/*
 * AdvancedAudioProcessor.cpp - Phase 3 Professional Audio Processing Engine
 * 
 * Advanced audio processing foundation implementation for VeniceDAW Phase 3.2.
 * Implements real DSP algorithms for professional audio effects.
 */

#include "AdvancedAudioProcessor.h"
#include <algorithm>
#include <cstring>
#include <cmath>

static constexpr float M_PI_F = 3.14159265358979323846f;

namespace VeniceDAW {

// AdvancedAudioBuffer implementation
AdvancedAudioBuffer::AdvancedAudioBuffer(ChannelConfiguration config, size_t frames, float sr)
    : frameCount(frames), sampleRate(sr), channelConfig(config) {
    size_t channelCount = static_cast<size_t>(config);
    channels.resize(channelCount);
    for (auto& channel : channels) {
        channel.resize(frames, 0.0f);
    }
}

void AdvancedAudioBuffer::Clear() {
    for (auto& channel : channels) {
        std::fill(channel.begin(), channel.end(), 0.0f);
    }
}

void AdvancedAudioBuffer::Resize(size_t frames) {
    frameCount = frames;
    for (auto& channel : channels) {
        channel.resize(frames, 0.0f);
    }
}

float* AdvancedAudioBuffer::GetChannelData(size_t channel) {
    if (channel >= channels.size()) return nullptr;
    return channels[channel].data();
}

const float* AdvancedAudioBuffer::GetChannelData(size_t channel) const {
    if (channel >= channels.size()) return nullptr;
    return channels[channel].data();
}

size_t AdvancedAudioBuffer::GetChannelCount() const {
    return channels.size();
}

// AudioEffect base class implementation
AudioEffect::AudioEffect(const std::string& name) : fName(name) {}

// ProfessionalEQ implementation
ProfessionalEQ::ProfessionalEQ() : AudioEffect("ProfessionalEQ") {
    // Initialize EQ bands with professional default settings
    fBands[0] = {60.0f, 0.0f, 0.707f, FilterType::HighPass, false};      // HPF at 60Hz
    fBands[1] = {150.0f, 0.0f, 1.0f, FilterType::LowShelf, false};       // Low shelf
    fBands[2] = {500.0f, 0.0f, 2.0f, FilterType::Peak, false};           // Low-mid peak
    fBands[3] = {1000.0f, 0.0f, 2.0f, FilterType::Peak, false};          // Mid peak
    fBands[4] = {2000.0f, 0.0f, 2.0f, FilterType::Peak, false};          // High-mid peak
    fBands[5] = {4000.0f, 0.0f, 2.0f, FilterType::Peak, false};          // Presence peak
    fBands[6] = {8000.0f, 0.0f, 1.0f, FilterType::HighShelf, false};     // High shelf
    fBands[7] = {16000.0f, 0.0f, 0.707f, FilterType::LowPass, false};    // Anti-aliasing LPF
}

void ProfessionalEQ::Initialize(float sampleRate) {
    fSampleRate = sampleRate;
    fInitialized = true;
    fNeedsUpdate.store(true);
}

void ProfessionalEQ::Process(AdvancedAudioBuffer& buffer) {
    if (fBypassed.load() || !fInitialized) {
        return;
    }
    
    InitializeChannels(buffer.GetChannelCount());
    
    if (fNeedsUpdate.load()) {
        UpdateFilters();
        fNeedsUpdate.store(false);
    }
    
    for (size_t channel = 0; channel < buffer.GetChannelCount(); ++channel) {
        float* channelData = buffer.GetChannelData(channel);
        
        for (size_t frame = 0; frame < buffer.frameCount; ++frame) {
            float sample = channelData[frame];
            
            // DC blocking
            sample = fDCBlockers[channel].ProcessSample(sample);
            
            // Process through all enabled EQ bands
            for (size_t band = 0; band < MAX_BANDS; ++band) {
                if (fBands[band].enabled) {
                    sample = fFilters[channel][band].ProcessSample(sample);
                }
            }
            
            channelData[frame] = sample;
        }
    }
}

void ProfessionalEQ::ProcessRealtime(AdvancedAudioBuffer& buffer) {
    Process(buffer);
}

float ProfessionalEQ::ProcessSample(float input, size_t channel) {
    if (fBypassed.load() || !fInitialized || channel >= fFilters.size()) {
        return input;
    }
    
    if (fNeedsUpdate.load()) {
        UpdateFilters();
        fNeedsUpdate.store(false);
    }
    
    float sample = input;
    
    // DC blocking
    sample = fDCBlockers[channel].ProcessSample(sample);
    
    // Process through all enabled EQ bands
    for (size_t band = 0; band < MAX_BANDS; ++band) {
        if (fBands[band].enabled) {
            sample = fFilters[channel][band].ProcessSample(sample);
        }
    }
    
    return sample;
}

void ProfessionalEQ::SetParameter(const std::string& param, float value) {
    // Parse parameter name to extract band and parameter type
    if (param.length() >= 7 && param.substr(0, 4) == "band") {
        size_t band = static_cast<size_t>(param[4] - '0');
        if (band < MAX_BANDS) {
            std::string paramType = param.substr(6);
            
            if (paramType == "freq") {
                SetBandFrequency(band, value);
            } else if (paramType == "gain") {
                SetBandGain(band, value);
            } else if (paramType == "q") {
                SetBandQ(band, value);
            } else if (paramType == "enabled") {
                SetBandEnabled(band, value > 0.5f);
            }
        }
    } else if (param == "bypass") {
        SetBypassed(value > 0.5f);
    }
}

float ProfessionalEQ::GetParameter(const std::string& param) const {
    if (param.length() >= 7 && param.substr(0, 4) == "band") {
        size_t band = static_cast<size_t>(param[4] - '0');
        if (band < MAX_BANDS) {
            std::string paramType = param.substr(6);
            
            if (paramType == "freq") {
                return fBands[band].frequency;
            } else if (paramType == "gain") {
                return fBands[band].gain;
            } else if (paramType == "q") {
                return fBands[band].Q;
            } else if (paramType == "enabled") {
                return fBands[band].enabled ? 1.0f : 0.0f;
            }
        }
    } else if (param == "bypass") {
        return fBypassed.load() ? 1.0f : 0.0f;
    }
    
    return 0.0f;
}

std::vector<std::string> ProfessionalEQ::GetParameterList() const {
    std::vector<std::string> params;
    
    for (size_t i = 0; i < MAX_BANDS; ++i) {
        std::string prefix = "band" + std::to_string(i) + "_";
        params.push_back(prefix + "freq");
        params.push_back(prefix + "gain");
        params.push_back(prefix + "q");
        params.push_back(prefix + "enabled");
    }
    
    params.push_back("bypass");
    
    return params;
}

void ProfessionalEQ::Reset() {
    for (auto& channelFilters : fFilters) {
        for (auto& filter : channelFilters) {
            filter.Reset();
        }
    }
    
    for (auto& dcBlocker : fDCBlockers) {
        dcBlocker.Reset();
    }
}

void ProfessionalEQ::SetBand(size_t band, float freq, float gain, float Q) {
    if (band < MAX_BANDS) {
        fBands[band].frequency = std::max(20.0f, std::min(20000.0f, freq));
        fBands[band].gain = std::max(-24.0f, std::min(24.0f, gain));
        fBands[band].Q = std::max(0.1f, std::min(20.0f, Q));
        fBands[band].enabled = true;
        fNeedsUpdate.store(true);
    }
}

void ProfessionalEQ::SetBandFrequency(size_t band, float freq) {
    if (band < MAX_BANDS) {
        fBands[band].frequency = std::max(20.0f, std::min(20000.0f, freq));
        fNeedsUpdate.store(true);
    }
}

void ProfessionalEQ::SetBandGain(size_t band, float gain) {
    if (band < MAX_BANDS) {
        fBands[band].gain = std::max(-24.0f, std::min(24.0f, gain));
        fNeedsUpdate.store(true);
    }
}

void ProfessionalEQ::SetBandQ(size_t band, float Q) {
    if (band < MAX_BANDS) {
        fBands[band].Q = std::max(0.1f, std::min(20.0f, Q));
        fNeedsUpdate.store(true);
    }
}

void ProfessionalEQ::SetBandType(size_t band, FilterType type) {
    if (band < MAX_BANDS) {
        fBands[band].type = type;
        fNeedsUpdate.store(true);
    }
}

void ProfessionalEQ::SetBandEnabled(size_t band, bool enabled) {
    if (band < MAX_BANDS) {
        fBands[band].enabled = enabled;
        fNeedsUpdate.store(true);
    }
}

void ProfessionalEQ::SetBypassed(bool bypassed) {
    fBypassed.store(bypassed);
}

ProfessionalEQ::EQBand ProfessionalEQ::GetBand(size_t band) const {
    if (band < MAX_BANDS) {
        return fBands[band];
    }
    return {0.0f, 0.0f, 1.0f, FilterType::Peak, false};
}

void ProfessionalEQ::EnableBand(size_t band, bool enabled) {
    SetBandEnabled(band, enabled);
}

float ProfessionalEQ::GetFrequencyResponse(float frequency) const {
    if (!fInitialized || fFilters.empty()) {
        return 1.0f;
    }
    
    float magnitude = 1.0f;
    
    for (size_t band = 0; band < MAX_BANDS; ++band) {
        if (fBands[band].enabled) {
            magnitude *= fFilters[0][band].GetMagnitudeResponse(frequency, fSampleRate);
        }
    }
    
    return magnitude;
}

void ProfessionalEQ::InitializeChannels(size_t channelCount) {
    if (fFilters.size() != channelCount) {
        fFilters.resize(channelCount);
        fDCBlockers.resize(channelCount);
        
        for (size_t channel = 0; channel < channelCount; ++channel) {
            fDCBlockers[channel].SetCutoff(20.0f, fSampleRate);
        }
        
        fNeedsUpdate.store(true);
    }
}

void ProfessionalEQ::UpdateFilters() {
    for (size_t band = 0; band < MAX_BANDS; ++band) {
        UpdateBandFilter(band);
    }
}

void ProfessionalEQ::UpdateBandFilter(size_t band) {
    if (band >= MAX_BANDS) return;
    
    const EQBand& bandData = fBands[band];
    DSP::BiquadFilter::FilterType dspType = ConvertFilterType(bandData.type);
    
    for (auto& channelFilters : fFilters) {
        channelFilters[band].CalculateCoefficients(
            dspType, 
            fSampleRate, 
            bandData.frequency, 
            bandData.Q, 
            bandData.gain
        );
    }
}

float ProfessionalEQ::ProcessBandFilter(size_t band, size_t channel, float input) {
    if (band >= MAX_BANDS || channel >= fFilters.size()) {
        return input;
    }
    
    return fFilters[channel][band].ProcessSample(input);
}

DSP::BiquadFilter::FilterType ProfessionalEQ::ConvertFilterType(FilterType type) const {
    switch (type) {
        case FilterType::LowPass:    return DSP::BiquadFilter::LowPass;
        case FilterType::HighPass:   return DSP::BiquadFilter::HighPass;
        case FilterType::LowShelf:   return DSP::BiquadFilter::LowShelf;
        case FilterType::HighShelf:  return DSP::BiquadFilter::HighShelf;
        case FilterType::Peak:       return DSP::BiquadFilter::Peak;
        case FilterType::Notch:      return DSP::BiquadFilter::Notch;
        case FilterType::BandPass:   return DSP::BiquadFilter::BandPass;
        case FilterType::AllPass:    return DSP::BiquadFilter::AllPass;
        default:                     return DSP::BiquadFilter::Peak;
    }
}

// DynamicsProcessor implementation
DynamicsProcessor::DynamicsProcessor() : AudioEffect("DynamicsProcessor") {}

void DynamicsProcessor::Initialize(float sampleRate) {
    fSampleRate = sampleRate;
    fInitialized = true;
    fNeedsUpdate.store(true);
    
    // Initialize lookahead parameters if needed
    if (fLookaheadEnabled) {
        UpdateLookaheadParameters();
    }
}

void DynamicsProcessor::Process(AdvancedAudioBuffer& buffer) {
    if (fBypassed.load() || !fInitialized) {
        return;
    }
    
    InitializeChannels(buffer.GetChannelCount());
    
    // Initialize lookahead buffers if enabled
    if (fLookaheadEnabled && (fLookaheadBuffers.empty() || fLookaheadBuffers.size() != buffer.GetChannelCount())) {
        InitializeLookaheadBuffers(buffer.GetChannelCount());
    }
    
    if (fNeedsUpdate.load()) {
        UpdateEnvelopeFollowers();
        fNeedsUpdate.store(false);
    }
    
    // Reset level meters
    fInputLevel = 0.0f;
    fOutputLevel = 0.0f;
    float maxGainReduction = 0.0f;
    
    for (size_t channel = 0; channel < buffer.GetChannelCount(); ++channel) {
        float* channelData = buffer.GetChannelData(channel);
        
        for (size_t frame = 0; frame < buffer.frameCount; ++frame) {
            float input = channelData[frame];
            float output;
            
            // Track input level
            float inputLevel = std::abs(input);
            fInputLevel = std::max(fInputLevel, inputLevel);
            
            // Use lookahead processing for limiters when enabled
            if (fLookaheadEnabled && fMode == Mode::LIMITER && !fLookaheadBuffers.empty()) {
                output = ProcessLookaheadSample(channel, input);
            } else {
                // Standard processing path
                // Process envelope
                float envelope = ProcessEnvelope(channel, input);
                
                // Calculate gain reduction
                float gainReduction = CalculateGainReduction(envelope);
                maxGainReduction = std::max(maxGainReduction, gainReduction);
                
                // Apply gain reduction (gainReduction is positive for reduction)
                float gain = std::pow(10.0f, -gainReduction / 20.0f); // Convert dB to linear
                output = input * gain;
                
                // Apply makeup gain
                if (fMakeupGain != 0.0f || fAutoMakeup) {
                    float makeupLinear = std::pow(10.0f, fMakeupGain / 20.0f);
                    output *= makeupLinear;
                }
            }
            
            // Track output level
            float outputLevel = std::abs(output);
            fOutputLevel = std::max(fOutputLevel, outputLevel);
            
            channelData[frame] = output;
        }
    }
    
    // Update gain reduction meter (use faster response for better accuracy)
    fGainReduction = fGainReduction * 0.5f + maxGainReduction * 0.5f;
}

void DynamicsProcessor::ProcessRealtime(AdvancedAudioBuffer& buffer) {
    Process(buffer);
}

void DynamicsProcessor::SetParameter(const std::string& param, float value) {
    if (param == "threshold") {
        fThreshold = std::max(-60.0f, std::min(0.0f, value));
        fNeedsUpdate.store(true);
    } else if (param == "ratio") {
        fRatio = std::max(1.0f, std::min(20.0f, value));
    } else if (param == "attack") {
        fAttack = std::max(0.1f, std::min(1000.0f, value));
        fNeedsUpdate.store(true);
    } else if (param == "release") {
        fRelease = std::max(1.0f, std::min(5000.0f, value));
        fNeedsUpdate.store(true);
    } else if (param == "knee") {
        fKnee = std::max(0.0f, std::min(10.0f, value));
    } else if (param == "makeup") {
        fMakeupGain = std::max(-20.0f, std::min(20.0f, value));
    } else if (param == "auto_makeup") {
        fAutoMakeup = value > 0.5f;
    } else if (param == "lookahead_enabled") {
        fLookaheadEnabled = value > 0.5f;
        if (fInitialized && fLookaheadEnabled) {
            UpdateLookaheadParameters();
        }
    } else if (param == "lookahead_time") {
        SetLookaheadTime(value);
    }
}

float DynamicsProcessor::GetParameter(const std::string& param) const {
    if (param == "threshold") return fThreshold;
    else if (param == "ratio") return fRatio;
    else if (param == "attack") return fAttack;
    else if (param == "release") return fRelease;
    else if (param == "knee") return fKnee;
    else if (param == "makeup") return fMakeupGain;
    else if (param == "auto_makeup") return fAutoMakeup ? 1.0f : 0.0f;
    else if (param == "lookahead_enabled") return fLookaheadEnabled ? 1.0f : 0.0f;
    else if (param == "lookahead_time") return fLookaheadTime;
    return 0.0f;
}

std::vector<std::string> DynamicsProcessor::GetParameterList() const {
    return {"threshold", "ratio", "attack", "release", "knee", "makeup", "auto_makeup", "lookahead_enabled", "lookahead_time"};
}

void DynamicsProcessor::Reset() {
    for (auto& follower : fEnvelopeFollowers) {
        follower.Reset();
    }
    fGainReduction = 0.0f;
    fInputLevel = 0.0f;
    fOutputLevel = 0.0f;
    
    // Reset lookahead buffers
    if (fLookaheadEnabled) {
        for (auto& buffer : fLookaheadBuffers) {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
        }
        std::fill(fBufferWritePos.begin(), fBufferWritePos.end(), 0);
        std::fill(fBufferReadPos.begin(), fBufferReadPos.end(), 0);
        std::fill(fPeakBuffer.begin(), fPeakBuffer.end(), 0.0f);
    }
}

void DynamicsProcessor::SetDetectionMode(DetectionMode mode) {
    fDetectionMode = mode;
    fNeedsUpdate.store(true);
}

void DynamicsProcessor::SetBypassed(bool bypassed) {
    fBypassed.store(bypassed);
}

void DynamicsProcessor::InitializeChannels(size_t channelCount) {
    if (fEnvelopeFollowers.size() != channelCount) {
        fEnvelopeFollowers.clear();
        fEnvelopeFollowers.reserve(channelCount);
        
        for (size_t i = 0; i < channelCount; ++i) {
            fEnvelopeFollowers.emplace_back(fSampleRate);
        }
        
        fNeedsUpdate.store(true);
    }
}

void DynamicsProcessor::UpdateEnvelopeFollowers() {
    for (auto& follower : fEnvelopeFollowers) {
        follower.SetAttack(fAttack);
        follower.SetRelease(fRelease);
        
        switch (fDetectionMode) {
            case DetectionMode::PEAK:
                follower.SetMode(false); // Peak mode
                break;
            case DetectionMode::RMS:
                follower.SetMode(true);  // RMS mode
                break;
            case DetectionMode::PEAK_RMS_HYBRID:
                follower.SetMode(true);  // Use RMS for now, can be enhanced
                break;
        }
    }
}

float DynamicsProcessor::ProcessEnvelope(size_t channel, float input) {
    if (channel >= fEnvelopeFollowers.size()) {
        return std::abs(input);
    }
    
    return fEnvelopeFollowers[channel].ProcessSample(input);
}

float DynamicsProcessor::CalculateGainReduction(float envelope) {
    if (envelope <= 0.0f) {
        return 0.0f;
    }
    
    // Convert envelope to dB
    float envelopedB = 20.0f * std::log10(envelope);
    
    switch (fMode) {
        case Mode::COMPRESSOR:
            return CalculateCompressorGain(envelopedB);
        case Mode::LIMITER:
            return CalculateLimiterGain(envelopedB);
        case Mode::GATE:
            return CalculateGateGain(envelopedB);
        case Mode::EXPANDER:
            return CalculateExpanderGain(envelopedB);
        default:
            return 0.0f;
    }
}

float DynamicsProcessor::ApplyKnee(float input, float threshold, float knee) {
    if (knee <= 0.0f) {
        // Hard knee
        return input >= threshold ? input : threshold;
    }
    
    // Soft knee
    float kneeStart = threshold - knee / 2.0f;
    float kneeEnd = threshold + knee / 2.0f;
    
    if (input <= kneeStart) {
        return input;
    } else if (input >= kneeEnd) {
        return threshold + (input - threshold) / fRatio;
    } else {
        // Smooth transition in knee region
        float x = (input - kneeStart) / knee;
        float y = x * x; // Quadratic curve
        float reduction = y * (input - threshold) / fRatio;
        return input - reduction;
    }
}

float DynamicsProcessor::CalculateCompressorGain(float inputdB) {
    if (inputdB <= fThreshold) {
        return 0.0f; // No compression below threshold
    }
    
    float overdB = inputdB - fThreshold;
    float compressedOverdB = overdB / fRatio;
    float gainReduction = overdB - compressedOverdB;
    
    // Apply knee if specified
    if (fKnee > 0.0f) {
        float kneeStart = fThreshold - fKnee / 2.0f;
        float kneeEnd = fThreshold + fKnee / 2.0f;
        
        if (inputdB >= kneeStart && inputdB <= kneeEnd) {
            // Soft knee interpolation
            float x = (inputdB - kneeStart) / fKnee;
            float softRatio = 1.0f + (fRatio - 1.0f) * x * x;
            compressedOverdB = overdB / softRatio;
            gainReduction = overdB - compressedOverdB;
        }
    }
    
    return gainReduction;
}

float DynamicsProcessor::CalculateLimiterGain(float inputdB) {
    if (inputdB <= fThreshold) {
        return 0.0f;
    }
    
    // Hard limiting - prevent signal from exceeding threshold
    return inputdB - fThreshold;
}

float DynamicsProcessor::CalculateGateGain(float inputdB) {
    if (inputdB >= fThreshold) {
        return 0.0f; // No gating above threshold
    }
    
    // Downward expansion below threshold (gate reduces signal)
    float belowdB = fThreshold - inputdB;
    float expandedBelowdB = belowdB * fRatio;
    // Return positive gain reduction (signal will be attenuated)
    return expandedBelowdB - belowdB;
}

float DynamicsProcessor::CalculateExpanderGain(float inputdB) {
    if (inputdB >= fThreshold) {
        return 0.0f; // No expansion above threshold
    }
    
    // Gentler expansion than gate (reduces signal but less aggressively)
    float belowdB = fThreshold - inputdB;
    float expandedBelowdB = belowdB * (1.0f + (fRatio - 1.0f) * 0.5f);
    // Return positive gain reduction
    return expandedBelowdB - belowdB;
}

// Lookahead functionality implementation
void DynamicsProcessor::SetLookaheadTime(float milliseconds) {
    fLookaheadTime = std::max(0.0f, std::min(milliseconds, 20.0f)); // Clamp to 0-20ms
    if (fInitialized) {
        UpdateLookaheadParameters();
    }
}

void DynamicsProcessor::InitializeLookaheadBuffers(size_t channelCount) {
    if (!fLookaheadEnabled) return;
    
    fLookaheadBuffers.clear();
    fBufferWritePos.clear();
    fBufferReadPos.clear();
    fPeakBuffer.clear();
    
    fLookaheadBuffers.resize(channelCount);
    fBufferWritePos.resize(channelCount, 0);
    fBufferReadPos.resize(channelCount, 0);
    fPeakBuffer.resize(fLookaheadSamples, 0.0f);
    
    for (size_t i = 0; i < channelCount; ++i) {
        fLookaheadBuffers[i].resize(fLookaheadSamples, 0.0f);
    }
}

void DynamicsProcessor::UpdateLookaheadParameters() {
    fLookaheadSamples = static_cast<size_t>(fLookaheadTime * fSampleRate / 1000.0f);
    
    if (fLookaheadEnabled && !fLookaheadBuffers.empty()) {
        // Reinitialize buffers with new size
        size_t channelCount = fLookaheadBuffers.size();
        InitializeLookaheadBuffers(channelCount);
    }
}

float DynamicsProcessor::ProcessLookaheadSample(size_t channel, float input) {
    if (!fLookaheadEnabled || channel >= fLookaheadBuffers.size()) {
        return input; // Bypass if lookahead disabled or invalid channel
    }
    
    // Write input sample to circular buffer
    WriteLookaheadSample(channel, input);
    
    // Analyze peak in lookahead window
    float lookaheadPeak = AnalyzeLookaheadPeak(channel);
    
    // Read delayed sample from buffer
    float delayedSample = ReadLookaheadSample(channel);
    
    // Calculate gain reduction based on lookahead peak
    float peakdB = 20.0f * std::log10(std::max(lookaheadPeak, 1e-6f));
    float gainReduction = 0.0f;
    
    if (fMode == Mode::LIMITER && peakdB > fThreshold) {
        // Apply immediate limiting for zero-latency response
        gainReduction = fThreshold - peakdB;
        gainReduction = std::max(gainReduction, -60.0f); // Limit maximum reduction
    }
    
    // Apply gain reduction to delayed sample
    float gainLinear = std::pow(10.0f, gainReduction / 20.0f);
    return delayedSample * gainLinear;
}

float DynamicsProcessor::AnalyzeLookaheadPeak(size_t channel) {
    if (channel >= fLookaheadBuffers.size()) return 0.0f;
    
    float peak = 0.0f;
    size_t readPos = fBufferReadPos[channel];
    
    // Analyze peak in the lookahead window
    for (size_t i = 0; i < fLookaheadSamples; ++i) {
        size_t pos = (readPos + i) % fLookaheadSamples;
        float sample = std::abs(fLookaheadBuffers[channel][pos]);
        peak = std::max(peak, sample);
    }
    
    return peak;
}

void DynamicsProcessor::WriteLookaheadSample(size_t channel, float sample) {
    if (channel >= fLookaheadBuffers.size()) return;
    
    fLookaheadBuffers[channel][fBufferWritePos[channel]] = sample;
    fBufferWritePos[channel] = (fBufferWritePos[channel] + 1) % fLookaheadSamples;
}

float DynamicsProcessor::ReadLookaheadSample(size_t channel) {
    if (channel >= fLookaheadBuffers.size()) return 0.0f;
    
    float sample = fLookaheadBuffers[channel][fBufferReadPos[channel]];
    fBufferReadPos[channel] = (fBufferReadPos[channel] + 1) % fLookaheadSamples;
    
    return sample;
}

// Professional SurroundProcessor implementation with 3D spatial audio
SurroundProcessor::SurroundProcessor(ChannelConfiguration config) : fChannelConfig(config) {
    InitializeChannelMixing();
}

SurroundProcessor::~SurroundProcessor() {
    // Unique pointers handle cleanup automatically
}

void SurroundProcessor::Initialize(float sampleRate) {
    fSampleRate = sampleRate;
    fInitialized = true;
    
    InitializeSpatialProcessing();
    InitializeHRTFProcessing();
    
    // Initialize crossfeed filters for stereo processing
    if (fCrossfeedEnabled) {
        fCrossfeedFilters.resize(2);
        for (auto& filter : fCrossfeedFilters) {
            filter.CalculateCoefficients(DSP::BiquadFilter::LowPass, sampleRate, 800.0f, 0.7f, 0.0f);
        }
    }
}

void SurroundProcessor::SetChannelConfiguration(ChannelConfiguration config) {
    fChannelConfig = config;
    InitializeChannelMixing();
    if (fInitialized) {
        InitializeSpatialProcessing();
    }
}

void SurroundProcessor::SetSpatialMode(SpatialMode mode) {
    fSpatialMode = mode;
    if (fInitialized) {
        UpdateSpatialParameters();
    }
}

void SurroundProcessor::ProcessStereoToSurround(const AdvancedAudioBuffer& stereo, AdvancedAudioBuffer& surround) {
    if (fSpatialMode == SpatialMode::BASIC_SURROUND) {
        ProcessIntelligentUpmix(stereo, surround);
    } else {
        // Advanced 3D upmixing
        size_t frames = std::min(stereo.frameCount, surround.frameCount);
        
        for (size_t frame = 0; frame < frames; ++frame) {
            float left = stereo.channels[0][frame];
            float right = stereo.channels[1][frame];
            float center = (left + right) * 0.5f;
            float surround_signal = (left - right) * 0.3f;
            
            if (surround.channels.size() >= 6) { // 5.1 surround
                // Apply spatial positioning
                float distance_atten = CalculateDistanceAttenuation();
                float doppler_factor = CalculateDopplerFactor();
                
                surround.channels[0][frame] = left * distance_atten * doppler_factor;     // Front left
                surround.channels[1][frame] = right * distance_atten * doppler_factor;   // Front right
                surround.channels[2][frame] = center * distance_atten * doppler_factor;  // Center
                surround.channels[3][frame] = center * 0.1f * distance_atten;           // LFE (low freq only)
                surround.channels[4][frame] = surround_signal * distance_atten;         // Rear left
                surround.channels[5][frame] = -surround_signal * distance_atten;        // Rear right
            }
        }
        
        // Apply bass management
        ProcessBassManagement(surround);
    }
}

void SurroundProcessor::ProcessSurroundToStereo(const AdvancedAudioBuffer& surround, AdvancedAudioBuffer& stereo) {
    size_t frames = std::min(surround.frameCount, stereo.frameCount);
    
    for (size_t frame = 0; frame < frames; ++frame) {
        float left = 0.0f, right = 0.0f;
        
        if (surround.channels.size() >= 6) { // 5.1 downmix with proper coefficients
            // ITU-R BS.775 downmix coefficients
            left = surround.channels[0][frame] +                           // Front left
                   surround.channels[2][frame] * 0.707f +                  // Center (−3 dB)
                   surround.channels[3][frame] * 0.707f +                  // LFE (−3 dB)
                   surround.channels[4][frame] * 0.707f;                   // Rear left (−3 dB)
                   
            right = surround.channels[1][frame] +                          // Front right
                    surround.channels[2][frame] * 0.707f +                 // Center (−3 dB)
                    surround.channels[3][frame] * 0.707f +                 // LFE (−3 dB)
                    surround.channels[5][frame] * 0.707f;                  // Rear right (−3 dB)
        }
        
        stereo.channels[0][frame] = left;
        stereo.channels[1][frame] = right;
    }
    
    // Apply crossfeed if enabled
    if (fCrossfeedEnabled) {
        ProcessCrossfeed(stereo);
    }
}

void SurroundProcessor::ProcessSpatial3D(AdvancedAudioBuffer& buffer) {
    if (fSpatialMode != SpatialMode::SPATIAL_3D) return;
    
    UpdateSpatialParameters();
    
    for (size_t channel = 0; channel < buffer.GetChannelCount(); ++channel) {
        if (fChannelMuted[channel]) continue;
        
        ProcessChannelSpatial(channel, buffer.GetChannelData(channel), buffer.frameCount);
    }
}

void SurroundProcessor::ProcessBinauralHRTF(const AdvancedAudioBuffer& mono, AdvancedAudioBuffer& stereo) {
    if (!fHRTFEnabled || !fLeftHRTF || !fRightHRTF) return;
    
    // Get relative position for HRTF lookup
    DSP::SphericalCoordinate relativePos = GetRelativePosition();
    UpdateHRTFParameters(relativePos.azimuth, relativePos.elevation);
    
    // Process HRTF convolution
    ProcessHRTFConvolution(mono.GetChannelData(0), 
                          stereo.GetChannelData(0), 
                          stereo.GetChannelData(1), 
                          mono.frameCount);
}

// Spatial positioning methods
void SurroundProcessor::SetSourcePosition(const DSP::Vector3D& position) {
    fSourcePosition = position;
}

void SurroundProcessor::SetListenerPosition(const DSP::Vector3D& position) {
    fListenerPosition = position;
}

void SurroundProcessor::SetListenerOrientation(const DSP::Vector3D& forward, const DSP::Vector3D& up) {
    fListenerForward = forward.Normalize();
    fListenerUp = up.Normalize();
}

void SurroundProcessor::SetSourceVelocity(const DSP::Vector3D& velocity) {
    fSourceVelocity = velocity;
}

void SurroundProcessor::SetListenerVelocity(const DSP::Vector3D& velocity) {
    fListenerVelocity = velocity;
}

// Environmental modeling
void SurroundProcessor::SetRoomSize(float width, float height, float depth) {
    fRoomSize = DSP::Vector3D(width, height, depth);
}

void SurroundProcessor::SetReverberation(float amount, float decay) {
    fReverbAmount = std::max(0.0f, std::min(1.0f, amount));
    fReverbDecay = std::max(0.1f, std::min(10.0f, decay));
}

void SurroundProcessor::SetAirAbsorption(bool enabled, float humidity) {
    fAirAbsorptionEnabled = enabled;
    fHumidity = std::max(0.0f, std::min(100.0f, humidity));
}

void SurroundProcessor::SetDopplerEffect(bool enabled, float speedOfSound) {
    fDopplerEnabled = enabled;
    fSpeedOfSound = speedOfSound;
}

// HRTF processing
void SurroundProcessor::SetHRTFDatabase(const float* leftHRTF, const float* rightHRTF, 
                                       size_t impulseLength, float azimuth, float elevation) {
    // Store current latency before HRTF
    size_t previousLatency = fLatencySamples;
    
    if (!fLeftHRTF) {
        fLeftHRTF = std::make_unique<DSP::ConvolutionEngine>(impulseLength);
        fRightHRTF = std::make_unique<DSP::ConvolutionEngine>(impulseLength);
    }
    
    fLeftHRTF->SetImpulseResponse(leftHRTF, impulseLength);
    fRightHRTF->SetImpulseResponse(rightHRTF, impulseLength);
    fHRTFEnabled = true;
    
    // HRTF adds convolution latency on top of base latency
    size_t hrtfLatency = impulseLength / 2; // Typical HRTF latency
    fLatencySamples = previousLatency + hrtfLatency;
    
    // Update processing load estimates with HRTF enabled
    UpdateSpatialParameters();
}

void SurroundProcessor::EnableCrossfeed(bool enabled, float amount) {
    fCrossfeedEnabled = enabled;
    fCrossfeedAmount = std::max(0.0f, std::min(1.0f, amount));
    
    // Initialize crossfeed filters if enabling and not already initialized
    if (enabled && fInitialized && fCrossfeedFilters.size() != 2) {
        fCrossfeedFilters.resize(2);
        for (auto& filter : fCrossfeedFilters) {
            filter.CalculateCoefficients(DSP::BiquadFilter::LowPass, fSampleRate, 800.0f, 0.7f, 0.0f);
        }
    }
}

// Spatial parameter getters
DSP::SphericalCoordinate SurroundProcessor::GetRelativePosition() const {
    return DSP::SpatialAudioMath::CalculateRelativePosition(
        fSourcePosition, fListenerPosition, fListenerForward, fListenerUp
    );
}

float SurroundProcessor::GetDistance() const {
    return fSourcePosition.Distance(fListenerPosition);
}

float SurroundProcessor::GetAzimuth() const {
    return GetRelativePosition().azimuth;
}

float SurroundProcessor::GetElevation() const {
    return GetRelativePosition().elevation;
}

// Channel routing implementation
void SurroundProcessor::SetChannelGain(size_t channel, float gain) {
    if (channel < fChannelGains.size()) {
        fChannelGains[channel] = gain;
    }
}

float SurroundProcessor::GetChannelGain(size_t channel) const {
    if (channel < fChannelGains.size()) {
        return fChannelGains[channel];
    }
    return 1.0f;
}

void SurroundProcessor::MuteChannel(size_t channel, bool muted) {
    if (channel < fChannelMuted.size()) {
        fChannelMuted[channel] = muted;
    }
}

bool SurroundProcessor::IsChannelMuted(size_t channel) const {
    if (channel < fChannelMuted.size()) {
        return fChannelMuted[channel];
    }
    return false;
}

void SurroundProcessor::ProcessSurroundDownmix(const AdvancedAudioBuffer& input, AdvancedAudioBuffer& output) {
    ProcessSurroundToStereo(input, output);
}

// Private implementation methods
void SurroundProcessor::InitializeChannelMixing() {
    size_t channelCount = static_cast<size_t>(fChannelConfig);
    fChannelGains.resize(channelCount, 1.0f);
    fChannelMuted.resize(channelCount, false);
}

void SurroundProcessor::InitializeSpatialProcessing() {
    if (!fInitialized) return;
    
    size_t channelCount = static_cast<size_t>(fChannelConfig);
    size_t maxDelaySamples = static_cast<size_t>(fSampleRate * 0.05f); // 50ms max delay
    
    fSpatialDelays.clear();
    for (size_t i = 0; i < channelCount; ++i) {
        fSpatialDelays.push_back(std::make_unique<DSP::DelayLine>(maxDelaySamples));
    }
}

void SurroundProcessor::InitializeHRTFProcessing() {
    // HRTF engines are created when HRTF database is loaded
    if (fSampleRate > 0) {
        fLatencySamples = static_cast<size_t>(fSampleRate * 0.005f); // 5ms default latency
    }
}

void SurroundProcessor::UpdateSpatialParameters() {
    // Update processing load estimate based on current mode and features
    float load = 0.1f; // Base load
    
    // Mode-specific loads
    if (fSpatialMode == SpatialMode::SPATIAL_3D) {
        load += 0.3f;
    } else if (fSpatialMode == SpatialMode::BINAURAL_HRTF) {
        load += 0.2f; // Base HRTF mode load
        if (fHRTFEnabled) {
            load += 0.5f; // Additional load for actual HRTF processing
        }
    } else if (fSpatialMode == SpatialMode::AMBISONICS) {
        load += 0.4f;
    }
    
    // Feature-specific loads
    if (fDopplerEnabled) load += 0.1f;
    if (fAirAbsorptionEnabled) load += 0.1f;
    if (fCrossfeedEnabled) load += 0.05f;
    
    fProcessingLoad.store(load);
}

float SurroundProcessor::CalculateChannelDelay(size_t channel) {
    DSP::SphericalCoordinate relativePos = GetRelativePosition();
    
    // Speaker position angles (5.1 setup)
    static const float speakerAngles[] = {
        -30.0f * M_PI / 180.0f,  // Front left
         30.0f * M_PI / 180.0f,  // Front right
          0.0f * M_PI / 180.0f,  // Center
          0.0f * M_PI / 180.0f,  // LFE (same as center)
        -110.0f * M_PI / 180.0f, // Rear left
         110.0f * M_PI / 180.0f  // Rear right
    };
    
    if (channel >= sizeof(speakerAngles) / sizeof(float)) {
        return 0.0f;
    }
    
    // Calculate delay based on angle difference
    float angleDiff = std::abs(relativePos.azimuth - speakerAngles[channel]);
    float delaySamples = (angleDiff / (2.0f * M_PI)) * (fSampleRate / 1000.0f); // Max 1ms delay
    
    return delaySamples;
}

float SurroundProcessor::CalculateChannelGain(size_t channel) {
    DSP::SphericalCoordinate relativePos = GetRelativePosition();
    float distance = relativePos.distance;
    
    // Basic distance attenuation
    float distanceGain = CalculateDistanceAttenuation();
    
    // Air absorption
    float airGain = 1.0f;
    if (fAirAbsorptionEnabled) {
        airGain = DSP::SpatialAudioMath::CalculateAirAbsorption(distance, 1000.0f, fHumidity);
    }
    
    return distanceGain * airGain * fChannelGains[channel];
}

float SurroundProcessor::CalculateDistanceAttenuation() {
    float distance = GetDistance();
    return DSP::SpatialAudioMath::CalculateDistanceAttenuation(distance, 1.0f);
}

float SurroundProcessor::CalculateAirAbsorptionFactor(float frequency) {
    if (!fAirAbsorptionEnabled) return 1.0f;
    
    float distance = GetDistance();
    return DSP::SpatialAudioMath::CalculateAirAbsorption(distance, frequency, fHumidity);
}

float SurroundProcessor::CalculateDopplerFactor() {
    if (!fDopplerEnabled) return 1.0f;
    
    DSP::Vector3D sourceToListener = fListenerPosition - fSourcePosition;
    if (sourceToListener.Magnitude() < 0.01f) return 1.0f; // Evita divisione per zero
    
    return DSP::SpatialAudioMath::CalculateDopplerShift(
        fSourceVelocity, fListenerVelocity, sourceToListener, fSpeedOfSound
    );
}

void SurroundProcessor::ProcessChannelSpatial(size_t channel, float* buffer, size_t numSamples) {
    if (channel >= fSpatialDelays.size()) return;
    
    // Apply spatial delay
    ApplySpatialDelay(channel, buffer, numSamples);
    
    // Apply spatial gain
    ApplySpatialGain(channel, buffer, numSamples);
}

void SurroundProcessor::ApplySpatialDelay(size_t channel, float* buffer, size_t numSamples) {
    if (channel >= fSpatialDelays.size()) return;
    
    float delaySamples = CalculateChannelDelay(channel);
    fSpatialDelays[channel]->SetDelay(delaySamples);
    
    for (size_t i = 0; i < numSamples; ++i) {
        buffer[i] = fSpatialDelays[channel]->ProcessSample(buffer[i]);
    }
}

void SurroundProcessor::ApplySpatialGain(size_t channel, float* buffer, size_t numSamples) {
    float gain = CalculateChannelGain(channel);
    
    for (size_t i = 0; i < numSamples; ++i) {
        buffer[i] *= gain;
    }
}

void SurroundProcessor::ProcessHRTFConvolution(const float* monoInput, float* leftOutput, 
                                              float* rightOutput, size_t numSamples) {
    if (!fLeftHRTF || !fRightHRTF) return;
    
    fLeftHRTF->ProcessBlock(monoInput, leftOutput, numSamples);
    fRightHRTF->ProcessBlock(monoInput, rightOutput, numSamples);
}

void SurroundProcessor::UpdateHRTFParameters(float azimuth, float elevation) {
    // In a real implementation, this would interpolate between HRTF measurements
    // For now, we assume the HRTF has been set for the current position
}

void SurroundProcessor::ProcessIntelligentUpmix(const AdvancedAudioBuffer& stereo, AdvancedAudioBuffer& surround) {
    size_t frames = std::min(stereo.frameCount, surround.frameCount);
    
    for (size_t frame = 0; frame < frames; ++frame) {
        float left = stereo.channels[0][frame];
        float right = stereo.channels[1][frame];
        float center = (left + right) * 0.5f;
        float side = (left - right) * 0.5f;
        
        if (surround.channels.size() >= 6) { // 5.1 intelligent upmix
            surround.channels[0][frame] = left;                    // Front left
            surround.channels[1][frame] = right;                   // Front right
            surround.channels[2][frame] = center * 0.7f;           // Center (attenuated)
            surround.channels[3][frame] = center * 0.1f;           // LFE (bass only)
            surround.channels[4][frame] = side * 0.5f;             // Rear left
            surround.channels[5][frame] = -side * 0.5f;            // Rear right
        }
    }
}

void SurroundProcessor::ProcessBassManagement(AdvancedAudioBuffer& buffer) {
    if (buffer.GetChannelCount() < 4) return; // Need at least LFE channel
    
    // Simple bass management: redirect low frequencies to LFE (80 Hz crossover)
    for (size_t frame = 0; frame < buffer.frameCount; ++frame) {
        float bassSum = 0.0f;
        
        // Sum bass from main channels (simplified)
        for (size_t channel = 0; channel < buffer.GetChannelCount(); ++channel) {
            if (channel == 3) continue; // Skip LFE channel itself
            
            float sample = buffer.channels[channel][frame];
            // Simple high-pass filtering (remove bass from main channels)
            // In practice, this would use proper filter implementation
            bassSum += sample * 0.1f; // Collect 10% for LFE
            buffer.channels[channel][frame] *= 0.9f; // Attenuate bass in main channel
        }
        
        // Add collected bass to LFE
        if (buffer.GetChannelCount() > 3) {
            buffer.channels[3][frame] += bassSum;
        }
    }
}

void SurroundProcessor::ProcessCrossfeed(AdvancedAudioBuffer& stereo) {
    if (!fCrossfeedEnabled || stereo.GetChannelCount() < 2 || fCrossfeedFilters.size() != 2) return;
    
    for (size_t frame = 0; frame < stereo.frameCount; ++frame) {
        float left = stereo.channels[0][frame];
        float right = stereo.channels[1][frame];
        
        // Apply crossfeed
        float crossfeedL = fCrossfeedFilters[0].ProcessSample(right) * fCrossfeedAmount;
        float crossfeedR = fCrossfeedFilters[1].ProcessSample(left) * fCrossfeedAmount;
        
        stereo.channels[0][frame] = left + crossfeedL;
        stereo.channels[1][frame] = right + crossfeedR;
    }
}

// AdvancedAudioProcessor implementation
AdvancedAudioProcessor::AdvancedAudioProcessor() : fSurroundProcessor(ChannelConfiguration::STEREO) {}

void AdvancedAudioProcessor::Initialize(float sampleRate, size_t bufferSize, ChannelConfiguration config) {
    fSampleRate = sampleRate;
    fBufferSize = bufferSize;
    fChannelConfig = config;
    
    // Initialize enhanced surround processor with spatial audio capabilities
    fSurroundProcessor.SetChannelConfiguration(config);
    fSurroundProcessor.Initialize(sampleRate);
    
    ValidateConfiguration();
    fInitialized = true;
}

void AdvancedAudioProcessor::Shutdown() {
    fEffects.clear();
    fInitialized = false;
}

void AdvancedAudioProcessor::AddEffect(std::unique_ptr<AudioEffect> effect) {
    if (effect) {
        fEffects.push_back(std::move(effect));
    }
}

void AdvancedAudioProcessor::RemoveEffect(const std::string& name) {
    fEffects.erase(
        std::remove_if(fEffects.begin(), fEffects.end(),
            [&name](const std::unique_ptr<AudioEffect>& effect) {
                return effect->GetName() == name;
            }),
        fEffects.end());
}

AudioEffect* AdvancedAudioProcessor::GetEffect(const std::string& name) {
    for (auto& effect : fEffects) {
        if (effect->GetName() == name) {
            return effect.get();
        }
    }
    return nullptr;
}

void AdvancedAudioProcessor::ClearEffects() {
    fEffects.clear();
}

void AdvancedAudioProcessor::ProcessBuffer(AdvancedAudioBuffer& buffer) {
    if (!fInitialized) return;
    
    // Process through effect chain
    for (auto& effect : fEffects) {
        if (!effect->IsBypassed()) {
            effect->Process(buffer);
        }
    }
}

void AdvancedAudioProcessor::ProcessRealtimeBuffer(AdvancedAudioBuffer& buffer) {
    if (!fInitialized) return;
    
    // Process through effect chain with realtime constraints
    for (auto& effect : fEffects) {
        if (!effect->IsBypassed()) {
            effect->ProcessRealtime(buffer);
        }
    }
}

float AdvancedAudioProcessor::GetTotalCPUUsage() const {
    float totalCPU = 0.0f;
    for (const auto& effect : fEffects) {
        totalCPU += effect->GetCPUUsage();
    }
    return totalCPU;
}

size_t AdvancedAudioProcessor::GetTotalLatency() const {
    size_t totalLatency = 0;
    for (const auto& effect : fEffects) {
        totalLatency += effect->GetLatencySamples();
    }
    return totalLatency;
}

void AdvancedAudioProcessor::UpdatePerformanceMetrics() {
    fTotalCPUUsage = GetTotalCPUUsage();
    fTotalLatency = GetTotalLatency();
}

void AdvancedAudioProcessor::SetChannelConfiguration(ChannelConfiguration config) {
    fChannelConfig = config;
    fSurroundProcessor.SetChannelConfiguration(config);
}

void AdvancedAudioProcessor::ValidateConfiguration() {
    // Stub - real implementation would validate sample rate, buffer size, etc.
}

} // namespace VeniceDAW
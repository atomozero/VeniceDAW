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

void DynamicsProcessor::Process(AdvancedAudioBuffer& buffer) {
    // Stub implementation - basic passthrough
    // Real implementation would apply compression/limiting
}

void DynamicsProcessor::ProcessRealtime(AdvancedAudioBuffer& buffer) {
    Process(buffer);
}

void DynamicsProcessor::SetParameter(const std::string& param, float value) {
    if (param == "threshold") fThreshold = value;
    else if (param == "ratio") fRatio = value;
    else if (param == "attack") fAttack = value;
    else if (param == "release") fRelease = value;
    else if (param == "knee") fKnee = value;
}

float DynamicsProcessor::GetParameter(const std::string& param) const {
    if (param == "threshold") return fThreshold;
    else if (param == "ratio") return fRatio;
    else if (param == "attack") return fAttack;
    else if (param == "release") return fRelease;
    else if (param == "knee") return fKnee;
    return 0.0f;
}

std::vector<std::string> DynamicsProcessor::GetParameterList() const {
    return {"threshold", "ratio", "attack", "release", "knee"};
}

void DynamicsProcessor::Reset() {
    fEnvelope.clear();
    fGainReduction = 0.0f;
}

float DynamicsProcessor::ProcessEnvelope(size_t channel, float input) {
    // Stub - real implementation would track signal envelope
    return std::abs(input);
}

float DynamicsProcessor::CalculateGainReduction(float envelope) {
    // Stub - real implementation would calculate compression curve
    return 0.0f;
}

// SurroundProcessor implementation
SurroundProcessor::SurroundProcessor(ChannelConfiguration config) : fChannelConfig(config) {
    InitializeChannelMixing();
}

void SurroundProcessor::SetChannelConfiguration(ChannelConfiguration config) {
    fChannelConfig = config;
    InitializeChannelMixing();
}

void SurroundProcessor::ProcessStereoToSurround(const AdvancedAudioBuffer& stereo, AdvancedAudioBuffer& surround) {
    // Stub implementation - basic channel duplication
    size_t frames = std::min(stereo.frameCount, surround.frameCount);
    for (size_t frame = 0; frame < frames; ++frame) {
        float left = stereo.channels[0][frame];
        float right = stereo.channels[1][frame];
        
        // Basic surround upmixing (center = mono sum, others = stereo)
        if (surround.channels.size() >= 6) { // 5.1 surround
            surround.channels[0][frame] = left;   // Front left
            surround.channels[1][frame] = right;  // Front right
            surround.channels[2][frame] = (left + right) * 0.5f; // Center
            surround.channels[3][frame] = 0.0f;   // LFE
            surround.channels[4][frame] = left * 0.3f;  // Rear left
            surround.channels[5][frame] = right * 0.3f; // Rear right
        }
    }
}

void SurroundProcessor::ProcessSurroundToStereo(const AdvancedAudioBuffer& surround, AdvancedAudioBuffer& stereo) {
    // Stub implementation - basic downmix
    size_t frames = std::min(surround.frameCount, stereo.frameCount);
    for (size_t frame = 0; frame < frames; ++frame) {
        float left = 0.0f, right = 0.0f;
        
        if (surround.channels.size() >= 6) { // 5.1 downmix
            left = surround.channels[0][frame] + surround.channels[2][frame] * 0.7f + surround.channels[4][frame];
            right = surround.channels[1][frame] + surround.channels[2][frame] * 0.7f + surround.channels[5][frame];
        }
        
        stereo.channels[0][frame] = left;
        stereo.channels[1][frame] = right;
    }
}

void SurroundProcessor::ProcessSurroundDownmix(const AdvancedAudioBuffer& input, AdvancedAudioBuffer& output) {
    ProcessSurroundToStereo(input, output);
}

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

void SurroundProcessor::SetSpatialPosition(float x, float y, float z) {
    fSourceX = x; fSourceY = y; fSourceZ = z;
}

void SurroundProcessor::SetListenerPosition(float x, float y, float z) {
    fListenerX = x; fListenerY = y; fListenerZ = z;
}

void SurroundProcessor::InitializeChannelMixing() {
    size_t channelCount = static_cast<size_t>(fChannelConfig);
    fChannelGains.resize(channelCount, 1.0f);
    fChannelMuted.resize(channelCount, false);
}

float SurroundProcessor::CalculateChannelDelay(size_t channel) {
    // Stub - real implementation would calculate delay based on spatial position
    return 0.0f;
}

float SurroundProcessor::CalculateChannelGain(size_t channel) {
    // Stub - real implementation would calculate gain based on spatial position
    return 1.0f;
}

// AdvancedAudioProcessor implementation
AdvancedAudioProcessor::AdvancedAudioProcessor() : fSurroundProcessor(ChannelConfiguration::STEREO) {}

void AdvancedAudioProcessor::Initialize(float sampleRate, size_t bufferSize, ChannelConfiguration config) {
    fSampleRate = sampleRate;
    fBufferSize = bufferSize;
    fChannelConfig = config;
    fSurroundProcessor.SetChannelConfiguration(config);
    
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
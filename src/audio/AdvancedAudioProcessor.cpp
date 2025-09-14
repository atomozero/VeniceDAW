/*
 * AdvancedAudioProcessor.cpp - Phase 3 Professional Audio Processing Engine
 * 
 * Advanced audio processing foundation implementation for VeniceDAW Phase 3.1.
 * Supports surround sound, professional effects, and spatial audio processing.
 */

#include "AdvancedAudioProcessor.h"
#include <algorithm>
#include <cstring>
#include <cmath>

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
    // Initialize EQ bands with default settings
    for (size_t i = 0; i < MAX_BANDS; ++i) {
        fBands[i] = {1000.0f * (i + 1), 0.0f, 1.0f, false};
    }
}

void ProfessionalEQ::Process(AdvancedAudioBuffer& buffer) {
    // Stub implementation - basic passthrough
    // Real implementation would apply EQ filtering
}

void ProfessionalEQ::ProcessRealtime(AdvancedAudioBuffer& buffer) {
    Process(buffer);
}

void ProfessionalEQ::SetParameter(const std::string& param, float value) {
    // Stub implementation for parameter setting
}

float ProfessionalEQ::GetParameter(const std::string& param) const {
    // Stub implementation for parameter getting
    return 0.0f;
}

std::vector<std::string> ProfessionalEQ::GetParameterList() const {
    return {"band0_freq", "band0_gain", "band0_q", "band1_freq", "band1_gain", "band1_q"};
}

void ProfessionalEQ::Reset() {
    // Clear delay lines and reset state
    fDelayLines.clear();
}

void ProfessionalEQ::SetBand(size_t band, float freq, float gain, float Q) {
    if (band < MAX_BANDS) {
        fBands[band] = {freq, gain, Q, true};
    }
}

ProfessionalEQ::EQBand ProfessionalEQ::GetBand(size_t band) const {
    if (band < MAX_BANDS) {
        return fBands[band];
    }
    return {0.0f, 0.0f, 1.0f, false};
}

void ProfessionalEQ::EnableBand(size_t band, bool enabled) {
    if (band < MAX_BANDS) {
        fBands[band].enabled = enabled;
    }
}

void ProfessionalEQ::UpdateFilters() {
    // Stub - real implementation would recalculate filter coefficients
}

float ProfessionalEQ::ProcessBandFilter(size_t band, size_t channel, float input) {
    // Stub - real implementation would apply biquad filtering
    return input;
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
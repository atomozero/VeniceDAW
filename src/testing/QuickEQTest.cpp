#include <iostream>
#include <cmath>
#include "../audio/AdvancedAudioProcessor.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace VeniceDAW;

int main() {
    std::cout << "🧪 Quick EQ Test - Phase 3.2" << std::endl;
    
    // Create and initialize EQ
    ProfessionalEQ eq;
    eq.Initialize(44100.0f);
    eq.SetBypassed(false);
    
    // Set up a simple peak filter at 1kHz with +12dB gain
    eq.SetBandEnabled(0, true);
    eq.SetBandType(0, ProfessionalEQ::kPeak);
    eq.SetBandFrequency(0, 1000.0f);
    eq.SetBandGain(0, 12.0f);
    eq.SetBandQ(0, 1.0f);
    
    std::cout << "✅ EQ configured: Peak filter at 1kHz, +12dB" << std::endl;
    
    // Test with a small buffer
    const size_t bufferSize = 1024;
    AdvancedAudioBuffer buffer(kMono, bufferSize, 44100.0f);
    
    // Generate 1kHz sine wave
    float* channelData = buffer.GetChannelData(0);
    for (size_t i = 0; i < bufferSize; ++i) {
        float t = static_cast<float>(i) / 44100.0f;
        channelData[i] = 0.5f * std::sin(2.0f * M_PI * 1000.0f * t);
    }
    
    // Calculate input RMS
    float inputRMS = 0.0f;
    for (size_t i = 0; i < bufferSize; ++i) {
        inputRMS += channelData[i] * channelData[i];
    }
    inputRMS = std::sqrt(inputRMS / bufferSize);
    
    std::cout << "📊 Input RMS: " << inputRMS << std::endl;
    
    // Process through EQ
    eq.Process(buffer);
    
    // Calculate output RMS
    float outputRMS = 0.0f;
    for (size_t i = 0; i < bufferSize; ++i) {
        outputRMS += channelData[i] * channelData[i];
    }
    outputRMS = std::sqrt(outputRMS / bufferSize);
    
    std::cout << "📊 Output RMS: " << outputRMS << std::endl;
    
    if (outputRMS > 0.0f && inputRMS > 0.0f) {
        float gainDB = 20.0f * std::log10(outputRMS / inputRMS);
        std::cout << "🎛️ Measured gain: " << gainDB << " dB (expected ~12dB)" << std::endl;
        
        if (std::abs(gainDB - 12.0f) < 2.0f) {
            std::cout << "✅ EQ is working correctly!" << std::endl;
            return 0;
        } else {
            std::cout << "❌ EQ gain is incorrect" << std::endl;
            return 1;
        }
    } else {
        std::cout << "❌ RMS calculation failed" << std::endl;
        return 1;
    }
}
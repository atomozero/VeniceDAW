#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <complex>
#include "../audio/AdvancedAudioProcessor.h"
#include "../audio/DSPAlgorithms.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace VeniceDAW;

class ProfessionalEQTest {
public:
    bool RunAllTests() {
        std::cout << "\n=== ProfessionalEQ DSP Tests ===" << std::endl;
        
        bool allPassed = true;
        
        allPassed &= TestBiquadFilter();
        allPassed &= TestFrequencyResponse();
        allPassed &= TestEQBandProcessing();
        allPassed &= TestFullEQChain();
        allPassed &= TestParameterSmoothing();
        allPassed &= TestBypassFunctionality();
        
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << (allPassed ? "✓ All tests PASSED" : "✗ Some tests FAILED") << std::endl;
        
        return allPassed;
    }

private:
    bool TestBiquadFilter() {
        std::cout << "\n[TEST] Biquad Filter Implementation..." << std::endl;
        
        DSP::BiquadFilter filter;
        const float sampleRate = 44100.0f;
        
        filter.CalculateCoefficients(DSP::BiquadFilter::Peak, sampleRate, 1000.0f, 1.0f, 6.0f);
        
        const float testFreq = 1000.0f;
        float magnitude = filter.GetMagnitudeResponse(testFreq, sampleRate);
        float magnitudeDB = 20.0f * std::log10(magnitude);
        
        std::cout << "  Peak filter at 1kHz with +6dB gain:" << std::endl;
        std::cout << "    Magnitude at 1kHz: " << magnitudeDB << " dB" << std::endl;
        
        bool peakTest = std::abs(magnitudeDB - 6.0f) < 0.5f;
        
        filter.CalculateCoefficients(DSP::BiquadFilter::LowPass, sampleRate, 1000.0f, 0.707f, 0.0f);
        magnitude = filter.GetMagnitudeResponse(500.0f, sampleRate);
        float mag500 = 20.0f * std::log10(magnitude);
        magnitude = filter.GetMagnitudeResponse(2000.0f, sampleRate);
        float mag2000 = 20.0f * std::log10(magnitude);
        
        std::cout << "  LowPass filter at 1kHz:" << std::endl;
        std::cout << "    Magnitude at 500Hz: " << mag500 << " dB" << std::endl;
        std::cout << "    Magnitude at 2kHz: " << mag2000 << " dB" << std::endl;
        
        bool lowpassTest = (mag500 > -1.0f) && (mag2000 < -6.0f);
        
        const size_t blockSize = 512;
        std::vector<float> input(blockSize, 0.0f);
        std::vector<float> output(blockSize);
        
        input[0] = 1.0f;
        
        filter.Reset();
        filter.CalculateCoefficients(DSP::BiquadFilter::Peak, sampleRate, 1000.0f, 2.0f, 0.0f);
        filter.ProcessBlock(input.data(), output.data(), blockSize);
        
        float energy = 0.0f;
        for (float sample : output) {
            energy += sample * sample;
        }
        
        std::cout << "  Impulse response energy: " << energy << std::endl;
        bool impulseTest = energy > 0.5f && energy < 2.0f;
        
        bool passed = peakTest && lowpassTest && impulseTest;
        std::cout << "  Result: " << (passed ? "PASSED ✓" : "FAILED ✗") << std::endl;
        
        return passed;
    }
    
    bool TestFrequencyResponse() {
        std::cout << "\n[TEST] Frequency Response Analysis..." << std::endl;
        
        const float testFrequencies[] = {100.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f};
        const float expectedGains[] = {0.0f, 3.0f, -3.0f, 6.0f, -6.0f, 3.0f, -3.0f, 0.0f};
        
        bool allPassed = true;
        
        for (int testBand = 0; testBand < 8; ++testBand) {
            // Skip bands with 0dB gain for this test
            if (std::abs(expectedGains[testBand]) < 0.1f) continue;
            
            ProfessionalEQ eq;
            eq.Initialize(44100.0f);
            eq.SetBypassed(false);
            
            // Enable only the current test band
            for (int band = 0; band < 8; ++band) {
                eq.SetBandEnabled(band, false);
            }
            
            eq.SetBandEnabled(testBand, true);
            eq.SetBandFrequency(testBand, testFrequencies[testBand]);
            eq.SetBandGain(testBand, expectedGains[testBand]);
            eq.SetBandQ(testBand, 2.0f);
            eq.SetBandType(testBand, ProfessionalEQ::FilterType::Peak);
            
            // Test with sine wave at target frequency
            const size_t testSize = 4096;
            AdvancedAudioBuffer buffer(ChannelConfiguration::MONO, testSize, 44100.0f);
            
            float* channelData = buffer.GetChannelData(0);
            for (size_t i = 0; i < testSize; ++i) {
                float t = static_cast<float>(i) / 44100.0f;
                channelData[i] = 0.5f * std::sin(2.0f * M_PI * testFrequencies[testBand] * t);
            }
            
            // Calculate input RMS
            float inputRMS = 0.0f;
            for (size_t i = testSize/4; i < 3*testSize/4; ++i) {
                inputRMS += channelData[i] * channelData[i];
            }
            inputRMS = std::sqrt(inputRMS / (testSize/2));
            
            // Process through EQ
            eq.Process(buffer);
            
            // Calculate output RMS
            float outputRMS = 0.0f;
            for (size_t i = testSize/4; i < 3*testSize/4; ++i) {
                outputRMS += channelData[i] * channelData[i];
            }
            outputRMS = std::sqrt(outputRMS / (testSize/2));
            
            float measuredGain = 20.0f * std::log10(outputRMS / inputRMS);
            
            std::cout << "    " << std::setw(5) << testFrequencies[testBand] << " Hz: "
                     << std::setw(6) << std::fixed << std::setprecision(1) << measuredGain 
                     << " dB (expected: " << expectedGains[testBand] << " dB)" << std::endl;
            
            float error = std::abs(measuredGain - expectedGains[testBand]);
            if (error > 1.5f) {
                allPassed = false;
            }
        }
        
        std::cout << "  Result: " << (allPassed ? "PASSED ✓" : "FAILED ✗") << std::endl;
        return allPassed;
    }
    
    bool TestEQBandProcessing() {
        std::cout << "\n[TEST] Individual Band Processing..." << std::endl;
        
        ProfessionalEQ eq;
        eq.Initialize(44100.0f);
        eq.SetBypassed(false);
        
        for (int i = 0; i < 8; ++i) {
            eq.SetBandEnabled(i, false);
        }
        
        eq.SetBandEnabled(3, true);
        eq.SetBandFrequency(3, 1000.0f);
        eq.SetBandGain(3, 12.0f);
        eq.SetBandQ(3, 1.0f);
        eq.SetBandType(3, ProfessionalEQ::FilterType::Peak);
        
        const size_t testSize = 44100;
        AdvancedAudioBuffer buffer(ChannelConfiguration::MONO, testSize, 44100.0f);
        
        // Generate 1kHz sine wave
        float* channelData = buffer.GetChannelData(0);
        for (size_t i = 0; i < testSize; ++i) {
            float t = static_cast<float>(i) / 44100.0f;
            channelData[i] = std::sin(2.0f * M_PI * 1000.0f * t);
        }
        
        // Calculate input RMS
        float inputRMS = 0.0f;
        for (size_t i = testSize/2; i < testSize; ++i) {
            inputRMS += channelData[i] * channelData[i];
        }
        inputRMS = std::sqrt(inputRMS / (testSize/2));
        
        // Process through EQ
        eq.Process(buffer);
        
        // Calculate output RMS
        float outputRMS = 0.0f;
        for (size_t i = testSize/2; i < testSize; ++i) {
            outputRMS += channelData[i] * channelData[i];
        }
        outputRMS = std::sqrt(outputRMS / (testSize/2));
        
        float gainDB = 20.0f * std::log10(outputRMS / inputRMS);
        
        std::cout << "  Single band test (1kHz, +12dB):" << std::endl;
        std::cout << "    Input RMS: " << inputRMS << std::endl;
        std::cout << "    Output RMS: " << outputRMS << std::endl;
        std::cout << "    Measured gain: " << gainDB << " dB" << std::endl;
        
        bool passed = std::abs(gainDB - 12.0f) < 1.0f;
        
        std::cout << "  Result: " << (passed ? "PASSED ✓" : "FAILED ✗") << std::endl;
        return passed;
    }
    
    bool TestFullEQChain() {
        std::cout << "\n[TEST] Full EQ Chain Processing..." << std::endl;
        
        ProfessionalEQ eq;
        eq.Initialize(48000.0f);
        eq.SetBypassed(false);
        
        eq.SetBandEnabled(0, true);
        eq.SetBandType(0, ProfessionalEQ::FilterType::HighPass);
        eq.SetBandFrequency(0, 80.0f);
        eq.SetBandQ(0, 0.707f);
        
        eq.SetBandEnabled(1, true);
        eq.SetBandType(1, ProfessionalEQ::FilterType::LowShelf);
        eq.SetBandFrequency(1, 200.0f);
        eq.SetBandGain(1, -3.0f);
        
        eq.SetBandEnabled(7, true);
        eq.SetBandType(7, ProfessionalEQ::FilterType::HighShelf);
        eq.SetBandFrequency(7, 10000.0f);
        eq.SetBandGain(7, 4.0f);
        
        const size_t blockSize = 1024;
        std::vector<float> whiteNoise(blockSize);
        std::vector<float> filtered(blockSize);
        
        for (size_t i = 0; i < blockSize; ++i) {
            whiteNoise[i] = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        }
        
        for (size_t i = 0; i < blockSize; ++i) {
            filtered[i] = eq.ProcessSample(whiteNoise[i], 0);
        }
        
        float lowFreqEnergy = 0.0f, highFreqEnergy = 0.0f;
        
        DSP::BiquadFilter analysisLow, analysisHigh;
        analysisLow.CalculateCoefficients(DSP::BiquadFilter::LowPass, 48000.0f, 500.0f, 0.707f, 0.0f);
        analysisHigh.CalculateCoefficients(DSP::BiquadFilter::HighPass, 48000.0f, 5000.0f, 0.707f, 0.0f);
        
        for (size_t i = 0; i < blockSize; ++i) {
            float low = analysisLow.ProcessSample(filtered[i]);
            float high = analysisHigh.ProcessSample(filtered[i]);
            lowFreqEnergy += low * low;
            highFreqEnergy += high * high;
        }
        
        float ratio = highFreqEnergy / lowFreqEnergy;
        
        std::cout << "  Multi-band EQ chain test:" << std::endl;
        std::cout << "    Low frequency energy: " << lowFreqEnergy << std::endl;
        std::cout << "    High frequency energy: " << highFreqEnergy << std::endl;
        std::cout << "    HF/LF ratio: " << ratio << std::endl;
        
        bool passed = ratio > 1.5f;
        
        std::cout << "  Result: " << (passed ? "PASSED ✓" : "FAILED ✗") << std::endl;
        return passed;
    }
    
    bool TestParameterSmoothing() {
        std::cout << "\n[TEST] Parameter Change Response..." << std::endl;
        
        ProfessionalEQ eq;
        eq.Initialize(44100.0f);
        eq.SetBypassed(false);
        
        eq.SetBandEnabled(0, true);
        eq.SetBandType(0, ProfessionalEQ::FilterType::Peak);
        eq.SetBandFrequency(0, 1000.0f);
        eq.SetBandQ(0, 1.0f);
        
        const size_t blockSize = 1024;
        
        // Test with 1kHz sine wave
        AdvancedAudioBuffer buffer1(ChannelConfiguration::MONO, blockSize, 44100.0f);
        AdvancedAudioBuffer buffer2(ChannelConfiguration::MONO, blockSize, 44100.0f);
        
        float* data1 = buffer1.GetChannelData(0);
        float* data2 = buffer2.GetChannelData(0);
        
        for (size_t i = 0; i < blockSize; ++i) {
            float t = static_cast<float>(i) / 44100.0f;
            float sample = 0.5f * std::sin(2.0f * M_PI * 1000.0f * t);
            data1[i] = sample;
            data2[i] = sample;
        }
        
        // Test with -12dB gain
        eq.SetBandGain(0, -12.0f);
        eq.Process(buffer1);
        
        // Test with +12dB gain  
        eq.SetBandGain(0, 12.0f);
        eq.Process(buffer2);
        
        // Calculate RMS for both
        float rms1 = 0.0f, rms2 = 0.0f;
        for (size_t i = blockSize/4; i < 3*blockSize/4; ++i) {
            rms1 += data1[i] * data1[i];
            rms2 += data2[i] * data2[i];
        }
        rms1 = std::sqrt(rms1 / (blockSize/2));
        rms2 = std::sqrt(rms2 / (blockSize/2));
        
        float gain1 = 20.0f * std::log10(rms1 / 0.5f);
        float gain2 = 20.0f * std::log10(rms2 / 0.5f);
        float gainDifference = gain2 - gain1;
        
        std::cout << "  Parameter change test (-12dB to +12dB):" << std::endl;
        std::cout << "    Gain at -12dB setting: " << std::fixed << std::setprecision(1) << gain1 << " dB" << std::endl;
        std::cout << "    Gain at +12dB setting: " << std::fixed << std::setprecision(1) << gain2 << " dB" << std::endl;
        std::cout << "    Measured difference: " << std::fixed << std::setprecision(1) << gainDifference << " dB (expected ~24dB)" << std::endl;
        
        bool passed = std::abs(gainDifference - 24.0f) < 3.0f;
        
        std::cout << "  Result: " << (passed ? "PASSED ✓" : "FAILED ✗") << std::endl;
        return passed;
    }
    
    bool TestBypassFunctionality() {
        std::cout << "\n[TEST] Bypass Functionality..." << std::endl;
        
        ProfessionalEQ eq;
        eq.Initialize(44100.0f);
        
        // Configure a single strong peak filter at 1kHz with +18dB
        for (int i = 0; i < 8; ++i) {
            eq.SetBandEnabled(i, false);
        }
        eq.SetBandEnabled(3, true);
        eq.SetBandType(3, ProfessionalEQ::FilterType::Peak);
        eq.SetBandFrequency(3, 1000.0f);
        eq.SetBandGain(3, 18.0f);
        eq.SetBandQ(3, 2.0f);
        
        const size_t blockSize = 1024;
        
        // Create 1kHz sine wave - matches the peak frequency
        AdvancedAudioBuffer bufferActive(ChannelConfiguration::MONO, blockSize, 44100.0f);
        AdvancedAudioBuffer bufferBypassed(ChannelConfiguration::MONO, blockSize, 44100.0f);
        
        float* dataActive = bufferActive.GetChannelData(0);
        float* dataBypassed = bufferBypassed.GetChannelData(0);
        
        for (size_t i = 0; i < blockSize; ++i) {
            float t = static_cast<float>(i) / 44100.0f;
            float sample = 0.3f * std::sin(2.0f * M_PI * 1000.0f * t);
            dataActive[i] = sample;
            dataBypassed[i] = sample;
        }
        
        // Test with EQ active
        eq.SetBypassed(false);
        eq.Process(bufferActive);
        
        // Test with EQ bypassed
        eq.SetBypassed(true);
        eq.Process(bufferBypassed);
        
        // Calculate RMS values
        float activeRMS = 0.0f, bypassRMS = 0.0f;
        const float expectedInputRMS = 0.3f / std::sqrt(2.0f); // RMS of sine wave with amplitude 0.3
        
        for (size_t i = blockSize/4; i < 3*blockSize/4; ++i) {
            activeRMS += dataActive[i] * dataActive[i];
            bypassRMS += dataBypassed[i] * dataBypassed[i];
        }
        
        activeRMS = std::sqrt(activeRMS / (blockSize/2));
        bypassRMS = std::sqrt(bypassRMS / (blockSize/2));
        
        float activeGain = 20.0f * std::log10(activeRMS / expectedInputRMS);
        float bypassGain = 20.0f * std::log10(bypassRMS / expectedInputRMS);
        
        std::cout << "  Bypass test results (1kHz sine, +18dB peak filter):" << std::endl;
        std::cout << "    Expected input RMS: " << std::fixed << std::setprecision(3) << expectedInputRMS << std::endl;
        std::cout << "    Active EQ RMS: " << std::fixed << std::setprecision(3) << activeRMS << std::endl;
        std::cout << "    Bypassed RMS: " << std::fixed << std::setprecision(3) << bypassRMS << std::endl;
        std::cout << "    Active gain: " << std::fixed << std::setprecision(1) << activeGain << " dB" << std::endl;
        std::cout << "    Bypass gain: " << std::fixed << std::setprecision(1) << bypassGain << " dB" << std::endl;
        
        bool bypassCorrect = std::abs(bypassGain) < 0.5f; // Bypass should be ~0dB
        bool activeCorrect = activeGain > 15.0f; // Active should be close to +18dB
        bool passed = bypassCorrect && activeCorrect;
        
        std::cout << "  Result: " << (passed ? "PASSED ✓" : "FAILED ✗") << std::endl;
        return passed;
    }
    
    void PerformFFT(std::vector<std::complex<float>>& data) {
        const size_t N = data.size();
        if (N <= 1) return;
        
        std::vector<std::complex<float>> even(N/2), odd(N/2);
        for (size_t i = 0; i < N/2; ++i) {
            even[i] = data[2*i];
            odd[i] = data[2*i + 1];
        }
        
        PerformFFT(even);
        PerformFFT(odd);
        
        for (size_t k = 0; k < N/2; ++k) {
            float angle = -2.0f * M_PI * k / N;
            std::complex<float> w(std::cos(angle), std::sin(angle));
            std::complex<float> t = w * odd[k];
            data[k] = even[k] + t;
            data[k + N/2] = even[k] - t;
        }
    }
};

int main() {
    std::cout << "\n╔════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  VeniceDAW Professional EQ DSP Test Suite  ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════╝" << std::endl;
    
    ProfessionalEQTest tester;
    bool success = tester.RunAllTests();
    
    return success ? 0 : 1;
}
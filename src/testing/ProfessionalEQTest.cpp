#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <complex>
#include "../audio/AdvancedAudioProcessor.h"
#include "../audio/DSPAlgorithms.h"

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
        
        ProfessionalEQ eq;
        eq.Initialize(44100.0f);
        
        const float testFrequencies[] = {100.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f};
        const float expectedGains[] = {0.0f, 3.0f, -3.0f, 6.0f, -6.0f, 3.0f, -3.0f, 0.0f};
        
        for (int band = 0; band < 8; ++band) {
            eq.SetBandFrequency(band, testFrequencies[band]);
            eq.SetBandGain(band, expectedGains[band]);
            eq.SetBandQ(band, 2.0f);
            eq.SetBandEnabled(band, true);
        }
        
        const size_t fftSize = 2048;
        std::vector<float> impulse(fftSize, 0.0f);
        std::vector<float> response(fftSize, 0.0f);
        impulse[0] = 1.0f;
        
        for (size_t i = 0; i < fftSize; ++i) {
            response[i] = eq.ProcessSample(impulse[i], 0);
        }
        
        std::vector<std::complex<float>> spectrum(fftSize);
        for (size_t i = 0; i < fftSize; ++i) {
            spectrum[i] = std::complex<float>(response[i], 0.0f);
        }
        
        PerformFFT(spectrum);
        
        std::cout << "  Frequency Response at key points:" << std::endl;
        bool passed = true;
        
        for (int i = 0; i < 8; ++i) {
            int bin = static_cast<int>(testFrequencies[i] * fftSize / 44100.0f);
            float magnitude = std::abs(spectrum[bin]);
            float magnitudeDB = 20.0f * std::log10(std::max(1e-10f, magnitude));
            
            std::cout << "    " << std::setw(5) << testFrequencies[i] << " Hz: "
                     << std::setw(6) << std::fixed << std::setprecision(1) << magnitudeDB 
                     << " dB (expected: " << expectedGains[i] << " dB)" << std::endl;
            
            float error = std::abs(magnitudeDB - expectedGains[i]);
            if (error > 3.0f) {
                passed = false;
            }
        }
        
        std::cout << "  Result: " << (passed ? "PASSED ✓" : "FAILED ✗") << std::endl;
        return passed;
    }
    
    bool TestEQBandProcessing() {
        std::cout << "\n[TEST] Individual Band Processing..." << std::endl;
        
        ProfessionalEQ eq;
        eq.Initialize(44100.0f);
        
        for (int i = 0; i < 8; ++i) {
            eq.SetBandEnabled(i, false);
        }
        
        eq.SetBandEnabled(3, true);
        eq.SetBandFrequency(3, 1000.0f);
        eq.SetBandGain(3, 12.0f);
        eq.SetBandQ(3, 1.0f);
        eq.SetBandType(3, ProfessionalEQ::FilterType::Peak);
        
        const size_t testSize = 44100;
        std::vector<float> sine(testSize);
        std::vector<float> processed(testSize);
        
        for (size_t i = 0; i < testSize; ++i) {
            float t = static_cast<float>(i) / 44100.0f;
            sine[i] = std::sin(2.0f * M_PI * 1000.0f * t);
        }
        
        for (size_t i = 0; i < testSize; ++i) {
            processed[i] = eq.ProcessSample(sine[i], 0);
        }
        
        float inputRMS = 0.0f, outputRMS = 0.0f;
        for (size_t i = testSize/2; i < testSize; ++i) {
            inputRMS += sine[i] * sine[i];
            outputRMS += processed[i] * processed[i];
        }
        
        inputRMS = std::sqrt(inputRMS / (testSize/2));
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
        std::cout << "\n[TEST] Parameter Smoothing..." << std::endl;
        
        ProfessionalEQ eq;
        eq.Initialize(44100.0f);
        
        eq.SetBandEnabled(0, true);
        eq.SetBandType(0, ProfessionalEQ::FilterType::Peak);
        eq.SetBandFrequency(0, 1000.0f);
        eq.SetBandQ(0, 1.0f);
        
        const size_t blockSize = 512;
        std::vector<float> testSignal(blockSize, 0.5f);
        std::vector<float> output1(blockSize);
        std::vector<float> output2(blockSize);
        
        eq.SetBandGain(0, -12.0f);
        for (size_t i = 0; i < blockSize; ++i) {
            output1[i] = eq.ProcessSample(testSignal[i], 0);
        }
        
        eq.SetBandGain(0, 12.0f);
        for (size_t i = 0; i < blockSize; ++i) {
            output2[i] = eq.ProcessSample(testSignal[i], 0);
        }
        
        bool hasTransition = false;
        float maxJump = 0.0f;
        
        for (size_t i = 1; i < blockSize; ++i) {
            float diff = std::abs(output2[i] - output2[i-1]);
            maxJump = std::max(maxJump, diff);
            if (diff > 0.001f && diff < 0.1f) {
                hasTransition = true;
            }
        }
        
        std::cout << "  Parameter change test (gain sweep -12dB to +12dB):" << std::endl;
        std::cout << "    Maximum sample jump: " << maxJump << std::endl;
        std::cout << "    Smooth transition detected: " << (hasTransition ? "Yes" : "No") << std::endl;
        
        bool passed = hasTransition && (maxJump < 0.2f);
        
        std::cout << "  Result: " << (passed ? "PASSED ✓" : "FAILED ✗") << std::endl;
        return passed;
    }
    
    bool TestBypassFunctionality() {
        std::cout << "\n[TEST] Bypass Functionality..." << std::endl;
        
        ProfessionalEQ eq;
        eq.Initialize(44100.0f);
        
        for (int i = 0; i < 8; ++i) {
            eq.SetBandEnabled(i, true);
            eq.SetBandGain(i, 12.0f);
        }
        
        const size_t blockSize = 256;
        std::vector<float> input(blockSize);
        std::vector<float> processedActive(blockSize);
        std::vector<float> processedBypassed(blockSize);
        
        for (size_t i = 0; i < blockSize; ++i) {
            input[i] = std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
        }
        
        eq.SetBypassed(false);
        for (size_t i = 0; i < blockSize; ++i) {
            processedActive[i] = eq.ProcessSample(input[i], 0);
        }
        
        eq.SetBypassed(true);
        for (size_t i = 0; i < blockSize; ++i) {
            processedBypassed[i] = eq.ProcessSample(input[i], 0);
        }
        
        float activeEnergy = 0.0f, bypassEnergy = 0.0f, inputEnergy = 0.0f;
        for (size_t i = 0; i < blockSize; ++i) {
            activeEnergy += processedActive[i] * processedActive[i];
            bypassEnergy += processedBypassed[i] * processedBypassed[i];
            inputEnergy += input[i] * input[i];
        }
        
        float bypassError = std::abs(bypassEnergy - inputEnergy) / inputEnergy;
        
        std::cout << "  Bypass test results:" << std::endl;
        std::cout << "    Input energy: " << inputEnergy << std::endl;
        std::cout << "    Active EQ energy: " << activeEnergy << std::endl;
        std::cout << "    Bypassed energy: " << bypassEnergy << std::endl;
        std::cout << "    Bypass error: " << (bypassError * 100.0f) << "%" << std::endl;
        
        bool passed = (bypassError < 0.01f) && (activeEnergy > inputEnergy * 2.0f);
        
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
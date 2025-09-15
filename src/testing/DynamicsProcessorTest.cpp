#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include "../audio/AdvancedAudioProcessor.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace VeniceDAW;

class DynamicsProcessorTest {
public:
    bool RunAllTests() {
        std::cout << "\n╔════════════════════════════════════════════╗" << std::endl;
        std::cout << "║  VeniceDAW Dynamics Processor Test Suite   ║" << std::endl;
        std::cout << "╚════════════════════════════════════════════╝" << std::endl;
        
        bool allPassed = true;
        
        allPassed &= TestCompressorMode();
        allPassed &= TestLimiterMode();
        allPassed &= TestGateMode();
        allPassed &= TestExpanderMode();
        allPassed &= TestSoftKnee();
        allPassed &= TestDetectionModes();
        allPassed &= TestBypassFunctionality();
        allPassed &= TestMakeupGain();
        allPassed &= TestLookaheadLimiting();
        
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << (allPassed ? "✓ All tests PASSED" : "✗ Some tests FAILED") << std::endl;
        
        return allPassed;
    }

private:
    bool TestCompressorMode() {
        std::cout << "\n[TEST] Compressor Mode..." << std::endl;
        
        DynamicsProcessor comp;
        comp.Initialize(44100.0f);
        comp.SetBypassed(false);
        comp.SetMode(DynamicsProcessor::Mode::COMPRESSOR);
        
        // Configure compressor: -12dB threshold, 4:1 ratio
        comp.SetParameter("threshold", -12.0f);
        comp.SetParameter("ratio", 4.0f);
        comp.SetParameter("attack", 1.0f);   // Fast but reasonable attack
        comp.SetParameter("release", 50.0f); // Moderate release
        comp.SetParameter("knee", 0.0f); // Hard knee for predictable results
        
        const size_t testSize = 4096;
        
        // Test with signal above threshold (-6dB)
        AdvancedAudioBuffer buffer(ChannelConfiguration::MONO, testSize, 44100.0f);
        float* data = buffer.GetChannelData(0);
        
        float inputAmplitude = std::pow(10.0f, -6.0f / 20.0f); // -6dB in linear
        
        // Process twice to stabilize envelope and gain reduction meter
        for (int pass = 0; pass < 2; ++pass) {
            for (size_t i = 0; i < testSize; ++i) {
                float t = static_cast<float>(i) / 44100.0f;
                data[i] = inputAmplitude * std::sin(2.0f * M_PI * 1000.0f * t);
            }
            comp.Process(buffer);
        }
        
        // Calculate output RMS
        float outputRMS = 0.0f;
        for (size_t i = testSize/2; i < testSize; ++i) { // Skip transient
            outputRMS += data[i] * data[i];
        }
        outputRMS = std::sqrt(outputRMS / (testSize/2));
        
        // Calculate absolute output level
        float outputdB = 20.0f * std::log10(outputRMS);
        float expectedGainReduction = (-6.0f - (-12.0f)) - (-6.0f - (-12.0f)) / 4.0f; // 6dB over threshold, compressed 4:1
        float expectedOutputdB = -6.0f - expectedGainReduction;
        
        std::cout << "  Compressor test (-6dB input, -12dB threshold, 4:1 ratio):" << std::endl;
        std::cout << "    Expected gain reduction: " << std::fixed << std::setprecision(1) << expectedGainReduction << " dB" << std::endl;
        std::cout << "    Expected output level: " << expectedOutputdB << " dB" << std::endl;
        std::cout << "    Measured output level: " << outputdB << " dB" << std::endl;
        std::cout << "    Measured gain reduction: " << comp.GetGainReduction() << " dB" << std::endl;
        
        bool passed = std::abs(outputdB - expectedOutputdB) < 4.0f && comp.GetGainReduction() > 2.5f;
        
        std::cout << "  Result: " << (passed ? "PASSED ✓" : "FAILED ✗") << std::endl;
        return passed;
    }
    
    bool TestLimiterMode() {
        std::cout << "\n[TEST] Limiter Mode..." << std::endl;
        
        DynamicsProcessor limiter;
        limiter.Initialize(44100.0f);
        limiter.SetBypassed(false);
        limiter.SetMode(DynamicsProcessor::Mode::LIMITER);
        
        // Configure limiter: -3dB threshold
        limiter.SetParameter("threshold", -3.0f);
        limiter.SetParameter("attack", 0.01f);   // Ultra-fast attack for limiting
        limiter.SetParameter("release", 10.0f);  // Fast release for testing
        
        const size_t testSize = 4096;
        AdvancedAudioBuffer buffer(ChannelConfiguration::MONO, testSize, 44100.0f);
        float* data = buffer.GetChannelData(0);
        
        // Test with signal well above threshold (0dB)
        float inputAmplitude = 1.0f; // 0dB
        for (size_t i = 0; i < testSize; ++i) {
            float t = static_cast<float>(i) / 44100.0f;
            data[i] = inputAmplitude * std::sin(2.0f * M_PI * 1000.0f * t);
        }
        
        limiter.Process(buffer);
        
        // Check peak level after processing
        float peakLevel = 0.0f;
        for (size_t i = testSize/2; i < testSize; ++i) {
            peakLevel = std::max(peakLevel, std::abs(data[i]));
        }
        
        float peakdB = 20.0f * std::log10(peakLevel);
        
        std::cout << "  Limiter test (0dB input, -3dB threshold):" << std::endl;
        std::cout << "    Input peak level: 0.0 dB" << std::endl;
        std::cout << "    Output peak level: " << std::fixed << std::setprecision(1) << peakdB << " dB" << std::endl;
        std::cout << "    Gain reduction: " << limiter.GetGainReduction() << " dB" << std::endl;
        
        bool passed = peakdB <= -2.0f && limiter.GetGainReduction() > 1.0f; // Should limit to near threshold
        
        std::cout << "  Result: " << (passed ? "PASSED ✓" : "FAILED ✗") << std::endl;
        return passed;
    }
    
    bool TestGateMode() {
        std::cout << "\n[TEST] Gate Mode..." << std::endl;
        
        DynamicsProcessor gate;
        gate.Initialize(44100.0f);
        gate.SetBypassed(false);
        gate.SetMode(DynamicsProcessor::Mode::GATE);
        
        // Configure gate: -30dB threshold, 4:1 expansion ratio
        gate.SetParameter("threshold", -30.0f);
        gate.SetParameter("ratio", 4.0f);
        gate.SetParameter("attack", 0.1f);  // Fast attack for testing
        gate.SetParameter("release", 10.0f); // Fast release for testing
        
        const size_t testSize = 4096;
        AdvancedAudioBuffer buffer(ChannelConfiguration::MONO, testSize, 44100.0f);
        float* data = buffer.GetChannelData(0);
        
        // Test with quiet signal below threshold (-40dB)
        float inputAmplitude = std::pow(10.0f, -40.0f / 20.0f);
        for (size_t i = 0; i < testSize; ++i) {
            float t = static_cast<float>(i) / 44100.0f;
            data[i] = inputAmplitude * std::sin(2.0f * M_PI * 1000.0f * t);
        }
        
        gate.Process(buffer);
        
        // Calculate output RMS
        float outputRMS = 0.0f;
        for (size_t i = testSize/2; i < testSize; ++i) {
            outputRMS += data[i] * data[i];
        }
        outputRMS = std::sqrt(outputRMS / (testSize/2));
        
        // Calculate absolute output level
        float outputdB = 20.0f * std::log10(outputRMS);
        
        std::cout << "  Gate test (-40dB input, -30dB threshold, 4:1 expansion):" << std::endl;
        std::cout << "    Input level: -40.0 dB" << std::endl;
        std::cout << "    Output level: " << std::fixed << std::setprecision(1) << outputdB << " dB" << std::endl;
        std::cout << "    Expected: more attenuation (expansion below threshold)" << std::endl;
        
        bool passed = outputdB < -42.0f; // Should expand/attenuate signal below threshold
        
        std::cout << "  Result: " << (passed ? "PASSED ✓" : "FAILED ✗") << std::endl;
        return passed;
    }
    
    bool TestExpanderMode() {
        std::cout << "\n[TEST] Expander Mode..." << std::endl;
        
        DynamicsProcessor expander;
        expander.Initialize(44100.0f);
        expander.SetBypassed(false);
        expander.SetMode(DynamicsProcessor::Mode::EXPANDER);
        
        // Configure expander: -20dB threshold, 2:1 expansion
        expander.SetParameter("threshold", -20.0f);
        expander.SetParameter("ratio", 2.0f);
        expander.SetParameter("attack", 0.1f);  // Fast attack for testing
        expander.SetParameter("release", 10.0f); // Fast release for testing
        
        const size_t testSize = 4096;
        AdvancedAudioBuffer buffer(ChannelConfiguration::MONO, testSize, 44100.0f);
        float* data = buffer.GetChannelData(0);
        
        // Test with signal below threshold (-30dB)
        float inputAmplitude = std::pow(10.0f, -30.0f / 20.0f);
        for (size_t i = 0; i < testSize; ++i) {
            float t = static_cast<float>(i) / 44100.0f;
            data[i] = inputAmplitude * std::sin(2.0f * M_PI * 1000.0f * t);
        }
        
        expander.Process(buffer);
        
        // Calculate output RMS
        float outputRMS = 0.0f;
        for (size_t i = testSize/2; i < testSize; ++i) {
            outputRMS += data[i] * data[i];
        }
        outputRMS = std::sqrt(outputRMS / (testSize/2));
        
        // Calculate absolute output level in dB
        float outputdB = 20.0f * std::log10(outputRMS);
        
        std::cout << "  Expander test (-30dB input, -20dB threshold, 2:1 expansion):" << std::endl;
        std::cout << "    Input level: -30.0 dB" << std::endl;
        std::cout << "    Output level: " << std::fixed << std::setprecision(1) << outputdB << " dB" << std::endl;
        std::cout << "    Expected: gentle expansion (less aggressive than gate)" << std::endl;
        
        bool passed = outputdB < -30.5f && outputdB > -40.0f; // Gentle expansion
        
        std::cout << "  Result: " << (passed ? "PASSED ✓" : "FAILED ✗") << std::endl;
        return passed;
    }
    
    bool TestSoftKnee() {
        std::cout << "\n[TEST] Soft Knee vs Hard Knee..." << std::endl;
        
        DynamicsProcessor compHard, compSoft;
        compHard.Initialize(44100.0f);
        compSoft.Initialize(44100.0f);
        
        // Configure identical compressors except knee
        float threshold = -12.0f;
        float ratio = 4.0f;
        
        compHard.SetBypassed(false);
        compHard.SetMode(DynamicsProcessor::Mode::COMPRESSOR);
        compHard.SetParameter("threshold", threshold);
        compHard.SetParameter("ratio", ratio);
        compHard.SetParameter("knee", 0.0f); // Hard knee
        
        compSoft.SetBypassed(false);
        compSoft.SetMode(DynamicsProcessor::Mode::COMPRESSOR);
        compSoft.SetParameter("threshold", threshold);
        compSoft.SetParameter("ratio", ratio);
        compSoft.SetParameter("knee", 6.0f); // Soft knee
        
        const size_t testSize = 4096;
        AdvancedAudioBuffer bufferHard(ChannelConfiguration::MONO, testSize, 44100.0f);
        AdvancedAudioBuffer bufferSoft(ChannelConfiguration::MONO, testSize, 44100.0f);
        
        // Test with signal around threshold (-10dB, just above threshold)
        float inputAmplitude = std::pow(10.0f, -10.0f / 20.0f);
        
        for (size_t i = 0; i < testSize; ++i) {
            float t = static_cast<float>(i) / 44100.0f;
            float sample = inputAmplitude * std::sin(2.0f * M_PI * 1000.0f * t);
            bufferHard.GetChannelData(0)[i] = sample;
            bufferSoft.GetChannelData(0)[i] = sample;
        }
        
        compHard.Process(bufferHard);
        compSoft.Process(bufferSoft);
        
        float grHard = compHard.GetGainReduction();
        float grSoft = compSoft.GetGainReduction();
        
        std::cout << "  Knee comparison (-10dB input, -12dB threshold):" << std::endl;
        std::cout << "    Hard knee gain reduction: " << std::fixed << std::setprecision(1) << grHard << " dB" << std::endl;
        std::cout << "    Soft knee gain reduction: " << std::fixed << std::setprecision(1) << grSoft << " dB" << std::endl;
        std::cout << "    Expected: soft knee should have less gain reduction near threshold" << std::endl;
        
        bool passed = grSoft < grHard && grSoft > 0.0f && grHard > 0.0f;
        
        std::cout << "  Result: " << (passed ? "PASSED ✓" : "FAILED ✗") << std::endl;
        return passed;
    }
    
    bool TestDetectionModes() {
        std::cout << "\n[TEST] Detection Modes (Peak vs RMS)..." << std::endl;
        
        DynamicsProcessor compPeak, compRMS;
        compPeak.Initialize(44100.0f);
        compRMS.Initialize(44100.0f);
        
        compPeak.SetBypassed(false);
        compPeak.SetMode(DynamicsProcessor::Mode::COMPRESSOR);
        compPeak.SetDetectionMode(DynamicsProcessor::DetectionMode::PEAK);
        compPeak.SetParameter("threshold", -12.0f);
        compPeak.SetParameter("ratio", 4.0f);
        
        compRMS.SetBypassed(false);
        compRMS.SetMode(DynamicsProcessor::Mode::COMPRESSOR);
        compRMS.SetDetectionMode(DynamicsProcessor::DetectionMode::RMS);
        compRMS.SetParameter("threshold", -12.0f);
        compRMS.SetParameter("ratio", 4.0f);
        
        const size_t testSize = 4096;
        AdvancedAudioBuffer bufferPeak(ChannelConfiguration::MONO, testSize, 44100.0f);
        AdvancedAudioBuffer bufferRMS(ChannelConfiguration::MONO, testSize, 44100.0f);
        
        // Test with signal above threshold
        float inputAmplitude = std::pow(10.0f, -6.0f / 20.0f);
        
        for (size_t i = 0; i < testSize; ++i) {
            float t = static_cast<float>(i) / 44100.0f;
            float sample = inputAmplitude * std::sin(2.0f * M_PI * 1000.0f * t);
            bufferPeak.GetChannelData(0)[i] = sample;
            bufferRMS.GetChannelData(0)[i] = sample;
        }
        
        compPeak.Process(bufferPeak);
        compRMS.Process(bufferRMS);
        
        float grPeak = compPeak.GetGainReduction();
        float grRMS = compRMS.GetGainReduction();
        
        std::cout << "  Detection mode comparison:" << std::endl;
        std::cout << "    Peak detection gain reduction: " << std::fixed << std::setprecision(1) << grPeak << " dB" << std::endl;
        std::cout << "    RMS detection gain reduction: " << std::fixed << std::setprecision(1) << grRMS << " dB" << std::endl;
        std::cout << "    Expected: both should show gain reduction, potentially different amounts" << std::endl;
        
        bool passed = grPeak > 1.0f && grRMS > 1.0f; // Both should compress
        
        std::cout << "  Result: " << (passed ? "PASSED ✓" : "FAILED ✗") << std::endl;
        return passed;
    }
    
    bool TestBypassFunctionality() {
        std::cout << "\n[TEST] Bypass Functionality..." << std::endl;
        
        DynamicsProcessor comp;
        comp.Initialize(44100.0f);
        comp.SetMode(DynamicsProcessor::Mode::COMPRESSOR);
        comp.SetParameter("threshold", -12.0f);
        comp.SetParameter("ratio", 8.0f); // High ratio for obvious effect
        
        const size_t testSize = 1024;
        AdvancedAudioBuffer bufferActive(ChannelConfiguration::MONO, testSize, 44100.0f);
        AdvancedAudioBuffer bufferBypassed(ChannelConfiguration::MONO, testSize, 44100.0f);
        
        // Test signal above threshold
        float inputAmplitude = std::pow(10.0f, -6.0f / 20.0f);
        
        for (size_t i = 0; i < testSize; ++i) {
            float t = static_cast<float>(i) / 44100.0f;
            float sample = inputAmplitude * std::sin(2.0f * M_PI * 1000.0f * t);
            bufferActive.GetChannelData(0)[i] = sample;
            bufferBypassed.GetChannelData(0)[i] = sample;
        }
        
        // Test with compressor active
        comp.SetBypassed(false);
        comp.Process(bufferActive);
        
        // Test with compressor bypassed
        comp.SetBypassed(true);
        comp.Process(bufferBypassed);
        
        // Calculate RMS levels
        float activeRMS = 0.0f, bypassRMS = 0.0f;
        for (size_t i = testSize/2; i < testSize; ++i) {
            activeRMS += bufferActive.GetChannelData(0)[i] * bufferActive.GetChannelData(0)[i];
            bypassRMS += bufferBypassed.GetChannelData(0)[i] * bufferBypassed.GetChannelData(0)[i];
        }
        activeRMS = std::sqrt(activeRMS / (testSize/2));
        bypassRMS = std::sqrt(bypassRMS / (testSize/2));
        
        float activedB = 20.0f * std::log10(activeRMS / (inputAmplitude / std::sqrt(2.0f)));
        float bypassdB = 20.0f * std::log10(bypassRMS / (inputAmplitude / std::sqrt(2.0f)));
        
        std::cout << "  Bypass test results:" << std::endl;
        std::cout << "    Active compressor level: " << std::fixed << std::setprecision(1) << activedB << " dB" << std::endl;
        std::cout << "    Bypassed level: " << bypassdB << " dB" << std::endl;
        std::cout << "    Expected: bypassed should be ~0dB, active should be reduced" << std::endl;
        
        bool passed = std::abs(bypassdB) < 0.5f && activedB < -1.0f;
        
        std::cout << "  Result: " << (passed ? "PASSED ✓" : "FAILED ✗") << std::endl;
        return passed;
    }
    
    bool TestMakeupGain() {
        std::cout << "\n[TEST] Makeup Gain..." << std::endl;
        
        DynamicsProcessor comp;
        comp.Initialize(44100.0f);
        comp.SetBypassed(false);
        comp.SetMode(DynamicsProcessor::Mode::COMPRESSOR);
        comp.SetParameter("threshold", -12.0f);
        comp.SetParameter("ratio", 4.0f);
        comp.SetParameter("makeup", 6.0f); // +6dB makeup gain
        
        const size_t testSize = 4096;
        AdvancedAudioBuffer buffer(ChannelConfiguration::MONO, testSize, 44100.0f);
        float* data = buffer.GetChannelData(0);
        
        // Test with signal above threshold
        float inputAmplitude = std::pow(10.0f, -6.0f / 20.0f);
        for (size_t i = 0; i < testSize; ++i) {
            float t = static_cast<float>(i) / 44100.0f;
            data[i] = inputAmplitude * std::sin(2.0f * M_PI * 1000.0f * t);
        }
        
        comp.Process(buffer);
        
        // Calculate output RMS
        float outputRMS = 0.0f;
        for (size_t i = testSize/2; i < testSize; ++i) {
            outputRMS += data[i] * data[i];
        }
        outputRMS = std::sqrt(outputRMS / (testSize/2));
        
        // Calculate input RMS for comparison
        float inputRMS = inputAmplitude / std::sqrt(2.0f); // RMS of sine wave
        float outputdB = 20.0f * std::log10(outputRMS / inputRMS);
        
        std::cout << "  Makeup gain test (+6dB makeup):" << std::endl;
        std::cout << "    Input level: -6.0 dB" << std::endl;
        std::cout << "    Output level with makeup: " << std::fixed << std::setprecision(1) << outputdB << " dB" << std::endl;
        std::cout << "    Expected: level boost from makeup gain" << std::endl;
        
        bool passed = outputdB > -3.0f; // Should be boosted by makeup gain
        
        std::cout << "  Result: " << (passed ? "PASSED ✓" : "FAILED ✗") << std::endl;
        return passed;
    }
    
    bool TestLookaheadLimiting() {
        std::cout << "\n[TEST] Lookahead Limiting..." << std::endl;
        
        DynamicsProcessor limiter;
        limiter.Initialize(44100.0f);
        limiter.SetBypassed(false);
        limiter.SetMode(DynamicsProcessor::Mode::LIMITER);
        
        // Configure lookahead limiter
        limiter.SetParameter("threshold", -6.0f);
        limiter.SetParameter("lookahead_enabled", 1.0f);
        limiter.SetParameter("lookahead_time", 5.0f); // 5ms lookahead
        
        const size_t testSize = 4096;
        AdvancedAudioBuffer buffer(ChannelConfiguration::STEREO, testSize, 44100.0f);
        
        // Create test signal with peak that would clip without lookahead
        float peakAmplitude = std::pow(10.0f, -3.0f / 20.0f); // -3dB peak (above -6dB threshold)
        
        for (size_t ch = 0; ch < 2; ++ch) {
            float* data = buffer.GetChannelData(ch);
            for (size_t i = 0; i < testSize; ++i) {
                float t = static_cast<float>(i) / 44100.0f;
                // Create impulse at 1/4 through the buffer
                if (i == testSize / 4) {
                    data[i] = peakAmplitude;
                } else {
                    data[i] = peakAmplitude * 0.1f * std::sin(2.0f * M_PI * 1000.0f * t);
                }
            }
        }
        
        // Process with lookahead limiter
        limiter.Process(buffer);
        
        // Analyze results - find peak level
        float maxLevel = 0.0f;
        for (size_t ch = 0; ch < 2; ++ch) {
            float* data = buffer.GetChannelData(ch);
            for (size_t i = 0; i < testSize; ++i) {
                maxLevel = std::max(maxLevel, std::abs(data[i]));
            }
        }
        
        float maxLeveldB = 20.0f * std::log10(std::max(maxLevel, 1e-6f));
        
        std::cout << "  Lookahead limiter test:" << std::endl;
        std::cout << "    Threshold: -6.0 dB" << std::endl;
        std::cout << "    Lookahead time: 5.0 ms" << std::endl;
        std::cout << "    Peak output level: " << std::fixed << std::setprecision(1) << maxLeveldB << " dB" << std::endl;
        std::cout << "    Expected: limited to threshold without overshoot" << std::endl;
        
        // Test lookahead parameter access
        float lookaheadTime = limiter.GetParameter("lookahead_time");
        bool lookaheadEnabled = limiter.GetParameter("lookahead_enabled") > 0.5f;
        
        std::cout << "    Lookahead enabled: " << (lookaheadEnabled ? "Yes" : "No") << std::endl;
        std::cout << "    Lookahead time: " << lookaheadTime << " ms" << std::endl;
        
        // Test should pass if:
        // 1. Output is limited (below threshold + margin)
        // 2. Lookahead parameters are correctly set
        bool levelLimited = maxLeveldB <= -5.0f; // Allow 1dB margin
        bool parametersCorrect = lookaheadEnabled && std::abs(lookaheadTime - 5.0f) < 0.1f;
        bool passed = levelLimited && parametersCorrect;
        
        std::cout << "  Result: " << (passed ? "PASSED ✓" : "FAILED ✗") << std::endl;
        if (!levelLimited) {
            std::cout << "    Warning: Peak level not properly limited" << std::endl;
        }
        if (!parametersCorrect) {
            std::cout << "    Warning: Lookahead parameters not correctly set" << std::endl;
        }
        
        return passed;
    }
};

int main() {
    DynamicsProcessorTest tester;
    bool success = tester.RunAllTests();
    
    return success ? 0 : 1;
}
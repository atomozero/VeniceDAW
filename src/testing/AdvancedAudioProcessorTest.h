/*
 * AdvancedAudioProcessorTest.h - Comprehensive test suite for Phase 3.1 foundation
 * 
 * Validates all components of the Advanced Audio Processing Engine:
 * - Multi-channel audio buffer management
 * - Professional effects framework
 * - Surround sound processing
 * - Performance characteristics
 */

#ifndef ADVANCED_AUDIO_PROCESSOR_TEST_H
#define ADVANCED_AUDIO_PROCESSOR_TEST_H

#include "../audio/AdvancedAudioProcessor.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>

namespace VeniceDAW {

struct TestResult {
    std::string testName;
    bool passed;
    float score;
    std::string details;
    float executionTimeMs;
};

class AdvancedAudioProcessorTest {
public:
    AdvancedAudioProcessorTest();
    ~AdvancedAudioProcessorTest() = default;
    
    // Main test execution
    std::vector<TestResult> RunAllTests();
    void PrintTestResults(const std::vector<TestResult>& results);
    void SaveTestReport(const std::vector<TestResult>& results, const std::string& filename);
    
    // Individual test categories
    std::vector<TestResult> TestAudioBufferManagement();
    std::vector<TestResult> TestEffectsFramework();
    std::vector<TestResult> TestSurroundProcessing();
    std::vector<TestResult> TestPerformanceCharacteristics();
    std::vector<TestResult> TestIntegrationScenarios();

private:
    // Audio buffer tests
    TestResult TestBufferCreationAndAccess();
    TestResult TestBufferResizing();
    TestResult TestMultiChannelOperations();
    TestResult TestChannelConfigurationHandling();
    
    // Effects framework tests
    TestResult TestEffectChainManagement();
    TestResult TestProfessionalEQFunctionality();
    TestResult TestDynamicsProcessing();
    TestResult TestEffectParameterSystem();
    TestResult TestBypassAndCPUMonitoring();
    
    // Surround sound tests
    TestResult TestSurroundChannelConfiguration();
    TestResult TestStereoToSurroundConversion();
    TestResult TestSurroundDownmixing();
    TestResult TestSpatialPositioning();
    
    // Performance tests
    TestResult TestRealtimeProcessingCapability();
    TestResult TestCPUUsageMonitoring();
    TestResult TestLatencyMeasurement();
    TestResult TestMemoryEfficiency();
    TestResult TestScalabilityLimits();
    
    // Integration tests
    TestResult TestFullProcessingChain();
    TestResult TestPhase2OptimizationCompatibility();
    TestResult TestProfessionalWorkflowScenario();
    
    // Utility methods
    void GenerateTestSignal(AdvancedAudioBuffer& buffer, float frequency, float amplitude = 0.5f);
    void GenerateWhiteNoise(AdvancedAudioBuffer& buffer, float amplitude = 0.1f);
    float CalculateRMS(const AdvancedAudioBuffer& buffer, size_t channel = 0);
    float CalculateTHD(const AdvancedAudioBuffer& input, const AdvancedAudioBuffer& output);
    bool CompareBuffers(const AdvancedAudioBuffer& a, const AdvancedAudioBuffer& b, float tolerance = 0.001f);
    
    // Performance measurement
    std::chrono::high_resolution_clock::time_point StartTimer();
    float StopTimer(std::chrono::high_resolution_clock::time_point start);
    
    // Test signal generation
    void CreateSweepSignal(AdvancedAudioBuffer& buffer, float startFreq, float endFreq);
    void CreateImpulse(AdvancedAudioBuffer& buffer);
    void CreateStereoTestSignal(AdvancedAudioBuffer& buffer);
    void CreateSurroundTestSignal(AdvancedAudioBuffer& buffer);
    
    // Validation helpers
    bool ValidateFrequencyResponse(const AdvancedAudioBuffer& input, const AdvancedAudioBuffer& output, float targetFreq);
    bool ValidateChannelSeparation(const AdvancedAudioBuffer& buffer);
    bool ValidateRealtimeConstraints(float processingTimeMs, float bufferDurationMs);
    
    float fSampleRate{44100.0f};
    size_t fBufferSize{1024};
    float fTestTolerance{0.001f};
};

// Professional audio test scenarios
class ProfessionalAudioTestScenarios {
public:
    static TestResult TestStudioMixdownScenario();
    static TestResult TestLiveSoundReinforcementScenario();
    static TestResult TestBroadcastPostProductionScenario();
    static TestResult TestGameAudioScenario();
    
private:
    static void SimulateComplexMixing(AdvancedAudioProcessor& processor);
    static void SimulateLiveProcessing(AdvancedAudioProcessor& processor);
    static void SimulatePostProduction(AdvancedAudioProcessor& processor);
    static void SimulateInteractiveAudio(AdvancedAudioProcessor& processor);
};

// Performance benchmarking for Phase 3 validation
class Phase3PerformanceBenchmark {
public:
    struct BenchmarkResult {
        float maxTrackCount;
        float averageProcessingTimeMs;
        float cpuUsagePercent;
        float memoryUsageMB;
        bool meetsPhase3Targets;
    };
    
    static BenchmarkResult RunPhase3Benchmark();
    static void ValidatePhase2Compatibility();
    static void TestScalabilityImprovement();
    
private:
    static float MeasureProcessingTime(AdvancedAudioProcessor& processor, const AdvancedAudioBuffer& buffer);
    static float MeasureCPUUsage();
    static float MeasureMemoryUsage();
};

} // namespace VeniceDAW

#endif // ADVANCED_AUDIO_PROCESSOR_TEST_H
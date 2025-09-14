/*
 * performance_test_runner.cpp - Advanced Performance Testing Runner
 * 
 * Comprehensive performance validation for VeniceDAW Phase 2 readiness
 * with 8-track scaling tests and real-time audio constraints.
 */

#include "testing/AdvancedPerformanceTests.h"

#ifdef __HAIKU__
    #include <Application.h>
    #include <Window.h>
    #include <View.h>
    #include <OS.h>
#else
    #include "testing/HaikuMockHeaders.h"
#endif

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <fstream>
#include <iomanip>
#include <algorithm>

using namespace VeniceDAWTesting;
using namespace std::chrono;

class VeniceDAWPerformanceApp : public BApplication {
public:
    VeniceDAWPerformanceApp() : BApplication("application/x-vnd.VeniceDAW-PerformanceTest") {}
    
    virtual void ReadyToRun() override {
        // Application is ready for performance testing
    }
};

struct PerformanceTestReport {
    std::vector<PerformanceMetrics> scalingResults;
    bool phase2Ready = false;
    float overallScore = 0.0f;
    std::string readinessLevel = "NOT_READY";
    std::vector<std::string> recommendations;
    std::chrono::milliseconds totalDuration;
};

void SavePerformanceReport(const PerformanceTestReport& report, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "❌ Failed to open " << filename << " for writing\n";
        return;
    }
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    file << "{\n";
    file << "  \"timestamp\": \"" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\",\n";
    file << "  \"test_type\": \"Performance Station 8-Track Scaling\",\n";
    file << "  \"phase2_readiness\": \"" << report.readinessLevel << "\",\n";
    file << "  \"overall_score\": " << std::fixed << std::setprecision(2) << report.overallScore << ",\n";
    file << "  \"test_duration_ms\": " << report.totalDuration.count() << ",\n";
    file << "  \"track_scaling_results\": [\n";
    
    for (size_t i = 0; i < report.scalingResults.size(); i++) {
        const auto& m = report.scalingResults[i];
        file << "    {\n";
        file << "      \"track_count\": " << m.activeTrackCount << ",\n";
        file << "      \"cpu_usage_percent\": " << m.cpuUsagePercent << ",\n";
        file << "      \"memory_usage_mb\": " << m.memoryUsageMB << ",\n";
        file << "      \"audio_latency_ms\": " << m.audioLatencyMs << ",\n";
        file << "      \"dropout_rate\": " << m.dropoutRate << ",\n";
        file << "      \"frame_rate\": " << m.frameRate << ",\n";
        file << "      \"cpu_per_track\": " << m.cpuPerTrack << ",\n";
        file << "      \"memory_per_track\": " << m.memoryPerTrack << ",\n";
        file << "      \"meets_cpu_threshold\": " << (m.meetsCPUThreshold ? "true" : "false") << ",\n";
        file << "      \"meets_latency_threshold\": " << (m.meetsLatencyThreshold ? "true" : "false") << ",\n";
        file << "      \"meets_dropout_threshold\": " << (m.meetsDropoutThreshold ? "true" : "false") << ",\n";
        file << "      \"meets_framerate_threshold\": " << (m.meetsFrameRateThreshold ? "true" : "false") << "\n";
        file << "    }";
        if (i < report.scalingResults.size() - 1) file << ",";
        file << "\n";
    }
    
    file << "  ],\n";
    file << "  \"linear_scaling_analysis\": {\n";
    
    if (!report.scalingResults.empty()) {
        PerformanceStation8TrackValidator validator;
        float cpuCoeff = validator.CalculateScalingCoefficient(report.scalingResults, "cpu");
        float memoryCoeff = validator.CalculateScalingCoefficient(report.scalingResults, "memory");
        bool isLinear = validator.ValidateLinearScaling(report.scalingResults);
        
        file << "    \"is_linear_scaling\": " << (isLinear ? "true" : "false") << ",\n";
        file << "    \"cpu_coefficient_per_track\": " << cpuCoeff << ",\n";
        file << "    \"memory_coefficient_per_track\": " << memoryCoeff << "\n";
    }
    
    file << "  },\n";
    file << "  \"recommendations\": [\n";
    
    for (size_t i = 0; i < report.recommendations.size(); i++) {
        file << "    \"" << report.recommendations[i] << "\"";
        if (i < report.recommendations.size() - 1) file << ",";
        file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    
    file.close();
    std::cout << "💾 Performance report saved to: " << filename << "\n";
}

PerformanceTestReport RunPerformanceStationScalingTest(int durationPerTrack = 30) {
    PerformanceTestReport report;
    auto startTime = steady_clock::now();
    
    std::cout << "🎛️ VeniceDAW Performance Station 8-Track Scaling Test\n";
    std::cout << "====================================================\n\n";
    std::cout << "🔍 Testing professional audio workstation performance...\n";
    std::cout << "Duration per track count: " << durationPerTrack << " seconds\n";
    std::cout << "Total estimated time: " << (durationPerTrack * 8) / 60 << " minutes\n\n";
    
    try {
        // Initialize performance validator
        PerformanceStation8TrackValidator validator;
        validator.SetTestDuration(durationPerTrack);
        validator.SetSampleRate(44100);
        validator.SetBufferSize(2048);
        
        // Run scaling test
        report.scalingResults = validator.RunScalingTest();
        
        // Print detailed results
        validator.PrintScalingResults(report.scalingResults);
        
        // Evaluate Phase 2 readiness
        report.phase2Ready = Phase2PerformanceGate::EvaluatePhase2Readiness(report.scalingResults);
        
        // Calculate overall score
        float totalScore = 0.0f;
        int validTests = 0;
        
        for (const auto& metrics : report.scalingResults) {
            float testScore = 0.0f;
            
            if (metrics.meetsCPUThreshold) testScore += 25.0f;
            if (metrics.meetsLatencyThreshold) testScore += 25.0f;
            if (metrics.meetsDropoutThreshold) testScore += 25.0f;
            if (metrics.meetsFrameRateThreshold) testScore += 25.0f;
            
            totalScore += testScore;
            validTests++;
        }
        
        report.overallScore = validTests > 0 ? totalScore / validTests : 0.0f;
        
        // Determine readiness level
        if (report.phase2Ready && report.overallScore >= 90.0f) {
            report.readinessLevel = "READY";
        } else if (report.overallScore >= 70.0f) {
            report.readinessLevel = "CONDITIONAL";
        } else {
            report.readinessLevel = "NOT_READY";
        }
        
        // Generate recommendations
        if (!report.phase2Ready) {
            if (report.overallScore < 50.0f) {
                report.recommendations.push_back("Critical performance issues detected - requires major optimization");
                report.recommendations.push_back("Consider reducing buffer size or implementing more efficient algorithms");
                report.recommendations.push_back("Profile audio processing code for performance bottlenecks");
            } else if (report.overallScore < 80.0f) {
                report.recommendations.push_back("Performance acceptable but not optimal for professional use");
                report.recommendations.push_back("Fine-tune audio thread priorities and real-time scheduling");
                report.recommendations.push_back("Optimize memory allocation patterns to reduce fragmentation");
            } else {
                report.recommendations.push_back("Performance nearly meets Phase 2 requirements");
                report.recommendations.push_back("Minor optimizations needed for full professional certification");
                report.recommendations.push_back("Consider implementing lock-free audio-GUI communication");
            }
        } else {
            report.recommendations.push_back("Excellent performance - ready for Phase 2 professional audio features");
            report.recommendations.push_back("Consider implementing advanced features like surround sound processing");
            report.recommendations.push_back("Ready for real-world professional audio workstation usage");
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Performance test failed: " << e.what() << "\n";
        report.readinessLevel = "ERROR";
        report.recommendations.push_back("Test execution failed - check system configuration");
    }
    
    auto endTime = steady_clock::now();
    report.totalDuration = duration_cast<milliseconds>(endTime - startTime);
    
    return report;
}

void PrintPerformanceSummary(const PerformanceTestReport& report) {
    std::cout << "\n🎯 VeniceDAW Performance Station Test Summary\n";
    std::cout << "===========================================\n\n";
    
    // Overall results
    std::cout << "📊 Overall Score: " << std::fixed << std::setprecision(1) 
              << report.overallScore << "%\n";
    std::cout << "⏱️ Total Duration: " << report.totalDuration.count() / 1000 << " seconds\n";
    std::cout << "🎵 Tracks Tested: " << report.scalingResults.size() << " configurations\n\n";
    
    // Phase 2 readiness
    if (report.readinessLevel == "READY") {
        std::cout << "✅ PHASE 2 READY - Professional audio workstation performance validated!\n";
        std::cout << "🚀 8-track simultaneous processing meets all industry thresholds\n";
    } else if (report.readinessLevel == "CONDITIONAL") {
        std::cout << "⚠️ CONDITIONAL - Good performance with minor issues\n";
        std::cout << "🔧 Some optimization needed before full professional deployment\n";
    } else {
        std::cout << "❌ NOT READY - Significant performance issues detected\n";
        std::cout << "🛠️ Major optimizations required before Phase 2\n";
    }
    
    std::cout << "\n💡 Recommendations:\n";
    for (const auto& rec : report.recommendations) {
        std::cout << "   • " << rec << "\n";
    }
    
    std::cout << "\n🎛️ Performance Station Scaling Analysis:\n";
    if (!report.scalingResults.empty()) {
        const auto& last = report.scalingResults.back(); // 8-track result
        std::cout << "   Final 8-track CPU: " << last.cpuUsagePercent << "%\n";
        std::cout << "   Final 8-track Memory: " << last.memoryUsageMB << "MB\n";
        std::cout << "   Audio Latency: " << last.audioLatencyMs << "ms\n";
        std::cout << "   Dropout Rate: " << (last.dropoutRate * 100) << "%\n";
        std::cout << "   Frame Rate: " << last.frameRate << " FPS\n";
    }
}

void PrintUsage(const char* programName) {
    std::cout << "VeniceDAW Performance Station 8-Track Scaling Test\n";
    std::cout << "=================================================\n\n";
    std::cout << "Usage: " << programName << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --duration SECONDS     Test duration per track count (default: 30)\n";
    std::cout << "  --json-output FILE     Save results to JSON file\n";
    std::cout << "  --quick                Quick test (10 seconds per track)\n";
    std::cout << "  --verbose              Enable verbose output\n";
    std::cout << "  --help                 Show this help\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " --quick\n";
    std::cout << "  " << programName << " --duration 60 --json-output perf_results.json\n\n";
    std::cout << "This test validates VeniceDAW's ability to handle professional\n";
    std::cout << "audio workstation loads with linear scaling up to 8 concurrent tracks.\n";
}

int main(int argc, char* argv[]) {
    int testDuration = 30;
    std::string jsonOutput = "";
    bool verbose = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            PrintUsage(argv[0]);
            return 0;
        } else if (arg == "--duration" && i + 1 < argc) {
            testDuration = std::stoi(argv[++i]);
        } else if (arg == "--json-output" && i + 1 < argc) {
            jsonOutput = argv[++i];
        } else if (arg == "--quick") {
            testDuration = 10;
        } else if (arg == "--verbose") {
            verbose = true;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            PrintUsage(argv[0]);
            return 1;
        }
    }
    
    // Validate parameters
    if (testDuration < 5 || testDuration > 300) {
        std::cerr << "❌ Invalid duration: " << testDuration << " (must be 5-300 seconds)\n";
        return 1;
    }
    
#ifdef __HAIKU__
    // Initialize BApplication for BeAPI context
    VeniceDAWPerformanceApp app;
    std::cout << "✅ Running on native Haiku with real BeAPI integration\n\n";
#else
    std::cout << "⚠️ Running on non-Haiku system with mock APIs\n";
    std::cout << "   Results are for testing purposes only!\n\n";
#endif
    
    // Run performance test
    auto report = RunPerformanceStationScalingTest(testDuration);
    
    // Print summary
    PrintPerformanceSummary(report);
    
    // Save results
    if (!jsonOutput.empty()) {
        SavePerformanceReport(report, jsonOutput);
    } else {
        SavePerformanceReport(report, "performance_scaling_results.json");
    }
    
    std::cout << "\n🎯 Next Steps:\n";
    if (report.readinessLevel == "READY") {
        std::cout << "   1. Deploy Phase 2 professional features\n";
        std::cout << "   2. Begin real-world audio workstation testing\n";
        std::cout << "   3. Consider advanced features (surround, automation)\n";
    } else {
        std::cout << "   1. Address performance recommendations above\n";
        std::cout << "   2. Re-run test after optimizations\n";
        std::cout << "   3. Profile critical audio processing paths\n";
    }
    
    return report.phase2Ready ? 0 : 1;
}
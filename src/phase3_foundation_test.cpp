/*
 * phase3_foundation_test.cpp - Phase 3.1 Foundation Validation Runner
 * 
 * Comprehensive test runner for Advanced Audio Processor foundation.
 * Validates all Phase 3.1 components before implementation proceeds.
 */

#include "testing/AdvancedAudioProcessorTest.h"
#include "audio/AdvancedAudioProcessor.h"

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

using namespace VeniceDAW;
using namespace std::chrono;

class Phase3TestApp : public BApplication {
public:
    Phase3TestApp() : BApplication("application/x-vnd.VeniceDAW-Phase3Test") {}
    
    virtual void ReadyToRun() override {
        // Application ready for Phase 3 testing
    }
};

void PrintUsage(const char* programName) {
    std::cout << "VeniceDAW Phase 3.1 Foundation Test Suite\n";
    std::cout << "=========================================\n\n";
    std::cout << "Usage: " << programName << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --quick              Run quick validation tests only\n";
    std::cout << "  --comprehensive      Run complete test suite (default)\n";
    std::cout << "  --performance        Focus on performance validation\n";
    std::cout << "  --compatibility      Test Phase 2 compatibility\n";
    std::cout << "  --output FILE        Save results to JSON file\n";
    std::cout << "  --verbose            Enable verbose output\n";
    std::cout << "  --help               Show this help\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " --quick\n";
    std::cout << "  " << programName << " --comprehensive --output phase3_results.json\n";
    std::cout << "  " << programName << " --performance --verbose\n\n";
    std::cout << "This suite validates the Phase 3.1 Advanced Audio Processing foundation:\n";
    std::cout << "  • Multi-channel audio buffer management\n";
    std::cout << "  • Professional effects framework structure\n";
    std::cout << "  • Surround sound processing architecture\n";
    std::cout << "  • Performance characteristics and compatibility\n\n";
    std::cout << "Target: Validate foundation before implementing Phase 3.1 features\n";
}

int main(int argc, char* argv[]) {
    bool quickMode = false;
    bool performanceMode = false;
    bool compatibilityMode = false;
    bool verbose = false;
    std::string outputFile = "";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            PrintUsage(argv[0]);
            return 0;
        } else if (arg == "--quick") {
            quickMode = true;
        } else if (arg == "--comprehensive") {
            quickMode = false;
        } else if (arg == "--performance") {
            performanceMode = true;
        } else if (arg == "--compatibility") {
            compatibilityMode = true;
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--output" && i + 1 < argc) {
            outputFile = argv[++i];
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            PrintUsage(argv[0]);
            return 1;
        }
    }
    
#ifdef __HAIKU__
    // Initialize BApplication for BeAPI context
    // Note: BApplication is needed but we don't run it in a separate thread for tests
    Phase3TestApp* app = new Phase3TestApp();
    if (app->InitCheck() != B_OK) {
        std::cerr << "❌ Failed to initialize BApplication\n";
        return 1;
    }
    std::cout << "✅ Running on native Haiku with real BeAPI\n\n";
#else
    std::cout << "⚠️ Running on non-Haiku system with mock APIs\n";
    std::cout << "   Foundation tests only - full validation requires Haiku\n\n";
#endif
    
    auto testStart = steady_clock::now();
    
    std::cout << "🚀 VeniceDAW Phase 3.1 Foundation Test Suite\n";
    std::cout << "============================================\n\n";
    std::cout << "🎯 Mission: Validate Advanced Audio Processing foundation\n";
    std::cout << "   Target: Multi-channel professional audio framework\n";
    std::cout << "   Building on: Phase 2 optimizations (54-track, 192.3% improvement)\n\n";
    
    if (verbose) {
        std::cout << "🔧 Test Configuration:\n";
        std::cout << "   Mode: " << (quickMode ? "Quick" : "Comprehensive") << "\n";
        std::cout << "   Performance Focus: " << (performanceMode ? "Yes" : "No") << "\n";
        std::cout << "   Compatibility Check: " << (compatibilityMode ? "Yes" : "No") << "\n";
        std::cout << "   Output File: " << (outputFile.empty() ? "Console only" : outputFile) << "\n\n";
    }
    
    // Initialize test suite
    AdvancedAudioProcessorTest testSuite;
    std::vector<TestResult> results;
    
    try {
        if (quickMode) {
            std::cout << "⚡ Running Quick Foundation Validation...\n\n";
            
            // Run essential tests only
            auto bufferTests = testSuite.TestAudioBufferManagement();
            auto performanceTests = testSuite.TestPerformanceCharacteristics();
            
            results.insert(results.end(), bufferTests.begin(), bufferTests.end());
            results.insert(results.end(), performanceTests.begin(), performanceTests.end());
            
        } else if (performanceMode) {
            std::cout << "⚡ Running Performance-Focused Validation...\n\n";
            
            auto performanceTests = testSuite.TestPerformanceCharacteristics();
            auto integrationTests = testSuite.TestIntegrationScenarios();
            
            results.insert(results.end(), performanceTests.begin(), performanceTests.end());
            results.insert(results.end(), integrationTests.begin(), integrationTests.end());
            
        } else if (compatibilityMode) {
            std::cout << "🔗 Running Phase 2 Compatibility Validation...\n\n";
            
            auto integrationTests = testSuite.TestIntegrationScenarios();
            results.insert(results.end(), integrationTests.begin(), integrationTests.end());
            
        } else {
            std::cout << "🧪 Running Comprehensive Foundation Validation...\n\n";
            
            // Run all tests
            results = testSuite.RunAllTests();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test suite execution failed: " << e.what() << "\n";
        return 1;
    }
    
    auto testEnd = steady_clock::now();
    auto totalDuration = duration_cast<milliseconds>(testEnd - testStart);
    
    // Print results
    testSuite.PrintTestResults(results);
    
    // Calculate summary metrics
    int totalTests = results.size();
    int passedTests = 0;
    float totalScore = 0.0f;
    
    for (const auto& result : results) {
        if (result.passed) passedTests++;
        totalScore += result.score;
    }
    
    float averageScore = totalTests > 0 ? totalScore / totalTests : 0.0f;
    float successRate = totalTests > 0 ? (float)passedTests / totalTests * 100.0f : 0.0f;
    
    std::cout << "🎯 Phase 3.1 Foundation Validation Summary\n";
    std::cout << "==========================================\n\n";
    
    std::cout << "📊 Overall Results:\n";
    std::cout << "   Total Duration: " << totalDuration.count() / 1000.0f << " seconds\n";
    std::cout << "   Tests Executed: " << totalTests << "\n";
    std::cout << "   Tests Passed: " << passedTests << "/" << totalTests << "\n";
    std::cout << "   Success Rate: " << std::fixed << std::setprecision(1) << successRate << "%\n";
    std::cout << "   Average Score: " << averageScore << "%\n\n";
    
    // Foundation readiness assessment
    if (averageScore >= 90.0f && successRate >= 90.0f) {
        std::cout << "✅ FOUNDATION READY - Phase 3.1 implementation can proceed\n";
        std::cout << "🚀 Advanced Audio Processing foundation is solid and validated\n";
        std::cout << "   Ready for: Professional effects, surround sound, spatial audio\n\n";
    } else if (averageScore >= 75.0f && successRate >= 80.0f) {
        std::cout << "⚠️ FOUNDATION MOSTLY READY - Minor issues need attention\n";
        std::cout << "🔧 Some components need refinement before full implementation\n";
        std::cout << "   Can proceed with caution and targeted fixes\n\n";
    } else {
        std::cout << "❌ FOUNDATION NOT READY - Significant issues detected\n";
        std::cout << "🛠️ Foundation needs substantial work before implementation\n";
        std::cout << "   Address core issues before proceeding to Phase 3.1 features\n\n";
    }
    
    std::cout << "🎯 Recommended Next Steps:\n";
    if (averageScore >= 85.0f) {
        std::cout << "   1. Implement AdvancedAudioProcessor.cpp with validated interfaces\n";
        std::cout << "   2. Begin ProfessionalEQ and DynamicsProcessor implementation\n";
        std::cout << "   3. Develop SurroundProcessor with multi-channel support\n";
    } else {
        std::cout << "   1. Address failing foundation tests and architecture issues\n";
        std::cout << "   2. Optimize buffer management and channel handling\n";
        std::cout << "   3. Re-run validation before proceeding with implementation\n";
    }
    
    // Save results if requested
    if (!outputFile.empty()) {
        testSuite.SaveTestReport(results, outputFile);
        std::cout << "\n💾 Test results saved to: " << outputFile << "\n";
    } else {
        testSuite.SaveTestReport(results, "phase3_foundation_validation.json");
        std::cout << "\n💾 Test results saved to: phase3_foundation_validation.json\n";
    }
    
    std::cout << "\n🎯 Foundation Validation Complete!\n";
    
    // Return appropriate exit code
    if (averageScore >= 85.0f && successRate >= 85.0f) {
        std::cout << "✅ Phase 3.1 foundation validated - proceed with implementation\n";
        return 0;
    } else {
        std::cout << "🔧 Foundation needs improvement before Phase 3.1 implementation\n";
        return 1;
    }
}
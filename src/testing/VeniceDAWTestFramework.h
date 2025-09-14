/*
 * VeniceDAWTestFramework.h - Comprehensive Testing System for Phase 2 Readiness
 * 
 * This framework implements the three-layer testing architecture:
 * 1. Core framework tests (CppUnit + Haiku's ThreadedTestCaller)
 * 2. Professional audio GUI validation 
 * 3. Go/No-Go determination with quantitative metrics
 */

#ifndef VENICEDAW_TEST_FRAMEWORK_H
#define VENICEDAW_TEST_FRAMEWORK_H

// Conditional BeAPI headers - use real ones on Haiku, mock ones for development
#ifdef __HAIKU__
    #include <Application.h>
    #include <Window.h>
    #include <View.h>
    #include <Message.h>
    #include <Looper.h>
    #include <Handler.h>
    #include <Messenger.h>
    #include <MediaKit.h>
#else
    #include "HaikuMockHeaders.h"
    #warning "Compilando con mock headers - funziona SOLO su Haiku nativo!"
#endif

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>

// Forward declarations
namespace HaikuDAW {
    class SimpleHaikuEngine;
    class PerformanceStation;
}

namespace VeniceDAWTesting {

// Phase 2 readiness thresholds
struct Phase2Thresholds {
    // Memory stability gates
    static constexpr float MAX_MEMORY_GROWTH_MB_PER_HOUR = 1.0f;
    static constexpr float MAX_MEMORY_FRAGMENTATION = 25.0f;
    
    // Performance gates
    static constexpr float MIN_FPS = 60.0f;
    static constexpr float MAX_RESPONSE_TIME_MS = 100.0f;
    static constexpr float MAX_CPU_USAGE_8_TRACKS = 70.0f;
    static constexpr float MAX_FRAME_DROP_PERCENTAGE = 5.0f;
    
    // Reliability gates
    static constexpr float MIN_MTBF_HOURS = 72.0f;
    static constexpr float MAX_CRASH_RATE = 0.01f;
    static constexpr float MAX_ERROR_RECOVERY_SEC = 5.0f;
    
    // Audio-specific gates
    static constexpr float MAX_RTL_MS = 12.0f;
    static constexpr float MAX_DROPOUT_RATE = 0.001f;
    static constexpr float MAX_JITTER_MS = 1.0f;
    
    // Go/No-Go score thresholds
    static constexpr float MIN_MEMORY_SCORE = 0.95f;
    static constexpr float MIN_PERFORMANCE_SCORE = 0.90f;
    static constexpr float MIN_RELIABILITY_SCORE = 0.98f;
    static constexpr float MIN_AUDIO_SCORE = 0.95f;
};

// Test result categories
enum class TestCategory {
    MEMORY_STABILITY,
    PERFORMANCE,
    RELIABILITY,
    AUDIO_REALTIME,
    THREAD_SAFETY,
    GUI_RESPONSIVENESS
};

// Enhanced test result with Phase 2 analysis
struct TestResult {
    std::string name;
    std::string description;
    TestCategory category;
    
    float actualValue;
    float targetValue;
    float score;         // 0.0 - 1.0
    std::string unit;
    
    std::chrono::milliseconds duration;
    std::string bottleneck;
    std::string recommendation;
    
    // Thread safety specific
    bool hasThreadSafetyIssues;
    std::vector<std::string> threadSafetyViolations;
    
    // Memory analysis
    size_t peakMemoryUsage;
    size_t memoryLeaks;
    float memoryFragmentation;
    
    TestResult() : actualValue(0.0f), targetValue(0.0f), score(0.0f),
                   hasThreadSafetyIssues(false), peakMemoryUsage(0),
                   memoryLeaks(0), memoryFragmentation(0.0f) {}
};

// Go/No-Go evaluation results
struct Phase2ReadinessReport {
    float memoryScore;
    float performanceScore;
    float reliabilityScore;
    float audioScore;
    float overallScore;
    
    bool isReady;
    std::vector<std::string> blockingIssues;
    std::vector<std::string> recommendations;
    
    Phase2ReadinessReport() : memoryScore(0.0f), performanceScore(0.0f),
                              reliabilityScore(0.0f), audioScore(0.0f),
                              overallScore(0.0f), isReady(false) {}
};

// Memory leak detection using Haiku's malloc_debug
class HaikuMemoryAnalyzer {
public:
    HaikuMemoryAnalyzer();
    ~HaikuMemoryAnalyzer();
    
    void StartMemoryMonitoring();
    void StopMemoryMonitoring();
    
    size_t GetCurrentMemoryUsage();
    size_t GetPeakMemoryUsage();
    std::vector<std::string> DetectMemoryLeaks();
    float CalculateFragmentation();
    
    // 8-hour stress test support
    void StartLongTermMonitoring();
    bool IsMemoryGrowthAcceptable(std::chrono::hours testDuration);
    
private:
    size_t fInitialMemory;
    size_t fPeakMemory;
    std::vector<size_t> fMemoryHistory;
    bool fMonitoring;
    thread_id fMonitorThread;
    
    static int32 MemoryMonitorThread(void* data);
};

// Thread safety validator for BeAPI components
class BeAPIThreadSafetyValidator {
public:
    BeAPIThreadSafetyValidator();
    ~BeAPIThreadSafetyValidator();
    
    // Test BLooper/BWindow threading issues
    bool ValidateBLooperThreadSafety(BLooper* looper);
    bool ValidateBWindowMessageHandling(BWindow* window);
    bool ValidateLockFreeAudioGUIComm();
    
    // Lock-free communication validation
    struct AudioGUICommTest {
        std::atomic<float> parameterValue{0.0f};
        std::atomic<bool> testRunning{false};
        std::atomic<int> dropoutCount{0};
        
        void RunLockFreeTest(int iterations = 10000);
        bool HasDropouts() const { return dropoutCount.load() > 0; }
    };
    
    AudioGUICommTest* CreateLockFreeTest();
    
private:
    std::vector<std::string> fThreadSafetyViolations;
};

// Performance Station 8-track scaling validator
class PerformanceStationScalingTest {
public:
    PerformanceStationScalingTest();
    ~PerformanceStationScalingTest();
    
    struct ScalingResult {
        int trackCount;
        float cpuUsage;
        float memoryUsageMB;
        float frameRate;
        bool successful;
        std::string failureReason;
    };
    
    std::vector<ScalingResult> TestTrackScaling(int maxTracks = 8);
    bool ValidateLinearResourceScaling(const std::vector<ScalingResult>& results);
    
private:
    HaikuDAW::SimpleHaikuEngine* fEngine;
    HaikuDAW::PerformanceStation* fStation;
};

// GUI responsiveness and B_FOLLOW_ALL validation
class GUIResponsivenessValidator {
public:
    GUIResponsivenessValidator();
    ~GUIResponsivenessValidator();
    
    // Test MixerWindow with 120 controls (8 channels × 15 controls)
    struct MixerControlTest {
        int channelCount;
        int controlsPerChannel;
        float updateRate; // updates per second
        float averageResponseTime;
        float frameRate;
        bool hasVisualArtifacts;
    };
    
    MixerControlTest TestMixerWindowControls();
    
    // Test TSoundView B_FOLLOW_ALL behavior
    bool ValidateTSoundViewResize();
    
    // OpenGL stability for Mixer3DWindow
    bool ValidateOpenGLContextManagement();
    
private:
    BWindow* fTestWindow;
    BView* fTestView;
};

// Main testing framework coordinating all tests
class VeniceDAWTestFramework {
public:
    VeniceDAWTestFramework();
    ~VeniceDAWTestFramework();
    
    // Main test execution
    Phase2ReadinessReport RunPhase2ValidationSuite();
    
    // Individual test categories
    std::vector<TestResult> RunMemoryStabilityTests();
    std::vector<TestResult> RunPerformanceTests();
    std::vector<TestResult> RunReliabilityTests();
    std::vector<TestResult> RunAudioRealtimeTests();
    std::vector<TestResult> RunThreadSafetyTests();
    std::vector<TestResult> RunGUIResponsivenessTests();
    
    // Go/No-Go evaluation
    Phase2ReadinessReport EvaluatePhase2Readiness(const std::vector<TestResult>& allResults);
    
    // Remediation strategies
    std::vector<std::string> GenerateRemediationPlan(const Phase2ReadinessReport& report);
    
    // Progress reporting
    typedef std::function<void(float progress, const std::string& currentTest)> ProgressCallback;
    void SetProgressCallback(ProgressCallback callback);
    
    // Result management
    void SaveResultsToFile(const std::string& filename, const Phase2ReadinessReport& report);
    void GenerateHTMLReport(const std::string& filename, const Phase2ReadinessReport& report);
    
private:
    std::unique_ptr<HaikuMemoryAnalyzer> fMemoryAnalyzer;
    std::unique_ptr<BeAPIThreadSafetyValidator> fThreadSafetyValidator;
    std::unique_ptr<PerformanceStationScalingTest> fScalingTest;
    std::unique_ptr<GUIResponsivenessValidator> fGUIValidator;
    
    std::vector<TestResult> fAllResults;
    ProgressCallback fProgressCallback;
    
    // Helper methods
    float CalculateCategoryScore(const std::vector<TestResult>& results, TestCategory category);
    std::string GetCategorySummary(const std::vector<TestResult>& results, TestCategory category);
    bool IsBlockingIssue(const TestResult& result);
};

// Automated test runner for CI/CD integration
class AutomatedTestRunner {
public:
    AutomatedTestRunner();
    ~AutomatedTestRunner();
    
    // Command-line interface for CI/CD
    int RunFromCommandLine(int argc, char** argv);
    
    // CI/CD specific methods
    bool RunQuickValidation();           // < 5 minutes
    bool RunFullValidationSuite();      // 8+ hours with stress tests
    void GenerateJUnitXMLReport(const std::string& filename);
    void GenerateGitHubActionsOutput();
    
    // Integration with hey tool for GUI automation
    bool RunGUIAutomationTests();
    bool ExecuteHeyCommand(const std::string& command);
    
private:
    std::unique_ptr<VeniceDAWTestFramework> fFramework;
    
    void PrintUsage(const char* programName);
    bool SetupTestEnvironment();
    void CleanupTestEnvironment();
};

} // namespace VeniceDAWTesting

#endif // VENICEDAW_TEST_FRAMEWORK_H
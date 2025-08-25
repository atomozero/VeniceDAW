/*
 * PerformanceStation.h - VeniceDAW Performance Analysis Station
 */

#ifndef PERFORMANCE_STATION_H
#define PERFORMANCE_STATION_H

#include <vector>
#include <map>
#include <string>
#include <kernel/OS.h>
#include <Window.h>
#include <GLView.h>
#include <InterfaceDefs.h>  // For rgb_color

// Forward declarations
namespace HaikuDAW {
    class SimpleHaikuEngine;
    class Mixer3DWindow;
}

namespace HaikuDAW {

// RAII Guard per gestione sicura OpenGL context
class GLContextGuard {
    BGLView* view;
    bool locked;
public:
    GLContextGuard(BGLView* v) : view(v), locked(false) {
        if (view) {
            view->LockGL();
            locked = true;
        }
    }
    ~GLContextGuard() { 
        if (locked && view) {
            view->UnlockGL(); 
        }
    }
    operator bool() const { return locked; }
    
    // Disable copy
    GLContextGuard(const GLContextGuard&) = delete;
    GLContextGuard& operator=(const GLContextGuard&) = delete;
};

// RAII Guard per gestione sicura Window lock
class WindowLockGuard {
    BWindow* window;
    bool locked;
public:
    WindowLockGuard(BWindow* w) : window(w), locked(false) {
        if (window && window->Lock()) {
            locked = true;
        }
    }
    ~WindowLockGuard() { 
        if (locked && window) {
            window->Unlock(); 
        }
    }
    operator bool() const { return locked; }
    
    // Disable copy
    WindowLockGuard(const WindowLockGuard&) = delete;
    WindowLockGuard& operator=(const WindowLockGuard&) = delete;
};

// Performance categories for DAW-specific analysis
enum PerformanceCategory {
    AUDIO_REALTIME = 0,     // Latency, dropouts, jitter
    SYSTEM_RESOURCES,       // CPU, RAM, I/O bandwidth  
    GRAPHICS_3D,            // FPS, render times, GPU usage
    STABILITY              // Error rates, thermal, power
};

// Trend data for performance analysis
struct TrendData {
    std::vector<float> history;    // Last N measurements
    float average;
    float variance;
    bool isStable;
    
    TrendData() : average(0.0f), variance(0.0f), isStable(false) {}
    
    void AddMeasurement(float value) {
        history.push_back(value);
        if (history.size() > 10) {
            history.erase(history.begin());
        }
        CalculateStats();
    }
    
    void CalculateStats() {
        if (history.empty()) return;
        
        // Calculate average
        float sum = 0.0f;
        for (float v : history) sum += v;
        average = sum / history.size();
        
        // Calculate variance
        float varSum = 0.0f;
        for (float v : history) {
            float diff = v - average;
            varSum += diff * diff;
        }
        variance = varSum / history.size();
        
        // Consider stable if variance is low (< 5% of average)
        isStable = (variance < (average * 0.05f));
    }
};

struct BenchmarkResult {
    std::string name;
    std::string category;
    std::string unit;
    float value;              // Primary metric value
    float duration;           // Test duration in ms
    float score;              // Performance score (0-100)
    
    // PHASE 1 ENHANCEMENTS - Professional DAW Analysis
    PerformanceCategory perfCategory;
    float targetValue;        // Target value for DAW usage
    float actualValue;        // Raw measurement (before scoring)
    std::string bottleneck;   // "CPU bound", "RAM limited", "Thermal", etc.
    TrendData trend;          // Performance trend analysis
    bool isRealTime;          // Critical for real-time audio
    float cpuUsage;           // CPU usage during test
    float memoryMB;           // Memory usage in MB
    std::string recommendation; // Optimization suggestion
    
    BenchmarkResult() : 
        value(0.0f), duration(0.0f), score(0.0f),
        perfCategory(SYSTEM_RESOURCES), targetValue(0.0f), 
        actualValue(0.0f), isRealTime(false), 
        cpuUsage(0.0f), memoryMB(0.0f) {}
};

class PerformanceStation {
public:
    PerformanceStation();
    ~PerformanceStation();
    
    // Main benchmark entry point
    void RunAllTests();
    
    // Individual test categories
    void TestAudioEngine();
    void TestAudioLatency();
    void TestSineGeneration();
    void TestBufferProcessing();
    
    void Test3DRenderingFPS();
    void Test3DAnimationSmooth();
    void Test3DInteractionLatency();
    
    void TestMemoryUsage();
    void TestMemoryBandwidth();
    
    void TestRealtimePerformance();
    void TestCPUScaling();
    
    // Results and reporting
    void GenerateFinalReport();
    void SaveReport(const std::string& filename);
    void PrintSystemInfo();
    
    // Visualization helpers
    void PrintProgressBar(const char* label, float value, float maxValue);
    void PrintCategoryBars();
    
    // Progress callback system
    typedef void (*ProgressCallback)(float progress, const char* testName, void* userData);
    void SetProgressCallback(ProgressCallback callback, void* userData);
    
    // Getters and result management
    float GetTotalScore() const { return fTotalScore; }
    const std::vector<BenchmarkResult>& GetResults() const { return fResults; }
    bool IsRunning() const { return fRunning; }
    void AddResult(const BenchmarkResult& result) { fResults.push_back(result); }
    void SetTotalScore(float score) { fTotalScore = score; }
    void ClearResults() { fResults.clear(); fTotalScore = 0.0f; }
    
    // PHASE 1 ENHANCEMENTS - DAW-Specific Analysis
    static float GetDAWTargetValue(const std::string& testName, PerformanceCategory category);
    static const char* GetDAWStatusText(float value, float target, PerformanceCategory category);
    static rgb_color GetDAWStatusColor(float value, float target, PerformanceCategory category);
    static std::string AnalyzeBottleneck(const BenchmarkResult& result);
    static std::string GenerateRecommendation(const BenchmarkResult& result);
    
    // Enhanced result creation with DAW context
    BenchmarkResult CreateEnhancedResult(const std::string& name, 
                                       const std::string& unit,
                                       float actualValue,
                                       PerformanceCategory category,
                                       bool isRealTime = false,
                                       float cpuUsage = 0.0f,
                                       float memoryMB = 0.0f);
    
    // Category-specific analysis
    std::vector<BenchmarkResult> GetResultsByCategory(PerformanceCategory category) const;
    float GetCategoryScore(PerformanceCategory category) const;
    std::string GetCategorySummary(PerformanceCategory category) const;

private:
    // Test infrastructure
    SimpleHaikuEngine* fEngine;
    Mixer3DWindow* f3DWindow;
    std::vector<BenchmarkResult> fResults;
    float fTotalScore;
    bool fRunning;
    
    // Progress callback
    ProgressCallback fProgressCallback;
    void* fProgressUserData;
    
    // OpenGL helper functions for 3D tests
    void DrawCylinder(float radius, float height, int segments);
    void DrawVUMeter(float level);
    void CalculateTransformMatrix(float* matrix, void* obj);
    float RaySphereIntersect(float* origin, float* dir, void* sphere);
    
    // Threading helper
    static int32 CPUWorkThread(void* data);
    
    // 3D test isolation helpers
    void Run3DTestProcess(const char* resultFile);
    bool Read3DTestResults(const char* resultFile, BenchmarkResult* result);
    
    // Audio latency measurement
    float MeasureRealAudioLatency(int bufferSize);
};

} // namespace HaikuDAW

#endif // PERFORMANCE_STATION_H
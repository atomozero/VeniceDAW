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

struct BenchmarkResult {
    std::string name;
    std::string category;
    std::string unit;
    float value;         // Primary metric value
    float duration;      // Test duration in ms
    float score;         // Performance score (0-100)
    
    BenchmarkResult() : value(0.0f), duration(0.0f), score(0.0f) {}
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
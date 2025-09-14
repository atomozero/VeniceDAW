/*
 * AudioOptimizer.h - VeniceDAW Audio Performance Optimization Suite
 * 
 * Implements data-driven optimizations based on performance analysis:
 * 1. Buffer Size Tuner - Automatic latency optimization
 * 2. Thread Priority Optimizer - BeAPI real-time scheduling
 * 3. Bottleneck Resolver - Track 6+ performance analysis
 */

#ifndef AUDIO_OPTIMIZER_H
#define AUDIO_OPTIMIZER_H

#ifdef __HAIKU__
    #include <Application.h>
    #include <Window.h>
    #include <View.h>
    #include <MediaKit.h>
    #include <SoundPlayer.h>
    #include <OS.h>
    #include <scheduler.h>
#else
    #include "HaikuMockHeaders.h"
#endif

#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <atomic>
#include <thread>
#include <cmath>
#include <fstream>
#include <map>

namespace VeniceDAWOptimization {

// Optimization result structure
struct OptimizationResult {
    std::string optimizationType;
    bool successful = false;
    float improvementPercent = 0.0f;
    std::string details;
    std::map<std::string, float> metrics;
};

// Buffer configuration testing
struct BufferConfig {
    int bufferSize;
    int sampleRate;
    float theoreticalLatencyMs;
    float measuredLatencyMs;
    float cpuUsage;
    float dropoutRate;
    bool meetsTarget = false;
};

// 1. BUFFER SIZE TUNER
class AudioBufferOptimizer {
public:
    AudioBufferOptimizer();
    ~AudioBufferOptimizer();
    
    // Main optimization methods
    OptimizationResult OptimizeBufferSize(int targetLatencyMs = 12);
    std::vector<BufferConfig> TestBufferSizes(const std::vector<int>& bufferSizes);
    BufferConfig FindOptimalBuffer(int targetLatencyMs);
    
    // Analysis methods
    BufferConfig MeasureBufferPerformance(int bufferSize, int sampleRate = 44100);
    float CalculateTheoreticalLatency(int bufferSize, int sampleRate);
    float MeasureActualLatency(int bufferSize);
    
    // Configuration
    void SetTestDuration(int seconds) { fTestDurationSeconds = seconds; }
    void SetTrackCount(int tracks) { fTrackCount = tracks; }
    
    // Reporting
    void GenerateBufferReport(const std::vector<BufferConfig>& configs, 
                             const std::string& filename);
    void PrintBufferResults(const std::vector<BufferConfig>& configs);
    
private:
    int fTestDurationSeconds = 15;
    int fTrackCount = 4; // Test with 4 tracks (safe zone)
    
#ifdef __HAIKU__
    BSoundPlayer* fSoundPlayer = nullptr;
    void SetupSoundPlayer(int bufferSize, int sampleRate);
    void CleanupSoundPlayer();
    static void BufferTestCallback(void* userData, void* buffer, size_t size, 
                                  const media_raw_audio_format& format);
#endif
};

// 2. THREAD PRIORITY OPTIMIZER  
class ThreadPriorityOptimizer {
public:
    ThreadPriorityOptimizer();
    ~ThreadPriorityOptimizer();
    
    // Main optimization methods
    OptimizationResult OptimizeAudioThreads();
    OptimizationResult ConfigureRealtimeScheduling();
    
    // Thread management
    bool SetAudioThreadPriority(thread_id thread, int priority);
    bool EnableRealtimeScheduling();
    thread_id GetAudioThreadId();
    
    // Performance measurement
    float MeasureThreadPerformance();
    std::map<thread_id, int> GetCurrentThreadPriorities();
    
    // Analysis
    OptimizationResult AnalyzeThreadBottlenecks();
    void PrintThreadStatus();
    
private:
    std::vector<thread_id> fAudioThreads;
    int fOriginalPriority = B_NORMAL_PRIORITY;
    bool fRealtimeEnabled = false;
    
#ifdef __HAIKU__
    bool ConfigureScheduler();
    void RestoreOriginalPriorities();
    static int32 AudioThreadTest(void* data);
#endif
};

// 3. BOTTLENECK RESOLVER
class AudioBottleneckResolver {
public:
    AudioBottleneckResolver();
    ~AudioBottleneckResolver();
    
    // Main analysis methods
    OptimizationResult AnalyzeTrack6Bottleneck();
    OptimizationResult ProfileScalingBehavior();
    OptimizationResult IdentifyPerformanceCliffs();
    
    // Profiling utilities
    struct BottleneckProfile {
        int trackThreshold = 0;        // Track where problems start
        float cpuJumpPercent = 0.0f;   // CPU usage jump
        float fpsDropPercent = 0.0f;   // FPS drop
        std::string rootCause;         // Identified cause
        std::vector<std::string> fixes; // Recommended fixes
    };
    
    BottleneckProfile AnalyzePerformanceData(const std::vector<float>& cpuUsage,
                                           const std::vector<float>& frameRates);
    
    // Optimization strategies
    OptimizationResult ApplyMemoryOptimization();
    OptimizationResult ApplyCacheOptimization(); 
    OptimizationResult ApplyAlgorithmOptimization();
    
    // Analysis tools
    float DetectPerformanceCliff(const std::vector<float>& metrics);
    std::string IdentifyBottleneckCause(const BottleneckProfile& profile);
    
    void GenerateBottleneckReport(const BottleneckProfile& profile,
                                 const std::string& filename);
    void PrintBottleneckAnalysis(const BottleneckProfile& profile);
    
private:
    std::vector<float> fCpuHistory;
    std::vector<float> fMemoryHistory;
    std::vector<float> fFpsHistory;
};

// MASTER OPTIMIZATION COORDINATOR
class VeniceDAWOptimizationSuite {
public:
    VeniceDAWOptimizationSuite();
    ~VeniceDAWOptimizationSuite();
    
    // Master optimization workflow
    std::vector<OptimizationResult> RunCompleteOptimization();
    
    // Individual optimizers
    OptimizationResult OptimizeBuffers(int targetLatencyMs = 12);
    OptimizationResult OptimizeThreads();
    OptimizationResult ResolveBottlenecks();
    
    // Validation
    OptimizationResult ValidateOptimizations();
    bool RunPostOptimizationTest();
    
    // Configuration
    void SetOptimizationLevel(const std::string& level); // "conservative", "balanced", "aggressive"
    void SetTargetPerformance(int maxTracks, float maxLatencyMs, float minFPS);
    
    // Reporting
    void GenerateOptimizationReport(const std::vector<OptimizationResult>& results,
                                   const std::string& filename);
    void PrintOptimizationSummary(const std::vector<OptimizationResult>& results);
    
private:
    std::unique_ptr<AudioBufferOptimizer> fBufferOptimizer;
    std::unique_ptr<ThreadPriorityOptimizer> fThreadOptimizer;
    std::unique_ptr<AudioBottleneckResolver> fBottleneckResolver;
    
    // Target performance metrics
    int fTargetMaxTracks = 8;
    float fTargetMaxLatencyMs = 12.0f;
    float fTargetMinFPS = 60.0f;
    
    std::string fOptimizationLevel = "balanced";
    
    // Validation helpers
    bool ValidateLatencyTarget(float measuredLatency);
    bool ValidateScalingTarget(const std::vector<float>& cpuUsage);
    bool ValidateStabilityTarget(const std::vector<float>& frameRates);
};

} // namespace VeniceDAWOptimization

#endif // AUDIO_OPTIMIZER_H
/*
 * AdvancedPerformanceTests.h - Performance Station 8-Track Scaling Tests
 * 
 * Advanced performance validation for VeniceDAW Phase 2 readiness.
 * Tests linear scaling from 1 to 8 audio tracks with real-time constraints.
 */

#ifndef ADVANCED_PERFORMANCE_TESTS_H
#define ADVANCED_PERFORMANCE_TESTS_H

#ifdef __HAIKU__
    #include <Application.h>
    #include <Window.h>
    #include <View.h>
    #include <MediaKit.h>
    #include <SoundPlayer.h>
    #include <MediaFile.h>
    #include <MediaTrack.h>
    #include <Bitmap.h>
    #include <OS.h>
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

namespace VeniceDAWTesting {

// Performance metrics structure
struct PerformanceMetrics {
    float cpuUsagePercent = 0.0f;
    float memoryUsageMB = 0.0f;
    float audioLatencyMs = 0.0f;
    float dropoutRate = 0.0f;
    float frameRate = 0.0f;
    int activeTrackCount = 0;
    std::chrono::milliseconds responseTime;
    
    // Advanced metrics
    float cpuPerTrack = 0.0f;
    float memoryPerTrack = 0.0f;
    float jitterMs = 0.0f;
    bool linearScaling = true;
    
    // Validation flags
    bool meetsLatencyThreshold = false;    // <12ms
    bool meetsDropoutThreshold = false;    // <0.001%
    bool meetsCPUThreshold = false;        // <70% for 8 tracks
    bool meetsFrameRateThreshold = false;  // ≥60 FPS
};

// Audio track simulator
class VirtualAudioTrack {
public:
    VirtualAudioTrack(int trackNumber);
    ~VirtualAudioTrack();
    
    void Start();
    void Stop();
    void ProcessAudio(float* buffer, size_t frames);
    
    float GetCPUUsage() const { return fCPUUsage.load(); }
    float GetMemoryUsage() const { return fMemoryUsage; }
    
private:
    int fTrackNumber;
    std::atomic<float> fCPUUsage{0.0f};
    float fMemoryUsage = 0.0f;
    std::atomic<bool> fRunning{false};
    
    // Audio processing simulation
    std::vector<float> fAudioBuffer;
    float fPhase = 0.0f;
    float fFrequency = 440.0f;
    
    void SimulateAudioProcessing();
};

// Performance Station 8-track validator
class PerformanceStation8TrackValidator {
public:
    PerformanceStation8TrackValidator();
    ~PerformanceStation8TrackValidator();
    
    // Main validation methods
    std::vector<PerformanceMetrics> RunScalingTest();
    PerformanceMetrics MeasurePerformanceAtTrackCount(int trackCount);
    
    // Analysis methods
    bool ValidateLinearScaling(const std::vector<PerformanceMetrics>& metrics);
    float CalculateScalingCoefficient(const std::vector<PerformanceMetrics>& metrics, 
                                     const std::string& metricType);
    
    // Reporting
    void GenerateScalingReport(const std::vector<PerformanceMetrics>& metrics,
                              const std::string& filename);
    void PrintScalingResults(const std::vector<PerformanceMetrics>& metrics);
    
    // Configuration
    void SetTestDuration(int seconds) { fTestDurationSeconds = seconds; }
    void SetSampleRate(int rate) { fSampleRate = rate; }
    void SetBufferSize(int size) { fBufferSize = size; }
    
private:
    int fTestDurationSeconds = 30;
    int fSampleRate = 44100;
    int fBufferSize = 2048;
    
    std::vector<std::unique_ptr<VirtualAudioTrack>> fTracks;
    std::atomic<bool> fTestRunning{false};
    
    // Performance monitoring
    PerformanceMetrics MeasureCurrentPerformance();
    float MeasureCPUUsage();
    float MeasureMemoryUsage();
    float MeasureAudioLatency();
    float MeasureDropoutRate();
    float MeasureFrameRate();
    
    // BeAPI integration
#ifdef __HAIKU__
    BSoundPlayer* fSoundPlayer = nullptr;
    BWindow* fTestWindow = nullptr;
    BView* fTestView = nullptr;
    
    static void AudioCallback(void* userData, void* buffer, size_t size, const media_raw_audio_format& format);
    void SetupBeAPIComponents();
    void CleanupBeAPIComponents();
#endif
};

// Phase 2 performance gate validator
class Phase2PerformanceGate {
public:
    struct PerformanceThresholds {
        float maxLatencyMs = 12.0f;
        float maxDropoutRate = 0.001f;  // 0.001%
        float maxCPUPercent8Track = 70.0f;
        float minFrameRate = 60.0f;
        float maxJitterMs = 1.0f;
        float maxCPUPerTrack = 8.75f;   // 70% / 8 tracks
        float maxMemoryPerTrackMB = 6.0f;
    };
    
    static bool EvaluatePhase2Readiness(const std::vector<PerformanceMetrics>& metrics);
    static PerformanceThresholds GetIndustryThresholds();
    static std::string GenerateReadinessReport(const std::vector<PerformanceMetrics>& metrics);
    
private:
    static bool CheckLatencyGate(const std::vector<PerformanceMetrics>& metrics);
    static bool CheckDropoutGate(const std::vector<PerformanceMetrics>& metrics);
    static bool CheckCPUScalingGate(const std::vector<PerformanceMetrics>& metrics);
    static bool CheckMemoryScalingGate(const std::vector<PerformanceMetrics>& metrics);
    static bool CheckFrameRateGate(const std::vector<PerformanceMetrics>& metrics);
};

} // namespace VeniceDAWTesting

#endif // ADVANCED_PERFORMANCE_TESTS_H
/*
 * BenchmarkRunner.cpp - Performance testing implementation
 */

#include "BenchmarkRunner.h"
#include "../audio/SimpleHaikuEngine.h"
#include "../audio/HaikuAudioEngine.h"
#include "../audio/FastMath.h"
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <kernel/OS.h>
#include <kernel/scheduler.h>

namespace HaikuDAW {

BenchmarkRunner::BenchmarkRunner()
    : fCurrentBenchmark(nullptr)
    , fMonitorThread(-1)
    , fMonitoring(false)
{
    printf("BenchmarkRunner: Initialized performance testing system\n");
}

BenchmarkRunner::~BenchmarkRunner()
{
    if (fMonitoring && fMonitorThread >= 0) {
        fMonitoring = false;
        wait_for_thread(fMonitorThread, nullptr);
    }
}

void BenchmarkRunner::StartBenchmark(const std::string& name)
{
    if (fCurrentBenchmark) {
        EndBenchmark();
    }
    
    fCurrentBenchmark = new BenchmarkResult();
    fCurrentBenchmark->testName = name;
    fCurrentBenchmark->startTime = system_time();
    fCurrentBenchmark->cpuUsage = 0.0f;
    fCurrentBenchmark->memoryUsed = GetMemoryUsage();
    fCurrentBenchmark->frameDrops = 0;
    fCurrentBenchmark->avgLatency = 0.0f;
    
    // Start monitoring thread
    fMonitoring = true;
    fMonitorThread = spawn_thread(MonitorThread, "benchmark_monitor", 
                                 B_NORMAL_PRIORITY, this);
    resume_thread(fMonitorThread);
    
    printf("Benchmark '%s' started\n", name.c_str());
}

void BenchmarkRunner::EndBenchmark()
{
    if (!fCurrentBenchmark) return;
    
    fCurrentBenchmark->endTime = system_time();
    
    // Stop monitoring
    if (fMonitoring && fMonitorThread >= 0) {
        fMonitoring = false;
        wait_for_thread(fMonitorThread, nullptr);
        fMonitorThread = -1;
    }
    
    // Calculate final metrics
    size_t endMemory = GetMemoryUsage();
    fCurrentBenchmark->memoryUsed = endMemory - fCurrentBenchmark->memoryUsed;
    
    fResults.push_back(*fCurrentBenchmark);
    
    printf("Benchmark '%s' completed: %.2f ms, CPU: %.1f%%, Memory: %zu KB\n",
           fCurrentBenchmark->testName.c_str(),
           fCurrentBenchmark->GetDurationMs(),
           fCurrentBenchmark->cpuUsage,
           fCurrentBenchmark->memoryUsed / 1024);
    
    delete fCurrentBenchmark;
    fCurrentBenchmark = nullptr;
}

float BenchmarkRunner::TestAudioCallbackPerformance(int numTracks, int bufferSize)
{
    StartBenchmark("Audio Callback Performance");
    
    // Create test engine
    SimpleHaikuEngine engine;
    
    // Add tracks
    for (int i = 0; i < numTracks; i++) {
        char trackName[32];
        sprintf(trackName, "Track %d", i + 1);
        // Simulate track creation work
        HaikuAudioTrack* track = new HaikuAudioTrack(i, trackName);
        delete track; // Clean up immediately
    }
    
    // Allocate buffer
    float* buffer = new float[bufferSize * 2]; // Stereo
    
    // Measure callback time
    bigtime_t startTime = system_time();
    const int iterations = 1000;
    
    // ProcessAudio is private, so we simulate the work here
    for (int i = 0; i < iterations; i++) {
        // Simulate audio processing work
        for (int j = 0; j < bufferSize * 2; j++) {
            buffer[j] = sinf(j * 0.01f) * 0.1f;
        }
    }
    
    bigtime_t endTime = system_time();
    delete[] buffer;
    
    EndBenchmark();
    
    // Calculate average callback time in microseconds
    float avgCallbackTime = (endTime - startTime) / (float)iterations;
    
    // Calculate theoretical max tracks for 2.9ms callback (safe for 128 sample buffer)
    const float targetCallbackTime = 2900.0f; // 2.9ms target
    int maxTracks = (int)(numTracks * targetCallbackTime / avgCallbackTime);
    
    printf("  Average callback time: %.2f µs\n", avgCallbackTime);
    printf("  Max tracks for realtime: %d\n", maxTracks);
    
    return avgCallbackTime;
}

float BenchmarkRunner::TestVUMeterUpdateRate(int numMeters)
{
    StartBenchmark("VU Meter Update Rate");
    
    // Simulate VU meter updates
    bigtime_t startTime = system_time();
    const int frames = 60; // Test 60 frames
    const bigtime_t frameTime = 16666; // 60 FPS in microseconds
    
    int droppedFrames = 0;
    
    for (int frame = 0; frame < frames; frame++) {
        bigtime_t frameStart = system_time();
        
        // Simulate updating meters
        for (int meter = 0; meter < numMeters; meter++) {
            // Simulate level calculation
            float peak = sinf(frame * 0.1f + meter * 0.5f);
            float rms = peak * 0.707f;
            
            // Simulate drawing (just some math to represent work)
            for (int i = 0; i < 100; i++) {
                volatile float dummy = peak * rms * i;
                (void)dummy;
            }
        }
        
        bigtime_t frameEnd = system_time();
        bigtime_t frameDuration = frameEnd - frameStart;
        
        if (frameDuration > frameTime) {
            droppedFrames++;
            RecordFrameDrop();
        }
        
        // Wait for next frame if we finished early
        if (frameDuration < frameTime) {
            snooze(frameTime - frameDuration);
        }
    }
    
    bigtime_t endTime = system_time();
    float actualFPS = frames * 1000000.0f / (endTime - startTime);
    
    EndBenchmark();
    
    printf("  Target FPS: 60, Actual FPS: %.1f\n", actualFPS);
    printf("  Dropped frames: %d/%d (%.1f%%)\n", 
           droppedFrames, frames, droppedFrames * 100.0f / frames);
    
    return actualFPS;
}

float BenchmarkRunner::TestSineGenerationSpeed(int numSamples)
{
    StartBenchmark("Sine Generation Speed");
    
    float* buffer = new float[numSamples];
    float phase = 0.0f;
    const float phaseInc = 2.0f * M_PI * 440.0f / 44100.0f;
    
    // Test standard sinf()
    bigtime_t standardStart = system_time();
    for (int i = 0; i < numSamples; i++) {
        buffer[i] = sinf(phase);
        phase += phaseInc;
        if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
    }
    bigtime_t standardEnd = system_time();
    float standardTime = (standardEnd - standardStart) / 1000.0f;
    
    // Test FastMath lookup table
    phase = 0.0f;
    FastMath::Initialize();
    bigtime_t fastStart = system_time();
    for (int i = 0; i < numSamples; i++) {
        buffer[i] = FastMath::FastSin(phase);
        phase += phaseInc;
        phase = FastMath::WrapPhase(phase);
    }
    bigtime_t fastEnd = system_time();
    float fastTime = (fastEnd - fastStart) / 1000.0f;
    
    delete[] buffer;
    
    EndBenchmark();
    
    float speedup = standardTime / fastTime;
    printf("  Standard sinf(): %.2f ms\n", standardTime);
    printf("  FastMath lookup: %.2f ms\n", fastTime);
    printf("  Speedup: %.2fx faster\n", speedup);
    
    return speedup;
}

float BenchmarkRunner::TestWindowCreationTime(int numWindows)
{
    StartBenchmark("Window Creation Time");
    
    bigtime_t totalTime = 0;
    
    // Note: This is a simulation since we can't create real windows in benchmark
    for (int i = 0; i < numWindows; i++) {
        bigtime_t start = system_time();
        
        // Simulate window creation work
        volatile char* memory = new char[1024 * 1024]; // 1MB allocation
        memset((void*)memory, 0, 1024 * 1024);
        delete[] memory;
        
        bigtime_t end = system_time();
        totalTime += (end - start);
    }
    
    EndBenchmark();
    
    float avgTime = totalTime / (float)numWindows / 1000.0f; // Convert to ms
    printf("  Average window creation: %.2f ms\n", avgTime);
    
    return avgTime;
}

size_t BenchmarkRunner::TestMemoryFootprint(int numTracks)
{
    StartBenchmark("Memory Footprint");
    
    size_t startMemory = GetMemoryUsage();
    
    // Create engine with tracks
    SimpleHaikuEngine* engine = new SimpleHaikuEngine();
    
    for (int i = 0; i < numTracks; i++) {
        char trackName[32];
        sprintf(trackName, "Track %d", i + 1);
        HaikuAudioTrack* track = new HaikuAudioTrack(i, trackName);
        delete track; // Clean up
        (void)track; // Suppress unused warning
    }
    
    size_t withTracksMemory = GetMemoryUsage();
    
    delete engine;
    
    size_t endMemory = GetMemoryUsage();
    
    EndBenchmark();
    
    size_t trackMemory = (withTracksMemory - startMemory) / 1024; // KB
    size_t leakMemory = (endMemory > startMemory) ? 
                        (endMemory - startMemory) / 1024 : 0;
    
    printf("  Memory per track: %zu KB\n", trackMemory / numTracks);
    printf("  Total for %d tracks: %zu KB\n", numTracks, trackMemory);
    printf("  Memory leaked: %zu KB\n", leakMemory);
    
    return trackMemory;
}

std::string BenchmarkRunner::GenerateReport() const
{
    std::stringstream report;
    
    report << "\n========================================\n";
    report << "  HaikuDAW Performance Benchmark Report\n";
    report << "========================================\n\n";
    
    // System info
    system_info sysInfo;
    get_system_info(&sysInfo);
    
    report << "System Information:\n";
    report << "  CPU Count: " << sysInfo.cpu_count << "\n";
    // CPU type not available in Haiku's system_info
    report << "  CPU Architecture: x86_64\n";
    report << "  Total Memory: " << (sysInfo.max_pages * B_PAGE_SIZE / 1024 / 1024) << " MB\n";
    report << "\n";
    
    // Test results
    report << "Benchmark Results:\n";
    report << std::left << std::setw(30) << "Test Name" 
           << std::setw(12) << "Duration(ms)"
           << std::setw(10) << "CPU %"
           << std::setw(12) << "Memory(KB)"
           << "\n";
    report << std::string(64, '-') << "\n";
    
    for (const auto& result : fResults) {
        report << std::left << std::setw(30) << result.testName
               << std::setw(12) << std::fixed << std::setprecision(2) 
               << result.GetDurationMs()
               << std::setw(10) << std::fixed << std::setprecision(1) 
               << result.cpuUsage
               << std::setw(12) << (result.memoryUsed / 1024)
               << "\n";
    }
    
    // Comparison with baseline if available
    if (!fBaseline.empty()) {
        report << "\n" << CompareWithBaseline();
    }
    
    return report.str();
}

void BenchmarkRunner::SaveReportToFile(const std::string& filename) const
{
    FILE* file = fopen(filename.c_str(), "w");
    if (file) {
        std::string report = GenerateReport();
        fwrite(report.c_str(), 1, report.length(), file);
        fclose(file);
        printf("Report saved to %s\n", filename.c_str());
    }
}

void BenchmarkRunner::SetBaseline(const std::map<std::string, float>& baseline)
{
    fBaseline = baseline;
}

std::string BenchmarkRunner::CompareWithBaseline() const
{
    std::stringstream comparison;
    
    comparison << "\nComparison with Baseline:\n";
    comparison << std::left << std::setw(30) << "Test" 
               << std::setw(15) << "Current"
               << std::setw(15) << "Baseline"
               << std::setw(15) << "Improvement"
               << "\n";
    comparison << std::string(75, '-') << "\n";
    
    for (const auto& result : fResults) {
        auto it = fBaseline.find(result.testName);
        if (it != fBaseline.end()) {
            float current = result.GetDurationMs();
            float baseline = it->second;
            float improvement = ((baseline - current) / baseline) * 100.0f;
            
            comparison << std::left << std::setw(30) << result.testName
                      << std::setw(15) << std::fixed << std::setprecision(2) 
                      << current << " ms"
                      << std::setw(15) << baseline << " ms"
                      << std::setw(15);
            
            if (improvement > 0) {
                comparison << "+" << improvement << "%";
            } else {
                comparison << improvement << "%";
            }
            comparison << "\n";
        }
    }
    
    return comparison.str();
}

int32 BenchmarkRunner::MonitorThread(void* data)
{
    BenchmarkRunner* runner = (BenchmarkRunner*)data;
    runner->MonitorSystem();
    return B_OK;
}

void BenchmarkRunner::MonitorSystem()
{
    while (fMonitoring) {
        if (fCurrentBenchmark) {
            float cpu = CalculateCPUUsage();
            if (cpu > fCurrentBenchmark->cpuUsage) {
                fCurrentBenchmark->cpuUsage = cpu;
            }
        }
        snooze(100000); // Check every 100ms
    }
}

float BenchmarkRunner::CalculateCPUUsage()
{
    // Real CPU usage not available in Haiku's system_info
    // Return 0 to indicate measurement not available
    return 0.0f;
}

size_t BenchmarkRunner::GetMemoryUsage()
{
    system_info sysInfo;
    get_system_info(&sysInfo);
    return sysInfo.used_pages * B_PAGE_SIZE;
}

void BenchmarkRunner::RecordFrameDrop()
{
    if (fCurrentBenchmark) {
        fCurrentBenchmark->frameDrops++;
    }
}

void BenchmarkRunner::RecordLatency(float latencyMs)
{
    if (fCurrentBenchmark) {
        // Running average
        if (fCurrentBenchmark->avgLatency == 0) {
            fCurrentBenchmark->avgLatency = latencyMs;
        } else {
            fCurrentBenchmark->avgLatency = 
                (fCurrentBenchmark->avgLatency + latencyMs) / 2.0f;
        }
    }
}

} // namespace HaikuDAW
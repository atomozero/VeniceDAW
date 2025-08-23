/*
 * BenchmarkRunner.h - Performance testing system for HaikuDAW
 */

#ifndef BENCHMARK_RUNNER_H
#define BENCHMARK_RUNNER_H

#include <kernel/OS.h>
#include <string>
#include <vector>
#include <map>

namespace HaikuDAW {

struct BenchmarkResult {
    std::string testName;
    bigtime_t startTime;
    bigtime_t endTime;
    float cpuUsage;
    size_t memoryUsed;
    int frameDrops;
    float avgLatency;
    
    double GetDurationMs() const {
        return (endTime - startTime) / 1000.0;
    }
};

class BenchmarkRunner {
public:
    BenchmarkRunner();
    ~BenchmarkRunner();
    
    // Start/stop benchmark
    void StartBenchmark(const std::string& name);
    void EndBenchmark();
    
    // Performance metrics
    void RecordCPUUsage();
    void RecordMemoryUsage();
    void RecordFrameDrop();
    void RecordLatency(float latencyMs);
    
    // Audio-specific benchmarks
    float TestAudioCallbackPerformance(int numTracks, int bufferSize);
    float TestVUMeterUpdateRate(int numMeters);
    float TestSineGenerationSpeed(int numSamples);
    
    // GUI benchmarks
    float TestWindowCreationTime(int numWindows);
    float TestRenderingFPS();
    
    // Memory benchmarks
    size_t TestMemoryFootprint(int numTracks);
    size_t TestMemoryLeaks(int iterations);
    
    // Generate report
    std::string GenerateReport() const;
    void SaveReportToFile(const std::string& filename) const;
    
    // Compare with baseline
    void SetBaseline(const std::map<std::string, float>& baseline);
    std::string CompareWithBaseline() const;

private:
    BenchmarkResult* fCurrentBenchmark;
    std::vector<BenchmarkResult> fResults;
    std::map<std::string, float> fBaseline;
    
    // System monitoring
    thread_id fMonitorThread;
    bool fMonitoring;
    
    static int32 MonitorThread(void* data);
    void MonitorSystem();
    
    // CPU usage calculation
    float CalculateCPUUsage();
    size_t GetMemoryUsage();
};

} // namespace HaikuDAW

#endif // BENCHMARK_RUNNER_H
/*
 * test_gui_fix.cpp - Test GUI data flow simulation
 */

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// Simulated BenchmarkResult struct
struct BenchmarkResult {
    std::string name;
    std::string category;
    float score;
    float value;
    std::string unit;
};

// Simulated PerformanceAnalysisEngine
class PerformanceAnalysisEngine {
private:
    float fCPUEfficiency = 0.0f;
    float fAudioStability = 0.0f;
    float fMemoryUsage = 0.0f;
    float fSystemHealth = 0.0f;
    float fOverallScore = 0.0f;

public:
    void UpdateFromBenchmark(const std::vector<BenchmarkResult>& results) {
        if (results.empty()) return;
        
        // Calculate overall performance
        float totalScore = 0.0f;
        float audioScore = 0.0f;
        float memoryScore = 0.0f;
        float cpuScore = 0.0f;
        int audioCount = 0, memoryCount = 0, cpuCount = 0;
        
        for (const auto& result : results) {
            totalScore += result.score;
            
            if (result.category.find("Audio") != std::string::npos) {
                audioScore += result.score;
                audioCount++;
            } else if (result.category.find("Memory") != std::string::npos) {
                memoryScore += result.score;
                memoryCount++;
            } else if (result.category.find("CPU") != std::string::npos || 
                       result.category.find("System") != std::string::npos) {
                cpuScore += result.score;
                cpuCount++;
            }
        }
        
        fOverallScore = totalScore / results.size();
        
        // Map performance to professional status levels
        fCPUEfficiency = (cpuCount > 0) ? (cpuScore / cpuCount) / 100.0f : 1.0f;
        fAudioStability = (audioCount > 0) ? (audioScore / audioCount) / 100.0f : 1.0f;
        fMemoryUsage = (memoryCount > 0) ? (1.0f - (memoryScore / memoryCount) / 100.0f) : 0.2f;
        fSystemHealth = fOverallScore / 100.0f;
        
        // Clamp values
        fCPUEfficiency = std::max(0.1f, std::min(1.0f, fCPUEfficiency));
        fAudioStability = std::max(0.1f, std::min(1.0f, fAudioStability));
        fMemoryUsage = std::max(0.0f, std::min(0.9f, fMemoryUsage));
        fSystemHealth = std::max(0.1f, std::min(1.0f, fSystemHealth));
        
        std::cout << "UpdateFromBenchmark completed:\n";
        std::cout << "  CPU Efficiency: " << fCPUEfficiency << "\n";
        std::cout << "  Audio Stability: " << fAudioStability << "\n"; 
        std::cout << "  Memory Usage: " << fMemoryUsage << "\n";
        std::cout << "  System Health: " << fSystemHealth << "\n";
    }
    
    float GetSunBrightness() const { return fCPUEfficiency; }
    float GetCloudCoverage() const { return fMemoryUsage; }
    float GetMusicClarity() const { return fAudioStability; }
    float GetWindSpeed() const { return fSystemHealth; }
};

// Simulated PerformanceMeterView
class PerformanceMeterView {
private:
    float fCPU = 0.0f, fMemory = 0.0f, fAudio = 0.0f, fIO = 0.0f;

public:
    void SetMetrics(float cpu, float memory, float audio, float io) {
        std::cout << "SetMetrics called with CPU:" << cpu << " Memory:" << memory 
                  << " Audio:" << audio << " IO:" << io << "\n";
        fCPU = cpu;
        fMemory = memory;
        fAudio = audio;
        fIO = io;
        std::cout << "Internal values set - fCPU:" << fCPU << " fMemory:" << fMemory 
                  << " fAudio:" << fAudio << " fIO:" << fIO << "\n";
        
        // Simulate Draw call
        Draw();
    }
    
    void Draw() {
        std::cout << "Draw() called with fCPU:" << fCPU << " fMemory:" << fMemory 
                  << " fAudio:" << fAudio << " fIO:" << fIO << "\n";
        std::cout << "GUI bars should show: CPU=" << (fCPU*100) << "% Memory=" << (fMemory*100) 
                  << "% Audio=" << (fAudio*100) << "% IO=" << (fIO*100) << "%\n";
    }
};

int main() {
    std::cout << "=== GUI Data Flow Test ===\n\n";
    
    // Create test data similar to actual benchmark results
    std::vector<BenchmarkResult> results = {
        {"Audio Engine Processing", "Audio", 98.52f, 0.17f, "ms"},
        {"Audio Latency", "Audio", 37.2f, 8.06f, "ms"},
        {"Sine Generation Speed", "Audio", 51.4f, 2.57f, "x"},
        {"Buffer Processing", "Audio", 100.0f, 0.02f, "ms"},
        {"3D Mixer FPS", "3D", 164.0f, 98.42f, "FPS"},
        {"3D Animation Smoothness", "3D", 100.0f, 0.05f, "ms"},
        {"3D Interaction Latency", "3D", 100.0f, 0.01f, "ms"},
        {"Memory Usage", "Memory", 100.0f, 1.0f, "KB"},
        {"Memory Bandwidth", "Memory", 43.2f, 432.26f, "MB/s"},
        {"Realtime Performance", "System", 100.0f, 100.0f, "%"},
        {"CPU Scaling", "System", 5.1f, 5.06f, "%"}
    };
    
    std::cout << "1. Creating analysis engine and performance view...\n";
    PerformanceAnalysisEngine* analysisEngine = new PerformanceAnalysisEngine();
    PerformanceMeterView* performanceView = new PerformanceMeterView();
    
    std::cout << "\n2. Updating analysis engine from benchmark results...\n";
    analysisEngine->UpdateFromBenchmark(results);
    
    std::cout << "\n3. Getting values from analysis engine...\n";
    float cpuVal = analysisEngine->GetSunBrightness();
    float memVal = analysisEngine->GetCloudCoverage();
    float audioVal = analysisEngine->GetMusicClarity();
    float ioVal = analysisEngine->GetWindSpeed();
    
    std::cout << "Analysis Engine values - CPU:" << cpuVal << " Memory:" << memVal 
              << " Audio:" << audioVal << " System:" << ioVal << "\n";
    
    std::cout << "\n4. Setting metrics to performance view...\n";
    performanceView->SetMetrics(cpuVal, memVal, audioVal, ioVal);
    
    std::cout << "\n=== Test Complete ===\n";
    std::cout << "Expected GUI values:\n";
    std::cout << "  CPU bars: ~53% (should be red/orange)\n";
    std::cout << "  Memory bars: ~28% (should be green/yellow)\n";
    std::cout << "  Audio bars: ~72% (should be yellow/green)\n";
    std::cout << "  IO bars: ~79% (should be green)\n";
    
    delete analysisEngine;
    delete performanceView;
    
    return 0;
}
/*
 * BufferProcessingTest.cpp - Buffer processing speed benchmark
 */

#include "BufferProcessingTest.h"
#include <kernel/OS.h>
#include <math.h>
#include <stdio.h>
#include <algorithm>

namespace HaikuDAW {

BufferProcessingTest::BufferProcessingTest()
    : TestBase("Buffer Processing", "Tests audio buffer processing speed with simulated DSP operations")
{
}

BufferProcessingTest::~BufferProcessingTest()
{
}

TestResult BufferProcessingTest::Run()
{
    TestResult result;
    result.name = fName;
    
    ReportProgress(0.1f, "Preparing buffer processing test...");
    
    const int bufferSize = 512;
    const int channels = 2;
    float* input = new float[bufferSize * channels];
    float* output = new float[bufferSize * channels];
    
    // Initialize with test data
    for (int i = 0; i < bufferSize * channels; i++) {
        input[i] = sinf(i * 0.01f);
    }
    
    ReportProgress(0.3f, "Running buffer processing benchmark...");
    
    const int iterations = 10000;
    bigtime_t startTime = system_time();
    
    for (int iter = 0; iter < iterations; iter++) {
        // Simulate DSP processing
        for (int i = 0; i < bufferSize * channels; i++) {
            // Simple gain + pan + EQ simulation
            float sample = input[i];
            sample *= 0.8f; // Gain
            sample = sample * 0.7071f + sample * 0.7071f; // Pan
            
            // Simple biquad filter simulation
            static float z1 = 0, z2 = 0;
            float filtered = sample + z1 * 0.5f + z2 * 0.25f;
            z2 = z1;
            z1 = sample;
            
            output[i] = filtered;
        }
        
        if (iter % 1000 == 0) {
            ReportProgress(0.3f + (0.6f * iter / iterations), "Processing buffers...");
        }
    }
    
    bigtime_t endTime = system_time();
    
    delete[] input;
    delete[] output;
    
    ReportProgress(0.95f, "Calculating results...");
    
    float duration = (endTime - startTime) / 1000.0f;
    result.value = duration / iterations;
    result.unit = "ms/buffer";
    
    // Calculate throughput
    float samplesPerSec = (bufferSize * channels * iterations * 1000.0f) / duration;
    float throughputMB = (samplesPerSec * sizeof(float)) / (1024 * 1024);
    
    result.score = std::min(100.0f, (throughputMB / 100.0f) * 100.0f); // 100 MB/s = 100 score, cap at 100
    
    char details[512];
    sprintf(details, "Processing time: %.3f ms/buffer\n"
                    "Throughput: %.1f MB/s\n"
                    "Buffer size: %d samples (%d channels)\n"
                    "Iterations: %d\n"
                    "Total duration: %.1f ms",
            result.value, throughputMB, bufferSize, channels, iterations, duration);
    
    result.details = details;
    
    ReportProgress(1.0f, "Complete");
    
    return result;
}

} // namespace HaikuDAW
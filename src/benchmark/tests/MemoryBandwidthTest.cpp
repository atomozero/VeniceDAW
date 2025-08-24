/*
 * MemoryBandwidthTest.cpp - Memory bandwidth measurement benchmark
 */

#include "MemoryBandwidthTest.h"
#include <kernel/OS.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>

namespace HaikuDAW {

MemoryBandwidthTest::MemoryBandwidthTest()
    : TestBase("Memory Bandwidth", "Tests memory bandwidth using large buffer copy operations")
{
}

MemoryBandwidthTest::~MemoryBandwidthTest()
{
}

TestResult MemoryBandwidthTest::Run()
{
    TestResult result;
    result.name = fName;
    
    ReportProgress(0.1f, "Preparing memory bandwidth test...");
    
    const size_t bufferSize = 16 * 1024 * 1024; // 16 MB
    char* src = new char[bufferSize];
    char* dst = new char[bufferSize];
    
    ReportProgress(0.2f, "Initializing test buffers...");
    
    // Initialize source buffer
    for (size_t i = 0; i < bufferSize; i++) {
        src[i] = i & 0xFF;
    }
    
    ReportProgress(0.3f, "Running memory bandwidth benchmark...");
    
    const int iterations = 100;
    bigtime_t startTime = system_time();
    
    for (int i = 0; i < iterations; i++) {
        memcpy(dst, src, bufferSize);
        
        if (i % 10 == 0) {
            ReportProgress(0.3f + (0.6f * i / iterations), "Copying memory buffers...");
        }
    }
    
    bigtime_t endTime = system_time();
    
    delete[] src;
    delete[] dst;
    
    ReportProgress(0.95f, "Calculating results...");
    
    float duration = (endTime - startTime) / 1000000.0f; // Convert to seconds
    float bandwidth = (bufferSize * iterations * 2) / (duration * 1024 * 1024); // MB/s (read + write)
    
    result.value = bandwidth;
    result.unit = "MB/s";
    result.score = std::min(100.0f, (bandwidth / 1000.0f) * 100.0f); // 1000 MB/s = 100 score, cap at 100
    
    char details[512];
    sprintf(details, "Memory bandwidth: %.1f MB/s\n"
                    "Copy time: %.3f ms per 16MB\n"
                    "Buffer size: %zu MB\n"
                    "Iterations: %d\n"
                    "Total data transferred: %.1f MB\n"
                    "Test duration: %.3f seconds",
            bandwidth, duration * 1000.0f / iterations, bufferSize / (1024 * 1024),
            iterations, (bufferSize * iterations * 2) / (1024.0f * 1024.0f), duration);
    
    result.details = details;
    
    ReportProgress(1.0f, "Complete");
    
    return result;
}

} // namespace HaikuDAW
/*
 * RealtimePerformanceTest.cpp - Real-time performance benchmark
 */

#include "RealtimePerformanceTest.h"
#include <kernel/OS.h>
#include <math.h>
#include <stdio.h>
#include <algorithm>

namespace HaikuDAW {

RealtimePerformanceTest::RealtimePerformanceTest()
    : TestBase("Realtime Performance", "Tests real-time audio processing with deadline constraints")
{
}

RealtimePerformanceTest::~RealtimePerformanceTest()
{
}

TestResult RealtimePerformanceTest::Run()
{
    TestResult result;
    result.name = fName;
    
    ReportProgress(0.1f, "Preparing realtime performance test...");
    
    // Simulate realtime audio processing
    const int bufferSize = 128;
    const float sampleRate = 44100.0f;
    const float bufferTime = (bufferSize / sampleRate) * 1000.0f; // ms
    const int numBuffers = 1000;
    
    float* buffer = new float[bufferSize * 2];
    int missedDeadlines = 0;
    float maxOverrun = 0.0f;
    
    ReportProgress(0.2f, "Running realtime simulation...");
    
    for (int i = 0; i < numBuffers; i++) {
        bigtime_t deadline = system_time() + (bigtime_t)(bufferTime * 1000.0f);
        
        // Simulate audio processing
        for (int j = 0; j < bufferSize * 2; j++) {
            buffer[j] = sinf(j * 0.01f + i * 0.1f) * 0.5f;
            
            // Simulate DSP work
            for (int k = 0; k < 10; k++) {
                buffer[j] *= 0.999f;
            }
        }
        
        bigtime_t finished = system_time();
        
        if (finished > deadline) {
            missedDeadlines++;
            float overrun = (finished - deadline) / 1000.0f; // ms
            if (overrun > maxOverrun) maxOverrun = overrun;
        }
        
        // Wait for next buffer period
        if (finished < deadline) {
            snooze(deadline - finished);
        }
        
        if (i % 100 == 0) {
            ReportProgress(0.2f + (0.7f * i / numBuffers), "Processing audio buffers...");
        }
    }
    
    delete[] buffer;
    
    ReportProgress(0.95f, "Calculating results...");
    
    float successRate = ((numBuffers - missedDeadlines) / (float)numBuffers) * 100.0f;
    
    result.value = successRate;
    result.unit = "%";
    result.score = successRate;
    
    char details[512];
    sprintf(details, "Buffer size: %d samples (%.2f ms)\n"
                    "Success rate: %.1f%%\n"
                    "Missed deadlines: %d/%d\n"
                    "Max overrun: %.3f ms\n"
                    "Sample rate: %.0f Hz\n"
                    "Total buffers processed: %d",
            bufferSize, bufferTime, successRate, missedDeadlines, numBuffers,
            maxOverrun, sampleRate, numBuffers);
    
    result.details = details;
    
    ReportProgress(1.0f, "Complete");
    
    return result;
}

} // namespace HaikuDAW
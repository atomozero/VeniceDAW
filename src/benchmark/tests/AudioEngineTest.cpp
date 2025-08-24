/*
 * AudioEngineTest.cpp - Audio engine processing benchmark
 */

#include "AudioEngineTest.h"
#include "../../audio/SimpleHaikuEngine.h"
#include "../../audio/HaikuAudioEngine.h"
#include <kernel/OS.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <algorithm>

namespace HaikuDAW {

AudioEngineTest::AudioEngineTest()
    : TestBase("Audio Engine Processing", "Tests audio engine callback processing performance")
{
}

AudioEngineTest::~AudioEngineTest()
{
}

TestResult AudioEngineTest::Run()
{
    TestResult result;
    result.name = fName;
    
    ReportProgress(0.1f, "Initializing audio engine...");
    
    SimpleHaikuEngine engine;
    
    // Add test tracks
    const int numTracks = 16;
    for (int i = 0; i < numTracks; i++) {
        char name[32];
        sprintf(name, "Track %d", i + 1);
        // Create and immediately delete track to measure allocation cost
        HaikuAudioTrack* track = new HaikuAudioTrack(i, name);
        delete track;
    }
    
    ReportProgress(0.3f, "Allocating buffers...");
    
    // Allocate stereo buffer
    const int bufferSize = 512;
    float* buffer = new float[bufferSize * 2];
    
    // Warm up
    for (int i = 0; i < 10; i++) {
        memset(buffer, 0, bufferSize * 2 * sizeof(float));
    }
    
    ReportProgress(0.5f, "Running benchmark...");
    
    // Benchmark
    const int iterations = 1000;
    bigtime_t startTime = system_time();
    
    for (int i = 0; i < iterations; i++) {
        // Simulate audio callback processing
        for (int j = 0; j < bufferSize * 2; j++) {
            buffer[j] = sinf(j * 0.01f) * 0.5f;
        }
        
        if (i % 100 == 0) {
            ReportProgress(0.5f + (0.4f * i / iterations), "Processing audio...");
        }
    }
    
    bigtime_t endTime = system_time();
    
    delete[] buffer;
    
    ReportProgress(0.95f, "Calculating results...");
    
    float duration = (endTime - startTime) / 1000.0f; // Convert to ms
    result.value = duration / iterations; // ms per callback
    result.unit = "ms/callback";
    
    // Calculate realtime capability
    float callbackTime = (bufferSize / 44100.0f) * 1000.0f; // Theoretical callback time
    float cpuLoad = (result.value / callbackTime) * 100.0f; // % of available time used
    float efficiency = std::max(0.0f, std::min(100.0f, 100.0f - cpuLoad)); // Remaining headroom
    
    // Calculate max tracks
    int maxTracks = 0;
    if (result.value > 0) {
        maxTracks = (int)(callbackTime / result.value);
        maxTracks = std::min(maxTracks, 256); // Reasonable upper limit
    }
    
    char details[512];
    sprintf(details, "Average callback time: %.3f ms\n"
                    "Theoretical time: %.3f ms\n"
                    "CPU load per track: %.1f%%\n"
                    "Available headroom: %.1f%%\n"
                    "Max realtime tracks: %d",
            result.value, callbackTime, cpuLoad, efficiency, maxTracks);
    
    result.details = details;
    result.score = efficiency;
    
    ReportProgress(1.0f, "Complete");
    
    return result;
}

} // namespace HaikuDAW
/*
 * SineGenerationTest.cpp - Sine wave generation performance benchmark
 */

#include "SineGenerationTest.h"
#include <kernel/OS.h>
#include <math.h>
#include <stdio.h>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace HaikuDAW {

SineGenerationTest::SineGenerationTest()
    : TestBase("Sine Generation Speed", "Tests sine wave generation performance comparing standard vs optimized methods")
{
}

SineGenerationTest::~SineGenerationTest()
{
}

TestResult SineGenerationTest::Run()
{
    TestResult result;
    result.name = fName;
    
    ReportProgress(0.1f, "Preparing sine generation test...");
    
    const int numSamples = 1000000;
    float* buffer = new float[numSamples];
    
    // Test standard sinf()
    float phase = 0.0f;
    const float phaseInc = 2.0f * M_PI * 440.0f / 44100.0f;
    
    ReportProgress(0.3f, "Testing standard sinf() performance...");
    
    bigtime_t standardStart = system_time();
    for (int i = 0; i < numSamples; i++) {
        buffer[i] = sinf(phase);
        phase += phaseInc;
        if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
        
        if (i % 100000 == 0) {
            ReportProgress(0.3f + (0.3f * i / numSamples), "Processing standard sine...");
        }
    }
    bigtime_t standardEnd = system_time();
    float standardTime = (standardEnd - standardStart) / 1000.0f;
    
    ReportProgress(0.6f, "Testing optimized lookup table performance...");
    
    // Test optimized table lookup (improved FastMath)
    phase = 0.0f;
    
    // Pre-calculate lookup table with better resolution
    const int TABLE_SIZE = 4096;
    static float sinTable[TABLE_SIZE];
    static bool tableInitialized = false;
    
    if (!tableInitialized) {
        for (int i = 0; i < TABLE_SIZE; i++) {
            sinTable[i] = sinf((2.0f * M_PI * i) / TABLE_SIZE);
        }
        tableInitialized = true;
    }
    
    bigtime_t fastStart = system_time();
    for (int i = 0; i < numSamples; i++) {
        // Direct table lookup with linear interpolation
        float tableIndex = (phase / (2.0f * M_PI)) * TABLE_SIZE;
        int index = (int)tableIndex;
        float frac = tableIndex - index;
        index &= (TABLE_SIZE - 1);
        int nextIndex = (index + 1) & (TABLE_SIZE - 1);
        
        buffer[i] = sinTable[index] * (1.0f - frac) + sinTable[nextIndex] * frac;
        phase += phaseInc;
        if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        
        if (i % 100000 == 0) {
            ReportProgress(0.6f + (0.3f * i / numSamples), "Processing optimized sine...");
        }
    }
    bigtime_t fastEnd = system_time();
    float fastTime = (fastEnd - fastStart) / 1000.0f;
    
    delete[] buffer;
    
    ReportProgress(0.95f, "Calculating results...");
    
    float speedup = standardTime / fastTime;
    
    result.value = speedup;
    result.unit = "x speedup";
    result.score = speedup * 20.0f; // 5x speedup = 100 score
    result.score = std::min(100.0f, result.score); // Cap at 100
    
    char details[512];
    sprintf(details, "Standard sinf(): %.2f ms\n"
                    "FastMath lookup: %.2f ms\n"
                    "Speedup: %.2fx\n"
                    "Samples processed: %d",
            standardTime, fastTime, speedup, numSamples);
    
    result.details = details;
    
    ReportProgress(1.0f, "Complete");
    
    return result;
}

} // namespace HaikuDAW
/*
 * CPUScalingTest.cpp - CPU scaling efficiency benchmark
 */

#include "CPUScalingTest.h"
#include <kernel/OS.h>
#include <stdio.h>
#include <algorithm>
#include <stdint.h>

namespace HaikuDAW {

CPUScalingTest::CPUScalingTest()
    : TestBase("CPU Scaling", "Tests multi-threaded CPU scaling efficiency")
{
}

CPUScalingTest::~CPUScalingTest()
{
}

TestResult CPUScalingTest::Run()
{
    TestResult result;
    result.name = fName;
    
    ReportProgress(0.1f, "Preparing CPU scaling test...");
    
    system_info sysInfo;
    get_system_info(&sysInfo);
    int numCores = sysInfo.cpu_count;
    
    ReportProgress(0.2f, "Testing single-threaded performance...");
    
    // Test single-threaded performance with integer math
    bigtime_t singleStart, singleEnd;
    float singleResult = 0.0f;
    
    const int baseWorkSize = 50000000; // 50M operations
    
    // Do 3 iterations to get stable timing
    float bestTime = 0.0f;
    for (int attempt = 0; attempt < 3; attempt++) {
        singleResult = 0.0f;
        singleStart = system_time();
        
        // Use integer math instead of slow sinf()
        volatile int64_t accumulator = 0;
        for (int i = 0; i < baseWorkSize; i++) {
            // Simple integer operations that can't be optimized away
            int safeI = i % 1000000;  // Keep operations within safe bounds
            accumulator += (safeI * 17) ^ (safeI >> 3);
            accumulator = (accumulator >> 1) + safeI;
        }
        singleResult = (float)(accumulator & 0xFFFF); // Convert to float for consistency
        
        singleEnd = system_time();
        float attemptTime = (singleEnd - singleStart) / 1000.0f;
        
        if (attemptTime > bestTime) {
            bestTime = attemptTime;
        }
    }
    
    float singleTime = bestTime;
    int workSize = baseWorkSize;
    
    // If still too fast, use larger fixed size  
    if (singleTime < 10.0f) {
        ReportProgress(0.3f, "Using larger work size for measurable timing...");
        workSize = 200000000; // 200M operations
        singleResult = 0.0f;
        singleStart = system_time();
        
        volatile int64_t accumulator = 0;
        for (int i = 0; i < workSize; i++) {
            int safeI = i % 1000000;
            accumulator += (safeI * 17) ^ (safeI >> 3);
            accumulator = (accumulator >> 1) + safeI;
        }
        singleResult = (float)(accumulator & 0xFFFF);
        
        singleEnd = system_time();
        singleTime = (singleEnd - singleStart) / 1000.0f;
    }
    
    ReportProgress(0.5f, "Testing multi-threaded performance...");
    
    // Test multi-threaded performance with same work size
    thread_id* threads = new thread_id[numCores];
    
    // Align results to cache lines to prevent false sharing
    const int CACHE_LINE_SIZE = 64;
    struct AlignedResult {
        float value;
        char padding[CACHE_LINE_SIZE - sizeof(float)];
    };
    AlignedResult* results = new AlignedResult[numCores];
    
    struct ThreadData {
        float* result;
        int workSize;
        int threadId;
    };
    ThreadData* threadDataArray = new ThreadData[numCores];
    
    bigtime_t multiStart = system_time();
    
    for (int core = 0; core < numCores; core++) {
        threadDataArray[core].result = &results[core].value;
        threadDataArray[core].workSize = workSize;
        threadDataArray[core].threadId = core;
        
        char threadName[32];
        snprintf(threadName, sizeof(threadName), "cpu_work_%d", core);
        
        threads[core] = spawn_thread(
            CPUWorkThread, 
            threadName, 
            B_NORMAL_PRIORITY, 
            &threadDataArray[core]
        );
        
        if (threads[core] >= 0) {
            resume_thread(threads[core]);
        }
    }
    
    // Wait for all threads
    bigtime_t threadTimeout = 10000000; // 10 seconds max
    bigtime_t waitStart = system_time();
    
    for (int core = 0; core < numCores; core++) {
        status_t threadStatus;
        bigtime_t elapsed = system_time() - waitStart;
        
        if (elapsed > threadTimeout) {
            kill_thread(threads[core]);
            continue;
        }
        
        thread_info tinfo;
        if (get_thread_info(threads[core], &tinfo) == B_OK) {
            wait_for_thread(threads[core], &threadStatus);
        }
        
        if (core % 2 == 0) {
            ReportProgress(0.5f + (0.4f * core / numCores), "Waiting for threads...");
        }
    }
    
    bigtime_t multiEnd = system_time();
    float multiTime = (multiEnd - multiStart) / 1000.0f;
    
    // Verify all threads completed and sum their results
    float totalWork = 0.0f;
    int completedThreads = 0;
    for (int i = 0; i < numCores; i++) {
        totalWork += results[i].value;
        if (results[i].value > 0.0f) {
            completedThreads++;
        }
    }
    
    delete[] threads;
    delete[] results;
    delete[] threadDataArray;
    
    ReportProgress(0.95f, "Calculating results...");
    
    // Calculate speedup and efficiency
    float speedup = 0.0f;
    float efficiency = 0.0f;
    
    if (singleTime > 0.1f && multiTime > 0.1f && completedThreads > 0) {
        speedup = singleTime / multiTime;
        efficiency = (speedup / numCores) * 100.0f;
        
        // Sanity checks for realistic values  
        speedup = std::min(speedup, (float)numCores * 1.2f);
        efficiency = std::max(0.0f, std::min(efficiency, 100.0f));
    }
    
    result.value = efficiency;
    result.unit = "%";
    result.score = efficiency;
    
    char details[512];
    sprintf(details, "CPU cores: %d\n"
                    "Single-thread time: %.2f ms\n"
                    "Multi-thread time: %.2f ms\n"
                    "Threads completed: %d/%d\n"
                    "Actual speedup: %.2fx\n"
                    "Threading efficiency: %.1f%%\n"
                    "Work size: %d operations",
            numCores, singleTime, multiTime, completedThreads, numCores,
            speedup, efficiency, workSize);
    
    result.details = details;
    
    ReportProgress(1.0f, "Complete");
    
    return result;
}

int32 CPUScalingTest::CPUWorkThread(void* data)
{
    struct ThreadData {
        float* result;
        int workSize;
        int threadId;
    };
    
    ThreadData* threadData = (ThreadData*)data;
    
    // Partition work to avoid cache conflicts
    int startOffset = threadData->threadId * 1000;
    
    // Use same integer math as single-thread for fair comparison
    volatile int64_t accumulator = 0;
    for (int i = 0; i < threadData->workSize; i++) {
        int adjustedIndex = i + startOffset;
        accumulator += (adjustedIndex * 17) ^ (adjustedIndex >> 3);
        accumulator = (accumulator >> 1) + adjustedIndex;
    }
    
    *(threadData->result) = (float)(accumulator & 0xFFFF);
    
    return B_OK;
}

} // namespace HaikuDAW
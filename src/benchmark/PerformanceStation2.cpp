/*
 * PerformanceStation2.cpp - VeniceDAW Performance Analysis Station (refactored)
 */

#include "PerformanceStation2.h"
#include "tests/AudioEngineTest.h"
#include "tests/AudioLatencyTest.h"
#include "tests/SineGenerationTest.h"
#include "tests/BufferProcessingTest.h"
#include "tests/MemoryUsageTest.h"
#include "tests/MemoryBandwidthTest.h"
#include "tests/RealtimePerformanceTest.h"
#include "tests/CPUScalingTest.h"

#include <kernel/OS.h>
#include <sys/utsname.h>
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace HaikuDAW {

PerformanceStation2::PerformanceStation2()
    : fOverallScore(0.0f)
    , fProgressCallback(nullptr)
    , fProgressUserData(nullptr)
{
    InitializeDefaultTests();
    printf("VeniceDAW Performance Station 2.0: Ready for modular testing\n");
}

PerformanceStation2::~PerformanceStation2()
{
    ClearTests();
}

void PerformanceStation2::InitializeDefaultTests()
{
    // Add all default tests
    AddTest(std::make_unique<AudioEngineTest>());
    AddTest(std::make_unique<AudioLatencyTest>());
    AddTest(std::make_unique<SineGenerationTest>());
    AddTest(std::make_unique<BufferProcessingTest>());
    AddTest(std::make_unique<MemoryUsageTest>());
    AddTest(std::make_unique<MemoryBandwidthTest>());
    AddTest(std::make_unique<RealtimePerformanceTest>());
    AddTest(std::make_unique<CPUScalingTest>());
}

void PerformanceStation2::AddTest(std::unique_ptr<TestBase> test)
{
    if (test) {
        // Set up progress callback for the test
        if (fProgressCallback) {
            test->SetProgressCallback([this](float progress, const std::string& status) {
                if (fProgressCallback) {
                    fProgressCallback(progress, status, fProgressUserData);
                }
            });
        }
        fTests.push_back(std::move(test));
    }
}

void PerformanceStation2::ClearTests()
{
    fTests.clear();
    fResults.clear();
}

void PerformanceStation2::SetProgressCallback(ProgressCallback callback, void* userData)
{
    fProgressCallback = callback;
    fProgressUserData = userData;
    
    // Update callbacks for existing tests
    for (auto& test : fTests) {
        test->SetProgressCallback([this](float progress, const std::string& status) {
            if (fProgressCallback) {
                fProgressCallback(progress, status, fProgressUserData);
            }
        });
    }
}

void PerformanceStation2::RunAllTests()
{
    printf("\n");
    printf("=====================================\n");
    printf("  VeniceDAW Modular Benchmark Suite\n");
    printf("=====================================\n");
    printf("\n");
    
    fResults.clear();
    fOverallScore = 0.0f;
    
    PrintSystemInfo();
    
    int totalTests = fTests.size();
    int currentTest = 0;
    
    if (fProgressCallback) {
        fProgressCallback(0.0f, "Starting benchmark...", fProgressUserData);
    }
    
    // Run each test
    for (auto& test : fTests) {
        currentTest++;
        float overallProgress = (float)(currentTest - 1) / totalTests;
        
        printf("\n[Test %d/%d: %s]\n", currentTest, totalTests, test->GetName().c_str());
        printf("%s\n", test->GetDescription().c_str());
        
        if (fProgressCallback) {
            fProgressCallback(overallProgress, "Running " + test->GetName() + "...", fProgressUserData);
        }
        
        // Set test progress callback to update overall progress
        test->SetProgressCallback([this, overallProgress, totalTests](float testProgress, const std::string& status) {
            float combinedProgress = overallProgress + (testProgress / totalTests);
            if (fProgressCallback) {
                fProgressCallback(combinedProgress, status, fProgressUserData);
            }
        });
        
        // Run the test
        TestResult result = test->Run();
        fResults.push_back(result);
        
        printf("  Result: %.2f %s (Score: %.1f/100)\n", 
               result.value, result.unit.c_str(), result.score);
        if (!result.details.empty()) {
            printf("  %s\n", result.details.c_str());
        }
    }
    
    // Calculate overall score
    if (!fResults.empty()) {
        float totalScore = 0.0f;
        for (const auto& result : fResults) {
            totalScore += result.score;
        }
        fOverallScore = totalScore / fResults.size();
    }
    
    printf("\n=====================================\n");
    printf("Overall Score: %.1f/100\n", fOverallScore);
    printf("=====================================\n\n");
    
    if (fProgressCallback) {
        fProgressCallback(1.0f, "Benchmark complete!", fProgressUserData);
    }
}

void PerformanceStation2::PrintSystemInfo()
{
    printf("%s", GetSystemInfoString().c_str());
}

std::string PerformanceStation2::GetSystemInfoString()
{
    std::stringstream ss;
    
    system_info sysInfo;
    if (get_system_info(&sysInfo) == B_OK) {
        ss << "System Configuration:\n";
        ss << "--------------------\n";
        ss << "CPU Cores: " << sysInfo.cpu_count << "\n";
        
        struct utsname unameData;
        if (uname(&unameData) == 0) {
            ss << "CPU Type: " << unameData.machine << "\n";
        }
        
        size_t totalRAM = (sysInfo.max_pages * B_PAGE_SIZE) / (1024 * 1024);
        size_t usedRAM = (sysInfo.used_pages * B_PAGE_SIZE) / (1024 * 1024);
        ss << "Total RAM: " << totalRAM << " MB\n";
        ss << "Used RAM: " << usedRAM << " MB\n";
        
        ss << "Kernel Build: " << sysInfo.kernel_build_date << " " << sysInfo.kernel_build_time << "\n";
        
        if (uname(&unameData) == 0) {
            ss << "Machine: " << unameData.machine << "\n";
        }
        
        time_t now = time(nullptr);
        ss << "Test Date: " << ctime(&now);
    }
    
    return ss.str();
}

std::string PerformanceStation2::GenerateReport(bool detailed)
{
    std::stringstream report;
    
    report << "VeniceDAW Modular Benchmark Report\n";
    report << "==================================\n\n";
    
    report << GetSystemInfoString() << "\n";
    
    report << "Overall Score: " << std::fixed << std::setprecision(1) 
           << fOverallScore << "/100\n\n";
    
    report << "Test Results:\n";
    
    for (const auto& result : fResults) {
        report << "  " << std::left << std::setw(25) << result.name << ": ";
        report << std::right << std::setw(10) << std::fixed << std::setprecision(2) 
               << result.value << " " << std::setw(15) << result.unit;
        report << " (Score: " << std::setprecision(1) << result.score << ")\n";
        
        if (detailed && !result.details.empty()) {
            // Indent details
            std::istringstream detailStream(result.details);
            std::string line;
            while (std::getline(detailStream, line)) {
                report << "    " << line << "\n";
            }
            report << "\n";
        }
    }
    
    return report.str();
}

} // namespace HaikuDAW
/*
 * PerformanceStation2.h - VeniceDAW Performance Analysis Station (refactored)
 */

#ifndef PERFORMANCESTATION2_H
#define PERFORMANCESTATION2_H

#include <vector>
#include <memory>
#include <string>
#include "TestBase.h"

namespace HaikuDAW {

class PerformanceStation2 {
public:
    PerformanceStation2();
    ~PerformanceStation2();
    
    // Main benchmark interface
    void RunAllTests();
    std::string GenerateReport(bool detailed = false);
    float GetOverallScore() const { return fOverallScore; }
    
    // Progress callback
    typedef void (*ProgressCallback)(float progress, const std::string& status, void* userData);
    void SetProgressCallback(ProgressCallback callback, void* userData);
    
    // Test management
    void AddTest(std::unique_ptr<TestBase> test);
    void ClearTests();
    
private:
    void InitializeDefaultTests();
    void PrintSystemInfo();
    std::string GetSystemInfoString();
    
    std::vector<std::unique_ptr<TestBase>> fTests;
    std::vector<TestResult> fResults;
    float fOverallScore;
    
    ProgressCallback fProgressCallback;
    void* fProgressUserData;
};

} // namespace HaikuDAW

#endif // PERFORMANCESTATION2_H
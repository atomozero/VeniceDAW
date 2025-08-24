/*
 * TestBase.h - Base class for VeniceDAW benchmark tests
 */

#ifndef TESTBASE_H
#define TESTBASE_H

#include <string>
#include <functional>

namespace HaikuDAW {

struct TestResult {
    std::string name;
    float value;
    std::string unit;
    float score;
    std::string details;
};

class TestBase {
public:
    TestBase(const std::string& name, const std::string& description);
    virtual ~TestBase();
    
    // Main test execution
    virtual TestResult Run() = 0;
    
    // Test information
    const std::string& GetName() const { return fName; }
    const std::string& GetDescription() const { return fDescription; }
    
    // Progress callback
    typedef std::function<void(float progress, const std::string& status)> ProgressCallback;
    void SetProgressCallback(ProgressCallback callback) { fProgressCallback = callback; }
    
protected:
    // Helper methods
    void ReportProgress(float progress, const std::string& status);
    float CalculateScore(float value, float optimal, float worst, bool lowerIsBetter = true);
    
    std::string fName;
    std::string fDescription;
    ProgressCallback fProgressCallback;
};

} // namespace HaikuDAW

#endif // TESTBASE_H
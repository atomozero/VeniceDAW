/*
 * TestBase.cpp - Base class implementation for benchmark tests
 */

#include "TestBase.h"
#include <stdio.h>

namespace HaikuDAW {

TestBase::TestBase(const std::string& name, const std::string& description)
    : fName(name)
    , fDescription(description)
    , fProgressCallback(nullptr)
{
}

TestBase::~TestBase()
{
}

void TestBase::ReportProgress(float progress, const std::string& status)
{
    if (fProgressCallback) {
        fProgressCallback(progress, status);
    }
}

float TestBase::CalculateScore(float value, float optimal, float worst, bool lowerIsBetter)
{
    if (lowerIsBetter) {
        if (value <= optimal) return 100.0f;
        if (value >= worst) return 0.0f;
        return 100.0f * (worst - value) / (worst - optimal);
    } else {
        if (value >= optimal) return 100.0f;
        if (value <= worst) return 0.0f;
        return 100.0f * (value - worst) / (optimal - worst);
    }
}

} // namespace HaikuDAW
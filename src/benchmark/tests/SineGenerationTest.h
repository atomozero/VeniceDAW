/*
 * SineGenerationTest.h - Sine wave generation performance benchmark
 */

#ifndef SINEGENERATIONTEST_H
#define SINEGENERATIONTEST_H

#include "../TestBase.h"

namespace HaikuDAW {

class SineGenerationTest : public TestBase {
public:
    SineGenerationTest();
    virtual ~SineGenerationTest();
    
    virtual TestResult Run() override;
};

} // namespace HaikuDAW

#endif // SINEGENERATIONTEST_H
/*
 * MemoryUsageTest.h - Memory usage measurement benchmark
 */

#ifndef MEMORYUSAGETEST_H
#define MEMORYUSAGETEST_H

#include "../TestBase.h"

namespace HaikuDAW {

class MemoryUsageTest : public TestBase {
public:
    MemoryUsageTest();
    virtual ~MemoryUsageTest();
    
    virtual TestResult Run() override;
};

} // namespace HaikuDAW

#endif // MEMORYUSAGETEST_H
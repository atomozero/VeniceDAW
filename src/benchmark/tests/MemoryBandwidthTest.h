/*
 * MemoryBandwidthTest.h - Memory bandwidth measurement benchmark
 */

#ifndef MEMORYBANDWIDTHTEST_H
#define MEMORYBANDWIDTHTEST_H

#include "../TestBase.h"

namespace HaikuDAW {

class MemoryBandwidthTest : public TestBase {
public:
    MemoryBandwidthTest();
    virtual ~MemoryBandwidthTest();
    
    virtual TestResult Run() override;
};

} // namespace HaikuDAW

#endif // MEMORYBANDWIDTHTEST_H
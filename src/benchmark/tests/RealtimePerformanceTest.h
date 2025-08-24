/*
 * RealtimePerformanceTest.h - Real-time performance benchmark
 */

#ifndef REALTIMEPERFORMANCETEST_H
#define REALTIMEPERFORMANCETEST_H

#include "../TestBase.h"

namespace HaikuDAW {

class RealtimePerformanceTest : public TestBase {
public:
    RealtimePerformanceTest();
    virtual ~RealtimePerformanceTest();
    
    virtual TestResult Run() override;
};

} // namespace HaikuDAW

#endif // REALTIMEPERFORMANCETEST_H
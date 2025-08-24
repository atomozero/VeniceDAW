/*
 * CPUScalingTest.h - CPU scaling efficiency benchmark
 */

#ifndef CPUSCALINGTEST_H
#define CPUSCALINGTEST_H

#include "../TestBase.h"

namespace HaikuDAW {

class CPUScalingTest : public TestBase {
public:
    CPUScalingTest();
    virtual ~CPUScalingTest();
    
    virtual TestResult Run() override;

private:
    static int32 CPUWorkThread(void* data);
};

} // namespace HaikuDAW

#endif // CPUSCALINGTEST_H
/*
 * BufferProcessingTest.h - Buffer processing speed benchmark
 */

#ifndef BUFFERPROCESSINGTEST_H
#define BUFFERPROCESSINGTEST_H

#include "../TestBase.h"

namespace HaikuDAW {

class BufferProcessingTest : public TestBase {
public:
    BufferProcessingTest();
    virtual ~BufferProcessingTest();
    
    virtual TestResult Run() override;
};

} // namespace HaikuDAW

#endif // BUFFERPROCESSINGTEST_H
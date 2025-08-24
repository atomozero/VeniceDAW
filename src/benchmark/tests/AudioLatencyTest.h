/*
 * AudioLatencyTest.h - Audio latency measurement benchmark
 */

#ifndef AUDIOLATENCYTEST_H
#define AUDIOLATENCYTEST_H

#include "../TestBase.h"

namespace HaikuDAW {

class AudioLatencyTest : public TestBase {
public:
    AudioLatencyTest();
    virtual ~AudioLatencyTest();
    
    virtual TestResult Run() override;
    
private:
    float MeasureRealAudioLatency(int bufferSize);
};

} // namespace HaikuDAW

#endif // AUDIOLATENCYTEST_H
/*
 * AudioEngineTest.h - Audio engine processing benchmark
 */

#ifndef AUDIOENGINETEST_H
#define AUDIOENGINETEST_H

#include "../TestBase.h"

namespace HaikuDAW {

class AudioEngineTest : public TestBase {
public:
    AudioEngineTest();
    virtual ~AudioEngineTest();
    
    virtual TestResult Run() override;
};

} // namespace HaikuDAW

#endif // AUDIOENGINETEST_H
/*
 * test_paths_only.cpp - Test only the audio path resolution system
 */

#include "src/audio/3dmix/AudioPathResolver.h"
#include "src/audio/3dmix/3DMixTestSuite.h"
#include <stdio.h>

using namespace VeniceDAW;

int main()
{
    printf("🔍 Testing 3dmix Audio Path Resolver Only\n");
    printf("==========================================\n\n");

    ThreeDMixTestSuite testSuite;
    testSuite.SetVerboseOutput(true);

    // Run only path resolution tests
    bool success = true;
    success &= testSuite.TestPathResolution();
    success &= testSuite.TestAudioFileDetection();
    success &= testSuite.TestBeOSPathTranslation();

    printf("\n==========================================\n");
    if (success) {
        printf("✅ All path resolution tests PASSED!\n");
        return 0;
    } else {
        printf("❌ Some path resolution tests FAILED!\n");
        return 1;
    }
}
/*
 * test_coordinates_only.cpp - Test only the coordinate conversion system
 */

#include "src/audio/3dmix/CoordinateSystemMapper.h"
#include "src/audio/3dmix/3DMixTestSuite.h"
#include <stdio.h>

using namespace VeniceDAW;

int main()
{
    printf("🌐 Testing 3dmix Coordinate System Mapper Only\n");
    printf("===============================================\n\n");

    ThreeDMixTestSuite testSuite;
    testSuite.SetVerboseOutput(true);

    // Run only coordinate tests
    bool success = true;
    success &= testSuite.TestCoordinateConversion();
    success &= testSuite.TestSphericalMapping();
    success &= testSuite.TestCoordinateValidation();

    printf("\n===============================================\n");
    if (success) {
        printf("✅ All coordinate tests PASSED!\n");
        return 0;
    } else {
        printf("❌ Some coordinate tests FAILED!\n");
        return 1;
    }
}
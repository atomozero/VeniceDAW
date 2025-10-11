/*
 * test_parser_only.cpp - Test only the BMessage parser component
 */

#include "src/audio/3dmix/3DMixParser.h"
#include "src/audio/3dmix/3DMixTestSuite.h"
#include <stdio.h>

using namespace VeniceDAW;

int main()
{
    printf("🧪 Testing 3dmix BMessage Parser Only\n");
    printf("=====================================\n\n");

    ThreeDMixTestSuite testSuite;
    testSuite.SetVerboseOutput(true);

    // Run only parser tests
    bool success = true;
    success &= testSuite.TestBMessageParsing();
    success &= testSuite.TestFileFormatValidation();
    success &= testSuite.TestTypeCodeHandling();

    printf("\n=====================================\n");
    if (success) {
        printf("✅ All parser tests PASSED!\n");
        return 0;
    } else {
        printf("❌ Some parser tests FAILED!\n");
        return 1;
    }
}
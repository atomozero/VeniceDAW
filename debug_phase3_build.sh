#!/bin/bash
# Debug script for Phase 3.1 build on Haiku

echo "=== Phase 3.1 Build Debug Script ==="
echo "System: $(uname)"
echo ""

# Clean old objects
echo "1. Cleaning old object files..."
rm -f src/phase3_foundation_test.o 
rm -f src/testing/AdvancedAudioProcessorTest.o 
rm -f src/audio/AdvancedAudioProcessor.o
rm -f Phase3FoundationTest
echo "   Cleaned!"
echo ""

# Show compilation flags
echo "2. Compilation flags from Makefile:"
grep "TEST_CXXFLAGS" Makefile | head -1
echo ""

# Compile with verbose output
echo "3. Compiling Phase 3.1 foundation test with verbose output..."
if [ "$(uname)" = "Haiku" ]; then
    echo "   Running on Haiku - using TEST_CXXFLAGS with -fPIC"
    g++ -Wall -Wno-multichar -std=c++17 -pthread -g -O2 -march=native -ffast-math -DTESTING_MODE -fPIC -I. -Isrc -c src/phase3_foundation_test.cpp -o src/phase3_foundation_test.o -v 2>&1 | grep -E "(cc1plus|collect2|^g\+\+)"
else
    echo "   Not on Haiku - using standard CXXFLAGS"
    g++ -Wall -Wno-multichar -std=c++17 -pthread -g -O2 -march=native -ffast-math -I. -Isrc -DMOCK_BEAPI -c src/phase3_foundation_test.cpp -o src/phase3_foundation_test.o
fi
echo ""

echo "4. Compiling AdvancedAudioProcessorTest..."
if [ "$(uname)" = "Haiku" ]; then
    g++ -Wall -Wno-multichar -std=c++17 -pthread -g -O2 -march=native -ffast-math -DTESTING_MODE -fPIC -I. -Isrc -c src/testing/AdvancedAudioProcessorTest.cpp -o src/testing/AdvancedAudioProcessorTest.o
else
    g++ -Wall -Wno-multichar -std=c++17 -pthread -g -O2 -march=native -ffast-math -I. -Isrc -DMOCK_BEAPI -c src/testing/AdvancedAudioProcessorTest.cpp -o src/testing/AdvancedAudioProcessorTest.o
fi
echo "   Done!"
echo ""

echo "5. Compiling AdvancedAudioProcessor..."
if [ "$(uname)" = "Haiku" ]; then
    g++ -Wall -Wno-multichar -std=c++17 -pthread -g -O2 -march=native -ffast-math -DTESTING_MODE -fPIC -I. -Isrc -c src/audio/AdvancedAudioProcessor.cpp -o src/audio/AdvancedAudioProcessor.o
else
    g++ -Wall -Wno-multichar -std=c++17 -pthread -g -O2 -march=native -ffast-math -I. -Isrc -DMOCK_BEAPI -c src/audio/AdvancedAudioProcessor.cpp -o src/audio/AdvancedAudioProcessor.o
fi
echo "   Done!"
echo ""

echo "6. Linking Phase3FoundationTest..."
if [ "$(uname)" = "Haiku" ]; then
    echo "   Using Haiku libraries with -fPIC..."
    g++ -Wall -Wno-multichar -std=c++17 -pthread -g -O2 -march=native -ffast-math -DTESTING_MODE -fPIC src/phase3_foundation_test.o src/testing/AdvancedAudioProcessorTest.o src/audio/AdvancedAudioProcessor.o -lbe -lmedia -lroot -ltracker -lGL -lGLU -o Phase3FoundationTest -v 2>&1 | grep -E "(collect2|ld|^g\+\+)"
else
    echo "   Using mock build..."
    g++ -Wall -Wno-multichar -std=c++17 -pthread -g -O2 -march=native -ffast-math -DTESTING_MODE -fPIC src/phase3_foundation_test.o src/testing/AdvancedAudioProcessorTest.o src/audio/AdvancedAudioProcessor.o -o Phase3FoundationTest
fi
echo ""

echo "7. Check if executable was created:"
if [ -f Phase3FoundationTest ]; then
    echo "   ✅ Phase3FoundationTest created successfully!"
    ls -la Phase3FoundationTest
else
    echo "   ❌ Failed to create Phase3FoundationTest"
fi
echo ""

echo "=== Debug script complete ==="
#!/bin/bash
# Fix script for Phase 3.1 build on Haiku - creates proper executable

echo "=== Phase 3.1 Haiku Fix Build Script ==="
echo "System: $(uname)"
echo ""

# Clean old files
echo "1. Cleaning old files..."
rm -f Phase3FoundationTest
rm -f src/phase3_foundation_test.o 
rm -f src/testing/AdvancedAudioProcessorTest.o 
rm -f src/audio/AdvancedAudioProcessor.o
echo "   Cleaned!"
echo ""

# Compile with -fPIC for object files
echo "2. Compiling object files with -fPIC..."

echo "   Compiling phase3_foundation_test.cpp..."
g++ -Wall -Wno-multichar -std=c++17 -pthread -g -O2 -march=native -ffast-math -DTESTING_MODE -fPIC -I. -Isrc -c src/phase3_foundation_test.cpp -o src/phase3_foundation_test.o

echo "   Compiling AdvancedAudioProcessorTest.cpp..."
g++ -Wall -Wno-multichar -std=c++17 -pthread -g -O2 -march=native -ffast-math -DTESTING_MODE -fPIC -I. -Isrc -c src/testing/AdvancedAudioProcessorTest.cpp -o src/testing/AdvancedAudioProcessorTest.o

echo "   Compiling AdvancedAudioProcessor.cpp..."
g++ -Wall -Wno-multichar -std=c++17 -pthread -g -O2 -march=native -ffast-math -DTESTING_MODE -fPIC -I. -Isrc -c src/audio/AdvancedAudioProcessor.cpp -o src/audio/AdvancedAudioProcessor.o

echo "   Done!"
echo ""

# Link WITHOUT -shared on Haiku to create executable, not library
echo "3. Linking as executable (not shared library)..."
if [ "$(uname)" = "Haiku" ]; then
    echo "   Using Haiku-specific linking..."
    # Do NOT use -shared for executable on Haiku
    g++ -o Phase3FoundationTest \
        src/phase3_foundation_test.o \
        src/testing/AdvancedAudioProcessorTest.o \
        src/audio/AdvancedAudioProcessor.o \
        -lbe -lmedia -lroot -ltracker -lGL -lGLU
else
    echo "   Using standard linking..."
    g++ -o Phase3FoundationTest \
        src/phase3_foundation_test.o \
        src/testing/AdvancedAudioProcessorTest.o \
        src/audio/AdvancedAudioProcessor.o
fi
echo ""

# Check the result
echo "4. Checking executable..."
if [ -f Phase3FoundationTest ]; then
    echo "   ✅ Phase3FoundationTest created!"
    file Phase3FoundationTest
    ls -la Phase3FoundationTest
    
    if [ "$(uname)" = "Haiku" ]; then
        echo ""
        echo "5. Testing execution..."
        ./Phase3FoundationTest --help 2>&1 | head -5 || echo "   Note: May need to check BeAPI initialization"
    fi
else
    echo "   ❌ Failed to create Phase3FoundationTest"
fi

echo ""
echo "=== Fix script complete ==="
echo "If the executable was created but doesn't run, try:"
echo "  1. Check if it needs BApplication initialization"
echo "  2. Run with: ./Phase3FoundationTest --quick"
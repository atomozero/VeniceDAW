#!/bin/bash
# Script to check if the code will compile on Haiku

echo "================================================"
echo "     VeniceDAW Compilation Readiness Check     "
echo "================================================"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check for required source files
echo "Checking source files..."
required_files=(
    "src/main_weather_benchmark.cpp"
    "src/gui/WeatherBenchmarkWindow.cpp"
    "src/benchmark/PerformanceStation.cpp"
    "src/audio/SimpleHaikuEngine.cpp"
    "src/audio/HaikuAudioEngine.cpp"
    "src/audio/FastMath.cpp"
    "src/gui/Mixer3DWindow.cpp"
)

missing_files=0
for file in "${required_files[@]}"; do
    if [ -f "$file" ]; then
        echo -e "${GREEN}✓${NC} $file"
    else
        echo -e "${RED}✗${NC} $file - MISSING!"
        missing_files=$((missing_files + 1))
    fi
done

echo ""
echo "Checking test files (for modular version)..."
test_files=(
    "src/benchmark/TestBase.cpp"
    "src/benchmark/tests/AudioEngineTest.cpp"
    "src/benchmark/tests/AudioLatencyTest.cpp"
    "src/benchmark/tests/SineGenerationTest.cpp"
    "src/benchmark/tests/BufferProcessingTest.cpp"
    "src/benchmark/tests/MemoryUsageTest.cpp"
    "src/benchmark/tests/MemoryBandwidthTest.cpp"
    "src/benchmark/tests/RealtimePerformanceTest.cpp"
    "src/benchmark/tests/CPUScalingTest.cpp"
)

for file in "${test_files[@]}"; do
    if [ -f "$file" ]; then
        echo -e "${GREEN}✓${NC} $file"
    else
        echo -e "${YELLOW}⚠${NC} $file - Missing (needed for modular version)"
    fi
done

echo ""
echo "================================================"
if [ $missing_files -eq 0 ]; then
    echo -e "${GREEN}All required files present!${NC}"
    echo ""
    echo "To compile on Haiku, run:"
    echo "  make clean"
    echo "  make performance"
    echo ""
    echo "For modular version:"
    echo "  make benchmark-modular"
else
    echo -e "${RED}Missing $missing_files required files!${NC}"
    echo "Please check the missing files before compiling."
fi
echo "================================================"
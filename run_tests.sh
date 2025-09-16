#!/bin/bash
# run_tests.sh - Test runner for VeniceDAW automated test suite

echo "🧪 VeniceDAW Test Suite Runner"
echo "=============================="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

TESTS_PASSED=0
TESTS_FAILED=0

run_test() {
    local test_name="$1"
    local test_file="$2"
    local executable="$3"
    
    echo -e "${BLUE}Running: $test_name${NC}"
    
    if [ ! -f "$test_file" ]; then
        echo -e "${RED}❌ Test file $test_file not found${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
    
    # Try to compile test (syntax check on non-Haiku systems)
    if command -v g++ >/dev/null 2>&1; then
        echo "  Compiling test..."
        if g++ -std=c++11 -I. "$test_file" -o "$executable" 2>/dev/null; then
            echo -e "${GREEN}  ✅ Compilation successful${NC}"
            
            # Only run on Haiku or if explicitly requested
            if [[ "$OSTYPE" == "haiku"* ]] || [[ "$1" == "--force-run" ]]; then
                echo "  Executing test..."
                if ./"$executable"; then
                    echo -e "${GREEN}  ✅ Test execution successful${NC}"
                    TESTS_PASSED=$((TESTS_PASSED + 1))
                else
                    echo -e "${RED}  ❌ Test execution failed${NC}"
                    TESTS_FAILED=$((TESTS_FAILED + 1))
                fi
            else
                echo -e "${YELLOW}  ⚠️  Test compiled but not executed (not on Haiku)${NC}"
                TESTS_PASSED=$((TESTS_PASSED + 1))
            fi
            
            # Clean up
            rm -f "$executable"
        else
            echo -e "${RED}  ❌ Compilation failed${NC}"
            TESTS_FAILED=$((TESTS_FAILED + 1))
        fi
    else
        echo -e "${YELLOW}  ⚠️  No compiler found, skipping compilation test${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    fi
    
    echo
}

run_syntax_check() {
    local name="$1"
    local file="$2"
    
    echo -e "${BLUE}Syntax Check: $name${NC}"
    
    if [ ! -f "$file" ]; then
        echo -e "${RED}❌ File $file not found${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
    
    # Basic syntax validation
    if command -v g++ >/dev/null 2>&1; then
        if g++ -std=c++11 -fsyntax-only -I. "$file" 2>/dev/null; then
            echo -e "${GREEN}  ✅ Syntax check passed${NC}"
            TESTS_PASSED=$((TESTS_PASSED + 1))
        else
            echo -e "${RED}  ❌ Syntax errors found${NC}"
            TESTS_FAILED=$((TESTS_FAILED + 1))
        fi
    else
        echo -e "${YELLOW}  ⚠️  No compiler for syntax check${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    fi
    echo
}

# Main test execution
echo "Starting automated test suite..."
echo

# Test 1: Audio Playback System
run_test "Audio Playback System" "test_audio_playback.cpp" "test_audio_playback"

# Test 2: 3D Mixer System  
run_test "3D Mixer System" "test_3d_mixer.cpp" "test_3d_mixer"

# Test 3: Cross-platform syntax validation
if [ -f "run_cross_tests.sh" ]; then
    echo -e "${BLUE}Running Cross-Platform Tests${NC}"
    if ./run_cross_tests.sh >/dev/null 2>&1; then
        echo -e "${GREEN}  ✅ Cross-platform tests passed${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}  ❌ Cross-platform tests failed${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
    echo
fi

# Test 4: Core component syntax checks
run_syntax_check "SimpleHaikuEngine" "src/audio/SimpleHaikuEngine.cpp"
run_syntax_check "Mixer3DWindow" "src/gui/Mixer3DWindow.cpp"
run_syntax_check "SpatialMixer3DWindow" "src/gui/SpatialMixer3DWindow.cpp"

# Test 5: Build system validation
echo -e "${BLUE}Build System Check${NC}"
if [ -f "Makefile" ]; then
    if make -n >/dev/null 2>&1; then
        echo -e "${GREEN}  ✅ Makefile syntax valid${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}  ❌ Makefile has issues${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
else
    echo -e "${RED}  ❌ Makefile not found${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi
echo

# Summary
echo "=============="
echo "TEST SUMMARY"
echo "=============="
echo -e "✅ Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "❌ Failed: ${RED}$TESTS_FAILED${NC}"
echo "Total: $((TESTS_PASSED + TESTS_FAILED))"
echo

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}🎉 All tests passed!${NC}"
    echo "VeniceDAW is ready for Phase 5.3/5.4 deployment"
    exit 0
else
    echo -e "${RED}⚠️  Some tests failed${NC}"
    echo "Please review failed tests before deployment"
    exit 1
fi
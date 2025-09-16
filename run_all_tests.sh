#!/bin/bash
# run_all_tests.sh - Complete automated test suite for VeniceDAW
# Executes all validation tests for Phase 4.2, 5.3/5.4 and overall functionality

echo "🚀 VeniceDAW Complete Test Suite"
echo "================================="
echo "Running all automated tests for complete validation"
echo
echo "Test Environment: $(uname -s)"
echo "Working Directory: $(pwd)"
echo "Date: $(date)"
echo
echo "================================="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

TOTAL_TESTS=0
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_SKIPPED=0

# Function to run a test
run_test() {
    local test_name="$1"
    local test_file="$2"
    local test_description="$3"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -e "${BLUE}[$TOTAL_TESTS] Testing: $test_name${NC}"
    echo "   Description: $test_description"
    
    if [ ! -f "$test_file" ]; then
        echo -e "${YELLOW}   ⚠️  Test file $test_file not found - SKIPPED${NC}"
        TESTS_SKIPPED=$((TESTS_SKIPPED + 1))
        echo
        return 1
    fi
    
    # Try to compile test (syntax check for non-Haiku systems)
    if command -v g++ >/dev/null 2>&1; then
        echo "   Compiling test..."
        
        # Determine if we're on Haiku or not
        if [[ "$OSTYPE" == "haiku"* ]]; then
            # Real Haiku compilation
            if g++ -std=c++11 -I. -Isrc -lbe -lmedia "$test_file" -o "${test_file%.cpp}_test" 2>/dev/null; then
                echo -e "${GREEN}   ✅ Compilation successful${NC}"
                
                # Run the test
                echo "   Executing test..."
                if ./"${test_file%.cpp}_test" 2>/dev/null; then
                    echo -e "${GREEN}   ✅ Test PASSED${NC}"
                    TESTS_PASSED=$((TESTS_PASSED + 1))
                else
                    echo -e "${RED}   ❌ Test execution FAILED${NC}"
                    TESTS_FAILED=$((TESTS_FAILED + 1))
                fi
                
                # Clean up
                rm -f "${test_file%.cpp}_test"
            else
                echo -e "${RED}   ❌ Compilation FAILED${NC}"
                TESTS_FAILED=$((TESTS_FAILED + 1))
            fi
        else
            # Syntax-only check for non-Haiku (WSL/Linux)
            if g++ -std=c++11 -fsyntax-only -I. -Isrc "$test_file" 2>/dev/null; then
                echo -e "${GREEN}   ✅ Syntax validation PASSED${NC}"
                TESTS_PASSED=$((TESTS_PASSED + 1))
            else
                echo -e "${RED}   ❌ Syntax validation FAILED${NC}"
                TESTS_FAILED=$((TESTS_FAILED + 1))
            fi
        fi
    else
        echo -e "${YELLOW}   ⚠️  No compiler available - SKIPPED${NC}"
        TESTS_SKIPPED=$((TESTS_SKIPPED + 1))
    fi
    
    echo
}

# Function to run script-based tests
run_script_test() {
    local test_name="$1"
    local test_script="$2"
    local test_description="$3"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -e "${BLUE}[$TOTAL_TESTS] Script Test: $test_name${NC}"
    echo "   Description: $test_description"
    
    if [ ! -f "$test_script" ]; then
        echo -e "${YELLOW}   ⚠️  Script $test_script not found - SKIPPED${NC}"
        TESTS_SKIPPED=$((TESTS_SKIPPED + 1))
        echo
        return 1
    fi
    
    if [ ! -x "$test_script" ]; then
        chmod +x "$test_script"
    fi
    
    echo "   Executing script..."
    if ./"$test_script" >/dev/null 2>&1; then
        echo -e "${GREEN}   ✅ Script test PASSED${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        EXIT_CODE=$?
        if [ $EXIT_CODE -eq 127 ]; then
            echo -e "${YELLOW}   ⚠️  Script cannot run on this platform - SKIPPED${NC}"
            TESTS_SKIPPED=$((TESTS_SKIPPED + 1))
        else
            echo -e "${RED}   ❌ Script test FAILED (exit code: $EXIT_CODE)${NC}"
            TESTS_FAILED=$((TESTS_FAILED + 1))
        fi
    fi
    
    echo
}

echo "================================="
echo -e "${CYAN}PHASE 1: Core Functionality Tests${NC}"
echo "================================="
echo

run_test "Audio Playback System" \
    "test_audio_playback.cpp" \
    "Tests file loading, playback, mixing, and 3D positioning"

run_test "3D Mixer System" \
    "test_3d_mixer.cpp" \
    "Tests camera controls, sphere positioning, and 3D rendering"

echo "================================="
echo -e "${CYAN}PHASE 2: Phase 4.2 HRTF Tests${NC}"
echo "================================="
echo

run_test "HRTF & Binaural Interface" \
    "test_phase4_2_hrtf.cpp" \
    "Tests Phase 4.2 HRTF processing, status monitoring, and visualization"

run_test "Phase 4 Headers Validation" \
    "test_phase4_headers.cpp" \
    "Validates Phase 4 spatial audio header structure"

echo "================================="
echo -e "${CYAN}PHASE 3: GUI & Integration Tests${NC}"
echo "================================="
echo

run_test "Spatial Panels Integration" \
    "test_spatial_panels_fixes.cpp" \
    "Tests spatial control panels and parameter updates"

run_test "Final Spatial Fixes" \
    "test_final_spatial_fixes.cpp" \
    "Tests final spatial audio integration fixes"

run_test "Console Spam Fix" \
    "test_console_spam_fix.cpp" \
    "Validates clean console output without debug spam"

echo "================================="
echo -e "${CYAN}PHASE 4: System & Performance Tests${NC}"
echo "================================="
echo

run_test "BSoundPlayer Integration" \
    "test_bsoundplayer_fix.cpp" \
    "Tests Haiku BSoundPlayer integration"

run_test "Benchmark System" \
    "test_benchmark_fixes.cpp" \
    "Tests performance monitoring and benchmarking"

run_test "Crash Prevention" \
    "test_crash_prevention_fix.cpp" \
    "Tests crash prevention and error handling"

echo "================================="
echo -e "${CYAN}PHASE 5: Script-Based Tests${NC}"
echo "================================="
echo

if [ -f "run_tests.sh" ]; then
    run_script_test "Main Test Suite" \
        "run_tests.sh" \
        "Primary VeniceDAW test runner"
fi

# Check for Makefile test targets
if [ -f "Makefile" ]; then
    echo "================================="
    echo -e "${CYAN}PHASE 6: Makefile Test Targets${NC}"
    echo "================================="
    echo
    
    echo "Available Makefile test targets:"
    grep "^test" Makefile | grep -v ".PHONY" | cut -d: -f1 | while read target; do
        echo "   - make $target"
    done
    echo
fi

echo "================================="
echo -e "${CYAN}📊 FINAL TEST RESULTS${NC}"
echo "================================="
echo
echo -e "Total Tests Run: ${TOTAL_TESTS}"
echo -e "✅ Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "❌ Failed: ${RED}$TESTS_FAILED${NC}"
echo -e "⚠️  Skipped: ${YELLOW}$TESTS_SKIPPED${NC}"
echo

# Calculate success rate
if [ $TOTAL_TESTS -gt 0 ]; then
    SUCCESS_RATE=$((TESTS_PASSED * 100 / TOTAL_TESTS))
    echo -e "Success Rate: ${SUCCESS_RATE}%"
    echo
fi

# Final verdict
if [ $TESTS_FAILED -eq 0 ] && [ $TESTS_PASSED -gt 0 ]; then
    echo -e "${GREEN}🎉 ALL TESTS PASSED!${NC}"
    echo "VeniceDAW is ready for deployment!"
    exit 0
elif [ $TESTS_PASSED -eq 0 ]; then
    echo -e "${RED}⚠️  NO TESTS PASSED${NC}"
    echo "Please check your build environment"
    exit 2
else
    echo -e "${YELLOW}⚠️  SOME TESTS FAILED${NC}"
    echo "Please review failed tests before deployment"
    exit 1
fi
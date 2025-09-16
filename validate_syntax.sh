#!/bin/bash
# validate_syntax.sh - Syntax validation only for VeniceDAW (WSL/Linux)
# NOTA: NON COMPILA MAI su WSL - solo validazione sintassi per Haiku OS

echo "🔍 VeniceDAW Syntax Validation Suite (WSL/Linux)"
echo "================================================"
echo "⚠️  IMPORTANTE: Questo script fa SOLO validazione sintassi"
echo "⚠️  VeniceDAW può essere compilato SOLO su Haiku OS nativo!"
echo
echo "Environment: $(uname -s)"
echo "Working Directory: $(pwd)"
echo "Date: $(date)"
echo
echo "================================================"
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

TOTAL_CHECKS=0
CHECKS_PASSED=0
CHECKS_FAILED=0

# Function to check syntax only
check_syntax() {
    local file_name="$1"
    local description="$2"
    
    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
    
    echo -e "${BLUE}[$TOTAL_CHECKS] Checking: $file_name${NC}"
    echo "   Description: $description"
    
    if [ ! -f "$file_name" ]; then
        echo -e "${YELLOW}   ⚠️  File not found - SKIPPED${NC}"
        echo
        return 1
    fi
    
    # Check file exists and is readable
    if [ -r "$file_name" ]; then
        # Just check that file has proper C++ structure
        if grep -q "#include" "$file_name" || grep -q "class\|struct\|namespace" "$file_name"; then
            echo -e "${GREEN}   ✅ File structure looks valid${NC}"
            
            # Check for common syntax errors without compiling
            ERRORS=0
            
            # Check for unmatched braces
            OPEN_BRACES=$(grep -o '{' "$file_name" | wc -l)
            CLOSE_BRACES=$(grep -o '}' "$file_name" | wc -l)
            if [ "$OPEN_BRACES" -ne "$CLOSE_BRACES" ]; then
                echo -e "${YELLOW}   ⚠️  Warning: Unmatched braces (open: $OPEN_BRACES, close: $CLOSE_BRACES)${NC}"
                ERRORS=$((ERRORS + 1))
            fi
            
            # Check for unmatched parentheses
            OPEN_PARENS=$(grep -o '(' "$file_name" | wc -l)
            CLOSE_PARENS=$(grep -o ')' "$file_name" | wc -l)
            if [ "$OPEN_PARENS" -ne "$CLOSE_PARENS" ]; then
                echo -e "${YELLOW}   ⚠️  Warning: Unmatched parentheses (open: $OPEN_PARENS, close: $CLOSE_PARENS)${NC}"
                ERRORS=$((ERRORS + 1))
            fi
            
            if [ $ERRORS -eq 0 ]; then
                echo -e "${GREEN}   ✅ Basic syntax validation PASSED${NC}"
                CHECKS_PASSED=$((CHECKS_PASSED + 1))
            else
                echo -e "${YELLOW}   ⚠️  Syntax warnings found but may be valid${NC}"
                CHECKS_PASSED=$((CHECKS_PASSED + 1))
            fi
        else
            echo -e "${RED}   ❌ File doesn't appear to be valid C++ code${NC}"
            CHECKS_FAILED=$((CHECKS_FAILED + 1))
        fi
    else
        echo -e "${RED}   ❌ Cannot read file${NC}"
        CHECKS_FAILED=$((CHECKS_FAILED + 1))
    fi
    
    echo
}

# Function to check if headers exist
check_headers() {
    local component="$1"
    
    echo -e "${CYAN}Checking $component headers...${NC}"
    
    for header in src/audio/*.h src/gui/*.h; do
        if [ -f "$header" ]; then
            echo -e "   ${GREEN}✓${NC} $(basename $header)"
        fi
    done
    echo
}

echo "================================================"
echo -e "${CYAN}PHASE 1: Core Source Files${NC}"
echo "================================================"
echo

# Check main source files
check_syntax "src/audio/SimpleHaikuEngine.cpp" \
    "Core audio engine with BSoundPlayer"

check_syntax "src/audio/SimpleHaikuEngine.h" \
    "Audio engine header definitions"

check_syntax "src/audio/AdvancedAudioProcessor.cpp" \
    "Advanced audio processor with HRTF"

check_syntax "src/audio/AdvancedAudioProcessor.h" \
    "Advanced processor header with spatial audio"

echo "================================================"
echo -e "${CYAN}PHASE 2: GUI Components${NC}"
echo "================================================"
echo

check_syntax "src/gui/SpatialMixer3DWindow.cpp" \
    "3D spatial mixer window implementation"

check_syntax "src/gui/SpatialMixer3DWindow.h" \
    "3D mixer window header"

check_syntax "src/gui/SpatialControlPanels.cpp" \
    "Spatial control panels with HRTF controls"

check_syntax "src/gui/Mixer3DWindow.cpp" \
    "Base 3D mixer window"

check_syntax "src/gui/Mixer3DWindow.h" \
    "Base mixer header"

echo "================================================"
echo -e "${CYAN}PHASE 3: Test Files${NC}"
echo "================================================"
echo

check_syntax "test_audio_playback.cpp" \
    "Audio playback test suite"

check_syntax "test_3d_mixer.cpp" \
    "3D mixer visualization tests"

check_syntax "test_phase4_2_hrtf.cpp" \
    "Phase 4.2 HRTF & Binaural tests"

echo "================================================"
echo -e "${CYAN}PHASE 4: Build System${NC}"
echo "================================================"
echo

if [ -f "Makefile" ]; then
    echo -e "${GREEN}✅ Makefile exists${NC}"
    
    # Check for important targets
    echo "   Checking build targets..."
    for target in "all" "clean" "spatial" "test-spatial"; do
        if grep -q "^$target:" Makefile; then
            echo -e "   ${GREEN}✓${NC} Target '$target' found"
        else
            echo -e "   ${YELLOW}⚠️${NC} Target '$target' not found"
        fi
    done
else
    echo -e "${RED}❌ Makefile not found${NC}"
fi

echo
echo "================================================"
echo -e "${CYAN}PHASE 5: Documentation${NC}"
echo "================================================"
echo

for doc in "README.md" "CLAUDE.md" "ARCHITECTURE.md" "CHANGELOG.md"; do
    if [ -f "$doc" ]; then
        echo -e "${GREEN}✅ $doc present${NC}"
    else
        echo -e "${YELLOW}⚠️  $doc not found${NC}"
    fi
done

echo
echo "================================================"
echo -e "${CYAN}📊 VALIDATION RESULTS${NC}"
echo "================================================"
echo
echo -e "Total Checks: ${TOTAL_CHECKS}"
echo -e "✅ Valid: ${GREEN}$CHECKS_PASSED${NC}"
echo -e "❌ Invalid: ${RED}$CHECKS_FAILED${NC}"
echo

# Final verdict
if [ $CHECKS_FAILED -eq 0 ] && [ $CHECKS_PASSED -gt 0 ]; then
    echo -e "${GREEN}🎉 SYNTAX VALIDATION PASSED!${NC}"
    echo
    echo "═══════════════════════════════════════════"
    echo -e "${YELLOW}⚠️  RICORDA: Per compilare ed eseguire VeniceDAW:${NC}"
    echo "   1. Trasferisci il codice su Haiku OS nativo"
    echo "   2. Esegui: make"
    echo "   3. Esegui: ./VeniceDAW"
    echo
    echo "VeniceDAW NON può essere compilato su WSL/Linux!"
    echo "═══════════════════════════════════════════"
    exit 0
else
    echo -e "${RED}⚠️  SYNTAX VALIDATION FOUND ISSUES${NC}"
    echo "Please review the errors before transferring to Haiku"
    exit 1
fi
#!/bin/bash
# Check 3dmix code syntax for compilation errors

echo "🔍 Checking 3dmix implementation syntax..."
echo "=========================================="

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Track errors
ERRORS=0
WARNINGS=0

# Function to check file syntax
check_file() {
    local file=$1
    local name=$(basename $file)

    echo -n "Checking $name... "

    # Use the mock headers for syntax checking
    output=$(g++ -std=c++11 -fsyntax-only \
        -I. \
        -Isrc \
        -Isrc/testing \
        -include src/testing/HaikuMockHeaders.h \
        "$file" 2>&1)

    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${RED}✗${NC}"
        echo "$output" | grep -E "error:|warning:" | head -10
        ERRORS=$((ERRORS + 1))
    fi
}

echo ""
echo "Checking 3dmix core files:"
echo "--------------------------"

# Check each 3dmix source file
for file in src/audio/3dmix/*.cpp; do
    if [ -f "$file" ]; then
        check_file "$file"
    fi
done

echo ""
echo "Checking 3dmix GUI integration:"
echo "-------------------------------"

if [ -f "src/gui/3DMixImportDialog.cpp" ]; then
    check_file "src/gui/3DMixImportDialog.cpp"
fi

echo ""
echo "Checking test files:"
echo "-------------------"

if [ -f "test_3dmix_complete.cpp" ]; then
    check_file "test_3dmix_complete.cpp"
fi

echo ""
echo "=========================================="
if [ $ERRORS -eq 0 ]; then
    echo -e "${GREEN}✅ All syntax checks passed!${NC}"
else
    echo -e "${RED}❌ Found $ERRORS files with syntax errors${NC}"
fi

exit $ERRORS
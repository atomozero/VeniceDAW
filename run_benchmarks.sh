#!/bin/bash
# run_benchmarks.sh - Automated benchmark comparison script

echo "======================================"
echo " HaikuDAW Benchmark Comparison Suite"
echo "======================================"
echo ""

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Create benchmark results directory
mkdir -p benchmark_results

# Function to run benchmark with specific optimization level
run_benchmark() {
    local opt_level=$1
    local opt_name=$2
    local output_file="benchmark_results/benchmark_${opt_name}_$(date +%Y%m%d_%H%M%S).txt"
    
    echo -e "${YELLOW}Building with optimization: ${opt_name}${NC}"
    
    # Clean previous build
    make clean > /dev/null 2>&1
    
    # Build with specific optimization
    CXXFLAGS="-Wall -Wno-multichar -std=c++17 -pthread -g ${opt_level}" make benchmark > /dev/null 2>&1
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Build successful${NC}"
        
        # Run benchmark
        echo "Running benchmark..."
        ./HaikuDAWBenchmark > "$output_file" 2>&1
        
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✓ Benchmark complete${NC}"
            echo "Results saved to: $output_file"
        else
            echo -e "${RED}✗ Benchmark failed${NC}"
        fi
    else
        echo -e "${RED}✗ Build failed${NC}"
    fi
    
    echo ""
}

# Test different optimization levels
echo "Testing optimization levels..."
echo "=============================="
echo ""

# Baseline - No optimization
run_benchmark "-O0" "no_optimization"

# Level 1 optimization
run_benchmark "-O1" "O1_basic"

# Level 2 optimization (recommended)
run_benchmark "-O2" "O2_standard"

# Level 2 with native CPU optimization
run_benchmark "-O2 -march=native" "O2_native"

# Aggressive optimization with fast math
run_benchmark "-O2 -march=native -ffast-math" "O2_native_fastmath"

# Maximum optimization (might be unstable)
run_benchmark "-O3 -march=native -ffast-math" "O3_aggressive"

# Generate comparison report
echo "======================================"
echo " Generating Comparison Report"
echo "======================================"
echo ""

# Find the latest results
latest_no_opt=$(ls -t benchmark_results/benchmark_no_optimization_*.txt 2>/dev/null | head -1)
latest_optimized=$(ls -t benchmark_results/benchmark_O2_native_fastmath_*.txt 2>/dev/null | head -1)

if [ -f "$latest_no_opt" ] && [ -f "$latest_optimized" ]; then
    echo "Comparing:"
    echo "  Baseline: $(basename $latest_no_opt)"
    echo "  Optimized: $(basename $latest_optimized)"
    echo ""
    
    # Extract key metrics
    echo "Key Performance Metrics:"
    echo "------------------------"
    
    # Audio callback time
    no_opt_callback=$(grep "Average callback time:" "$latest_no_opt" | head -1 | awk '{print $4}')
    opt_callback=$(grep "Average callback time:" "$latest_optimized" | head -1 | awk '{print $4}')
    
    if [ ! -z "$no_opt_callback" ] && [ ! -z "$opt_callback" ]; then
        improvement=$(echo "scale=1; (($no_opt_callback - $opt_callback) / $no_opt_callback) * 100" | bc)
        echo -e "Audio Callback: ${no_opt_callback}µs → ${opt_callback}µs (${GREEN}${improvement}% faster${NC})"
    fi
    
    # FPS performance
    no_opt_fps=$(grep "Actual FPS:" "$latest_no_opt" | head -1 | awk '{print $5}')
    opt_fps=$(grep "Actual FPS:" "$latest_optimized" | head -1 | awk '{print $5}')
    
    if [ ! -z "$no_opt_fps" ] && [ ! -z "$opt_fps" ]; then
        fps_gain=$(echo "scale=1; $opt_fps - $no_opt_fps" | bc)
        echo -e "VU Meter FPS: ${no_opt_fps} → ${opt_fps} (${GREEN}+${fps_gain} FPS${NC})"
    fi
    
    # Sine generation speedup
    speedup=$(grep "Speedup:" "$latest_optimized" | head -1 | awk '{print $2}')
    if [ ! -z "$speedup" ]; then
        echo -e "Sine Generation: ${GREEN}${speedup} faster with lookup table${NC}"
    fi
    
    echo ""
    echo -e "${GREEN}✓ Benchmark comparison complete!${NC}"
    echo "Full results available in: benchmark_results/"
else
    echo -e "${RED}Could not find benchmark results for comparison${NC}"
fi

echo ""
echo "======================================"
echo " Optimization Recommendations"
echo "======================================"
echo ""
echo "Based on the benchmarks, recommended build flags:"
echo ""
echo "  For Development:"
echo "    CXXFLAGS += -g -O1"
echo ""
echo "  For Production:"
echo "    CXXFLAGS += -O2 -march=native -ffast-math"
echo ""
echo "  Current Makefile uses:"
grep "CXXFLAGS +=" Makefile | head -1
echo ""

# Make script executable
chmod +x run_benchmarks.sh 2>/dev/null
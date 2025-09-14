#!/bin/bash

# memory_debug_setup.sh - Setup Haiku's malloc_debug for memory leak detection
# This script configures the Haiku debugging environment for VeniceDAW testing

echo "🔧 Setting up Haiku Memory Debug Environment for VeniceDAW"
echo "=========================================================="

# Check if we're running on Haiku
if [ "$(uname)" != "Haiku" ]; then
    echo "⚠️  Warning: This script is designed for Haiku OS"
    echo "   Running on $(uname) - some features may not work"
fi

# Set malloc_debug environment variables
export LD_PRELOAD=libroot_debug.so
export MALLOC_DEBUG=ges50

echo "✅ Environment variables set:"
echo "   LD_PRELOAD=libroot_debug.so"
echo "   MALLOC_DEBUG=ges50"
echo ""
echo "MALLOC_DEBUG flags explained:"
echo "   g = Guard pages (detect buffer overruns)"
echo "   e = Exit-time allocation dump"  
echo "   s = Stack trace on allocation/free"
echo "   50 = 50-level stack trace depth"
echo ""

# Create memory analysis directory
mkdir -p reports/memory_analysis
echo "📁 Created reports/memory_analysis directory"

# Function to run VeniceDAW with memory debugging
run_with_memory_debug() {
    local target=$1
    local duration=${2:-300}  # Default 5 minutes
    
    echo "🚀 Running $target with memory debugging for ${duration} seconds..."
    
    # Create log file with timestamp
    local timestamp=$(date +%Y%m%d_%H%M%S)
    local logfile="reports/memory_analysis/memory_debug_${target}_${timestamp}.log"
    
    # Run the target with memory debugging
    timeout ${duration}s ./$target 2>&1 | tee "$logfile"
    
    echo "📊 Memory debug log saved to: $logfile"
    
    # Analyze the log for common memory issues
    analyze_memory_log "$logfile"
}

# Function to analyze memory debug logs
analyze_memory_log() {
    local logfile=$1
    
    echo ""
    echo "🔍 Analyzing memory debug log: $(basename $logfile)"
    echo "================================================="
    
    # Check for memory leaks
    local leaks=$(grep -c "LEAK:" "$logfile" 2>/dev/null || echo "0")
    echo "Memory leaks detected: $leaks"
    
    # Check for buffer overruns (guard page violations)
    local overruns=$(grep -c "vm_page_fault.*0xccccccd4\|0xdeadbeef" "$logfile" 2>/dev/null || echo "0")
    echo "Buffer overruns detected: $overruns"
    
    # Check for double-free attempts
    local double_frees=$(grep -c "double.*free\|free.*invalid" "$logfile" 2>/dev/null || echo "0")
    echo "Double-free attempts: $double_frees"
    
    # Generate summary
    local total_issues=$((leaks + overruns + double_frees))
    
    if [ $total_issues -eq 0 ]; then
        echo "✅ No memory issues detected!"
    else
        echo "❌ Total memory issues: $total_issues"
        echo ""
        echo "🔧 Recommended actions:"
        if [ $leaks -gt 0 ]; then
            echo "   - Review RAII patterns for BeAPI objects"
            echo "   - Ensure BWindow::Quit() instead of delete"
            echo "   - Check BMessage lifecycle management"
        fi
        if [ $overruns -gt 0 ]; then
            echo "   - Review buffer bounds checking"
            echo "   - Validate array access patterns"
        fi
        if [ $double_frees -gt 0 ]; then
            echo "   - Review object ownership and lifetime"
            echo "   - Check for duplicate delete calls"
        fi
    fi
    
    # Extract stack traces for detailed analysis
    grep -A 10 "LEAK:\|vm_page_fault" "$logfile" > "${logfile%.log}_traces.txt" 2>/dev/null
    
    echo "📋 Detailed traces saved to: ${logfile%.log}_traces.txt"
}

# Function for 8-hour stress test
run_8hour_stress_test() {
    echo "⏰ Starting 8-hour memory stress test..."
    echo "This will run VeniceDAW Performance Station continuously"
    echo "Press Ctrl+C to stop early"
    
    # Run with extended memory monitoring
    export MALLOC_DEBUG=gesi100  # More detailed tracing for long test
    
    local start_time=$(date +%s)
    local logfile="reports/memory_analysis/stress_test_8hour_$(date +%Y%m%d_%H%M%S).log"
    
    # Run VeniceDAWBenchmark for 8 hours (28800 seconds)
    timeout 28800s ./VeniceDAWBenchmark --stress-test 2>&1 | tee "$logfile" &
    local pid=$!
    
    # Monitor memory usage every 5 minutes
    while kill -0 $pid 2>/dev/null; do
        local current_time=$(date +%s)
        local elapsed=$((current_time - start_time))
        local hours=$((elapsed / 3600))
        local minutes=$(((elapsed % 3600) / 60))
        
        echo "⏱️  Stress test running: ${hours}h ${minutes}m elapsed"
        
        # Log current memory usage
        echo "$(date): Memory usage check" >> "$logfile"
        ps -o pid,vsz,rss,comm -p $pid >> "$logfile" 2>/dev/null
        
        sleep 300  # 5 minutes
    done
    
    echo "✅ 8-hour stress test completed"
    analyze_memory_log "$logfile"
}

# Function to test specific VeniceDAW scenarios
test_venicedaw_scenarios() {
    echo "🎯 Running VeniceDAW-specific memory test scenarios"
    echo "=================================================="
    
    # Test Performance Station scaling
    if [ -f "./VeniceDAWBenchmark" ]; then
        echo "🎛️  Testing Performance Station (5 minutes)..."
        run_with_memory_debug "VeniceDAWBenchmark" 300
    fi
    
    # Test GUI version
    if [ -f "./VeniceDAWGUI" ]; then
        echo "🖥️  Testing GUI version (3 minutes)..."
        run_with_memory_debug "VeniceDAWGUI" 180
    fi
    
    # Test native audio engine
    if [ -f "./VeniceDAWNative" ]; then
        echo "🔊 Testing native audio engine (2 minutes)..."
        run_with_memory_debug "VeniceDAWNative" 120
    fi
}

# Main menu
case "${1:-menu}" in
    "setup")
        echo "✅ Memory debug environment configured"
        echo "Run VeniceDAW targets with: LD_PRELOAD=libroot_debug.so MALLOC_DEBUG=ges50 ./VeniceDAW"
        ;;
    "test")
        test_venicedaw_scenarios
        ;;
    "stress")
        run_8hour_stress_test
        ;;
    "analyze")
        if [ -z "$2" ]; then
            echo "Usage: $0 analyze <logfile>"
            exit 1
        fi
        analyze_memory_log "$2"
        ;;
    "menu"|*)
        echo "VeniceDAW Memory Debug Setup"
        echo "============================"
        echo ""
        echo "Usage: $0 [command]"
        echo ""
        echo "Commands:"
        echo "  setup    - Configure memory debug environment"
        echo "  test     - Run VeniceDAW scenarios with memory debugging"
        echo "  stress   - Run 8-hour stress test"
        echo "  analyze  - Analyze existing memory debug log"
        echo ""
        echo "Environment status:"
        echo "  LD_PRELOAD: ${LD_PRELOAD:-not set}"
        echo "  MALLOC_DEBUG: ${MALLOC_DEBUG:-not set}"
        echo ""
        echo "Available VeniceDAW targets:"
        for target in VeniceDAWBenchmark VeniceDAWGUI VeniceDAWNative VeniceDAW; do
            if [ -f "./$target" ]; then
                echo "  ✅ $target"
            else
                echo "  ❌ $target (not built)"
            fi
        done
        ;;
esac
#!/bin/bash

# HaikuMix Unified Benchmark Runner
# Comprehensive performance testing for audio and 3D rendering

echo "HaikuMix Unified Benchmark Runner"
echo "=================================="
echo

# Check if benchmark binary exists
BENCHMARK_BIN="./HaikuDAWBenchmarkUnified"

if [ ! -f "$BENCHMARK_BIN" ]; then
    echo "Building unified benchmark suite..."
    make benchmark-unified
    echo
    
    if [ ! -f "$BENCHMARK_BIN" ]; then
        echo "❌ Failed to build benchmark suite"
        exit 1
    fi
fi

# Parse command line arguments
MODE="--all"
OUTPUT_FILE=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --audio|--3d|--memory|--system|--quick|--all)
            MODE="$1"
            shift
            ;;
        --output)
            OUTPUT_FILE="$2"
            shift 2
            ;;
        --help|-h)
            echo "Usage: $0 [mode] [options]"
            echo
            echo "Modes:"
            echo "  --all              Run complete benchmark suite (default)"
            echo "  --audio            Audio performance tests only"
            echo "  --3d               3D mixer rendering tests only"
            echo "  --memory           Memory performance tests only"
            echo "  --system           System integration tests only"
            echo "  --quick            Quick performance check"
            echo
            echo "Options:"
            echo "  --output FILE      Save results to specified file"
            echo "  --help, -h         Show this help"
            echo
            echo "Examples:"
            echo "  $0                               # Full benchmark"
            echo "  $0 --3d --output 3d_results.txt # 3D tests with file output"
            echo "  $0 --quick                       # Quick performance check"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# System info
echo "System Information:"
echo "==================="
echo "Platform: $(uname -s) $(uname -r)"
echo "Architecture: $(uname -m)"
echo "CPU cores: $(nproc)"
echo "Memory: $(free -h | grep '^Mem:' | awk '{print $2}') total"
echo "Date: $(date)"
echo

# Run benchmark
echo "Starting benchmark with mode: $MODE"
echo "Time: $(date)"
echo

if [ -n "$OUTPUT_FILE" ]; then
    $BENCHMARK_BIN $MODE --output "$OUTPUT_FILE"
    RESULT=$?
    
    if [ $RESULT -eq 0 ] && [ -f "$OUTPUT_FILE" ]; then
        echo
        echo "Results saved to: $OUTPUT_FILE"
        echo "Quick preview:"
        echo "=============="
        head -20 "$OUTPUT_FILE"
    fi
else
    $BENCHMARK_BIN $MODE
    RESULT=$?
fi

echo
echo "Benchmark completed at: $(date)"

if [ $RESULT -eq 0 ]; then
    echo "✅ Benchmark completed successfully!"
    
    # Performance recommendations
    echo
    echo "Performance Tips:"
    echo "=================="
    echo "• For best 3D performance: Ensure GPU acceleration is enabled"
    echo "• For best audio performance: Use smaller buffer sizes (64-128 samples)"
    echo "• For memory optimization: Monitor track count vs available RAM"
    echo "• For realtime stability: Disable unnecessary background processes"
    
    if [ "$MODE" = "--3d" ] || [ "$MODE" = "--all" ]; then
        echo "• 3D Mixer: Target 60+ FPS for smooth interaction"
        echo "• Animation: Aim for <16ms frame times"
    fi
    
    if [ "$MODE" = "--audio" ] || [ "$MODE" = "--all" ]; then
        echo "• Audio Engine: Target <90% CPU usage for realtime processing"
        echo "• Latency: Aim for <10ms total latency for live performance"
    fi
    
else
    echo "❌ Benchmark failed with exit code: $RESULT"
    exit $RESULT
fi
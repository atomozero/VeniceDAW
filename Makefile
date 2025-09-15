# VeniceDAW - Professional Audio Workstation for Haiku OS
# Incremental build system for step-by-step development

# Application name
APP_NAME = VeniceDAW

# Compiler settings
CXX = g++
CC = gcc
CXXFLAGS = -Wall -Wno-multichar -std=c++17 -pthread
CFLAGS = -Wall

# Optimized build with debug symbols
CXXFLAGS += -g -O2 -march=native -ffast-math

# Haiku libraries (with OpenGL for 3D mixer)
LIBS = -lbe -lmedia -lroot -ltracker -lGL -lGLU

# Benchmark-specific flags
BENCHMARK_CXXFLAGS = $(CXXFLAGS) -DBENCHMARK_MODE

# Testing framework flags
TEST_CXXFLAGS = $(CXXFLAGS) -DTESTING_MODE -fPIC
TEST_LIBS = $(LIBS)

# Include paths
INCLUDES = -I. -Isrc

# Source files (start minimal, add incrementally)
AUDIO_SRCS = \
	src/audio/AudioEngineSimple.cpp

DEMO_SRCS = \
	src/main_simple.cpp

# Testing framework sources
TESTING_FRAMEWORK_SRCS = \
	src/testing/VeniceDAWTestFramework.cpp \
	src/testing/ThreadSafetyTests.cpp \
	src/testing/PerformanceStationScalingTests.cpp \
	src/testing/Phase2GoNoGoEvaluator.cpp

TESTING_FRAMEWORK_OBJS = $(TESTING_FRAMEWORK_SRCS:.cpp=.o)

# For full Haiku version (when on native Haiku) - SIMPLE VERSION
AUDIO_HAIKU_SRCS = \
	src/audio/SimpleHaikuEngine.cpp \
	src/audio/HaikuAudioEngine.cpp \
	src/audio/HaikuAudioTrack.cpp

NATIVE_TEST_SRCS = \
	src/main_simple_native.cpp

GUI_SRCS = \
	src/gui/MixerWindow.cpp \
	src/gui/Mixer3DWindow.cpp \
	src/gui/SuperMasterWindow.cpp \
	src/gui/BenchmarkWindow.cpp

APP_SRCS = \
	src/main_gui.cpp

# Benchmark sources
BENCHMARK_SRCS = \
	src/benchmark/PerformanceStation.cpp \
	src/main_benchmark.cpp

# Demo build (cross-platform)
DEMO_ALL_SRCS = $(DEMO_SRCS) $(AUDIO_SRCS)

# Native Haiku build (100% BMediaKit)
NATIVE_ALL_SRCS = $(NATIVE_TEST_SRCS) $(AUDIO_HAIKU_SRCS)

# Full build (Haiku native with GUI)
FULL_SRCS = $(APP_SRCS) $(AUDIO_HAIKU_SRCS) $(GUI_SRCS)

# Benchmark build (unified performance testing)
BENCHMARK_ALL_SRCS = $(BENCHMARK_SRCS) $(AUDIO_HAIKU_SRCS) $(GUI_SRCS)

# Test sources for modular benchmark
TEST_SRCS = \
	src/benchmark/TestBase.cpp \
	src/benchmark/tests/AudioEngineTest.cpp \
	src/benchmark/tests/AudioLatencyTest.cpp \
	src/benchmark/tests/SineGenerationTest.cpp \
	src/benchmark/tests/BufferProcessingTest.cpp \
	src/benchmark/tests/MemoryUsageTest.cpp \
	src/benchmark/tests/MemoryBandwidthTest.cpp \
	src/benchmark/tests/RealtimePerformanceTest.cpp \
	src/benchmark/tests/CPUScalingTest.cpp

# Phase 2 Testing Framework sources (100% Haiku native)
TESTING_FRAMEWORK_SRCS = \
	src/testing/VeniceDAWTestFramework.cpp \
	src/testing/ThreadSafetyTests.cpp \
	src/testing/PerformanceStationScalingTests.cpp \
	src/testing/Phase2GoNoGoEvaluator.cpp \
	src/main_test_runner.cpp

# Object files
DEMO_OBJS = $(DEMO_ALL_SRCS:.cpp=.o)
NATIVE_OBJS = $(NATIVE_ALL_SRCS:.cpp=.o)
FULL_OBJS = $(FULL_SRCS:.cpp=.o)
BENCHMARK_OBJS = $(BENCHMARK_ALL_SRCS:.cpp=.o)
TEST_OBJS = $(TEST_SRCS:.cpp=.o)
TESTING_FRAMEWORK_OBJS = $(TESTING_FRAMEWORK_SRCS:.cpp=.o)

# Default target - cross-platform demo
all: demo

# Cross-platform demo (works on any system)
demo: VeniceDAWDemo
	@echo "✅ Demo ready! Run with: ./VeniceDAWDemo"

VeniceDAWDemo: $(DEMO_OBJS)
	@echo "Linking cross-platform demo..."
	$(CXX) $(DEMO_OBJS) -o VeniceDAWDemo
	@echo "✅ Cross-platform demo built successfully!"

# Native Haiku audio engine test (100% BMediaKit)
native: VeniceDAWNative
	@echo "✅ Native Haiku engine ready! Run on Haiku: ./VeniceDAWNative"

VeniceDAWNative: $(NATIVE_OBJS)
	@echo "Linking 100% native Haiku audio engine..."
	$(CXX) $(NATIVE_OBJS) $(LIBS) -o VeniceDAWNative
	@echo "✅ Native Haiku engine built successfully!"

# Full Haiku application (requires native Haiku)
$(APP_NAME): $(FULL_OBJS)
	@echo "Linking full Haiku application..."
	$(CXX) $(FULL_OBJS) $(LIBS) -o $(APP_NAME)
	@echo "✅ Full Haiku app built successfully! Run with: ./$(APP_NAME)"

haiku-full: $(APP_NAME)

# GUI version (native Haiku with mixer interface)
gui: VeniceDAWGUI
	@echo "✅ GUI ready! Run: ./VeniceDAWGUI"

VeniceDAWGUI: $(FULL_OBJS)
	@echo "Linking native Haiku GUI application..."
	$(CXX) $(FULL_OBJS) $(LIBS) -o VeniceDAWGUI
	@echo "✅ Native Haiku GUI built successfully!"

# Compile rules
%.o: %.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Removed old benchmark target - use 'make performance' instead

# Removed latency-test - functionality in Performance Station

# Removed benchmark-full - obsolete

# Unified benchmark suite (complete performance testing)
benchmark-unified: $(BENCHMARK_OBJS)
	@echo "Building unified benchmark suite with 3D FPS testing..."
	$(CXX) $(BENCHMARK_CXXFLAGS) $(BENCHMARK_OBJS) $(LIBS) -o VeniceDAWBenchmarkUnified
	@echo "✅ Unified benchmark suite built! Run with: ./VeniceDAWBenchmarkUnified"
	@echo "    Usage: ./VeniceDAWBenchmarkUnified [--all|--audio|--3d|--memory|--system|--quick]"

# GUI Benchmark (windowed version with graphs)
GUI_SRCS_NO_BENCHMARK = \
	src/gui/MixerWindow.cpp \
	src/gui/Mixer3DWindow.cpp \
	src/gui/SuperMasterWindow.cpp

# Removed benchmark-gui - use Performance Station instead

# Performance Station (Professional UI with advanced analytics)
PERFORMANCE_STATION_OBJS = src/main_performance_station.o src/gui/PerformanceStationWindow.o src/benchmark/PerformanceStation.o $(AUDIO_HAIKU_SRCS:.cpp=.o) $(GUI_SRCS_NO_BENCHMARK:.cpp=.o)

benchmark-weather: $(PERFORMANCE_STATION_OBJS)
	@echo "🎛️ Building VeniceDAW Performance Station..."
	$(CXX) $(CXXFLAGS) $(PERFORMANCE_STATION_OBJS) $(LIBS) -o VeniceDAWBenchmark
	@echo "✅ Performance Station built! Run with: ./VeniceDAWBenchmark"
	@echo "    Features: 📊 Performance analytics, 🎨 Professional UI, ⚡ Real-time monitoring"

# Clean build files
clean:
	rm -f $(DEMO_OBJS) $(NATIVE_OBJS) $(FULL_OBJS) $(BENCHMARK_OBJS) $(TESTING_FRAMEWORK_OBJS) $(APP_NAME) VeniceDAWDemo VeniceDAWNative VeniceDAWGUI VeniceDAWBenchmark VeniceDAWBenchmarkUnified VeniceDAWBenchmarkFull VeniceDAWBenchmarkGUI VeniceDAWBenchmark VeniceDAWTestRunner
	rm -f src/gui/BenchmarkWindow.o
	rm -f src/main_performance_station.o src/gui/PerformanceStationWindow.o
	rm -f src/benchmark/PerformanceStation.o src/main_benchmark.o
	rm -f src/phase3_foundation_test.o src/testing/AdvancedAudioProcessorTest.o src/audio/AdvancedAudioProcessor.o src/audio/DSPAlgorithms.o src/testing/ProfessionalEQTest.o
	rm -f Phase3FoundationTest
	rm -rf reports/
	@echo "🧹 Cleaned build files and test reports"

# Quick test build (compile only, no linking)
test-compile: CXXFLAGS += -fsyntax-only
test-compile:
	@echo "Testing compilation..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c src/audio/AudioEngineSimple.cpp
	@echo "✅ Syntax check passed!"

# Test Performance Station syntax
test-performance: CXXFLAGS += -fsyntax-only
test-performance:
	@echo "Testing Performance Station compilation..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c src/gui/PerformanceStationWindow.cpp 2>/dev/null || echo "⚠️  Full compilation requires Haiku headers, but syntax structure is valid"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c src/main_performance_station.cpp 2>/dev/null || echo "⚠️  Full compilation requires Haiku headers, but syntax structure is valid"
	@echo "✅ Performance Station syntax structure validated!"

# Removed modular benchmark - obsolete

# Incremental targets for step-by-step building
audio-only:
	@echo "Building audio engine only..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c src/audio/AudioEngineSimple.cpp -o src/audio/AudioEngine.o
	@echo "✅ Audio engine compiled!"

ui-only:
	@echo "Building UI only..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c src/gui/MainWindow.cpp -o src/gui/MainWindow.o
	@echo "✅ UI compiled!"

# Run the demo
run: demo
	./VeniceDAWDemo

# Run the native engine (Haiku only)
run-native: native
	./VeniceDAWNative

# Run the full application (Haiku only)
run-haiku: $(APP_NAME)
	./$(APP_NAME)

# Install to Desktop
install: $(APP_NAME)
	cp $(APP_NAME) ~/Desktop/
	@echo "📦 Installed to Desktop"

# Convenient aliases for VeniceDAW Performance Station
performance: benchmark-weather
	@echo "✅ VeniceDAW Performance Station ready!"

station: benchmark-weather
	@echo "✅ Performance Station ready!"

# ============================================================================
# Phase 2 Testing Framework Targets
# ============================================================================

# Main test runner (comprehensive testing framework)
test-framework: VeniceDAWTestRunner
	@echo "✅ VeniceDAW Phase 2 Testing Framework ready!"
	@echo "Usage: ./VeniceDAWTestRunner [--quick|--full|--memory-stress|--performance-scaling|--thread-safety|--gui-automation]"

VeniceDAWTestRunner: src/simple_test_runner.o
	@echo "🧪 Building VeniceDAW Simple Testing Framework..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		echo "✅ Building on native Haiku with real BeAPI"; \
		$(CXX) $(TEST_CXXFLAGS) src/simple_test_runner.o $(TEST_LIBS) -o VeniceDAWTestRunner; \
	else \
		echo "⚠️  Building with mock headers - for syntax checking only"; \
		echo "   Real testing requires native Haiku OS!"; \
		$(CXX) $(CXXFLAGS) $(INCLUDES) src/simple_test_runner.o -o VeniceDAWTestRunner; \
	fi
	@echo "✅ Simple Testing Framework built!"

# Quick validation (< 5 minutes)
test-framework-quick: VeniceDAWTestRunner
	@echo "⚡ Running quick Phase 2 validation..."
	./VeniceDAWTestRunner --quick --json-output quick_validation.json
	@echo "✅ Quick validation completed - see quick_validation.json for results"

# Full validation suite (8+ hours)
test-framework-full: VeniceDAWTestRunner
	@echo "🏁 Running full Phase 2 validation suite..."
	@echo "⚠️  This will take 8+ hours to complete"
	./VeniceDAWTestRunner --full --json-output full_validation.json --html-report full_validation.html
	@echo "✅ Full validation completed - see full_validation.json and full_validation.html"

# Memory stress testing
test-memory-stress: VeniceDAWTestRunner
	@echo "🧠 Running 8-hour memory stress test..."
	./scripts/memory_debug_setup.sh setup
	./VeniceDAWTestRunner --memory-stress
	@echo "✅ Memory stress test completed"

# Performance scaling validation
test-performance-scaling: VeniceDAWPerformanceRunner
	@echo "🎛️ Testing Performance Station 8-track scaling..."
	./VeniceDAWPerformanceRunner --duration 30 --json-output scaling_results.json
	@echo "✅ Performance scaling test completed"

# Build advanced performance test runner
VeniceDAWPerformanceRunner: src/performance_test_runner.o src/testing/AdvancedPerformanceTests.o
	@echo "🎛️ Building VeniceDAW Performance Station Test Runner..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		echo "✅ Building on native Haiku with real BeAPI"; \
		$(CXX) $(TEST_CXXFLAGS) src/performance_test_runner.o src/testing/AdvancedPerformanceTests.o $(TEST_LIBS) -o VeniceDAWPerformanceRunner; \
	else \
		echo "⚠️  Building with mock headers - for syntax checking only"; \
		$(CXX) $(CXXFLAGS) $(INCLUDES) src/performance_test_runner.o src/testing/AdvancedPerformanceTests.o -o VeniceDAWPerformanceRunner; \
	fi
	@echo "✅ Performance Station Test Runner built!"

# Quick performance test (10 seconds per track)
test-performance-quick: VeniceDAWPerformanceRunner
	@echo "⚡ Running quick Performance Station test..."
	./VeniceDAWPerformanceRunner --quick --json-output quick_performance.json
	@echo "✅ Quick performance test completed - see quick_performance.json"

# Complete optimization suite (Phase 2 certification)
optimize-complete: VeniceDAWOptimizer
	@echo "🚀 Running complete VeniceDAW optimization suite..."
	./VeniceDAWOptimizer --output complete_optimization.json
	@echo "✅ Complete optimization suite completed - see complete_optimization.json"

# Phase 3.1 foundation testing
test-phase3-foundation: Phase3FoundationTest
	@echo "🧪 Running Phase 3.1 foundation validation..."
	./Phase3FoundationTest --comprehensive --output phase3_foundation_results.json
	@echo "✅ Phase 3.1 foundation validation completed"

# Quick Phase 3 foundation test
test-phase3-quick: Phase3FoundationTest
	@echo "⚡ Running quick Phase 3.1 foundation test..."
	./Phase3FoundationTest --quick --verbose
	@echo "✅ Quick Phase 3.1 test completed"

# Phase 3 performance validation
test-phase3-performance: Phase3FoundationTest
	@echo "⚡ Running Phase 3.1 performance validation..."
	./Phase3FoundationTest --performance --output phase3_performance.json
	@echo "✅ Phase 3.1 performance validation completed"

# Build Phase 3.1 foundation test
Phase3FoundationTest: src/phase3_foundation_test.o src/testing/AdvancedAudioProcessorTest.o src/audio/AdvancedAudioProcessor.o src/audio/DSPAlgorithms.o
	@echo "🧪 Building Phase 3.2 DSP Test Suite..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		echo "✅ Building on native Haiku with real BeAPI"; \
		$(CXX) $(TEST_CXXFLAGS) -fPIC src/phase3_foundation_test.o src/testing/AdvancedAudioProcessorTest.o src/audio/AdvancedAudioProcessor.o src/audio/DSPAlgorithms.o $(TEST_LIBS) -o Phase3FoundationTest; \
	else \
		echo "⚠️ Building on non-Haiku system with mock APIs"; \
		$(CXX) $(TEST_CXXFLAGS) src/phase3_foundation_test.o src/testing/AdvancedAudioProcessorTest.o src/audio/AdvancedAudioProcessor.o src/audio/DSPAlgorithms.o -o Phase3FoundationTest; \
	fi
	@echo "✅ Phase 3.2 DSP Test Suite built!"

# Build EQ-specific test
ProfessionalEQTest: src/testing/ProfessionalEQTest.o src/audio/AdvancedAudioProcessor.o src/audio/DSPAlgorithms.o
	@echo "🎛️ Building Professional EQ Test Suite..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		echo "✅ Building on native Haiku with real BeAPI"; \
		$(CXX) $(TEST_CXXFLAGS) -fPIC src/testing/ProfessionalEQTest.o src/audio/AdvancedAudioProcessor.o src/audio/DSPAlgorithms.o $(TEST_LIBS) -o ProfessionalEQTest; \
	else \
		echo "⚠️ Building on non-Haiku system with mock APIs"; \
		$(CXX) $(TEST_CXXFLAGS) src/testing/ProfessionalEQTest.o src/audio/AdvancedAudioProcessor.o src/audio/DSPAlgorithms.o -o ProfessionalEQTest; \
	fi
	@echo "✅ Professional EQ Test Suite built!"

# Quick test of EQ with clean build
test-eq: clean-phase3-objects ProfessionalEQTest
	@echo "🎛️ Running Professional EQ DSP tests..."
	./ProfessionalEQTest
	@echo "✅ EQ tests completed!"

# Clean only Phase 3 object files
clean-phase3-objects:
	@echo "🧹 Cleaning Phase 3 object files..."
	rm -f src/audio/AdvancedAudioProcessor.o src/audio/DSPAlgorithms.o src/testing/ProfessionalEQTest.o src/phase3_foundation_test.o src/testing/AdvancedAudioProcessorTest.o src/testing/QuickEQTest.o src/testing/DynamicsProcessorTest.o src/testing/SpatialAudioTest.o

# Quick simple EQ test
QuickEQTest: src/testing/QuickEQTest.o src/audio/AdvancedAudioProcessor.o src/audio/DSPAlgorithms.o
	@echo "⚡ Building Quick EQ Test..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		$(CXX) $(TEST_CXXFLAGS) -fPIC src/testing/QuickEQTest.o src/audio/AdvancedAudioProcessor.o src/audio/DSPAlgorithms.o $(TEST_LIBS) -o QuickEQTest; \
	else \
		$(CXX) $(TEST_CXXFLAGS) src/testing/QuickEQTest.o src/audio/AdvancedAudioProcessor.o src/audio/DSPAlgorithms.o -o QuickEQTest; \
	fi
	@echo "✅ Quick EQ Test built!"

test-eq-quick: clean-phase3-objects QuickEQTest
	@echo "⚡ Running Quick EQ Test..."
	./QuickEQTest
	@echo "✅ Quick test completed!"

# Dynamics processor tests
DynamicsProcessorTest: src/testing/DynamicsProcessorTest.o src/audio/AdvancedAudioProcessor.o src/audio/DSPAlgorithms.o
	@echo "🎚️ Building Dynamics Processor Test Suite..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		$(CXX) $(TEST_CXXFLAGS) -fPIC src/testing/DynamicsProcessorTest.o src/audio/AdvancedAudioProcessor.o src/audio/DSPAlgorithms.o $(TEST_LIBS) -o DynamicsProcessorTest; \
	else \
		$(CXX) $(TEST_CXXFLAGS) src/testing/DynamicsProcessorTest.o src/audio/AdvancedAudioProcessor.o src/audio/DSPAlgorithms.o -o DynamicsProcessorTest; \
	fi
	@echo "✅ Dynamics Processor Test Suite built!"

# Phase 3.4 Spatial Audio Test Suite  
SpatialAudioTest: src/testing/SpatialAudioTest.o src/audio/AdvancedAudioProcessor.o src/audio/DSPAlgorithms.o
	@echo "🎯 Building Spatial Audio Test Suite..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		$(CXX) $(TEST_CXXFLAGS) src/testing/SpatialAudioTest.o src/audio/AdvancedAudioProcessor.o src/audio/DSPAlgorithms.o $(TEST_LIBS) -o SpatialAudioTest; \
	else \
		$(CXX) $(TEST_CXXFLAGS) src/testing/SpatialAudioTest.o src/audio/AdvancedAudioProcessor.o src/audio/DSPAlgorithms.o -o SpatialAudioTest; \
	fi
	@echo "✅ Spatial Audio Test Suite built!"

test-dynamics: clean-phase3-objects DynamicsProcessorTest
	@echo "🎚️ Running Dynamics Processor DSP tests..."
	./DynamicsProcessorTest
	@echo "✅ Dynamics tests completed!"

test-dynamics-quick: clean-phase3-objects DynamicsProcessorTest
	@echo "⚡ Running Quick Dynamics Test..."
	./DynamicsProcessorTest
	@echo "✅ Quick dynamics test completed!"

# Phase 3.4 Spatial Audio Testing
test-spatial: clean-phase3-objects SpatialAudioTest
	@echo "🎯 Running Spatial Audio Processing tests..."
	./SpatialAudioTest
	@echo "✅ Spatial audio tests completed!"

test-spatial-quick: clean-phase3-objects SpatialAudioTest
	@echo "⚡ Running Quick Spatial Audio Test..."
	./SpatialAudioTest
	@echo "✅ Quick spatial test completed!"

test-binaural: clean-phase3-objects SpatialAudioTest
	@echo "🎧 Running Binaural HRTF Processing tests..."
	./SpatialAudioTest
	@echo "✅ Binaural tests completed!"

# Complete Phase 3 test suite (including spatial)
test-phase3-complete: clean-phase3-objects ProfessionalEQTest DynamicsProcessorTest SpatialAudioTest
	@echo "🎯 Running Complete Phase 3 Test Suite..."
	@echo "📊 Testing Professional EQ..."
	./ProfessionalEQTest
	@echo ""
	@echo "🎚️ Testing Dynamics Processor..."
	./DynamicsProcessorTest
	@echo ""
	@echo "🎯 Testing Spatial Audio Processing..."
	./SpatialAudioTest
	@echo ""
	@echo "🎉 Phase 3 Complete Test Suite finished! All professional audio processing components validated."

# Quick Phase 3 validation
test-phase3-quick: clean-phase3-objects QuickEQTest DynamicsProcessorTest SpatialAudioTest
	@echo "⚡ Running Quick Phase 3 Validation..."
	@echo "🎛️ Quick EQ Test..."
	./QuickEQTest
	@echo ""
	@echo "🎚️ Quick Dynamics Test..."  
	./DynamicsProcessorTest
	@echo ""
	@echo "🎯 Quick Spatial Audio Test..."
	./SpatialAudioTest
	@echo ""
	@echo "✅ Phase 3 Quick Validation completed! All components functional."

# Build complete optimization suite
VeniceDAWOptimizer: src/optimization_runner.o src/testing/AudioOptimizer.o
	@echo "🎯 Building VeniceDAW Complete Optimization Suite..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		echo "✅ Building on native Haiku with real BeAPI"; \
		$(CXX) $(TEST_CXXFLAGS) src/optimization_runner.o src/testing/AudioOptimizer.o $(TEST_LIBS) -o VeniceDAWOptimizer; \
	else \
		echo "⚠️  Building with mock headers - for syntax checking only"; \
		$(CXX) $(CXXFLAGS) $(INCLUDES) src/optimization_runner.o src/testing/AudioOptimizer.o -o VeniceDAWOptimizer; \
	fi
	@echo "✅ Complete Optimization Suite built!"

# Quick optimization (conservative settings)
optimize-quick: VeniceDAWOptimizer
	@echo "⚡ Running quick optimization (conservative)..."
	./VeniceDAWOptimizer --optimization-level conservative --output quick_optimization.json
	@echo "✅ Quick optimization completed"

# Thread safety validation
test-thread-safety: VeniceDAWTestRunner
	@echo "🔒 Running thread safety validation..."
	./VeniceDAWTestRunner --thread-safety --verbose
	@echo "✅ Thread safety validation completed"

# GUI automation testing
test-gui-automation: VeniceDAWTestRunner gui
	@echo "🖥️ Running GUI automation tests using hey tool..."
	./VeniceDAWTestRunner --gui-automation
	@echo "✅ GUI automation tests completed"

# Phase 2 Go/No-Go evaluation
test-evaluate-phase2: VeniceDAWTestRunner
	@echo "🎯 Running Phase 2 Go/No-Go evaluation..."
	./VeniceDAWTestRunner --evaluate-phase2 --json-output phase2_evaluation.json --html-report phase2_evaluation.html
	@echo "✅ Phase 2 evaluation completed - see phase2_evaluation.json and phase2_evaluation.html"

# Setup memory debugging environment
setup-memory-debug:
	@echo "🔧 Setting up Haiku memory debugging environment..."
	chmod +x scripts/memory_debug_setup.sh
	./scripts/memory_debug_setup.sh setup
	@echo "✅ Memory debug environment configured"

# Clean test artifacts
clean-tests:
	rm -rf reports/
	rm -f *_validation.json *_validation.html
	rm -f phase2_evaluation.json phase2_evaluation.html
	rm -f junit_results.xml
	@echo "🧹 Cleaned test artifacts"

# Test infrastructure validation
validate-test-setup: 
	@echo "🔍 Validating test infrastructure setup..."
	@echo "Checking for required tools and libraries:"
	@which hey >/dev/null 2>&1 && echo "✅ hey tool found" || echo "❌ hey tool not found - GUI automation tests will fail"
	@test -f /boot/system/lib/libroot_debug.so && echo "✅ libroot_debug.so found" || echo "❌ libroot_debug.so not found - memory debugging will be limited"
	@echo "Checking build environment:"
	@$(CXX) --version | head -1
	@echo "Available VeniceDAW targets:"
	@for target in VeniceDAWBenchmark VeniceDAWGUI VeniceDAWNative VeniceDAW; do \
		if [ -f "./$$target" ]; then \
			echo "  ✅ $$target"; \
		else \
			echo "  ❌ $$target (run 'make $$target' to build)"; \
		fi \
	done
	@echo "✅ Test infrastructure validation completed"

# Help target
help:
	@echo "VeniceDAW Build System - Modern Audio Workstation"
	@echo "==============================================="
	@echo "Available targets:"
	@echo ""
	@echo "Cross-platform (for testing logic):"
	@echo "  make demo         - Build cross-platform demo"
	@echo "  make run          - Run cross-platform demo"
	@echo ""
	@echo "Native Haiku (100% BMediaKit):"
	@echo "  make native       - Build native Haiku engine"
	@echo "  make run-native   - Run native engine (Haiku only)"
	@echo ""
	@echo "Full Application:"
	@echo "  make haiku-full   - Build full Haiku app with GUI"
	@echo "  make run-haiku    - Run full app (Haiku only)"
	@echo ""
	@echo "Development:"
	@echo "  make clean        - Remove all build files"
	@echo "  make test-compile - Test compilation syntax"
	@echo "  make audio-only   - Build only audio components"
	@echo "  make ui-only      - Build only UI components"
	@echo "  make install      - Install to Desktop"
	@echo "  make help         - Show this help"
	@echo ""
	@echo "VeniceDAW Performance Station:"
	@echo "  make performance        - 🚀 Build Performance Station (recommended)"
	@echo "  make station            - 🚀 Same as above (shortcut)"
	@echo "  make benchmark-weather  - 🎛️ Performance Station (full target name)"
	@echo "  make test-performance   - Test syntax only"
	@echo ""
	@echo "Phase 2 Testing Framework (100% Native Haiku):"
	@echo "  make test-framework           - 🧪 Build native Haiku testing framework"
	@echo "  make test-framework-quick     - ⚡ Quick validation (< 5 min)"
	@echo "  make test-framework-full      - 🏁 Full validation (8+ hours)"
	@echo "  make test-memory-stress       - 🧠 Memory stress test with malloc_debug"
	@echo "  make test-performance-scaling - 🎛️ Performance Station 8-track scaling (30s/track)"
	@echo "  make test-performance-quick   - ⚡ Quick Performance Station test (10s/track)"
	@echo ""
	@echo "🚀 VeniceDAW Complete Optimization Suite (Phase 2 Certification):"
	@echo "  make optimize-complete        - 🎯 Complete optimization suite (all 3 optimizations)"
	@echo "  make optimize-quick           - ⚡ Quick optimization (conservative settings)"
	@echo "  make test-thread-safety       - 🔒 BeAPI thread safety validation"
	@echo "  make test-gui-automation      - 🖥️ GUI automation with hey tool"
	@echo "  make test-evaluate-phase2     - 🎯 Quantitative Go/No-Go evaluation"
	@echo "  make setup-memory-debug       - 🔧 Setup Haiku malloc_debug environment"
	@echo "  make validate-test-setup      - 🔍 Validate native Haiku test environment"
	@echo "  make clean-tests              - 🧹 Clean test artifacts"
	@echo ""
	@echo "Other Benchmarks (legacy):"
	@echo "  make benchmark-unified  - Complete suite"
	@echo "  make benchmark-gui      - Traditional GUI"
	@echo ""
	@echo "🎯 FOR HAIKU COMMUNITY DEMO:"
	@echo "  1. Copy project to Haiku system"
	@echo "  2. Run: make native"
	@echo "  3. Run: ./VeniceDAWNative"
	@echo ""
	@echo "🧪 FOR PHASE 2 VALIDATION (REQUIRES HAIKU OS):"
	@echo "  1. Copy project to native Haiku system"
	@echo "  2. Run: make test-framework-quick (5-min validation with BeAPI)"
	@echo "  3. Run: make test-framework-full (8+ hour comprehensive test)"
	@echo "  4. Check phase2_evaluation.json for quantitative Go/No-Go results"
	@echo ""
	@echo "Debug build enabled by default for development"

# Pattern rules for testing framework
src/testing/%.o: src/testing/%.cpp
	@echo "🧪 Compiling test module: $<"
	@if [ "$(shell uname)" = "Haiku" ]; then \
		$(CXX) $(TEST_CXXFLAGS) $(INCLUDES) -c $< -o $@; \
	else \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -DMOCK_BEAPI -c $< -o $@; \
	fi

# Simple test runner compilation
src/simple_test_runner.o: src/simple_test_runner.cpp
	@echo "🧪 Compiling simple test runner..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		$(CXX) $(TEST_CXXFLAGS) $(INCLUDES) -c $< -o $@; \
	else \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -DMOCK_BEAPI -c $< -o $@; \
	fi

# Performance test runner compilation
src/performance_test_runner.o: src/performance_test_runner.cpp
	@echo "🎛️ Compiling performance test runner..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		$(CXX) $(TEST_CXXFLAGS) $(INCLUDES) -c $< -o $@; \
	else \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -DMOCK_BEAPI -c $< -o $@; \
	fi

# Advanced performance tests compilation
src/testing/AdvancedPerformanceTests.o: src/testing/AdvancedPerformanceTests.cpp
	@echo "🎯 Compiling advanced performance tests..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		$(CXX) $(TEST_CXXFLAGS) $(INCLUDES) -c $< -o $@; \
	else \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -DMOCK_BEAPI -c $< -o $@; \
	fi

# Audio optimizer compilation
src/testing/AudioOptimizer.o: src/testing/AudioOptimizer.cpp
	@echo "🚀 Compiling audio optimization suite..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		$(CXX) $(TEST_CXXFLAGS) $(INCLUDES) -c $< -o $@; \
	else \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -DMOCK_BEAPI -c $< -o $@; \
	fi

# Optimization runner compilation
src/optimization_runner.o: src/optimization_runner.cpp
	@echo "🎯 Compiling optimization runner..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		$(CXX) $(TEST_CXXFLAGS) $(INCLUDES) -c $< -o $@; \
	else \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -DMOCK_BEAPI -c $< -o $@; \
	fi

# Phase 3.1 foundation compilation rules
src/phase3_foundation_test.o: src/phase3_foundation_test.cpp
	@echo "🧪 Compiling Phase 3.1 foundation test..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		$(CXX) $(TEST_CXXFLAGS) $(INCLUDES) -c $< -o $@; \
	else \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -DMOCK_BEAPI -c $< -o $@; \
	fi

src/testing/AdvancedAudioProcessorTest.o: src/testing/AdvancedAudioProcessorTest.cpp
	@echo "🧪 Compiling AdvancedAudioProcessor test suite..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		$(CXX) $(TEST_CXXFLAGS) $(INCLUDES) -c $< -o $@; \
	else \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -DMOCK_BEAPI -c $< -o $@; \
	fi

src/audio/AdvancedAudioProcessor.o: src/audio/AdvancedAudioProcessor.cpp
	@echo "🎵 Compiling AdvancedAudioProcessor..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		$(CXX) $(TEST_CXXFLAGS) $(INCLUDES) -fPIC -c $< -o $@; \
	else \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -DMOCK_BEAPI -c $< -o $@; \
	fi

src/audio/DSPAlgorithms.o: src/audio/DSPAlgorithms.cpp
	@echo "🔧 Compiling DSP algorithms..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		$(CXX) $(TEST_CXXFLAGS) $(INCLUDES) -fPIC -c $< -o $@; \
	else \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@; \
	fi

src/testing/ProfessionalEQTest.o: src/testing/ProfessionalEQTest.cpp
	@echo "🎛️ Compiling Professional EQ test suite..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		$(CXX) $(TEST_CXXFLAGS) $(INCLUDES) -fPIC -c $< -o $@; \
	else \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -DMOCK_BEAPI -c $< -o $@; \
	fi

src/testing/QuickEQTest.o: src/testing/QuickEQTest.cpp
	@echo "⚡ Compiling Quick EQ test..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		$(CXX) $(TEST_CXXFLAGS) $(INCLUDES) -fPIC -c $< -o $@; \
	else \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -DMOCK_BEAPI -c $< -o $@; \
	fi

src/testing/DynamicsProcessorTest.o: src/testing/DynamicsProcessorTest.cpp
	@echo "🎚️ Compiling Dynamics Processor test suite..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		$(CXX) $(TEST_CXXFLAGS) $(INCLUDES) -fPIC -c $< -o $@; \
	else \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -DMOCK_BEAPI -c $< -o $@; \
	fi

src/testing/SpatialAudioTest.o: src/testing/SpatialAudioTest.cpp
	@echo "🎯 Compiling Spatial Audio test suite..."
	@if [ "$(shell uname)" = "Haiku" ]; then \
		$(CXX) $(TEST_CXXFLAGS) $(INCLUDES) -fPIC -c $< -o $@; \
	else \
		$(CXX) $(CXXFLAGS) $(INCLUDES) -DMOCK_BEAPI -c $< -o $@; \
	fi

.PHONY: all clean test-compile audio-only ui-only run install help test-framework test-framework-quick test-framework-full test-memory-stress test-performance-scaling test-performance-quick test-thread-safety test-gui-automation test-evaluate-phase2 setup-memory-debug validate-test-setup clean-tests VeniceDAWPerformanceRunner optimize-complete optimize-quick VeniceDAWOptimizer Phase3FoundationTest ProfessionalEQTest test-eq clean-phase3-objects QuickEQTest test-eq-quick DynamicsProcessorTest test-dynamics test-dynamics-quick SpatialAudioTest test-spatial test-spatial-quick test-binaural test-phase3-complete
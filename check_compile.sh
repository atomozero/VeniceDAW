#!/bin/bash

echo "VeniceDAW Performance Station - Compilation Check"
echo "=================================================="

# Test main components for syntax errors
echo "Testing main_weather_benchmark.cpp syntax..."
g++ -std=c++17 -fsyntax-only -I. -Isrc src/main_weather_benchmark.cpp 2>/dev/null
if [ $? -eq 0 ]; then
    echo "✅ main_weather_benchmark.cpp syntax OK"
else
    echo "❌ main_weather_benchmark.cpp has syntax errors"
fi

echo "Testing WeatherBenchmarkWindow.cpp syntax..."
g++ -std=c++17 -fsyntax-only -I. -Isrc src/gui/WeatherBenchmarkWindow.cpp 2>/dev/null
if [ $? -eq 0 ]; then
    echo "✅ WeatherBenchmarkWindow.cpp syntax OK"
else
    echo "❌ WeatherBenchmarkWindow.cpp has syntax errors"
fi

echo "Testing PerformanceStation.cpp syntax..."
g++ -std=c++17 -fsyntax-only -I. -Isrc src/benchmark/PerformanceStation.cpp 2>/dev/null
if [ $? -eq 0 ]; then
    echo "✅ PerformanceStation.cpp syntax OK"
else
    echo "❌ PerformanceStation.cpp has syntax errors"
fi

echo ""
echo "Cross-compilation check complete!"
echo "Note: Full compilation requires native Haiku headers"
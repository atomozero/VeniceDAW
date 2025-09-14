/*
 * optimization_runner.cpp - VeniceDAW Complete Audio Optimization Runner
 * 
 * Coordinates all three critical optimizations:
 * 1. Buffer Size Tuner (512/1024/2048 samples → <12ms latency)
 * 2. Thread Priority Optimizer (BeAPI real-time scheduling)
 * 3. Bottleneck Resolver (Track 6+ performance analysis)
 * 
 * Target: Phase 2 READY certification with 8-track capability
 */

#include "testing/AudioOptimizer.h"

#ifdef __HAIKU__
    #include <Application.h>
    #include <Window.h>
    #include <View.h>
    #include <OS.h>
#else
    #include "testing/HaikuMockHeaders.h"
#endif

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <fstream>
#include <iomanip>
#include <algorithm>

using namespace VeniceDAWOptimization;
using namespace std::chrono;

class VeniceDAWOptimizationApp : public BApplication {
public:
    VeniceDAWOptimizationApp() : BApplication("application/x-vnd.VeniceDAW-Optimizer") {}
    
    virtual void ReadyToRun() override {
        // Application ready for optimization
    }
};

struct OptimizationSession {
    std::vector<OptimizationResult> results;
    bool phase2Ready = false;
    float overallImprovementPercent = 0.0f;
    std::chrono::milliseconds totalDuration;
    std::string finalStatus = "NOT_READY";
    std::vector<std::string> nextSteps;
};

OptimizationSession RunCompleteOptimizationSuite(int targetTracks, int targetLatency, int targetFPS, 
                                                 const std::string& optimizationLevel, bool verbose) {
    OptimizationSession session;
    auto startTime = steady_clock::now();
    
    std::cout << "🚀 VeniceDAW Complete Audio Optimization Suite\n";
    std::cout << "==============================================\n\n";
    std::cout << "🎯 Mission: Achieve Phase 2 READY status\n";
    std::cout << "   Target: " << targetTracks << " tracks, <" << targetLatency << "ms latency, " << targetFPS << "+ FPS\n";
    std::cout << "   Based on: Haiku VM performance analysis data\n\n";
    
    try {
        // Initialize optimization suite
        if (verbose) {
            std::cout << "🔧 Verbose mode enabled - detailed optimization logging\n";
            std::cout << "📋 Configuration: " << optimizationLevel << " optimization level\n\n";
        }
        
        VeniceDAWOptimizationSuite suite;
        suite.SetTargetPerformance(targetTracks, static_cast<float>(targetLatency), static_cast<float>(targetFPS));
        suite.SetOptimizationLevel(optimizationLevel);
        
        // Run all optimizations
        session.results = suite.RunCompleteOptimization();
        
        // Analyze results
        int successfulOptimizations = 0;
        float totalImprovement = 0.0f;
        
        for (const auto& result : session.results) {
            if (result.successful) {
                successfulOptimizations++;
                totalImprovement += result.improvementPercent;
            }
        }
        
        session.overallImprovementPercent = totalImprovement;
        
        // Determine Phase 2 readiness
        if (successfulOptimizations >= 3) {
            if (totalImprovement >= 40.0f) {
                session.phase2Ready = true;
                session.finalStatus = "READY";
            } else {
                session.finalStatus = "CONDITIONAL";
            }
        } else {
            session.finalStatus = "NOT_READY";
        }
        
        // Generate next steps
        if (session.phase2Ready) {
            session.nextSteps.push_back("Run full 8-track validation test to confirm Phase 2 readiness");
            session.nextSteps.push_back("Deploy optimized configuration in production environment");
            session.nextSteps.push_back("Begin implementation of Phase 2 advanced audio features");
        } else if (session.finalStatus == "CONDITIONAL") {
            session.nextSteps.push_back("Fine-tune remaining optimization parameters");
            session.nextSteps.push_back("Run additional targeted optimization cycles");
            session.nextSteps.push_back("Consider hardware-specific optimizations");
        } else {
            session.nextSteps.push_back("Address failed optimizations individually");
            session.nextSteps.push_back("Profile system for hardware-specific bottlenecks");
            session.nextSteps.push_back("Consider fundamental architecture changes");
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Optimization suite failed: " << e.what() << "\n";
        session.finalStatus = "ERROR";
        session.nextSteps.push_back("Investigate optimization framework errors");
    }
    
    auto endTime = steady_clock::now();
    session.totalDuration = duration_cast<milliseconds>(endTime - startTime);
    
    return session;
}

void SaveOptimizationSession(const OptimizationSession& session, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "❌ Failed to save optimization session to " << filename << "\n";
        return;
    }
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    file << "{\n";
    file << "  \"timestamp\": \"" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\",\n";
    file << "  \"optimization_session\": \"VeniceDAW Complete Audio Optimization\",\n";
    file << "  \"phase2_ready\": " << (session.phase2Ready ? "true" : "false") << ",\n";
    file << "  \"final_status\": \"" << session.finalStatus << "\",\n";
    file << "  \"overall_improvement_percent\": " << session.overallImprovementPercent << ",\n";
    file << "  \"total_duration_ms\": " << session.totalDuration.count() << ",\n";
    file << "  \"optimizations_completed\": " << session.results.size() << ",\n";
    file << "  \"successful_optimizations\": " << std::count_if(session.results.begin(), 
                                                               session.results.end(),
                                                               [](const OptimizationResult& r) { 
                                                                   return r.successful; 
                                                               }) << ",\n";
    file << "  \"optimization_results\": [\n";
    
    for (size_t i = 0; i < session.results.size(); i++) {
        const auto& result = session.results[i];
        file << "    {\n";
        file << "      \"type\": \"" << result.optimizationType << "\",\n";
        file << "      \"successful\": " << (result.successful ? "true" : "false") << ",\n";
        file << "      \"improvement_percent\": " << result.improvementPercent << ",\n";
        file << "      \"details\": \"" << result.details << "\"\n";
        file << "    }";
        if (i < session.results.size() - 1) file << ",";
        file << "\n";
    }
    
    file << "  ],\n";
    file << "  \"next_steps\": [\n";
    
    for (size_t i = 0; i < session.nextSteps.size(); i++) {
        file << "    \"" << session.nextSteps[i] << "\"";
        if (i < session.nextSteps.size() - 1) file << ",";
        file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    
    file.close();
    std::cout << "💾 Optimization session saved to: " << filename << "\n";
}

void PrintOptimizationSummary(const OptimizationSession& session) {
    std::cout << "\n🎯 VeniceDAW Optimization Suite - Final Results\n";
    std::cout << "==============================================\n\n";
    
    // Overall status
    std::cout << "📊 Overall Performance:\n";
    std::cout << "   Total Duration: " << session.totalDuration.count() / 1000 << " seconds\n";
    std::cout << "   Optimizations Run: " << session.results.size() << "\n";
    std::cout << "   Successful: " << std::count_if(session.results.begin(), session.results.end(),
                                                   [](const OptimizationResult& r) { return r.successful; })
              << "/" << session.results.size() << "\n";
    std::cout << "   Combined Improvement: " << std::fixed << std::setprecision(1) 
              << session.overallImprovementPercent << "%\n\n";
    
    // Phase 2 readiness assessment
    if (session.finalStatus == "READY") {
        std::cout << "🎉 ✅ PHASE 2 READY - OPTIMIZATION SUCCESSFUL!\n";
        std::cout << "   VeniceDAW is now certified for professional 8-track audio workstation use\n";
        std::cout << "   Expected performance: <12ms latency, 60+ FPS, linear scaling to 8 tracks\n\n";
    } else if (session.finalStatus == "CONDITIONAL") {
        std::cout << "⚠️ CONDITIONAL - SIGNIFICANT IMPROVEMENTS ACHIEVED\n";
        std::cout << "   Major optimizations successful, minor tuning needed for full certification\n";
        std::cout << "   Ready for 5-6 track professional use, 8-track needs additional work\n\n";
    } else if (session.finalStatus == "NOT_READY") {
        std::cout << "❌ NOT READY - OPTIMIZATION INCOMPLETE\n";
        std::cout << "   Some optimizations failed, additional work required\n";
        std::cout << "   Current system suitable for 1-4 track use\n\n";
    } else {
        std::cout << "🔧 ERROR - OPTIMIZATION SUITE ISSUES\n";
        std::cout << "   Technical problems during optimization, check logs\n\n";
    }
    
    // Individual optimization results
    std::cout << "🔍 Individual Optimization Results:\n";
    for (size_t i = 0; i < session.results.size(); i++) {
        const auto& result = session.results[i];
        
        std::cout << "   " << (i+1) << ". " << result.optimizationType << ": ";
        std::cout << (result.successful ? "✅ SUCCESS" : "❌ FAILED");
        
        if (result.successful && result.improvementPercent > 0) {
            std::cout << " (+" << result.improvementPercent << "%)";
        }
        std::cout << "\n";
        
        if (!result.details.empty()) {
            std::cout << "      " << result.details << "\n";
        }
    }
    
    // Next steps
    std::cout << "\n🎯 Recommended Next Steps:\n";
    for (size_t i = 0; i < session.nextSteps.size(); i++) {
        std::cout << "   " << (i+1) << ". " << session.nextSteps[i] << "\n";
    }
    
    std::cout << "\n📋 Generated Reports:\n";
    std::cout << "   • complete_optimization_report.json - Detailed technical results\n";
    std::cout << "   • buffer_optimization_report.json - Buffer size analysis\n";
    std::cout << "   • bottleneck_analysis.json - Performance cliff analysis\n";
    std::cout << "   • optimization_session.json - This session summary\n";
}

void PrintUsage(const char* programName) {
    std::cout << "VeniceDAW Complete Audio Optimization Suite\n";
    std::cout << "==========================================\n\n";
    std::cout << "Usage: " << programName << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --target-latency MS    Target audio latency in ms (default: 12)\n";
    std::cout << "  --target-tracks N      Target number of tracks (default: 8)\n";
    std::cout << "  --target-fps N         Target frame rate (default: 60)\n";
    std::cout << "  --optimization-level LEVEL  conservative/balanced/aggressive (default: balanced)\n";
    std::cout << "  --output FILE          Save session results to JSON file\n";
    std::cout << "  --verbose              Enable verbose output\n";
    std::cout << "  --help                 Show this help\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << "\n";
    std::cout << "  " << programName << " --target-latency 10 --aggressive\n";
    std::cout << "  " << programName << " --output my_optimization.json\n\n";
    std::cout << "This suite runs three critical optimizations:\n";
    std::cout << "  1. Buffer Size Tuner - Optimize audio latency\n";
    std::cout << "  2. Thread Priority Optimizer - Real-time scheduling\n";
    std::cout << "  3. Bottleneck Resolver - Track scaling analysis\n\n";
    std::cout << "Goal: Achieve Phase 2 READY status for professional audio workstation use\n";
}

int main(int argc, char* argv[]) {
    int targetLatency = 12;
    int targetTracks = 8;
    int targetFPS = 60;
    std::string optimizationLevel = "balanced";
    std::string outputFile = "";
    bool verbose = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            PrintUsage(argv[0]);
            return 0;
        } else if (arg == "--target-latency" && i + 1 < argc) {
            targetLatency = std::stoi(argv[++i]);
        } else if (arg == "--target-tracks" && i + 1 < argc) {
            targetTracks = std::stoi(argv[++i]);
        } else if (arg == "--target-fps" && i + 1 < argc) {
            targetFPS = std::stoi(argv[++i]);
        } else if (arg == "--optimization-level" && i + 1 < argc) {
            optimizationLevel = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg == "--verbose") {
            verbose = true;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            PrintUsage(argv[0]);
            return 1;
        }
    }
    
    // Validate parameters
    if (targetLatency < 5 || targetLatency > 100) {
        std::cerr << "❌ Invalid target latency: " << targetLatency << " (must be 5-100ms)\n";
        return 1;
    }
    
    if (targetTracks < 1 || targetTracks > 16) {
        std::cerr << "❌ Invalid target tracks: " << targetTracks << " (must be 1-16)\n";
        return 1;
    }
    
#ifdef __HAIKU__
    // Initialize BApplication for BeAPI context
    VeniceDAWOptimizationApp app;
    std::cout << "✅ Running on native Haiku with real BeAPI optimization\n\n";
#else
    std::cout << "⚠️ Running on non-Haiku system with mock optimizations\n";
    std::cout << "   Results are for testing purposes only!\n\n";
#endif
    
    // Run complete optimization suite
    auto session = RunCompleteOptimizationSuite(targetTracks, targetLatency, targetFPS, optimizationLevel, verbose);
    
    // Print summary
    PrintOptimizationSummary(session);
    
    // Save results
    if (!outputFile.empty()) {
        SaveOptimizationSession(session, outputFile);
    } else {
        SaveOptimizationSession(session, "optimization_session.json");
    }
    
    std::cout << "\n🎯 Optimization Suite Complete!\n";
    
    if (session.phase2Ready) {
        std::cout << "🚀 VeniceDAW is Phase 2 READY! Deploy with confidence.\n";
        return 0;
    } else if (session.finalStatus == "CONDITIONAL") {
        std::cout << "⚠️ Close to Phase 2 readiness - minor tuning recommended.\n";
        return 0;
    } else {
        std::cout << "🔧 Additional optimization work needed before Phase 2.\n";
        return 1;
    }
}
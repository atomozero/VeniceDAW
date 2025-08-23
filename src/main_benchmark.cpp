/*
 * main_benchmark.cpp - Unified benchmark executable
 */

#include "benchmark/PerformanceStation.h"
#include <Application.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace HaikuDAW;

// Forward declaration
void RunBenchmarkTests(int argc, char* argv[]);

// BApplication for benchmark with proper message handling
class BenchmarkApp : public BApplication {
public:
    BenchmarkApp(int argc, char** argv) 
        : BApplication("application/x-vnd.HaikuMix-Benchmark")
        , fArgc(argc)
        , fArgv(argv) {}
    
    virtual void ReadyToRun() override {
        // Run the benchmark tests in the app context
        RunBenchmarkTests(fArgc, fArgv);
        
        // Quit the app when done
        be_app->PostMessage(B_QUIT_REQUESTED);
    }
    
    virtual bool QuitRequested() override {
        // Allow quit
        return true;
    }
    
private:
    int fArgc;
    char** fArgv;
};

void PrintUsage(const char* progName)
{
    printf("HaikuMix Unified Benchmark Suite\n");
    printf("=================================\n\n");
    printf("Usage: %s [options]\n\n", progName);
    printf("Options:\n");
    printf("  --all              Run all benchmark tests (default)\n");
    printf("  --audio            Run only audio performance tests\n");
    printf("  --3d               Run only 3D rendering tests\n");
    printf("  --memory           Run only memory tests\n");
    printf("  --system           Run only system integration tests\n");
    printf("  --quick            Run a quick subset of tests\n");
    printf("  --output FILE      Save results to specified file\n");
    printf("  --help, -h         Show this help message\n\n");
    printf("Examples:\n");
    printf("  %s --all                    # Full benchmark suite\n", progName);
    printf("  %s --3d --output 3d.txt     # 3D tests only, save to file\n", progName);
    printf("  %s --quick                  # Quick performance check\n", progName);
}

void RunBenchmarkTests(int argc, char* argv[])
{
    // Parse command line arguments
    bool runAudio = false;
    bool run3D = false;
    bool runMemory = false;
    bool runSystem = false;
    bool runAll = true;
    bool quickMode = false;
    const char* outputFile = nullptr;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            PrintUsage(argv[0]);
            return;
        }
        else if (strcmp(argv[i], "--all") == 0) {
            runAll = true;
        }
        else if (strcmp(argv[i], "--audio") == 0) {
            runAudio = true;
            runAll = false;
        }
        else if (strcmp(argv[i], "--3d") == 0) {
            run3D = true;
            runAll = false;
        }
        else if (strcmp(argv[i], "--memory") == 0) {
            runMemory = true;
            runAll = false;
        }
        else if (strcmp(argv[i], "--system") == 0) {
            runSystem = true;
            runAll = false;
        }
        else if (strcmp(argv[i], "--quick") == 0) {
            quickMode = true;
            runAll = false;
        }
        else if (strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                outputFile = argv[++i];
            } else {
                printf("Error: --output requires a filename\n");
                return;
            }
        }
        else {
            printf("Error: Unknown option '%s'\n", argv[i]);
            PrintUsage(argv[0]);
            return;
        }
    }
    
    // Create benchmark instance
    PerformanceStation* benchmark = new PerformanceStation();
    
    try {
        // Run selected tests
        if (runAll) {
            printf("Running complete benchmark suite...\n\n");
            benchmark->RunAllTests();
        } else {
            printf("Running selected benchmark tests...\n\n");
            
            // Print system info for all modes
            benchmark->PrintSystemInfo();
            
            if (quickMode) {
                printf("\n[Quick Performance Check]\n");
                printf("-------------------------\n");
                benchmark->TestAudioEngine();
                benchmark->Test3DRenderingFPS();
                benchmark->TestMemoryUsage();
            } else {
                if (runAudio) {
                    printf("\n[Audio Performance Tests]\n");
                    printf("-------------------------\n");
                    benchmark->TestAudioEngine();
                    benchmark->TestAudioLatency();
                    benchmark->TestSineGeneration();
                    benchmark->TestBufferProcessing();
                }
                
                if (run3D) {
                    printf("\n[3D Mixer Rendering Tests]\n");
                    printf("---------------------------\n");
                    benchmark->Test3DRenderingFPS();
                    benchmark->Test3DAnimationSmooth();
                    benchmark->Test3DInteractionLatency();
                }
                
                if (runMemory) {
                    printf("\n[Memory Performance Tests]\n");
                    printf("--------------------------\n");
                    benchmark->TestMemoryUsage();
                    benchmark->TestMemoryBandwidth();
                }
                
                if (runSystem) {
                    printf("\n[System Integration Tests]\n");
                    printf("--------------------------\n");
                    benchmark->TestRealtimePerformance();
                    benchmark->TestCPUScaling();
                }
            }
            
            // Generate report for partial runs
            benchmark->GenerateFinalReport();
        }
        
        // Save to custom output file if specified
        if (outputFile) {
            benchmark->SaveReport(outputFile);
            printf("Results saved to: %s\n", outputFile);
        }
        
        // Performance summary
        printf("\n");
        printf("=====================================\n");
        printf("Summary: %.1f/100 ", benchmark->GetTotalScore());
        
        if (benchmark->GetTotalScore() >= 90) {
            printf("(EXCELLENT)\n");
        } else if (benchmark->GetTotalScore() >= 75) {
            printf("(VERY GOOD)\n");
        } else if (benchmark->GetTotalScore() >= 60) {
            printf("(GOOD)\n");
        } else if (benchmark->GetTotalScore() >= 45) {
            printf("(FAIR)\n");
        } else {
            printf("(NEEDS IMPROVEMENT)\n");
        }
        printf("=====================================\n");
        
    } catch (...) {
        printf("Error: Benchmark crashed, cleaning up...\n");
    }
    
    // Proper cleanup
    delete benchmark;
    
    // Extended delay to ensure all child processes terminate cleanly
    printf("Finalizing benchmark cleanup...\n");
    snooze(500000); // 500ms for child process cleanup
    
    // Force any remaining child process cleanup
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) {
        // Clean up any remaining zombie processes
    }
}

int main(int argc, char* argv[])
{
    printf("HaikuMix Unified Benchmark Suite v1.0\n");
    printf("======================================\n\n");
    
    // Check if we need 3D tests (which require BApplication)
    bool needs3D = false;
    
    // Quick scan of arguments to determine if we need BApplication
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--all") == 0 || 
            strcmp(argv[i], "--3d") == 0 || 
            strcmp(argv[i], "--quick") == 0) {
            needs3D = true;
            break;
        }
    }
    
    if (needs3D) {
        // Create and run BApplication for 3D tests
        BenchmarkApp app(argc, argv);
        app.Run();  // This will call ReadyToRun which runs the tests
    } else {
        // Run tests without BApplication (audio, memory, system only)
        RunBenchmarkTests(argc, argv);
    }
    
    return 0;
}
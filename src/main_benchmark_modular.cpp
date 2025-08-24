/*
 * main_benchmark_modular.cpp - Entry point for modular benchmark
 */

#include <Application.h>
#include <Path.h>
#include <FindDirectory.h>
#include <File.h>
#include <stdio.h>
#include "benchmark/PerformanceStation2.h"

using namespace HaikuDAW;

class ModularBenchmarkApp : public BApplication {
public:
    ModularBenchmarkApp() : BApplication("application/x-vnd.VeniceDAW-BenchmarkModular") {
        printf("\n===============================================\n");
        printf("   VeniceDAW Modular Benchmark Suite v2.0\n");
        printf("===============================================\n\n");
    }
    
    virtual void ReadyToRun() override {
        // Run benchmark in console mode
        PerformanceStation2 station;
        
        // Set up progress callback for console output
        station.SetProgressCallback([](float progress, const std::string& status, void* userData) {
            printf("[%.0f%%] %s\n", progress * 100.0f, status.c_str());
        }, nullptr);
        
        // Run all tests
        station.RunAllTests();
        
        // Generate and print report
        std::string report = station.GenerateReport(true); // Detailed report
        printf("\n%s\n", report.c_str());
        
        // Write report to file
        BPath path;
        if (find_directory(B_DESKTOP_DIRECTORY, &path) == B_OK) {
            path.Append("benchmark_results_modular.txt");
            BFile file(path.Path(), B_CREATE_FILE | B_WRITE_ONLY | B_ERASE_FILE);
            if (file.InitCheck() == B_OK) {
                file.Write(report.c_str(), report.length());
                printf("Report saved to: %s\n", path.Path());
            }
        }
        
        PostMessage(B_QUIT_REQUESTED);
    }
};

int main(int argc, char** argv)
{
    ModularBenchmarkApp app;
    app.Run();
    return 0;
}
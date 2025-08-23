/*
 * main_benchmark_gui.cpp - GUI Benchmark Application Entry Point
 */

#include <Application.h>
#include <Alert.h>
#include "gui/BenchmarkWindow.h"

using namespace HaikuDAW;

class BenchmarkGUIApp : public BApplication {
public:
    BenchmarkGUIApp() 
        : BApplication("application/x-vnd.HaikuMix-BenchmarkGUI") {
    }
    
    virtual void ReadyToRun() override {
        // Create and show benchmark window
        BenchmarkWindow* window = new BenchmarkWindow(
            BRect(100, 100, 900, 700));
        window->Show();
    }
    
    virtual void AboutRequested() override {
        BAlert* about = new BAlert("About",
            "HaikuMix Performance Benchmark\n"
            "Version 1.0\n\n"
            "A comprehensive benchmark suite for\n"
            "testing audio and 3D performance.\n\n"
            "© 2024 HaikuMix Project",
            "OK", nullptr, nullptr,
            B_WIDTH_AS_USUAL, B_INFO_ALERT);
        about->Go();
    }
};

int main(int argc, char** argv)
{
    BenchmarkGUIApp app;
    app.Run();
    return 0;
}
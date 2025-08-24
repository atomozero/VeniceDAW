/*
 * main_weather_benchmark.cpp - VeniceDAW Performance Station
 */

#include <Application.h>
#include <Alert.h>
#include <Screen.h>
#include "gui/WeatherBenchmarkWindow.h"

using namespace HaikuDAW;

class WeatherBenchmarkApp : public BApplication {
public:
    WeatherBenchmarkApp() : BApplication("application/x-vnd.VeniceDAW-PerformanceStation") {}
    
    virtual void ReadyToRun() override {
        // Center window on screen
        BScreen screen;
        BRect screenFrame = screen.Frame();
        
        BRect windowFrame(0, 0, 1000, 700);
        windowFrame.OffsetTo(
            (screenFrame.Width() - windowFrame.Width()) / 2,
            (screenFrame.Height() - windowFrame.Height()) / 2
        );
        
        WeatherBenchmarkWindow* window = new WeatherBenchmarkWindow(windowFrame);
        window->Show();
    }
    
    virtual void AboutRequested() override {
        BAlert* alert = new BAlert("About", 
            "🌤️ VeniceDAW Weather Performance Station\n"
            "Revolutionary benchmark interface that makes\n"
            "system performance as easy to understand\n"
            "as checking the weather forecast!\n\n"
            "Built with ❤️ for Haiku OS",
            "Cool!");
        alert->Go();
    }
};

int main() {
    WeatherBenchmarkApp app;
    app.Run();
    return 0;
}
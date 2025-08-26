/*
 * main_weather_benchmark.cpp - VeniceDAW Performance Station
 */

#include <Application.h>
#include <Alert.h>
#include <Screen.h>
#include <MessageRunner.h>
#include "gui/PerformanceStationWindow.h"

using namespace HaikuDAW;

class PerformanceStationApp : public BApplication {
public:
    PerformanceStationApp() : BApplication("application/x-vnd.VeniceDAW-PerformanceStation") {}
    
    virtual void ReadyToRun() override {
        // Center window on screen
        BScreen screen;
        BRect screenFrame = screen.Frame();
        
        BRect windowFrame(0, 0, 1000, 700);
        windowFrame.OffsetTo(
            (screenFrame.Width() - windowFrame.Width()) / 2,
            (screenFrame.Height() - windowFrame.Height()) / 2
        );
        
        PerformanceStationWindow* window = new PerformanceStationWindow(windowFrame);
        window->Show();
        
        // Auto-start benchmark immediately (tests already ran in terminal)
        BMessage msg('arun');
        BMessageRunner::StartSending(window, &msg, 500000, 1); // 0.5 seconds
    }
    
    virtual void AboutRequested() override {
        BAlert* alert = new BAlert("About", 
            "📊 VeniceDAW Performance Station\n"
            "Professional audio workstation performance\n"
            "analysis and optimization tool\n"
            "for Haiku OS\n\n"
            "Built with ❤️ for Haiku OS",
            "Cool!");
        alert->Go();
    }
};

int main() {
    PerformanceStationApp app;
    app.Run();
    return 0;
}
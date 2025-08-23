/*
 * main.cpp - HaikuDAW entry point
 * 
 * Simple demo application to showcase the audio engine
 */

#include <app/Application.h>
#include <interface/Alert.h>
#include <stdio.h>
#include "ui/MainWindow.h"

using namespace HaikuDAW;

class HaikuDAWApp : public BApplication {
public:
    HaikuDAWApp();
    virtual ~HaikuDAWApp();
    
    virtual void ReadyToRun();
    virtual bool QuitRequested();
    
private:
    MainWindow* fMainWindow;
};

HaikuDAWApp::HaikuDAWApp()
    : BApplication("application/x-vnd.HaikuDAW")
    , fMainWindow(nullptr)
{
    printf("=== HaikuDAW - Digital Audio Workstation ===\n");
    printf("Modern audio engine demo for Haiku OS\n");
    printf("==========================================\n");
}

HaikuDAWApp::~HaikuDAWApp()
{
    printf("HaikuDAWApp: Application destroyed\n");
}

void HaikuDAWApp::ReadyToRun()
{
    printf("HaikuDAWApp: Launching main window\n");
    
    fMainWindow = new MainWindow();
    if (fMainWindow) {
        fMainWindow->Show();
        printf("HaikuDAWApp: Demo is ready!\n");
        printf("\n");
        printf("Demo Features:\n");
        printf("- Modern BMediaKit audio engine\n");
        printf("- Real-time CPU monitoring\n");
        printf("- Multi-track support\n");
        printf("- Live audio visualization\n");
        printf("- Master volume control\n");
        printf("\n");
        printf("Click 'Start Engine' to begin!\n");
    } else {
        printf("HaikuDAWApp: ERROR - Failed to create main window\n");
        
        BAlert* alert = new BAlert("Error", 
                                   "Failed to create main window.\n"
                                   "Please check your Haiku system.",
                                   "OK");
        alert->Go();
        PostMessage(B_QUIT_REQUESTED);
    }
}

bool HaikuDAWApp::QuitRequested()
{
    printf("HaikuDAWApp: Quit requested\n");
    printf("Thanks for trying HaikuDAW! 🎵\n");
    return true;
}

// Application entry point
int main()
{
    printf("Starting HaikuDAW...\n");
    
    HaikuDAWApp app;
    
    status_t result = app.InitCheck();
    if (result != B_OK) {
        printf("ERROR: Failed to initialize application: %s\n", strerror(result));
        return 1;
    }
    
    app.Run();
    
    printf("HaikuDAW: Clean shutdown\n");
    return 0;
}
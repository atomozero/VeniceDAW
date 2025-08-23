/*
 * main_gui.cpp - HaikuDAW with native GUI
 */

#include "gui/MixerWindow.h"
#include "gui/Mixer3DWindow.h"
#include "gui/SuperMasterWindow.h"
#include "audio/SimpleHaikuEngine.h"
#include <Application.h>
#include <Alert.h>
#include <stdio.h>
#include <vector>
#include <algorithm>

using namespace HaikuDAW;

class HaikuDAWApp : public BApplication {
public:
    HaikuDAWApp();
    virtual ~HaikuDAWApp();
    
    virtual void ReadyToRun();
    virtual bool QuitRequested();
    virtual void MessageReceived(BMessage* message);

private:
    HaikuDAW::SimpleHaikuEngine* fAudioEngine;
    std::vector<HaikuDAW::MixerWindow*> fMixerWindows;  // Multiple mixer windows
    HaikuDAW::Mixer3DWindow* f3DMixerWindow;
    HaikuDAW::SuperMasterWindow* fSuperMasterWindow;  // Global control
    
    // Window management
    void CreateNewMixerWindow();
    void RemoveMixerWindow(HaikuDAW::MixerWindow* window);
    void CheckSuperMasterVisibility();
};

HaikuDAWApp::HaikuDAWApp()
    : BApplication("application/x-vnd.HaikuDAW-GUI")
    , fAudioEngine(nullptr)
    , f3DMixerWindow(nullptr)
    , fSuperMasterWindow(nullptr)
{
    printf("Starting HaikuDAW...\n");
}

HaikuDAWApp::~HaikuDAWApp()
{
    delete fAudioEngine;
    printf("HaikuDAW: GUI Application destroyed\n");
}

void HaikuDAWApp::ReadyToRun()
{
    printf("Initializing audio engine...\n");
    
    // Create audio engine
    fAudioEngine = new HaikuDAW::SimpleHaikuEngine();
    
    // Create first mixer window
    CreateNewMixerWindow();
    
    // Don't create 3D mixer automatically - let user open it when needed
    f3DMixerWindow = nullptr;
    
    printf("Ready!\n");
}

bool HaikuDAWApp::QuitRequested()
{
    printf("HaikuDAW: Application quit requested\n");
    
    // Stop audio engine
    if (fAudioEngine) {
        fAudioEngine->Stop();
    }
    
    // Close all mixer windows
    for (HaikuDAW::MixerWindow* window : fMixerWindows) {
        if (window && !window->IsHidden()) {
            window->Close();
        }
    }
    fMixerWindows.clear();
    
    // Close 3D window
    if (f3DMixerWindow && !f3DMixerWindow->IsHidden()) {
        f3DMixerWindow->Close();
    }
    
    // Close Super Master window
    if (fSuperMasterWindow && !fSuperMasterWindow->IsHidden()) {
        fSuperMasterWindow->Close();
    }
    
    return true;
}

void HaikuDAWApp::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case 'sh3d':  // MSG_SHOW_3D_MIXER
        {
            // Show 3D mixer window if it exists
            if (f3DMixerWindow) {
                if (f3DMixerWindow->IsHidden()) {
                    f3DMixerWindow->Show();
                } else {
                    // Bring to front
                    f3DMixerWindow->Activate();
                }
            } else {
                // Recreate if it was closed
                f3DMixerWindow = new HaikuDAW::Mixer3DWindow(fAudioEngine);
                f3DMixerWindow->Show();
            }
            break;
        }
        
        case '3dcl':  // 3D window closed
        {
            // Set pointer to null when 3D window is closed
            f3DMixerWindow = nullptr;
            break;
        }
        
        case 'newm':  // New mixer window request
        {
            bool addTrack = false;
            message->FindBool("add_track", &addTrack);
            
            CreateNewMixerWindow();
            
            // If requested, add a track to the new window
            if (addTrack && fMixerWindows.size() > 0) {
                HaikuDAW::MixerWindow* newWindow = fMixerWindows.back();
                if (newWindow) {
                    printf("HaikuDAW: Adding first track to new mixer window\n");
                    newWindow->AddTrack();
                }
            }
            break;
        }
        
        case 'rmvm':  // Remove mixer window
        {
            HaikuDAW::MixerWindow* window = nullptr;
            if (message->FindPointer("window", (void**)&window) == B_OK) {
                RemoveMixerWindow(window);
            }
            break;
        }
        
        case 'supc':  // Super master closed
        {
            fSuperMasterWindow = nullptr;
            break;
        }
        
        case 'shsm':  // Show super master request
        {
            if (fMixerWindows.size() >= 2) {
                if (!fSuperMasterWindow) {
                    // Create new super master
                    CheckSuperMasterVisibility();
                } else if (fSuperMasterWindow->IsHidden()) {
                    // Show existing super master
                    fSuperMasterWindow->Show();
                    fSuperMasterWindow->Activate();
                } else {
                    // Bring to front
                    fSuperMasterWindow->Activate();
                }
            } else {
                BAlert* alert = new BAlert("Super Master", 
                    "Super Master is only available when there are 2 or more mixer windows.",
                    "OK");
                alert->Go();
            }
            break;
        }
        
        default:
            BApplication::MessageReceived(message);
            break;
    }
}

// Application entry point
int main()
{
    printf("HaikuDAW - Professional Digital Audio Workstation\n");
    
    HaikuDAWApp app;
    
    status_t result = app.InitCheck();
    if (result != B_OK) {
        printf("ERROR: Failed to initialize application: %s\n", strerror(result));
        return 1;
    }
    
    app.Run();
    
    printf("HaikuDAW: Clean exit\n");
    return 0;
}

// =====================================
// Multi-Window Management
// =====================================

void HaikuDAWApp::CreateNewMixerWindow()
{
    printf("HaikuDAW: Creating new mixer window...\n");
    
    // Safety check - limit to reasonable number of windows
    // 32 tracks max / 8 tracks per window = 4 windows max
    const int kMaxWindows = 4;
    if (fMixerWindows.size() >= kMaxWindows) {
        printf("HaikuDAW: Maximum number of mixer windows (%d) reached\n", kMaxWindows);
        BAlert* alert = new BAlert("Max Windows", 
            "Maximum number of mixer windows (4) has been reached.\n32 tracks total is the limit.",
            "OK");
        alert->Go();
        return;
    }
    
    int windowNumber = fMixerWindows.size() + 1;
    BString windowTitle;
    windowTitle << "HaikuDAW Mixer " << windowNumber;
    
    // Calculate position offset for new windows with wrap-around
    // Limit to 10 windows in a row, then start new row
    int windowIndex = fMixerWindows.size();
    int row = windowIndex / 5;  // New row every 5 windows
    int col = windowIndex % 5;  // Column within row
    
    float xOffset = col * 40;
    float yOffset = row * 40;
    
    // Calculate track range for this window
    int startTrack = fMixerWindows.size() * 8;  // Each window handles 8 tracks
    int maxTracks = 8;
    
    printf("HaikuDAW: Window %d will handle tracks %d-%d\n", 
           windowNumber, startTrack, startTrack + maxTracks - 1);
    
    // Verify audio engine is valid
    if (!fAudioEngine) {
        printf("HaikuDAW: ERROR - Audio engine is null!\n");
        return;
    }
    
    printf("HaikuDAW: Creating MixerWindow object...\n");
    
    try {
        // For the new window, it should start with 1 track and allow up to 8
        HaikuDAW::MixerWindow* newWindow = new HaikuDAW::MixerWindow(fAudioEngine, startTrack, maxTracks);
        
        if (!newWindow) {
            printf("HaikuDAW: ERROR - Failed to create MixerWindow!\n");
            return;
        }
        
        printf("HaikuDAW: MixerWindow created successfully, setting title...\n");
        newWindow->SetTitle(windowTitle.String());
        
        printf("HaikuDAW: Moving window to position...\n");
        // Position window with offset
        newWindow->MoveTo(100 + xOffset, 100 + yOffset);
        
        printf("HaikuDAW: Adding to window list...\n");
        fMixerWindows.push_back(newWindow);
        
        printf("HaikuDAW: Showing window...\n");
        newWindow->Show();
        
        // Check if we need to show/hide super master
        printf("HaikuDAW: Checking super master visibility...\n");
        CheckSuperMasterVisibility();
        
        printf("HaikuDAW: Created mixer window %d (tracks %d-%d) successfully\n", 
               windowNumber, startTrack, startTrack + maxTracks - 1);
               
    } catch (...) {
        printf("HaikuDAW: EXCEPTION caught while creating mixer window!\n");
    }
}

void HaikuDAWApp::RemoveMixerWindow(HaikuDAW::MixerWindow* window)
{
    if (!window || fMixerWindows.size() <= 1) {
        printf("HaikuDAW: Cannot remove window - must keep at least one mixer\n");
        return;
    }
    
    // Find and remove from vector
    auto it = std::find(fMixerWindows.begin(), fMixerWindows.end(), window);
    if (it != fMixerWindows.end()) {
        fMixerWindows.erase(it);
        printf("HaikuDAW: Removed mixer window (remaining: %d)\n", (int)fMixerWindows.size());
        
        // Renumber remaining windows
        for (size_t i = 0; i < fMixerWindows.size(); i++) {
            BString newTitle;
            newTitle << "HaikuDAW Mixer " << (i + 1);
            fMixerWindows[i]->SetTitle(newTitle.String());
        }
        
        // Check if we need to show/hide super master
        CheckSuperMasterVisibility();
    }
}

void HaikuDAWApp::CheckSuperMasterVisibility()
{
    printf("HaikuDAW: Checking super master visibility - %d windows\n", (int)fMixerWindows.size());
    
    // Show Super Master when there are 2 or more mixer windows
    bool shouldShow = (fMixerWindows.size() >= 2);
    
    if (shouldShow && !fSuperMasterWindow) {
        printf("HaikuDAW: Creating Super Master window...\n");
        
        // Verify audio engine is valid
        if (!fAudioEngine) {
            printf("HaikuDAW: ERROR - Cannot create SuperMaster, audio engine is null!\n");
            return;
        }
        
        try {
            // Create and show super master
            fSuperMasterWindow = new HaikuDAW::SuperMasterWindow(fAudioEngine);
            
            if (!fSuperMasterWindow) {
                printf("HaikuDAW: ERROR - Failed to create SuperMasterWindow!\n");
                return;
            }
            
            printf("HaikuDAW: SuperMasterWindow created, setting window count...\n");
            fSuperMasterWindow->SetWindowCount(fMixerWindows.size());
            
            printf("HaikuDAW: Moving SuperMasterWindow...\n");
            fSuperMasterWindow->MoveTo(400, 50);  // Position it to the right
            
            printf("HaikuDAW: Showing SuperMasterWindow...\n");
            fSuperMasterWindow->Show();
            
            printf("HaikuDAW: Starting SuperMaster update timer...\n");
            fSuperMasterWindow->StartUpdateTimer();
            
            printf("HaikuDAW: Super Master window created successfully\n");
        } catch (...) {
            printf("HaikuDAW: EXCEPTION caught while creating SuperMaster!\n");
            fSuperMasterWindow = nullptr;
        }
    } else if (!shouldShow && fSuperMasterWindow) {
        // Hide and destroy super master
        printf("HaikuDAW: Closing Super Master window...\n");
        if (!fSuperMasterWindow->IsHidden()) {
            fSuperMasterWindow->Close();
        }
        fSuperMasterWindow = nullptr;
        printf("HaikuDAW: Super Master window closed\n");
    } else if (shouldShow && fSuperMasterWindow) {
        // Update window count
        printf("HaikuDAW: Updating Super Master window count to %d\n", (int)fMixerWindows.size());
        if (fSuperMasterWindow->Lock()) {
            fSuperMasterWindow->SetWindowCount(fMixerWindows.size());
            fSuperMasterWindow->Unlock();
        } else {
            printf("HaikuDAW: Failed to lock SuperMaster window for update\n");
        }
    }
}
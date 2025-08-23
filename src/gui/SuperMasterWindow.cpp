/*
 * SuperMasterWindow.cpp - Global master control implementation
 */

#include "SuperMasterWindow.h"
#include "MixerWindow.h"  // For LevelMeter class
#include "../audio/SimpleHaikuEngine.h"
#include <Alert.h>
#include <Application.h>
#include <stdio.h>

namespace HaikuDAW {

SuperMasterWindow::SuperMasterWindow(SimpleHaikuEngine* engine)
    : BWindow(BRect(50, 50, 350, 400), "HaikuDAW SUPER Master", 
              B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 
              B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_ZOOMABLE)
    , fEngine(engine)
    , fMainView(nullptr)
    , fControlSection(nullptr)
    , fGlobalVolume(nullptr)
    , fGlobalPlayButton(nullptr)
    , fGlobalStopButton(nullptr)
    , fWindowCountDisplay(nullptr)
    , fStatusDisplay(nullptr)
    , fGlobalLevelLeft(nullptr)
    , fGlobalLevelRight(nullptr)
    , fUpdateRunner(nullptr)
{
    printf("SuperMasterWindow: Constructor started\n");
    
    if (!engine) {
        printf("SuperMasterWindow: ERROR - Audio engine is null!\n");
        return;
    }
    
    printf("SuperMasterWindow: Creating controls...\n");
    CreateControls();
    
    // We'll start the update timer later to avoid initialization issues
    fUpdateRunner = nullptr;
    
    printf("SuperMasterWindow: Setting size limits...\n");
    // Set compact size limits like other master sections
    SetSizeLimits(220.0f, 300.0f, 180.0f, 220.0f);
    
    printf("SuperMasterWindow: Resizing to compact size...\n");
    // Compact size like the master sections
    ResizeTo(240, 200);
    
    printf("SuperMasterWindow: Created global master control successfully\n");
}

SuperMasterWindow::~SuperMasterWindow()
{
    if (fUpdateRunner) {
        delete fUpdateRunner;
    }
    printf("SuperMasterWindow: Destroyed\n");
}

void SuperMasterWindow::CreateControls()
{
    printf("SuperMasterWindow: CreateControls() started\n");
    
    // Create main view with proper background color
    fMainView = new BView("super_main_view", B_WILL_DRAW);
    fMainView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    
    // Create horizontal layout like other masters (VU meter | Volume slider)
    BGroupLayout* mainLayout = new BGroupLayout(B_HORIZONTAL);
    mainLayout->SetSpacing(8);
    mainLayout->SetInsets(15, 15, 15, 15);
    fMainView->SetLayout(mainLayout);
    
    AddChild(fMainView);
    
    printf("SuperMasterWindow: Creating master-style controls...\n");
    
    // Left VU meter
    BGroupLayout* leftMeterLayout = new BGroupLayout(B_VERTICAL);
    leftMeterLayout->SetSpacing(3);
    BView* leftMeterView = new BView("left_meter", B_WILL_DRAW);
    leftMeterView->SetLayout(leftMeterLayout);
    leftMeterView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    
    BStringView* leftLabel = new BStringView("left_label", "L");
    leftLabel->SetAlignment(B_ALIGN_CENTER);
    leftLabel->SetFont(be_bold_font);
    leftMeterLayout->AddView(leftLabel);
    
    fGlobalLevelLeft = new LevelMeter();
    fGlobalLevelLeft->SetExplicitMinSize(BSize(25, 120));
    fGlobalLevelLeft->SetExplicitMaxSize(BSize(30, 150));
    fGlobalLevelLeft->SetExplicitPreferredSize(BSize(28, 140));
    leftMeterLayout->AddView(fGlobalLevelLeft);
    mainLayout->AddView(leftMeterView);
    
    // Right VU meter  
    BGroupLayout* rightMeterLayout = new BGroupLayout(B_VERTICAL);
    rightMeterLayout->SetSpacing(3);
    BView* rightMeterView = new BView("right_meter", B_WILL_DRAW);
    rightMeterView->SetLayout(rightMeterLayout);
    rightMeterView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    
    BStringView* rightLabel = new BStringView("right_label", "R");
    rightLabel->SetAlignment(B_ALIGN_CENTER);
    rightLabel->SetFont(be_bold_font);
    rightMeterLayout->AddView(rightLabel);
    
    fGlobalLevelRight = new LevelMeter();
    fGlobalLevelRight->SetExplicitMinSize(BSize(25, 120));
    fGlobalLevelRight->SetExplicitMaxSize(BSize(30, 150));
    fGlobalLevelRight->SetExplicitPreferredSize(BSize(28, 140));
    rightMeterLayout->AddView(fGlobalLevelRight);
    mainLayout->AddView(rightMeterView);
    
    // Master volume slider (vertical)
    fGlobalVolume = new BSlider("master_vol", "SUPER",
                              new BMessage(MSG_GLOBAL_VOLUME),
                              0, 100, B_VERTICAL);
    fGlobalVolume->SetValue(100);  // 100%
    fGlobalVolume->SetTarget(this);
    fGlobalVolume->SetLimitLabels("0", "100%");
    fGlobalVolume->SetModificationMessage(new BMessage(MSG_GLOBAL_VOLUME));  // Real-time
    fGlobalVolume->SetHashMarks(B_HASH_MARKS_BOTH);
    fGlobalVolume->SetHashMarkCount(5);
    fGlobalVolume->SetExplicitMinSize(BSize(50, 120));
    fGlobalVolume->SetExplicitMaxSize(BSize(60, 150));
    fGlobalVolume->SetExplicitPreferredSize(BSize(55, 140));
    mainLayout->AddView(fGlobalVolume);
    
    // Transport controls in a vertical group on the right
    BGroupLayout* controlLayout = new BGroupLayout(B_VERTICAL);
    BView* controlView = new BView("controls", B_WILL_DRAW);
    controlView->SetLayout(controlLayout);
    controlView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    controlLayout->SetSpacing(5);
    
    fGlobalPlayButton = new BButton("play", "▶", new BMessage(MSG_GLOBAL_PLAY));
    fGlobalStopButton = new BButton("stop", "⏹", new BMessage(MSG_GLOBAL_STOP));
    
    fGlobalPlayButton->SetTarget(this);
    fGlobalStopButton->SetTarget(this);
    
    controlLayout->AddView(fGlobalPlayButton);
    controlLayout->AddView(fGlobalStopButton);
    
    // Status display
    fStatusDisplay = new BStringView("status", "SUPER");
    fStatusDisplay->SetAlignment(B_ALIGN_CENTER);
    fStatusDisplay->SetFont(be_plain_font);
    controlLayout->AddView(fStatusDisplay);
    
    mainLayout->AddView(controlView);
    
    // Window count display (not used in this layout)
    fWindowCountDisplay = nullptr;
    fControlSection = nullptr;
    
    printf("SuperMasterWindow: CreateControls() completed successfully\n");
}

bool SuperMasterWindow::QuitRequested()
{
    // Notify app that super master is closing
    be_app->PostMessage(new BMessage('supc'));  // Super master closed
    
    // Don't quit the whole app, just close this window
    return true;
}

void SuperMasterWindow::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case MSG_GLOBAL_PLAY:
            if (fEngine) {
                status_t result = fEngine->Start();
                if (result == B_OK) {
                    fStatusDisplay->SetText("Global: Playing All");
                    printf("SuperMasterWindow: Started global playback\n");
                } else {
                    fStatusDisplay->SetText("Global: Error");
                    printf("SuperMasterWindow: Failed to start global playback\n");
                }
            }
            break;
            
        case MSG_GLOBAL_STOP:
            if (fEngine) {
                fEngine->Stop();
                fStatusDisplay->SetText("Global: Stopped All");
                printf("SuperMasterWindow: Stopped global playback\n");
            }
            break;
            
        case MSG_GLOBAL_VOLUME:
            if (fEngine && fGlobalVolume) {
                float volume = fGlobalVolume->Value() / 100.0f;
                fEngine->SetMasterVolume(volume);
                printf("SuperMasterWindow: Global volume %.2f\n", volume);
            }
            break;
            
        case MSG_UPDATE_GLOBAL:
            UpdateMeter();
            break;
            
        default:
            BWindow::MessageReceived(message);
            break;
    }
}

void SuperMasterWindow::UpdateMeter()
{
    // Safety check - make sure window is locked
    if (!LockLooper()) {
        return;
    }
    
    // Update global level meters (same as master levels)
    if (fEngine && fGlobalLevelLeft && fGlobalLevelRight) {
        float peakLeft = fEngine->GetMasterPeakLeft();
        float peakRight = fEngine->GetMasterPeakRight();
        float rmsLeft = fEngine->GetMasterRMSLeft();
        float rmsRight = fEngine->GetMasterRMSRight();
        
        // Apply master volume to display
        float masterVolume = fEngine->GetMasterVolume();
        peakLeft *= masterVolume;
        peakRight *= masterVolume;
        rmsLeft *= masterVolume;
        rmsRight *= masterVolume;
        
        // Check that level meters are valid before updating
        if (fGlobalLevelLeft->Window() && fGlobalLevelRight->Window()) {
            fGlobalLevelLeft->SetLevel(peakLeft, rmsLeft);
            fGlobalLevelRight->SetLevel(peakRight, rmsRight);
        }
    }
    
    // Update status display
    if (fEngine && fStatusDisplay && fStatusDisplay->Window()) {
        BString status;
        if (fEngine->IsRunning()) {
            status << "🎵 Playing";
        } else {
            status << "⏹ Stopped";
        }
        status << " | " << fEngine->GetTrackCount() << " tracks"
               << " | Vol: " << (int)(fEngine->GetMasterVolume() * 100) << "%";
        fStatusDisplay->SetText(status.String());
    }
    
    UnlockLooper();
}

void SuperMasterWindow::SetWindowCount(int count)
{
    if (fStatusDisplay) {
        BString text;
        text << "W:" << count;  // "W:2", "W:3", etc.
        fStatusDisplay->SetText(text.String());
    }
}

void SuperMasterWindow::StartUpdateTimer()
{
    // Don't start if we already have a runner
    if (fUpdateRunner) {
        printf("SuperMasterWindow: Update timer already running\n");
        return;
    }
    
    printf("SuperMasterWindow: Starting update timer...\n");
    
    // Create the message for periodic updates
    BMessage* updateMsg = new BMessage(MSG_UPDATE_GLOBAL);
    
    // Create the runner with 30 FPS update rate
    fUpdateRunner = new BMessageRunner(BMessenger(this), updateMsg, 50000); // 50ms = 20 FPS - optimized
    
    if (fUpdateRunner) {
        status_t result = fUpdateRunner->InitCheck();
        if (result == B_OK) {
            printf("SuperMasterWindow: Update timer started successfully (30 FPS)\n");
            // Don't do immediate update - let the timer handle it
            // UpdateMeter();
        } else {
            printf("SuperMasterWindow: Failed to start update timer: %s\n", strerror(result));
            delete fUpdateRunner;
            fUpdateRunner = nullptr;
        }
    } else {
        printf("SuperMasterWindow: Failed to create BMessageRunner\n");
    }
    
    delete updateMsg;  // BMessageRunner makes its own copy
}

} // namespace HaikuDAW
/*
 * SuperMasterWindow.cpp - Global master control implementation
 */

#include "SuperMasterWindow.h"
#include "MixerWindow.h"  // For LevelMeter class
#include "VeniceTheme.h"
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
    // Constructor started
    
    if (!engine) {
        printf("❌ SuperMasterWindow: ERROR - Audio engine is null!\n");
        return;
    }
    
    // Creating controls
    CreateControls();
    
    // We'll start the update timer later to avoid initialization issues
    fUpdateRunner = nullptr;
    
    // Setting limits
    // Set compact size limits like other master sections
    SetSizeLimits(220.0f, 300.0f, 180.0f, 220.0f);
    
    // Resizing
    // Compact size like the master sections
    ResizeTo(240, 200);
    
    // Master control ready
}

SuperMasterWindow::~SuperMasterWindow()
{
    if (fUpdateRunner) {
        delete fUpdateRunner;
    }
    // Destroyed
}

void SuperMasterWindow::CreateControls()
{
    // Creating controls
    
    // Create main view with VeniceTheme background
    fMainView = new BView("super_main_view", B_WILL_DRAW);
    fMainView->SetViewColor(VeniceDAW::VeniceTheme::PanelBackground());

    // Create horizontal layout using VeniceTheme spacing
    BGroupLayout* mainLayout = new BGroupLayout(B_HORIZONTAL);
    mainLayout->SetSpacing(VeniceDAW::VeniceTheme::SPACING);
    mainLayout->SetInsets(VeniceDAW::VeniceTheme::MARGIN, VeniceDAW::VeniceTheme::MARGIN,
                          VeniceDAW::VeniceTheme::MARGIN, VeniceDAW::VeniceTheme::MARGIN);
    fMainView->SetLayout(mainLayout);
    
    AddChild(fMainView);
    
    printf("SuperMasterWindow: Creating master-style controls...\n");
    
    // Left VU meter - LARGE professional meter
    BGroupLayout* leftMeterLayout = new BGroupLayout(B_VERTICAL);
    leftMeterLayout->SetSpacing(VeniceDAW::VeniceTheme::SPACING);
    BView* leftMeterView = new BView("left_meter", B_WILL_DRAW);
    leftMeterView->SetLayout(leftMeterLayout);
    leftMeterView->SetViewColor(VeniceDAW::VeniceTheme::PanelBackground());

    BStringView* leftLabel = new BStringView("left_label", "L");
    leftLabel->SetAlignment(B_ALIGN_CENTER);
    leftLabel->SetFont(be_bold_font);
    leftMeterLayout->AddView(leftLabel);

    fGlobalLevelLeft = new LevelMeter();
    // Large professional VU meters for SuperMaster (Phase 4.2)
    float largeMeterWidth = VeniceDAW::VeniceTheme::METER_WIDTH * 2.5f;  // 20px
    float largeMeterHeight = 180.0f;
    fGlobalLevelLeft->SetExplicitMinSize(BSize(largeMeterWidth, largeMeterHeight));
    fGlobalLevelLeft->SetExplicitMaxSize(BSize(largeMeterWidth + 5, largeMeterHeight + 20));
    fGlobalLevelLeft->SetExplicitPreferredSize(BSize(largeMeterWidth, largeMeterHeight));
    leftMeterLayout->AddView(fGlobalLevelLeft);
    mainLayout->AddView(leftMeterView);

    // Right VU meter - LARGE professional meter
    BGroupLayout* rightMeterLayout = new BGroupLayout(B_VERTICAL);
    rightMeterLayout->SetSpacing(VeniceDAW::VeniceTheme::SPACING);
    BView* rightMeterView = new BView("right_meter", B_WILL_DRAW);
    rightMeterView->SetLayout(rightMeterLayout);
    rightMeterView->SetViewColor(VeniceDAW::VeniceTheme::PanelBackground());

    BStringView* rightLabel = new BStringView("right_label", "R");
    rightLabel->SetAlignment(B_ALIGN_CENTER);
    rightLabel->SetFont(be_bold_font);
    rightMeterLayout->AddView(rightLabel);

    fGlobalLevelRight = new LevelMeter();
    fGlobalLevelRight->SetExplicitMinSize(BSize(largeMeterWidth, largeMeterHeight));
    fGlobalLevelRight->SetExplicitMaxSize(BSize(largeMeterWidth + 5, largeMeterHeight + 20));
    fGlobalLevelRight->SetExplicitPreferredSize(BSize(largeMeterWidth, largeMeterHeight));
    rightMeterLayout->AddView(fGlobalLevelRight);
    mainLayout->AddView(rightMeterView);
    
    // Master volume slider (vertical) - larger for SuperMaster
    fGlobalVolume = new BSlider("master_vol", "SUPER",
                              new BMessage(MSG_GLOBAL_VOLUME),
                              0, 100, B_VERTICAL);
    fGlobalVolume->SetValue(100);  // 100%
    fGlobalVolume->SetTarget(this);
    fGlobalVolume->SetLimitLabels("0", "100%");
    fGlobalVolume->SetModificationMessage(new BMessage(MSG_GLOBAL_VOLUME));
    fGlobalVolume->SetHashMarks(B_HASH_MARKS_BOTH);
    fGlobalVolume->SetHashMarkCount(5);
    fGlobalVolume->SetExplicitMinSize(BSize(60, largeMeterHeight));
    fGlobalVolume->SetExplicitMaxSize(BSize(70, largeMeterHeight + 20));
    fGlobalVolume->SetExplicitPreferredSize(BSize(65, largeMeterHeight));
    mainLayout->AddView(fGlobalVolume);

    // Transport controls in a vertical group with VeniceTheme
    BGroupLayout* controlLayout = new BGroupLayout(B_VERTICAL);
    controlLayout->SetSpacing(VeniceDAW::VeniceTheme::SPACING);
    BView* controlView = new BView("controls", B_WILL_DRAW);
    controlView->SetLayout(controlLayout);
    controlView->SetViewColor(VeniceDAW::VeniceTheme::PanelBackground());

    fGlobalPlayButton = new BButton("play", "▶", new BMessage(MSG_GLOBAL_PLAY));
    fGlobalPlayButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, VeniceDAW::VeniceTheme::BUTTON_HEIGHT));
    fGlobalStopButton = new BButton("stop", "⏹", new BMessage(MSG_GLOBAL_STOP));
    fGlobalStopButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, VeniceDAW::VeniceTheme::BUTTON_HEIGHT));

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

    // Use try-catch to ensure unlock happens even if exception occurs
    try {
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
    } catch (...) {
        // Ensure we always unlock, even on exception
        UnlockLooper();
        throw;  // Re-throw exception after unlocking
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
/*
 * MixerWindow.cpp - Native Haiku mixer implementation
 */

#include "MixerWindow.h"
#include "../audio/SimpleHaikuEngine.h"
#include <Alert.h>
#include <Application.h>
#include <SpaceLayoutItem.h>
#include <stdio.h>
#include <algorithm>
#include <cmath>

namespace HaikuDAW {

// =====================================
// ToggleButton Implementation
// =====================================

ToggleButton::ToggleButton(const char* name, const char* label, BMessage* message)
    : BButton(name, label, message)
    , fToggled(false)
    , fNormalColor({216, 216, 216, 255})  // Light grey
    , fPressedColor({255, 100, 100, 255})  // Light red
{
}

ToggleButton::~ToggleButton()
{
}

void ToggleButton::MouseDown(BPoint where)
{
    // Toggle state on mouse down
    fToggled = !fToggled;
    
    // Send message immediately
    if (Message() && Target()) {
        BMessage copy(*Message());
        copy.AddBool("toggled", fToggled);
        Target()->Looper()->PostMessage(&copy, Target());
    }
    
    // Visual feedback
    Invalidate();
    
    // Don't call BButton::MouseDown() as it would send the message again
}

void ToggleButton::Draw(BRect updateRect)
{
    // Set background color based on toggle state
    if (fToggled) {
        SetViewColor(fPressedColor);
        SetLowColor(fPressedColor);
    } else {
        SetViewColor(fNormalColor);
        SetLowColor(fNormalColor);
    }
    
    // Draw as normal button
    BButton::Draw(updateRect);
}

void ToggleButton::SetToggled(bool toggled)
{
    if (fToggled != toggled) {
        fToggled = toggled;
        Invalidate();
    }
}

void ToggleButton::SetToggleColors(rgb_color normal, rgb_color pressed)
{
    fNormalColor = normal;
    fPressedColor = pressed;
    Invalidate();
}

// =====================================
// ChannelStrip Implementation
// =====================================

ChannelStrip::ChannelStrip(SimpleTrack* track)
    : BView(BRect(0, 0, 130, 380), track ? track->GetName() : "EmptyStrip", B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW)
    , fTrack(track)
    , fTrackName(nullptr)
    , fVolumeSlider(nullptr)
    , fPanSlider(nullptr)
    , fMuteButton(nullptr)
    , fLevelMeter(nullptr)
    , fSoloButton(nullptr)
{
    if (!track) {
        printf("ChannelStrip: WARNING - Creating empty channel strip\n");
        // Create a disabled/empty strip view
        SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_2_TINT));
        // SetEnabled(false); // Not available in BView
        return;
    }
    
    printf("ChannelStrip: Creating channel strip for track '%s' (ID: %d)\n", 
           track->GetName(), track->GetId());
           
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

ChannelStrip::~ChannelStrip()
{
}

void ChannelStrip::AttachedToWindow()
{
    BView::AttachedToWindow();
    CreateControls();
}

void ChannelStrip::CreateControls()
{
    // Create main vertical layout
    BGroupLayout* mainLayout = new BGroupLayout(B_VERTICAL);
    SetLayout(mainLayout);
    mainLayout->SetSpacing(5);
    mainLayout->SetInsets(5, 5, 5, 5);
    
    // Set channel strip size - compact but functional
    SetExplicitMinSize(BSize(120, 350));
    SetExplicitMaxSize(BSize(150, 450));
    SetExplicitPreferredSize(BSize(130, 380));
    
    // Check if track exists before creating controls
    if (!fTrack) {
        // Create placeholder for empty strip
        BStringView* emptyLabel = new BStringView("empty", "No Track");
        emptyLabel->SetAlignment(B_ALIGN_CENTER);
        mainLayout->AddView(emptyLabel);
        mainLayout->AddItem(BSpaceLayoutItem::CreateGlue());
        return;
    }
    
    // Track name at the top
    fTrackName = new BStringView("name", fTrack->GetName());
    fTrackName->SetAlignment(B_ALIGN_CENTER);
    mainLayout->AddView(fTrackName);
    
    // Create horizontal group for VU meter and volume slider side by side
    BGroupLayout* metersLayout = new BGroupLayout(B_HORIZONTAL);
    metersLayout->SetSpacing(3);
    BView* metersContainer = new BView("meters", B_WILL_DRAW);
    metersContainer->SetLayout(metersLayout);
    
    // Level meter (VU meter) on the left
    fLevelMeter = new LevelMeter();
    fLevelMeter->SetExplicitMinSize(BSize(25, 150));
    fLevelMeter->SetExplicitMaxSize(BSize(30, 200));
    fLevelMeter->SetExplicitPreferredSize(BSize(28, 180));
    metersLayout->AddView(fLevelMeter);
    
    // Volume slider on the right, parallel to VU meter
    fVolumeSlider = new BSlider("volume", "Vol", 
                               new BMessage(MSG_VOLUME_CHANGED),
                               0, 200, B_VERTICAL);
    fVolumeSlider->SetValue((int)(fTrack->GetVolume() * 100));
    fVolumeSlider->SetTarget(this);
    fVolumeSlider->SetLimitLabels("0", "200%");
    fVolumeSlider->SetModificationMessage(new BMessage(MSG_VOLUME_CHANGED));  // Real-time updates
    fVolumeSlider->SetHashMarks(B_HASH_MARKS_BOTH);  // Mostra le tacche
    fVolumeSlider->SetHashMarkCount(5);  // 0, 50, 100, 150, 200
    fVolumeSlider->SetExplicitMinSize(BSize(60, 150));
    fVolumeSlider->SetExplicitMaxSize(BSize(70, 200));
    fVolumeSlider->SetExplicitPreferredSize(BSize(65, 180));
    metersLayout->AddView(fVolumeSlider);
    
    // Add the meters container to main layout
    mainLayout->AddView(metersContainer);
    
    // Pan slider (horizontal) 
    fPanSlider = new BSlider("pan", "Pan",
                            new BMessage(MSG_PAN_CHANGED),
                            -100, 100, B_HORIZONTAL);
    fPanSlider->SetValue((int)(fTrack->GetPan() * 100));  // Convert -1..1 to -100..100
    fPanSlider->SetTarget(this);
    fPanSlider->SetLimitLabels("L", "R");
    fPanSlider->SetModificationMessage(new BMessage(MSG_PAN_CHANGED));  // Real-time pan
    mainLayout->AddView(fPanSlider);
    
    // Mute toggle button (looks like button, acts like checkbox)
    fMuteButton = new ToggleButton("mute", "Mute", new BMessage(MSG_MUTE_TOGGLED));
    fMuteButton->SetTarget(this);
    fMuteButton->SetToggled(fTrack->IsMuted());
    fMuteButton->SetToggleColors(
        {200, 200, 200, 255},  // Normal grey
        {255, 120, 120, 255}   // Muted red
    );
    mainLayout->AddView(fMuteButton);
    
    // Solo toggle button
    fSoloButton = new ToggleButton("solo", "Solo", new BMessage(MSG_SOLO_TOGGLED));
    fSoloButton->SetTarget(this);
    fSoloButton->SetToggleColors(
        {200, 200, 200, 255},  // Normal grey  
        {120, 255, 120, 255}   // Solo green
    );
    mainLayout->AddView(fSoloButton);
}

void ChannelStrip::MessageReceived(BMessage* message)
{
    // Debug: uncomment if needed
    // printf("ChannelStrip: Received message 0x%08x\n", message->what);
    
    switch (message->what) {
        case MSG_VOLUME_CHANGED:
        case '_MM_':  // BSlider ModificationMessage - Haiku internal code
        {
            if (message->what == '_MM_' && fVolumeSlider) {
                // Real-time volume updates from slider drag
                int sliderValue = fVolumeSlider->Value();
                float volume = sliderValue / 100.0f;  // Convert 0-200 to 0.0-2.0
                fTrack->SetVolume(volume);
                printf("Track '%s' volume slider: %d → volume: %.2f\n", 
                       fTrack->GetName(), sliderValue, volume);
            } else if (message->what == MSG_VOLUME_CHANGED) {
                // Final volume change (mouse up)
                int sliderValue = fVolumeSlider->Value();
                float volume = sliderValue / 100.0f;
                fTrack->SetVolume(volume);
                printf("Track '%s' volume final: %.2f\n", fTrack->GetName(), volume);
            }
            break;
        }
        
        case MSG_PAN_CHANGED:
        {
            float pan = fPanSlider->Value() / 100.0f;  // Convert -100..100 to -1..1
            fTrack->SetPan(pan);
            printf("Track '%s' pan: %.2f\n", fTrack->GetName(), pan);
            break;
        }
        
        case MSG_MUTE_TOGGLED:
        {
            bool buttonToggled = false;
            message->FindBool("toggled", &buttonToggled);
            bool trackWasMuted = fTrack->IsMuted();
            fTrack->SetMute(buttonToggled);
            printf("Track '%s': button=%s, was_muted=%s → now %s\n", 
                   fTrack->GetName(), 
                   buttonToggled ? "PRESSED" : "RELEASED",
                   trackWasMuted ? "YES" : "NO",
                   buttonToggled ? "MUTED" : "UNMUTED");
            break;
        }
        
        case MSG_SOLO_TOGGLED:
        {
            bool soloToggled = false;
            message->FindBool("toggled", &soloToggled);
            printf("Track '%s' solo %s\n", fTrack->GetName(), soloToggled ? "ON" : "OFF");
            
            // Use engine's solo management (only one track can be solo)
            MixerWindow* mixerWindow = dynamic_cast<MixerWindow*>(Window());
            if (mixerWindow) {
                mixerWindow->SetTrackSolo(fTrack->GetId() - 1, soloToggled);  // ID is 1-based, index is 0-based
            }
            break;
        }
        
        default:
            BView::MessageReceived(message);
            break;
    }
}

void ChannelStrip::UpdateLevels()
{
    // Update level meter from real track data
    if (fLevelMeter && fTrack) {
        fLevelMeter->SetLevel(fTrack->GetPeakLevel(), fTrack->GetRMSLevel());
    }
}

void ChannelStrip::UpdateControls()
{
    if (fVolumeSlider) {
        fVolumeSlider->SetValue((int)(fTrack->GetVolume() * 100));
    }
    if (fPanSlider) {
        fPanSlider->SetValue((int)(fTrack->GetPan() * 100));
    }
    if (fMuteButton) {
        fMuteButton->SetToggled(fTrack->IsMuted());
    }
}

// =====================================
// AnalogVolumeBar implementation removed - class no longer used











// =====================================
// LevelMeter Implementation  
// =====================================

LevelMeter::LevelMeter()
    : BView(BRect(0, 0, 20, 120), "level_meter", B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW)
    , fPeakLevel(0.0f)
    , fRMSLevel(0.0f)
    , fLastUpdate(0)
{
    SetViewColor(B_TRANSPARENT_COLOR);
}

LevelMeter::~LevelMeter()
{
}

void LevelMeter::GetPreferredSize(float* width, float* height)
{
    *width = 20;
    *height = 120;
}

void LevelMeter::Draw(BRect updateRect)
{
    BRect bounds = Bounds();
    
    // Background
    SetHighColor(50, 50, 50);
    FillRect(bounds);
    
    // Border
    SetHighColor(0, 0, 0);
    StrokeRect(bounds);
    
    if (fRMSLevel > 0 || fPeakLevel > 0) {
        float height = bounds.Height() - 2;
        
        // RMS level (average) - green
        if (fRMSLevel > 0) {
            float rmsHeight = fRMSLevel * height;
            SetHighColor(0, 255, 0);  // Green
            BRect rmsRect(bounds.left + 1, bounds.bottom - 1 - rmsHeight, 
                         bounds.right - 1, bounds.bottom - 1);
            FillRect(rmsRect);
        }
        
        // Peak level - red line
        if (fPeakLevel > 0) {
            float peakY = bounds.bottom - 1 - (fPeakLevel * height);
            SetHighColor(255, 0, 0);  // Red
            StrokeLine(BPoint(bounds.left + 1, peakY), 
                      BPoint(bounds.right - 1, peakY));
        }
    }
}

void LevelMeter::SetLevel(float peak, float rms)
{
    fPeakLevel = std::max(0.0f, std::min(1.0f, peak));
    fRMSLevel = std::max(0.0f, std::min(1.0f, rms));
    fLastUpdate = system_time();
    
    if (Window() && Window()->LockLooper()) {
        Invalidate();
        Window()->UnlockLooper();
    }
}

// =====================================
// MixerWindow Implementation
// =====================================

MixerWindow::MixerWindow(SimpleHaikuEngine* engine, int startTrack, int maxTracks)
    : BWindow(BRect(100, 100, 400, 300), "HaikuDAW Mixer", 
              B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 
              B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_ZOOMABLE)
    , fEngine(engine)
    , fStartTrack(startTrack)
    , fMaxTracks(maxTracks)
    , fMenuBar(nullptr)
    , fMainView(nullptr)
    , fChannelArea(nullptr)
    , fMasterSection(nullptr)
    , fMasterVolume(nullptr)
    , fPlayButton(nullptr)
    , fStopButton(nullptr)
    , fStatusDisplay(nullptr)
    , fMasterLevelLeft(nullptr)
    , fMasterLevelRight(nullptr)
    , fUpdateRunner(nullptr)
{
    printf("MixerWindow: Constructor called for tracks %d-%d\n", startTrack, startTrack + maxTracks - 1);
    
    if (!engine) {
        printf("MixerWindow: ERROR - Audio engine is null!\n");
        return;
    }
    
    printf("MixerWindow: Creating menu bar...\n");
    CreateMenuBar();
    
    printf("MixerWindow: Creating mixer view...\n");
    CreateMixerView();
    
    printf("MixerWindow: Starting update timer...\n");
    // Start update timer (30 FPS for smooth meters)
    BMessage updateMsg(MSG_UPDATE_METERS);
    fUpdateRunner = new BMessageRunner(BMessenger(this), &updateMsg, 50000); // 20 FPS - optimized for performance
    
    // Set window size limits to allow dynamic resizing but ensure minimum usability
    float minWidth = 600.0f;   // Minimum to show at least 4 channels + master
    float minHeight = 450.0f;  // Minimum to show all controls including Solo buttons
    float maxWidth = 1200.0f;  // Maximum reasonable width
    float maxHeight = 800.0f;  // Maximum reasonable height
    
    SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);
    
    // Resize to fit content initially
    ResizeToPreferred();
    
    printf("MixerWindow: Created with %d tracks\n", (int)fChannelStrips.size());
}

MixerWindow::~MixerWindow()
{
    delete fUpdateRunner;
    printf("MixerWindow: Destroyed\n");
}

void MixerWindow::CreateMenuBar()
{
    // Create menu bar
    fMenuBar = new BMenuBar("menubar");
    
    // File menu
    BMenu* fileMenu = new BMenu("File");
    fileMenu->AddItem(new BMenuItem("New Project", new BMessage('new_')));
    fileMenu->AddItem(new BMenuItem("Open Project", new BMessage('open')));
    fileMenu->AddSeparatorItem();
    fileMenu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
    fMenuBar->AddItem(fileMenu);
    
    // Transport menu
    BMenu* transportMenu = new BMenu("Transport");
    transportMenu->AddItem(new BMenuItem("Play", new BMessage(MSG_PLAY), ' '));
    transportMenu->AddItem(new BMenuItem("Stop", new BMessage(MSG_STOP), 'S'));
    fMenuBar->AddItem(transportMenu);
    
    // Track menu
    BMenu* trackMenu = new BMenu("Track");
    trackMenu->AddItem(new BMenuItem("Add Track", new BMessage(MSG_ADD_TRACK), 'T'));
    trackMenu->AddItem(new BMenuItem("Remove Track", new BMessage(MSG_REMOVE_TRACK), 'R'));
    fMenuBar->AddItem(trackMenu);
    
    // Windows menu (with all windows)
    BMenu* windowMenu = new BMenu("Windows");
    windowMenu->AddItem(new BMenuItem("Show 3D Mixer", new BMessage(MSG_SHOW_3D_MIXER), '3'));
    windowMenu->AddSeparatorItem();
    
    // We'll add mixer windows and super master dynamically
    windowMenu->AddItem(new BMenuItem("Refresh Windows List", new BMessage(MSG_REFRESH_WINDOWS), 'R'));
    fMenuBar->AddItem(windowMenu);
    
    // Help menu
    BMenu* helpMenu = new BMenu("Help");
    helpMenu->AddItem(new BMenuItem("About HaikuDAW", new BMessage('abou')));
    fMenuBar->AddItem(helpMenu);
}

void MixerWindow::CreateMixerView()
{
    printf("MixerWindow: CreateMixerView() called\n");
    
    // Main view with horizontal layout
    printf("MixerWindow: Creating main view...\n");
    fMainView = new BView("main_view", B_WILL_DRAW);
    BGroupLayout* mainLayout = new BGroupLayout(B_HORIZONTAL);
    fMainView->SetLayout(mainLayout);
    mainLayout->SetSpacing(10);
    mainLayout->SetInsets(10, 10, 10, 10);
    
    printf("MixerWindow: Creating channel strips...\n");
    CreateChannelStrips();
    
    printf("MixerWindow: Creating master section...\n");
    CreateMasterSection();
    
    printf("MixerWindow: Setting up window layout...\n");
    // Add to window
    BGroupLayout* windowLayout = new BGroupLayout(B_VERTICAL);
    SetLayout(windowLayout);
    windowLayout->SetSpacing(0);
    
    windowLayout->AddView(fMenuBar);
    windowLayout->AddView(fMainView);
    
    printf("MixerWindow: CreateMixerView() completed\n");
}

void MixerWindow::CreateChannelStrips()
{
    // Channel area (horizontal group of strips)
    fChannelArea = new BView("channel_area", B_WILL_DRAW);
    BGroupLayout* channelLayout = new BGroupLayout(B_HORIZONTAL);
    fChannelArea->SetLayout(channelLayout);
    channelLayout->SetSpacing(5);
    
    // Get current track count
    int totalTracks = fEngine->GetTrackCount();
    int endTrack = fStartTrack + fMaxTracks;
    
    // If this is the first window and there are no tracks yet, create initial tracks
    if (fStartTrack == 0 && totalTracks == 0) {
        for (int i = 0; i < 4; i++) {
            BString trackName;
            trackName.SetToFormat("Track %d", i + 1);
            
            SimpleTrack* track = new SimpleTrack(i, trackName.String());
            
            // Set some initial positions for demo
            switch (i) {
                case 0: track->SetPosition(-2, 0, 1); break;  // Left
                case 1: track->SetPosition(0, 0, -1); break;  // Center-back
                case 2: track->SetPosition(2, 1, 0); break;   // Right-high
                case 3: track->SetPosition(0, -1, 0); break;  // Center-low
            }
            
            // Add to audio engine
            fEngine->AddTrack(track);
        }
        totalTracks = fEngine->GetTrackCount();
    }
    
    // For new windows (not the first), start empty and only show tracks that exist in their range
    // Create GUI strips ONLY for tracks that actually exist in our assigned range
    for (int i = fStartTrack; i < totalTracks && i < endTrack; i++) {
        SimpleTrack* track = fEngine->GetTrack(i);
        if (track) {
            ChannelStrip* strip = new ChannelStrip(track);
            fChannelStrips.push_back(strip);
            channelLayout->AddView(strip);
        }
    }
    
    printf("MixerWindow: Created %d channel strips for range %d-%d (total tracks: %d)\n", 
           (int)fChannelStrips.size(), fStartTrack, endTrack-1, totalTracks);
    
    // Always add the channel area to the layout, even if empty
    if (fMainView && fMainView->GetLayout() && fChannelArea) {
        fMainView->GetLayout()->AddView(fChannelArea);
        printf("MixerWindow: Channel area added to main view layout\n");
    } else {
        printf("MixerWindow: ERROR - Failed to add channel area to layout!\n");
    }
}

void MixerWindow::CreateMasterSection()
{
    // Master section (right side)
    fMasterSection = new BBox("master_section");
    fMasterSection->SetLabel("Master");
    
    BGroupLayout* masterLayout = new BGroupLayout(B_VERTICAL);
    fMasterSection->SetLayout(masterLayout);
    masterLayout->SetSpacing(10);
    masterLayout->SetInsets(10, 15, 10, 10);
    
    // Transport controls
    BGroupLayout* transportLayout = new BGroupLayout(B_HORIZONTAL);
    BView* transportView = new BView("transport", B_WILL_DRAW);
    transportView->SetLayout(transportLayout);
    
    fPlayButton = new BButton("play", "Play", new BMessage(MSG_PLAY));
    fStopButton = new BButton("stop", "Stop", new BMessage(MSG_STOP));
    
    transportLayout->AddView(fPlayButton);
    transportLayout->AddView(fStopButton);
    masterLayout->AddView(transportView);
    
    // Master VU meters and volume (horizontal layout)
    BGroupLayout* metersLayout = new BGroupLayout(B_HORIZONTAL);
    metersLayout->SetSpacing(5);
    BView* metersContainer = new BView("master_meters", B_WILL_DRAW);
    metersContainer->SetLayout(metersLayout);
    
    // Left channel VU meter
    BGroupLayout* leftMeterLayout = new BGroupLayout(B_VERTICAL);
    BView* leftMeterView = new BView("left_meter_view", B_WILL_DRAW);
    leftMeterView->SetLayout(leftMeterLayout);
    
    BStringView* leftLabel = new BStringView("left_label", "L");
    leftLabel->SetAlignment(B_ALIGN_CENTER);
    leftMeterLayout->AddView(leftLabel);
    
    fMasterLevelLeft = new LevelMeter();
    fMasterLevelLeft->SetExplicitMinSize(BSize(25, 150));
    fMasterLevelLeft->SetExplicitMaxSize(BSize(30, 200));
    fMasterLevelLeft->SetExplicitPreferredSize(BSize(28, 180));
    leftMeterLayout->AddView(fMasterLevelLeft);
    metersLayout->AddView(leftMeterView);
    
    // Right channel VU meter
    BGroupLayout* rightMeterLayout = new BGroupLayout(B_VERTICAL);
    BView* rightMeterView = new BView("right_meter_view", B_WILL_DRAW);
    rightMeterView->SetLayout(rightMeterLayout);
    
    BStringView* rightLabel = new BStringView("right_label", "R");
    rightLabel->SetAlignment(B_ALIGN_CENTER);
    rightMeterLayout->AddView(rightLabel);
    
    fMasterLevelRight = new LevelMeter();
    fMasterLevelRight->SetExplicitMinSize(BSize(25, 150));
    fMasterLevelRight->SetExplicitMaxSize(BSize(30, 200));
    fMasterLevelRight->SetExplicitPreferredSize(BSize(28, 180));
    rightMeterLayout->AddView(fMasterLevelRight);
    metersLayout->AddView(rightMeterView);
    
    // Master volume
    fMasterVolume = new BSlider("master_vol", "Master Volume",
                               new BMessage(MSG_MASTER_VOLUME),
                               0, 100, B_VERTICAL);
    fMasterVolume->SetValue(100);  // 100%
    fMasterVolume->SetTarget(this);
    fMasterVolume->SetLimitLabels("0", "100%");
    fMasterVolume->SetModificationMessage(new BMessage(MSG_MASTER_VOLUME));  // Real-time
    fMasterVolume->SetHashMarks(B_HASH_MARKS_BOTH);
    fMasterVolume->SetHashMarkCount(5);  // 0, 25, 50, 75, 100
    fMasterVolume->SetExplicitMinSize(BSize(60, 150));
    fMasterVolume->SetExplicitMaxSize(BSize(70, 200));
    fMasterVolume->SetExplicitPreferredSize(BSize(65, 180));
    metersLayout->AddView(fMasterVolume);
    
    masterLayout->AddView(metersContainer);
    
    // Status display
    fStatusDisplay = new BStringView("status", "Ready");
    fStatusDisplay->SetAlignment(B_ALIGN_CENTER);
    masterLayout->AddView(fStatusDisplay);
    
    fMainView->GetLayout()->AddView(fMasterSection);
}

bool MixerWindow::QuitRequested()
{
    // Notify application that this mixer window is closing
    BMessage removeMsg('rmvm');
    removeMsg.AddPointer("window", this);
    be_app->PostMessage(&removeMsg);
    
    // Don't quit the whole app, just close this window
    return true;
}

void MixerWindow::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case MSG_PLAY:
            if (fEngine) {
                status_t result = fEngine->Start();
                if (result == B_OK) {
                    fStatusDisplay->SetText("Playing");
                    printf("MixerWindow: Started audio engine\n");
                } else {
                    fStatusDisplay->SetText("Error");
                    printf("MixerWindow: Failed to start engine\n");
                }
            }
            break;
            
        case MSG_STOP:
            if (fEngine) {
                fEngine->Stop();
                fStatusDisplay->SetText("Stopped");
                printf("MixerWindow: Stopped audio engine\n");
            }
            break;
            
        case MSG_MASTER_VOLUME:
            if (fEngine && fMasterVolume) {
                float volume = fMasterVolume->Value() / 100.0f;
                fEngine->SetMasterVolume(volume);
                printf("MixerWindow: Master volume %.2f\n", volume);
            }
            break;
            
        case MSG_UPDATE_METERS:
            UpdateMeter();
            break;
            
        case MSG_SHOW_3D_MIXER:
        {
            // Send message to app to show 3D mixer window
            be_app->PostMessage(new BMessage(MSG_SHOW_3D_MIXER));
            break;
        }
        
        case MSG_ADD_TRACK:
        {
            AddTrack();
            break;
        }
        
        case MSG_REMOVE_TRACK:
        {
            RemoveTrack();
            break;
        }
        
        case MSG_REFRESH_WINDOWS:
        {
            UpdateWindowsMenu();
            break;
        }
        
        case MSG_SHOW_SUPER_MASTER:
        {
            BMessage showSuperMsg('shsm');
            be_app->PostMessage(&showSuperMsg);
            break;
        }
            
        case 'abou':
        {
            BAlert* alert = new BAlert("About", 
                "HaikuDAW v1.0\n"
                "Native Digital Audio Workstation\n"
                "Built with Haiku BMediaKit\n\n"
                "🎵 Powered by Haiku OS 🎵",
                "Cool!");
            alert->Go();
            break;
        }
            
        default:
            BWindow::MessageReceived(message);
            break;
    }
}

void MixerWindow::UpdateMeter()
{
    // Update all level meters for individual tracks
    for (ChannelStrip* strip : fChannelStrips) {
        strip->UpdateLevels();
    }
    
    // Calculate master level meters ONLY for this window's tracks
    if (fEngine && fMasterLevelLeft && fMasterLevelRight) {
        float mixPeakLeft = 0.0f;
        float mixPeakRight = 0.0f; 
        float mixRMSLeft = 0.0f;
        float mixRMSRight = 0.0f;
        
        // Sum levels only for tracks belonging to this window
        for (ChannelStrip* strip : fChannelStrips) {
            if (strip && strip->GetTrack()) {
                SimpleTrack* track = strip->GetTrack();
                
                // Skip if track is muted
                if (track->IsMuted()) continue;
                
                // Get track levels
                float trackPeak = track->GetPeakLevel();
                float trackRMS = track->GetRMSLevel();
                
                // Apply track volume and pan
                float volume = track->GetVolume();
                float pan = track->GetPan();  // -1.0 (left) to +1.0 (right)
                
                // Calculate left/right gains from pan
                float leftGain = (1.0f - pan) * 0.5f;   // pan=-1 → leftGain=1.0, pan=+1 → leftGain=0.0
                float rightGain = (1.0f + pan) * 0.5f;  // pan=-1 → rightGain=0.0, pan=+1 → rightGain=1.0
                
                // Apply volume, pan, and master volume
                float masterVolume = fEngine->GetMasterVolume();
                float finalLeft = trackPeak * volume * leftGain * masterVolume;
                float finalRight = trackPeak * volume * rightGain * masterVolume;
                float finalRMSLeft = trackRMS * volume * leftGain * masterVolume;
                float finalRMSRight = trackRMS * volume * rightGain * masterVolume;
                
                // Accumulate levels (take maximum for peak, sum for RMS)
                mixPeakLeft = std::max(mixPeakLeft, finalLeft);
                mixPeakRight = std::max(mixPeakRight, finalRight);
                mixRMSLeft += finalRMSLeft * finalRMSLeft;  // Sum of squares for RMS
                mixRMSRight += finalRMSRight * finalRMSRight;
            }
        }
        
        // Convert RMS back from sum of squares
        mixRMSLeft = sqrt(mixRMSLeft / std::max(1, (int)fChannelStrips.size()));
        mixRMSRight = sqrt(mixRMSRight / std::max(1, (int)fChannelStrips.size()));
        
        // Update master VU meters with this window's mix
        fMasterLevelLeft->SetLevel(mixPeakLeft, mixRMSLeft);
        fMasterLevelRight->SetLevel(mixPeakRight, mixRMSRight);
    }
    
    // Update status display
    if (fEngine && fStatusDisplay) {
        BString status;
        status << fEngine->GetStatus().String() << " (W" << ((fStartTrack / 8) + 1) << ")";
        fStatusDisplay->SetText(status.String());
    }
}

void MixerWindow::RefreshControls()
{
    // Refresh all channel strip controls
    for (ChannelStrip* strip : fChannelStrips) {
        strip->UpdateControls();
    }
}

void MixerWindow::SetTrackSolo(int trackIndex, bool solo)
{
    if (!fEngine) return;
    
    // Use engine's solo logic (automatically handles "only one solo at a time")
    fEngine->SetTrackSolo(trackIndex, solo);
    
    // Update all solo button states to reflect the change
    RefreshAllSoloButtons();
}

void MixerWindow::RefreshAllSoloButtons()
{
    // Update all solo buttons to reflect current engine state
    for (size_t i = 0; i < fChannelStrips.size(); i++) {
        ChannelStrip* strip = fChannelStrips[i];
        SimpleTrack* track = strip->GetTrack();
        
        if (track && strip->fSoloButton) {
            strip->fSoloButton->SetToggled(track->IsSolo());
        }
    }
    
    printf("MixerWindow: Refreshed all solo button states\n");
}

// =====================================
// Track Management
// =====================================

bool MixerWindow::CanAddTrack() const
{
    return fChannelStrips.size() < static_cast<size_t>(fMaxTracks);
}

bool MixerWindow::CanRemoveTrack() const
{
    return fChannelStrips.size() > kMinTracks;
}

void MixerWindow::AddTrack()
{
    // Global limit check - max 32 tracks total
    const int kMaxGlobalTracks = 32;
    if (fEngine->GetTrackCount() >= kMaxGlobalTracks) {
        BAlert* alert = new BAlert("Maximum Tracks", 
            "Maximum number of tracks (32) has been reached.\nThis is the limit for optimal performance.",
            "OK");
        alert->Go();
        printf("MixerWindow: Maximum global track limit (32) reached\n");
        return;
    }
    
    if (!CanAddTrack()) {
        // Request new mixer window from application WITH a track to add
        BMessage newWindowMsg('newm');
        newWindowMsg.AddBool("add_track", true);  // Tell app to add a track after creating window
        be_app->PostMessage(&newWindowMsg);
        
        // No alert needed - the user will see the new window appear with the track
        printf("MixerWindow: Maximum tracks reached, creating new window with next track\n");
        return;
    }
    
    // Use the current engine track count as the track ID to ensure sequential IDs
    int trackId = fEngine->GetTrackCount();
    
    // Calculate display name based on window range
    int displayNumber = fStartTrack + fChannelStrips.size() + 1;
    
    // Verify we can still add tracks to this window
    if (fChannelStrips.size() >= static_cast<size_t>(fMaxTracks)) {
        printf("MixerWindow: Error - window already at maximum %d tracks\n", fMaxTracks);
        return;
    }
    
    BString trackName;
    trackName << "Track " << displayNumber;
    
    printf("MixerWindow: Creating track with ID %d, display name '%s'\n", trackId, trackName.String());
    
    SimpleTrack* newTrack = new SimpleTrack(trackId, trackName.String());
    status_t result = fEngine->AddTrack(newTrack);
    
    if (result == B_OK) {
        // Create new channel strip
        ChannelStrip* newStrip = new ChannelStrip(newTrack);
        fChannelStrips.push_back(newStrip);
        
        // Add to channel area layout
        BGroupLayout* layout = dynamic_cast<BGroupLayout*>(fChannelArea->GetLayout());
        if (layout) {
            layout->AddView(newStrip);
        }
        
        // Resize window to accommodate new track
        ResizeToPreferred();
        
        printf("MixerWindow: Added %s (%d tracks total)\n", trackName.String(), (int)fChannelStrips.size());
    } else {
        delete newTrack;
        printf("MixerWindow: Failed to add track\n");
    }
}

void MixerWindow::RemoveTrack()
{
    if (!CanRemoveTrack()) {
        BAlert* alert = new BAlert("Cannot Remove Track", 
            "Cannot remove the last track.\nAt least one track must remain.",
            "OK");
        alert->Go();
        return;
    }
    
    // Remove last track (index = size - 1)
    int trackIndex = fChannelStrips.size() - 1;
    ChannelStrip* lastStrip = fChannelStrips.back();
    fChannelStrips.pop_back();
    
    // Remove from layout
    BGroupLayout* layout = dynamic_cast<BGroupLayout*>(fChannelArea->GetLayout());
    if (layout) {
        layout->RemoveView(lastStrip);
    }
    
    // Clean up GUI
    delete lastStrip;
    
    // Remove from engine
    status_t result = fEngine->RemoveTrack(trackIndex);
    if (result != B_OK) {
        printf("MixerWindow: Failed to remove track from engine\n");
    }
    
    // Resize window
    ResizeToPreferred();
    
    printf("MixerWindow: Removed track (%d tracks remaining)\n", (int)fChannelStrips.size());
}

void MixerWindow::UpdateWindowsMenu()
{
    // Find the Windows menu
    BMenuBar* menuBar = fMenuBar;
    if (!menuBar) return;
    
    BMenu* windowsMenu = nullptr;
    for (int i = 0; i < menuBar->CountItems(); i++) {
        BMenuItem* item = menuBar->ItemAt(i);
        if (item && item->Submenu() && strcmp(item->Label(), "Windows") == 0) {
            windowsMenu = item->Submenu();
            break;
        }
    }
    
    if (!windowsMenu) return;
    
    // Remove old dynamic items (keep Show 3D Mixer and Refresh)
    for (int i = windowsMenu->CountItems() - 1; i >= 0; i--) {
        BMenuItem* item = windowsMenu->ItemAt(i);
        if (item && item->Message()) {
            uint32 what = item->Message()->what;
            if (what != MSG_SHOW_3D_MIXER && what != MSG_REFRESH_WINDOWS) {
                windowsMenu->RemoveItem(item);
                delete item;
            }
        }
    }
    
    // Add Super Master menu item before Refresh
    BMenuItem* refreshItem = windowsMenu->FindItem(MSG_REFRESH_WINDOWS);
    int insertIndex = refreshItem ? windowsMenu->IndexOf(refreshItem) : windowsMenu->CountItems();
    
    BMenuItem* superMasterItem = new BMenuItem("Show Super Master", new BMessage(MSG_SHOW_SUPER_MASTER), 'S');
    windowsMenu->AddItem(superMasterItem, insertIndex);
    
    printf("MixerWindow: Updated Windows menu with Super Master option\n");
}

} // namespace HaikuDAW
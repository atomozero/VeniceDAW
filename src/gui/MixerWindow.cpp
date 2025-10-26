/*
 * MixerWindow.cpp - Native Haiku mixer implementation
 */

#include "MixerWindow.h"
#include "../audio/SimpleHaikuEngine.h"
#include "../audio/LevelMeterMapper.h"
#include "AudioPreviewPanel.h"
#include "3DMixImportDialog.h"
#include "TrackInspectorPanel.h"
#include "KeyboardShortcuts.h"
#include <Alert.h>
#include <Application.h>
#include <SpaceLayoutItem.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <FilePanel.h>
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
    , fSelected(false)
    , fTrackName(nullptr)
    , fVolumeSlider(nullptr)
    , fPanSlider(nullptr)
    , fMuteButton(nullptr)
    , fLevelMeter(nullptr)
    , fSoloButton(nullptr)
{
    if (!track) {
        // WARNING: Creating empty channel strip
        // Create a disabled/empty strip view
        SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_2_TINT));
        // SetEnabled(false); // Not available in BView
        return;
    }
    
    // printf("ChannelStrip: Creating channel strip for track '%s' (ID: %d)\n",
           track->GetName(), track->GetId());

    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

    // Enable drag & drop for this channel strip
    SetFlags(Flags() | B_FRAME_EVENTS);
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
        
        case MSG_LOAD_AUDIO_TO_TRACK:
        {
            // Forward to mixer window with track ID
            BMessage msg(MixerWindow::MSG_LOAD_AUDIO_TO_SPECIFIC_TRACK);
            msg.AddInt32("track_id", fTrack->GetId());
            Parent()->Looper()->PostMessage(&msg);
            break;
        }

        case MSG_CLEAR_TRACK:
        {
            // Clear audio content from track
            fTrack->UnloadFile();
            BString newName;
            newName << "Track " << fTrack->GetId();
            fTrack->SetName(newName.String());
            UpdateControls();
            break;
        }

        default:
            BView::MessageReceived(message);
            break;
    }
}

void ChannelStrip::SetSelected(bool selected)
{
    if (fSelected != selected) {
        fSelected = selected;

        // Visual feedback for selection
        if (fSelected) {
            SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_1_TINT));
        } else {
            SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
        }
        Invalidate();
    }
}

void ChannelStrip::MouseDown(BPoint where)
{
    uint32 buttons = 0;
    BMessage* currentMessage = Window()->CurrentMessage();
    if (currentMessage) {
        currentMessage->FindInt32("buttons", (int32*)&buttons);
    }

    if (buttons & B_SECONDARY_MOUSE_BUTTON) {
        // Right-click: Show context menu
        BPopUpMenu* contextMenu = new BPopUpMenu("track_context", false, false);

        // Check if track has audio content
        bool hasAudio = fTrack->HasAudioFile();

        if (hasAudio) {
            contextMenu->AddItem(new BMenuItem("Replace Audio File...",
                                             new BMessage(MSG_LOAD_AUDIO_TO_TRACK)));
            contextMenu->AddItem(new BMenuItem("Clear Track",
                                             new BMessage(MSG_CLEAR_TRACK)));
        } else {
            contextMenu->AddItem(new BMenuItem("Load Audio File...",
                                             new BMessage(MSG_LOAD_AUDIO_TO_TRACK)));
        }

        contextMenu->AddSeparatorItem();
        contextMenu->AddItem(new BMenuItem("Track Properties...", nullptr));  // Future feature

        contextMenu->SetTargetForItems(this);

        // Convert to screen coordinates and show
        BPoint screenWhere = ConvertToScreen(where);
        contextMenu->Go(screenWhere, true, true, true);
    } else {
        // Left-click: select this track for inspector
        MixerWindow* mixerWindow = dynamic_cast<MixerWindow*>(Window());
        if (mixerWindow && fTrack) {
            // Send selection message to mixer window
            BMessage selectMsg('slct');
            selectMsg.AddInt32("track_index", fTrack->GetId() - 1);  // 0-based
            mixerWindow->PostMessage(&selectMsg);
        }
        BView::MouseDown(where);
    }
}

void ChannelStrip::DragEnter(BMessage* message)
{
    // Check if this is a file drop
    if (message && (message->what == B_SIMPLE_DATA || message->what == B_REFS_RECEIVED)) {
        entry_ref ref;
        if (message->FindRef("refs", 0, &ref) == B_OK) {
            // Visual feedback: highlight the channel strip
            SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_MAX_TINT));
            Invalidate();
        }
    }
}

void ChannelStrip::DragLeave()
{
    // Remove highlight
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    Invalidate();
}

void ChannelStrip::Drop(BMessage* message, BPoint where)
{
    // Remove highlight first
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    Invalidate();

    if (!message || !fTrack) return;

    // Check if this is a file drop
    if (message->what == B_SIMPLE_DATA || message->what == B_REFS_RECEIVED) {
        entry_ref ref;
        if (message->FindRef("refs", 0, &ref) == B_OK) {
            // printf("ChannelStrip: File '%s' dropped on track '%s'\n",
                   ref.name, fTrack->GetName());

            // Create message to load to specific track
            BMessage loadMsg(MixerWindow::MSG_LOAD_AUDIO_TO_SPECIFIC_TRACK);
            loadMsg.AddRef("refs", &ref);
            loadMsg.AddInt32("target_track_index", fTrack->GetId() - 1);  // Convert to 0-based

            // Send to parent mixer window
            Parent()->Looper()->PostMessage(&loadMsg);
        }
    }
}

void ChannelStrip::UpdateControls()
{
    if (!fTrack) return;

    // Update track name
    if (fTrackName) {
        fTrackName->SetText(fTrack->GetName());
    }

    // Update volume slider
    if (fVolumeSlider) {
        fVolumeSlider->SetValue((int)(fTrack->GetVolume() * 100.0f));
    }

    // Update pan slider
    if (fPanSlider) {
        fPanSlider->SetValue((int)(fTrack->GetPan() * 100.0f));
    }

    // Update mute button
    if (fMuteButton) {
        fMuteButton->SetToggled(fTrack->IsMuted());
    }

    // Update solo button
    if (fSoloButton) {
        fSoloButton->SetToggled(fTrack->IsSolo());
    }

    // Force redraw
    Invalidate();
}

void ChannelStrip::UpdateLevels()
{
    // Update level meter from real track data
    if (fLevelMeter && fTrack) {
        fLevelMeter->SetLevel(fTrack->GetPeakLevel(), fTrack->GetRMSLevel());
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

        // Get color mapper instance
        const LevelMeterMapper& mapper = LevelMeterMapper::GetInstance();

        // RMS level (average) - smooth gradient based on level
        if (fRMSLevel > 0) {
            float rmsHeight = fRMSLevel * height;

            // Use mapper to get color based on actual level
            auto color = mapper.GetColor(fRMSLevel);
            SetHighColor(color.r, color.g, color.b);

            BRect rmsRect(bounds.left + 1, bounds.bottom - 1 - rmsHeight,
                         bounds.right - 1, bounds.bottom - 1);
            FillRect(rmsRect);
        }

        // Peak level - colored line based on peak value
        if (fPeakLevel > 0) {
            float peakY = bounds.bottom - 1 - (fPeakLevel * height);

            // Use mapper for peak color (will be red if clipping)
            auto peakColor = mapper.GetColor(fPeakLevel);
            SetHighColor(peakColor.r, peakColor.g, peakColor.b);

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
    , f3DMixImporter(nullptr)
    , fInspectorPanel(nullptr)
{
    // Constructor called
    
    if (!engine) {
        printf("❌ MixerWindow: ERROR - Audio engine is null!\n");
        return;
    }
    
    // Creating menu bar
    CreateMenuBar();
    
    // Creating mixer view
    CreateMixerView();

    // Enable drag & drop for audio files
    // Enabling drag & drop
    SetPulseRate(0);  // Disable pulse, we don't need it
    if (fMainView) {
        fMainView->SetFlags(fMainView->Flags() | B_WILL_DRAW | B_FRAME_EVENTS);
    }

    // Starting timer
    // Start update timer (30 FPS for smooth meters)
    BMessage updateMsg(MSG_UPDATE_METERS);
    fUpdateRunner = new BMessageRunner(BMessenger(this), &updateMsg, 50000); // 20 FPS - optimized for performance

    // Initialize 3dmix importer
    f3DMixImporter = new VeniceDAW::ThreeDMixProjectImporter();
    
    // Set window size limits to allow dynamic resizing but ensure minimum usability
    float minWidth = 600.0f;   // Minimum to show at least 4 channels + master
    float minHeight = 450.0f;  // Minimum to show all controls including Solo buttons
    float maxWidth = 1200.0f;  // Maximum reasonable width
    float maxHeight = 800.0f;  // Maximum reasonable height
    
    SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);
    
    // Resize to fit content initially
    ResizeToPreferred();
    
    // Created
}

MixerWindow::~MixerWindow()
{
    delete fUpdateRunner;
    delete f3DMixImporter;
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
    transportMenu->AddItem(new BMenuItem("Play/Pause", new BMessage(MSG_PLAY), ' '));
    transportMenu->AddItem(new BMenuItem("Stop", new BMessage(MSG_STOP), '.'));
    fMenuBar->AddItem(transportMenu);
    
    // Track menu
    BMenu* trackMenu = new BMenu("Track");
    trackMenu->AddItem(new BMenuItem("Add Track", new BMessage(MSG_ADD_TRACK), 'T'));
    trackMenu->AddSeparatorItem();
    trackMenu->AddItem(new BMenuItem("Import Audio File...", new BMessage(MSG_IMPORT_AUDIO), 'I'));
    trackMenu->AddItem(new BMenuItem("Import Multiple Files...", new BMessage(MSG_IMPORT_MULTIPLE), 'M'));
    trackMenu->AddSeparatorItem();
    trackMenu->AddItem(new BMenuItem("Load to Specific Track...", new BMessage(MSG_SHOW_TRACK_SELECTION_DIALOG), 'L'));
    trackMenu->AddSeparatorItem();
    trackMenu->AddItem(new BMenuItem("Import 3dmix Project...", new BMessage(MSG_IMPORT_3DMIX)));
    trackMenu->AddSeparatorItem();
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
    helpMenu->AddItem(new BMenuItem("Keyboard Shortcuts", new BMessage('keys')));
    helpMenu->AddSeparatorItem();
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

    // Create track inspector panel (right sidebar)
    printf("MixerWindow: Creating track inspector panel...\n");
    BRect inspectorRect(0, 0, 280, 500);
    fInspectorPanel = new TrackInspectorPanel(inspectorRect);
    mainLayout->AddView(fInspectorPanel);

    printf("MixerWindow: Creating master section...\n");
    CreateMasterSection();
    
    printf("MixerWindow: Setting up window layout...\n");
    // Add to window
    BGroupLayout* windowLayout = new BGroupLayout(B_VERTICAL);
    SetLayout(windowLayout);
    windowLayout->SetSpacing(0);
    
    windowLayout->AddView(fMenuBar);
    windowLayout->AddView(fMainView);
    
    // MixerView completed
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
    
    // printf("MixerWindow: Created %d channel strips for range %d-%d (total tracks: %d)\n", 
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

void MixerWindow::DispatchMessage(BMessage* message, BHandler* handler)
{
    // Intercept keyboard messages for global shortcuts
    if (message->what == B_KEY_DOWN) {
        int32 key;
        uint32 modifiers;

        if (message->FindInt32("raw_char", &key) == B_OK &&
            message->FindInt32("modifiers", (int32*)&modifiers) == B_OK) {

            // Transport shortcuts (no modifiers)
            if (KeyboardShortcuts::HasExactModifiers(modifiers, MOD_NONE)) {
                switch (key) {
                    case KEY_PLAY_PAUSE:  // Spacebar - toggle play/pause
                        if (fEngine && fEngine->IsPlaying()) {
                            PostMessage(MSG_STOP);
                        } else {
                            PostMessage(MSG_PLAY);
                        }
                        return;

                    case KEY_STOP:  // Period - stop
                        PostMessage(MSG_STOP);
                        return;

                    case KEY_MUTE:  // M - toggle mute on selected track
                        // TODO: implement when track selection is available
                        break;

                    case KEY_SOLO:  // S - toggle solo on selected track
                        // TODO: implement when track selection is available
                        break;
                }
            }

            // Command+key shortcuts
            if (KeyboardShortcuts::HasExactModifiers(modifiers, MOD_COMMAND)) {
                switch (key) {
                    case KEY_NEW_TRACK:  // Cmd+T - add new track
                        PostMessage(MSG_ADD_TRACK);
                        return;

                    case KEY_REMOVE_TRACK:  // Cmd+R - remove track
                        PostMessage(MSG_REMOVE_TRACK);
                        return;

                    case KEY_IMPORT_AUDIO:  // Cmd+I - import audio
                        PostMessage(MSG_IMPORT_AUDIO);
                        return;

                    case KEY_SHOW_MIXER:  // Cmd+1 - already in mixer
                        // Already in mixer window
                        return;

                    case KEY_SHOW_3D:  // Cmd+2 - show 3D mixer
                        PostMessage(MSG_SHOW_3D_MIXER);
                        return;

                    case '3':  // Cmd+3 - show timeline (future)
                        // TODO: implement when timeline is ready
                        break;

                    case KEY_QUIT:  // Cmd+Q - quit (handled by app)
                        be_app->PostMessage(B_QUIT_REQUESTED);
                        return;
                }
            }

            // Command+Shift+key shortcuts
            if (KeyboardShortcuts::HasExactModifiers(modifiers, MOD_COMMAND | MOD_SHIFT)) {
                switch (key) {
                    case KEY_IMPORT_MULTIPLE:  // Cmd+Shift+M - import multiple
                        PostMessage(MSG_IMPORT_MULTIPLE);
                        return;

                    case '3':  // Cmd+Shift+3 - import 3dmix
                        PostMessage(MSG_IMPORT_3DMIX);
                        return;
                }
            }

            // Navigation shortcuts (arrow keys)
            if (KeyboardShortcuts::HasExactModifiers(modifiers, MOD_NONE)) {
                switch (key) {
                    case KEY_SELECT_NEXT_TRACK:  // Down arrow
                    case KEY_SELECT_PREV_TRACK:  // Up arrow
                        // TODO: implement track selection navigation
                        break;
                }
            }
        }
    }

    // Let default handling proceed
    BWindow::DispatchMessage(message, handler);
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
            // Also update inspector panel levels
            if (fInspectorPanel) {
                fInspectorPanel->UpdateLevels();
            }
            break;

        case 'slct':  // Track selection
        {
            int32 trackIndex = -1;
            if (message->FindInt32("track_index", &trackIndex) == B_OK) {
                // Update all channel strips' selection state
                for (ChannelStrip* strip : fChannelStrips) {
                    if (strip && strip->GetTrack()) {
                        bool isSelected = (strip->GetTrack()->GetId() - 1 == trackIndex);
                        strip->SetSelected(isSelected);
                    }
                }

                // Update inspector panel to show selected track
                SimpleTrack* selectedTrack = fEngine->GetTrack(trackIndex);
                if (fInspectorPanel && selectedTrack) {
                    fInspectorPanel->SetTrack(selectedTrack);
                }
            }
            break;
        }
            
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

        case MSG_IMPORT_AUDIO:
        {
            ImportAudioFile();
            break;
        }

        case MSG_IMPORT_MULTIPLE:
        {
            ImportMultipleFiles();
            break;
        }

        case MSG_IMPORT_3DMIX:
        {
            Import3DMixProject();
            break;
        }

        case MSG_3DMIX_IMPORT_COMPLETE:
        {
            // Handle 3dmix import completion
            VeniceDAW::ImportResult* result;
            if (message->FindPointer("result", (void**)&result) == B_OK) {
                Handle3DMixImportResult(*result);
            }
            break;
        }

        case B_SIMPLE_DATA:
        case B_REFS_RECEIVED:
        {
            // Check if this is for a specific track
            int32 targetTrackIndex = -1;
            if (message->FindInt32("target_track_index", &targetTrackIndex) == B_OK) {
                // Load to specific track
                entry_ref ref;
                if (message->FindRef("refs", 0, &ref) == B_OK) {
                    LoadAudioFileToTrack(ref, targetTrackIndex);
                }
            } else {
                // Handle normal drag & drop (creates new tracks)
                HandleDroppedFiles(message);
            }
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
            
        case 'keys':  // Show keyboard shortcuts
        {
            BAlert* shortcutsAlert = new BAlert("Keyboard Shortcuts",
                "VeniceDAW Keyboard Shortcuts\n\n"
                "TRANSPORT:\n"
                "  Space       - Play/Pause\n"
                "  .           - Stop\n\n"
                "TRACKS:\n"
                "  Cmd+T       - Add new track\n"
                "  Cmd+R       - Remove track\n"
                "  M           - Mute selected track\n"
                "  S           - Solo selected track\n\n"
                "IMPORT:\n"
                "  Cmd+I       - Import audio file\n"
                "  Cmd+Shift+M - Import multiple files\n"
                "  Cmd+Shift+3 - Import 3dmix project\n\n"
                "WINDOWS:\n"
                "  Cmd+1       - Show Mixer\n"
                "  Cmd+2       - Show 3D Mixer\n"
                "  Cmd+3       - Show Timeline (future)\n\n"
                "GENERAL:\n"
                "  Cmd+Q       - Quit application",
                "Got it!", nullptr, nullptr,
                B_WIDTH_AS_USUAL, B_INFO_ALERT);
            shortcutsAlert->Go();
            break;
        }

        case 'abou':
        {
            BAlert* alert = new BAlert("About",
                "VeniceDAW v1.0\n"
                "Professional Audio Workstation for Haiku OS\n"
                "Built with native Haiku BMediaKit\n\n"
                "Features:\n"
                "• Real-time multi-track mixing\n"
                "• 3D spatial audio positioning\n"
                "• Professional track inspector\n"
                "• BeOS 3dmix project import\n\n"
                "🎵 Powered by Haiku OS 🎵",
                "Cool!");
            alert->Go();
            break;
        }

        case MSG_LOAD_AUDIO_TO_SPECIFIC_TRACK:
        {
            int32 trackId = -1;
            if (message->FindInt32("track_id", &trackId) == B_OK) {
                LoadAudioFileToSpecificTrack(trackId - 1);  // Convert to 0-based index
            }
            break;
        }

        case MSG_SHOW_TRACK_SELECTION_DIALOG:
        {
            ShowTrackSelectionDialog();
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
        if (strip) {
            strip->UpdateLevels();
        }
    }
    
    // Calculate master level meters ONLY for this window's tracks
    if (fEngine && fMasterLevelLeft && fMasterLevelRight) {
        float mixPeakLeft = 0.0f;
        float mixPeakRight = 0.0f; 
        float mixRMSLeft = 0.0f;
        float mixRMSRight = 0.0f;
        
        // Sum levels only for tracks belonging to this window
        for (ChannelStrip* strip : fChannelStrips) {
            if (!strip || !strip->GetTrack()) {
                continue;
            }

            SimpleTrack* track = strip->GetTrack();
            if (track) {
                
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

// =====================================
// Audio File Import Methods
// =====================================

void MixerWindow::ImportAudioFile()
{
    printf("MixerWindow: Opening file dialog for single audio file import\n");

    // Create enhanced file panel with audio preview
    VeniceDAW::AudioFilePanel* panel = new VeniceDAW::AudioFilePanel(B_OPEN_PANEL,
                                                                      new BMessenger(this),
                                                                      nullptr,  // Start directory (use default)
                                                                      B_FILE_NODE,
                                                                      false,    // Allow single selection
                                                                      nullptr,  // Message to send
                                                                      nullptr,  // RefFilter (we'll add MIME type filtering later)
                                                                      true,     // Modal
                                                                      true);    // Hide when done

    // Set window title
    panel->Window()->SetTitle("Import Audio File - VeniceDAW (with Preview)");

    // Show the panel
    panel->Show();

    // Note: The panel will send B_REFS_RECEIVED message when file is selected
}

void MixerWindow::ImportMultipleFiles()
{
    printf("MixerWindow: Opening file dialog for multiple audio files import\n");

    // Create enhanced file panel for multiple files (preview shows last selected)
    VeniceDAW::AudioFilePanel* panel = new VeniceDAW::AudioFilePanel(B_OPEN_PANEL,
                                                                      new BMessenger(this),
                                                                      nullptr,  // Start directory
                                                                      B_FILE_NODE,
                                                                      true,     // Allow multiple selection
                                                                      nullptr,  // Message to send
                                                                      nullptr,  // RefFilter
                                                                      true,     // Modal
                                                                      true);    // Hide when done

    // Set window title
    panel->Window()->SetTitle("Import Multiple Audio Files - VeniceDAW (with Preview)");

    // Show the panel
    panel->Show();
}

void MixerWindow::HandleDroppedFiles(BMessage* message)
{
    if (!message || !fEngine) {
        printf("MixerWindow: Invalid message or engine in HandleDroppedFiles\n");
        return;
    }

    printf("MixerWindow: Processing dropped/selected files\n");

    // Collect all file references first
    std::vector<entry_ref> files;
    entry_ref ref;
    int32 index = 0;

    while (message->FindRef("refs", index, &ref) == B_OK) {
        BEntry entry(&ref);
        if (entry.IsFile()) {
            files.push_back(ref);
        }
        index++;
    }

    if (files.empty()) {
        BAlert* alert = new BAlert("No Files", "No valid audio files found to import.", "OK");
        alert->Go();
        return;
    }

    // Use batch loading for better performance and user feedback
    LoadAudioFilesBatch(files);
}

status_t MixerWindow::LoadAudioFilesBatch(const std::vector<entry_ref>& files)
{
    if (files.empty() || !fEngine) {
        return B_BAD_VALUE;
    }

    printf("MixerWindow: Starting batch import of %d files\n", (int)files.size());

    int32 filesProcessed = 0;
    int32 filesSkipped = 0;
    BString lastErrorMessage;

    // Process files in batch
    for (size_t i = 0; i < files.size(); i++) {
        const entry_ref& ref = files[i];
        BPath path(&ref);

        // Show progress to user
        ShowBatchImportProgress(i + 1, files.size(), ref.name);

        printf("MixerWindow: Batch processing [%d/%d]: %s\n",
               (int)(i + 1), (int)files.size(), path.Path());

        // Try to load the audio file
        status_t result = fEngine->LoadAudioFileAsTrack(ref);

        if (result == B_OK) {
            printf("MixerWindow: Successfully loaded: %s\n", path.Path());
            filesProcessed++;
        } else {
            printf("MixerWindow: Failed to load: %s (%s)\n", path.Path(), strerror(result));
            filesSkipped++;
            lastErrorMessage.SetToFormat("Last error: %s - %s", ref.name, strerror(result));
        }
    }

    // Refresh GUI once after all files are processed
    if (filesProcessed > 0) {
        CreateChannelStrips();  // Refresh channel strips to show new tracks
    }

    // Show comprehensive summary
    BString summary;
    BString details;

    if (filesProcessed > 0 && filesSkipped == 0) {
        summary.SetToFormat("✅ Successfully imported all %d audio files", filesProcessed);
        details.SetToFormat("All files were loaded successfully into VeniceDAW tracks.");
    } else if (filesProcessed > 0 && filesSkipped > 0) {
        summary.SetToFormat("⚠️ Imported %d files, skipped %d", filesProcessed, filesSkipped);
        details.SetToFormat("Some files could not be loaded. VeniceDAW supports WAV, AIFF, MP3, and OGG formats.\n\n%s",
                           lastErrorMessage.String());
    } else {
        summary.SetTo("❌ No files were imported");
        details.SetToFormat("None of the selected files could be loaded. Please check:\n"
                          "• File formats (supported: WAV, AIFF, MP3, OGG)\n"
                          "• File integrity\n"
                          "• Available system memory\n\n%s",
                          lastErrorMessage.String());
    }

    printf("MixerWindow: Batch import complete: %s\n", summary.String());

    // Update status display
    if (fStatusDisplay) {
        fStatusDisplay->SetText(summary.String());
    }

    // Show detailed result dialog
    alert_type alertType = filesProcessed > 0 ? B_INFO_ALERT : B_WARNING_ALERT;
    BString alertMessage;
    alertMessage.SetToFormat("%s\n\n%s", summary.String(), details.String());
    BAlert* alert = new BAlert("Batch Import Complete",
                               alertMessage.String(),
                               "OK", nullptr, nullptr, B_WIDTH_AS_USUAL, alertType);
    alert->Go();

    return filesProcessed > 0 ? B_OK : B_ERROR;
}

void MixerWindow::ShowBatchImportProgress(int32 current, int32 total, const char* filename)
{
    // Update status display with progress
    if (fStatusDisplay) {
        BString progress;
        progress.SetToFormat("Importing [%d/%d]: %s", (int)current, (int)total, filename);
        fStatusDisplay->SetText(progress.String());

        // Force UI update
        fStatusDisplay->Invalidate();
        UpdateIfNeeded();
    }

    printf("MixerWindow: Import progress [%d/%d]: %s\n", (int)current, (int)total, filename);
}

// =====================================
// 3dmix Project Import
// =====================================

void MixerWindow::Import3DMixProject()
{
    printf("MixerWindow: Import 3dmix project requested\n");

    // Create file panel with 3dmix filter
    BFilePanel* filePanel = VeniceDAW::ThreeDMixUIUtils::CreateImportFilePanel(this);
    filePanel->Show();
}

void MixerWindow::Show3DMixImportDialog(const char* filePath)
{
    if (!filePath) {
        printf("MixerWindow: Invalid 3dmix file path\n");
        return;
    }

    printf("MixerWindow: Showing 3dmix import dialog for: %s\n", filePath);

    // Show import dialog
    VeniceDAW::ThreeDMixImportDialog* dialog = VeniceDAW::ThreeDMixUIUtils::ShowImportDialog(filePath, this);
    if (!dialog) {
        printf("MixerWindow: Failed to create import dialog\n");
        return;
    }

    // The dialog will be modal and handle the import process
    dialog->Show();
}

void MixerWindow::Handle3DMixImportResult(const VeniceDAW::ImportResult& result)
{
    printf("MixerWindow: Handling 3dmix import result\n");

    if (result.success) {
        // Import successful
        BString message;
        message.SetToFormat("Successfully imported '%s'\n"
                           "Tracks imported: %d\n"
                           "Audio files resolved: %d\n"
                           "Import time: %.2f seconds",
                           result.projectName.String(),
                           result.tracksImported,
                           result.audioFilesResolved,
                           result.importTime / 1000000.0f);

        BAlert* alert = new BAlert("Import Successful", message.String(), "OK",
                                  nullptr, nullptr, B_WIDTH_AS_USUAL, B_INFO_ALERT);
        alert->Go();

        // Update status display
        if (fStatusDisplay) {
            BString status;
            status.SetToFormat("Imported %s (%d tracks)", result.projectName.String(), result.tracksImported);
            fStatusDisplay->SetText(status.String());
        }

        // Refresh controls to show new tracks
        RefreshControls();

        printf("MixerWindow: 3dmix import successful - %d tracks imported\n", result.tracksImported);
    } else {
        // Import failed
        BString errorMessage;
        errorMessage.SetToFormat("Failed to import 3dmix project:\n\n%s", result.errorMessage.String());

        BAlert* alert = new BAlert("Import Failed", errorMessage.String(), "OK",
                                  nullptr, nullptr, B_WIDTH_AS_USUAL, B_STOP_ALERT);
        alert->Go();

        printf("MixerWindow: 3dmix import failed: %s\n", result.errorMessage.String());
    }
}

// =====================================
// Selective Track Loading (NEW)
// =====================================

void MixerWindow::LoadAudioFileToSpecificTrack(int32 trackIndex)
{
    printf("MixerWindow: Loading audio file to specific track %d\n", trackIndex);

    // Validate track index
    if (trackIndex < 0 || trackIndex >= fEngine->GetTrackCount()) {
        BAlert* alert = new BAlert("Invalid Track",
            "Selected track does not exist.", "OK");
        alert->Go();
        return;
    }

    // Create message with track info
    BMessage* panelMessage = new BMessage(MSG_LOAD_AUDIO_TO_SPECIFIC_TRACK);
    panelMessage->AddInt32("target_track_index", trackIndex);

    // Create file panel
    BFilePanel* panel = new BFilePanel(B_OPEN_PANEL,
                                      new BMessenger(this),
                                      nullptr,  // Start directory
                                      B_FILE_NODE,
                                      false,    // Single selection
                                      panelMessage,  // Message with track info
                                      nullptr,  // RefFilter
                                      true,     // Modal
                                      true);    // Hide when done

    // Set window title
    SimpleTrack* track = fEngine->GetTrack(trackIndex);
    BString title;
    title << "Load Audio File to " << (track ? track->GetName() : "Track") << " - VeniceDAW";
    panel->Window()->SetTitle(title.String());

    // Show the panel
    panel->Show();
}

void MixerWindow::ShowTrackSelectionDialog()
{
    printf("MixerWindow: Showing track selection dialog\n");

    if (!fEngine || fEngine->GetTrackCount() == 0) {
        BAlert* alert = new BAlert("No Tracks",
            "No tracks available. Add some tracks first.", "OK");
        alert->Go();
        return;
    }

    // Create a simple selection alert first
    BAlert* trackAlert = new BAlert("Select Track",
        "Choose which track method to use:\n\n"
        "• Right-click on any track for direct loading\n"
        "• Use this dialog to select from list\n"
        "• Drag & drop files directly onto tracks\n\n"
        "Would you like to select a track now?",
        "Cancel", "Select Track", nullptr,
        B_WIDTH_AS_USUAL, B_INFO_ALERT);

    int32 choice = trackAlert->Go();
    if (choice == 1) {
        // Show track list - for now, just use the first available track
        // In a full implementation, this would show a proper selection dialog
        for (int i = 0; i < fEngine->GetTrackCount(); i++) {
            SimpleTrack* track = fEngine->GetTrack(i);
            if (track) {
                LoadAudioFileToSpecificTrack(i);
                break;
            }
        }
    }
}

status_t MixerWindow::LoadAudioFileToTrack(const entry_ref& ref, int32 trackIndex)
{
    printf("MixerWindow: Loading '%s' to track %d\n", ref.name, trackIndex);

    if (!fEngine || trackIndex < 0 || trackIndex >= fEngine->GetTrackCount()) {
        return B_BAD_VALUE;
    }

    SimpleTrack* track = fEngine->GetTrack(trackIndex);
    if (!track) {
        return B_BAD_VALUE;
    }

    // Load audio file directly to the specified track
    status_t result = track->LoadAudioFile(ref);
    if (result == B_OK) {
        // Update track name to file name (without extension)
        BString fileName = ref.name;
        int32 dotPos = fileName.FindLast('.');
        if (dotPos > 0) {
            fileName.Truncate(dotPos);
        }
        track->SetName(fileName.String());

        // Refresh GUI to show changes
        ChannelStrip* strip = FindChannelStripByTrackIndex(trackIndex);
        if (strip) {
            strip->UpdateControls();
        }

        printf("MixerWindow: Successfully loaded '%s' to track '%s'\n",
               ref.name, track->GetName());
    } else {
        printf("MixerWindow: Failed to load '%s' to track %d: %s\n",
               ref.name, trackIndex, strerror(result));
    }

    return result;
}

ChannelStrip* MixerWindow::FindChannelStripByTrackIndex(int32 trackIndex)
{
    for (ChannelStrip* strip : fChannelStrips) {
        if (strip && strip->GetTrack() && strip->GetTrack()->GetId() == (trackIndex + 1)) {
            return strip;
        }
    }
    return nullptr;
}

} // namespace HaikuDAW
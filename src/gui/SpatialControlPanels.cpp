/*
 * SpatialControlPanels.cpp - Professional Spatial Audio Control Interface
 * 
 * Implementation of the control panels for HRTF, environmental parameters,
 * and spatial audio visualization controls for VeniceDAW Phase 4.
 */

#include "SpatialMixer3DWindow.h"
#include "../audio/SimpleHaikuEngine.h"
#include <interface/Alert.h>
#include <LayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <CardLayout.h>
#include <SeparatorView.h>
#include <FilePanel.h>
#include <Path.h>
#include <Directory.h>

using namespace VeniceDAW;
using namespace VeniceDAW::DSP;

namespace HaikuDAW {

// =====================================
// SpatialControlPanel Implementation  
// =====================================

SpatialControlPanel::SpatialControlPanel(BRect frame, SpatialMixer3DView* spatialView, 
                                       AdvancedAudioProcessor* processor)
    : BView(frame, "spatial_controls", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS)
    , fSpatialView(spatialView)
    , fAudioProcessor(processor)
    , fTabView(nullptr)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    
    // Create tabbed interface for organized controls
    BRect tabFrame = Bounds();
    tabFrame.InsetBy(5, 5);
    
    fTabView = new BTabView(tabFrame, "spatial_tabs");
    
    CreateSpatialModeTab();
    CreateHRTFTab();
    CreateEnvironmentTab();
    CreateVisualizationTab();
    
    AddChild(fTabView);
    
    printf("SpatialControlPanel: Professional spatial audio controls initialized\n");
}

SpatialControlPanel::~SpatialControlPanel()
{
    printf("SpatialControlPanel: Spatial control panel destroyed\n");
}

void SpatialControlPanel::AttachedToWindow()
{
    BView::AttachedToWindow();
    
    // Set targets for all controls
    if (fSpatialEnabledBox) fSpatialEnabledBox->SetTarget(this);
    if (fSpatialModeMenu) fSpatialModeMenu->SetTargetForItems(this);
    if (fHRTFEnabledBox) fHRTFEnabledBox->SetTarget(this);
    if (fCrossfeedSlider) fCrossfeedSlider->SetTarget(this);
    if (fLoadHRTFButton) fLoadHRTFButton->SetTarget(this);
    if (fRoomWidthSlider) fRoomWidthSlider->SetTarget(this);
    if (fRoomHeightSlider) fRoomHeightSlider->SetTarget(this);
    if (fRoomDepthSlider) fRoomDepthSlider->SetTarget(this);
    if (fReverbAmountSlider) fReverbAmountSlider->SetTarget(this);
    if (fReverbDecaySlider) fReverbDecaySlider->SetTarget(this);
    if (fAirAbsorptionBox) fAirAbsorptionBox->SetTarget(this);
    if (fDopplerBox) fDopplerBox->SetTarget(this);
    if (fShowIndicatorsBox) fShowIndicatorsBox->SetTarget(this);
    if (fShowListenerBox) fShowListenerBox->SetTarget(this);
    if (fShowRoomBoundsBox) fShowRoomBoundsBox->SetTarget(this);
    if (fShowSpeakersBox) fShowSpeakersBox->SetTarget(this);
    
    // Initialize control states from processor
    UpdateFromProcessor();
}

void SpatialControlPanel::CreateSpatialModeTab()
{
    BView* modeTab = new BView("spatial_mode_tab", B_WILL_DRAW);
    modeTab->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    
    // Spatial processing enable/disable
    fSpatialEnabledBox = new BCheckBox("spatial_enabled", 
        "Enable Spatial Audio Processing", 
        new BMessage(MSG_SPATIAL_ENABLED));
    fSpatialEnabledBox->SetValue(B_CONTROL_ON);
    
    // Spatial mode selection
    fSpatialModeMenu = new BPopUpMenu("Select Spatial Mode");
    fSpatialModeMenu->AddItem(new BMenuItem("Basic Surround", 
        new BMessage(MSG_SPATIAL_MODE)));
    fSpatialModeMenu->AddItem(new BMenuItem("3D Spatial Positioning", 
        new BMessage(MSG_SPATIAL_MODE)));
    fSpatialModeMenu->AddItem(new BMenuItem("Binaural HRTF", 
        new BMessage(MSG_SPATIAL_MODE)));
    fSpatialModeMenu->AddItem(new BMenuItem("Ambisonics", 
        new BMessage(MSG_SPATIAL_MODE)));
    
    // Set default selection
    fSpatialModeMenu->ItemAt(1)->SetMarked(true);  // 3D Spatial default
    
    fSpatialModeField = new BMenuField("spatial_mode_field", 
        "Spatial Mode:", fSpatialModeMenu);
    
    // Status display
    BStringView* statusLabel = new BStringView("status_label", "Status:");
    BStringView* statusValue = new BStringView("status_value", 
        "3D Spatial Processing Active");
    statusValue->SetFont(be_bold_font);
    statusValue->SetHighColor(make_color(0, 150, 0));  // Green for active
    
    // Layout the mode tab
    BLayoutBuilder::Group<>(modeTab, B_VERTICAL, 10)
        .SetInsets(15, 15, 15, 15)
        .Add(fSpatialEnabledBox)
        .Add(new BSeparatorView(B_HORIZONTAL))
        .AddGroup(B_HORIZONTAL)
            .Add(fSpatialModeField)
            .AddGlue()
        .End()
        .Add(new BSeparatorView(B_HORIZONTAL))
        .AddGroup(B_HORIZONTAL)
            .Add(statusLabel)
            .Add(statusValue)
            .AddGlue()
        .End()
        .AddGlue();
    
    BTab* modeTabItem = new BTab(modeTab);
    fTabView->AddTab(modeTab, modeTabItem);
    modeTabItem->SetLabel("Spatial Mode");
}

void SpatialControlPanel::CreateHRTFTab()
{
    BView* hrtfTab = new BView("hrtf_tab", B_WILL_DRAW);
    hrtfTab->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    
    // HRTF processing enable/disable
    fHRTFEnabledBox = new BCheckBox("hrtf_enabled",
        "Enable HRTF Binaural Processing",
        new BMessage(MSG_HRTF_ENABLED));
    fHRTFEnabledBox->SetValue(B_CONTROL_OFF);  // Default disabled
    
    // Crossfeed amount for headphone listening
    fCrossfeedSlider = new BSlider("crossfeed_slider",
        "Crossfeed Amount:",
        new BMessage(MSG_CROSSFEED),
        0, 100, B_HORIZONTAL);
    fCrossfeedSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
    fCrossfeedSlider->SetHashMarkCount(11);
    fCrossfeedSlider->SetValue(30);  // 30% default crossfeed
    fCrossfeedSlider->SetLimitLabels("0%", "100%");
    
    // HRTF database loading
    fLoadHRTFButton = new BButton("load_hrtf_button",
        "Load HRTF Database...",
        new BMessage(MSG_LOAD_HRTF));
    
    // HRTF status display
    fHRTFStatusView = new BStringView("hrtf_status",
        "Status: Using built-in generic HRTF");
    fHRTFStatusView->SetFont(be_plain_font);
    
    // Information text about HRTF
    BTextView* infoText = new BTextView("hrtf_info");
    infoText->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    infoText->MakeEditable(false);
    infoText->SetText(
        "Head-Related Transfer Function (HRTF) processing provides "
        "realistic 3D audio positioning for headphone listening. "
        "Individual HRTF measurements provide the most accurate "
        "spatial localization, but generic HRTFs work well for most users.\n\n"
        "Crossfeed reduces fatigue during extended headphone listening "
        "by mixing a small amount of the left channel into the right "
        "channel and vice versa, simulating the natural crossfeed "
        "that occurs with speakers.");
    infoText->SetExplicitMinSize(BSize(300, 100));
    
    // Layout the HRTF tab
    BLayoutBuilder::Group<>(hrtfTab, B_VERTICAL, 10)
        .SetInsets(15, 15, 15, 15)
        .Add(fHRTFEnabledBox)
        .Add(new BSeparatorView(B_HORIZONTAL))
        .Add(fCrossfeedSlider)
        .Add(new BSeparatorView(B_HORIZONTAL))
        .AddGroup(B_HORIZONTAL)
            .Add(fLoadHRTFButton)
            .AddGlue()
        .End()
        .Add(fHRTFStatusView)
        .Add(new BSeparatorView(B_HORIZONTAL))
        .Add(infoText)
        .AddGlue();
    
    BTab* hrtfTabItem = new BTab(hrtfTab);
    fTabView->AddTab(hrtfTab, hrtfTabItem);
    hrtfTabItem->SetLabel("HRTF & Binaural");
}

void SpatialControlPanel::CreateEnvironmentTab()
{
    BView* envTab = new BView("environment_tab", B_WILL_DRAW);
    envTab->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    
    // Room size controls
    fRoomWidthSlider = new BSlider("room_width_slider",
        "Room Width (m):",
        new BMessage(MSG_ROOM_WIDTH),
        30, 300, B_HORIZONTAL);  // 3.0m to 30.0m (values * 10)
    fRoomWidthSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
    fRoomWidthSlider->SetHashMarkCount(10);
    fRoomWidthSlider->SetValue(100);  // 10.0m default
    fRoomWidthSlider->SetLimitLabels("3m", "30m");
    
    fRoomHeightSlider = new BSlider("room_height_slider", 
        "Room Height (m):",
        new BMessage(MSG_ROOM_HEIGHT),
        20, 150, B_HORIZONTAL);  // 2.0m to 15.0m
    fRoomHeightSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
    fRoomHeightSlider->SetHashMarkCount(8);
    fRoomHeightSlider->SetValue(80);  // 8.0m default
    fRoomHeightSlider->SetLimitLabels("2m", "15m");
    
    fRoomDepthSlider = new BSlider("room_depth_slider",
        "Room Depth (m):",  
        new BMessage(MSG_ROOM_DEPTH),
        20, 200, B_HORIZONTAL);  // 2.0m to 20.0m
    fRoomDepthSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
    fRoomDepthSlider->SetHashMarkCount(10);
    fRoomDepthSlider->SetValue(30);  // 3.0m default
    fRoomDepthSlider->SetLimitLabels("2m", "20m");
    
    // Reverberation controls
    fReverbAmountSlider = new BSlider("reverb_amount_slider",
        "Reverb Amount:",
        new BMessage(MSG_REVERB_AMOUNT), 
        0, 100, B_HORIZONTAL);
    fReverbAmountSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
    fReverbAmountSlider->SetHashMarkCount(11);
    fReverbAmountSlider->SetValue(20);  // 20% default reverb
    fReverbAmountSlider->SetLimitLabels("Dry", "Wet");
    
    fReverbDecaySlider = new BSlider("reverb_decay_slider",
        "Reverb Decay Time:",
        new BMessage(MSG_REVERB_DECAY),
        5, 80, B_HORIZONTAL);  // 0.5s to 8.0s (values / 10)
    fReverbDecaySlider->SetHashMarks(B_HASH_MARKS_BOTTOM); 
    fReverbDecaySlider->SetHashMarkCount(8);
    fReverbDecaySlider->SetValue(15);  // 1.5s default decay
    fReverbDecaySlider->SetLimitLabels("0.5s", "8.0s");
    
    // Environmental effects
    fAirAbsorptionBox = new BCheckBox("air_absorption",
        "Enable Air Absorption (High Frequency Roll-off)",
        new BMessage(MSG_AIR_ABSORPTION));
    fAirAbsorptionBox->SetValue(B_CONTROL_ON);
    
    fDopplerBox = new BCheckBox("doppler_effect",
        "Enable Doppler Effect",
        new BMessage(MSG_DOPPLER));
    fDopplerBox->SetValue(B_CONTROL_ON);
    
    // Layout the environment tab
    BLayoutBuilder::Group<>(envTab, B_VERTICAL, 10)
        .SetInsets(15, 15, 15, 15)
        .AddGroup(B_VERTICAL, 5)
            .Add(new BStringView("room_label", "Room Dimensions:"))
            .Add(fRoomWidthSlider)
            .Add(fRoomHeightSlider) 
            .Add(fRoomDepthSlider)
        .End()
        .Add(new BSeparatorView(B_HORIZONTAL))
        .AddGroup(B_VERTICAL, 5)
            .Add(new BStringView("reverb_label", "Reverberation:"))
            .Add(fReverbAmountSlider)
            .Add(fReverbDecaySlider)
        .End()
        .Add(new BSeparatorView(B_HORIZONTAL))
        .AddGroup(B_VERTICAL, 5)
            .Add(new BStringView("effects_label", "Environmental Effects:"))
            .Add(fAirAbsorptionBox)
            .Add(fDopplerBox)
        .End()
        .AddGlue();
    
    BTab* envTabItem = new BTab(envTab);
    fTabView->AddTab(envTab, envTabItem);
    envTabItem->SetLabel("Environment");
}

void SpatialControlPanel::CreateVisualizationTab()
{
    BView* vizTab = new BView("visualization_tab", B_WILL_DRAW);
    vizTab->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    
    // Visualization toggles
    fShowIndicatorsBox = new BCheckBox("show_indicators",
        "Show Spatial Coordinate Indicators",
        new BMessage(MSG_SHOW_INDICATORS));
    fShowIndicatorsBox->SetValue(B_CONTROL_ON);
    
    fShowListenerBox = new BCheckBox("show_listener",
        "Show Listener Position and Orientation", 
        new BMessage(MSG_SHOW_LISTENER));
    fShowListenerBox->SetValue(B_CONTROL_ON);
    
    fShowRoomBoundsBox = new BCheckBox("show_room_bounds",
        "Show Room Boundaries",
        new BMessage(MSG_SHOW_ROOM_BOUNDS));
    fShowRoomBoundsBox->SetValue(B_CONTROL_ON);
    
    fShowSpeakersBox = new BCheckBox("show_speakers",
        "Show Speaker Layout (Surround Mode)",
        new BMessage(MSG_SHOW_SPEAKERS));
    fShowSpeakersBox->SetValue(B_CONTROL_OFF);
    
    // Instructions text
    BTextView* instructionsText = new BTextView("instructions");
    instructionsText->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    instructionsText->MakeEditable(false);
    instructionsText->SetText(
        "3D Spatial Audio Controls:\n\n"
        "• Left-click and drag audio sources to reposition in 3D space\n"
        "• Left-click and drag the green listener icon to move listener position\n"
        "• Use mouse wheel or right-click-drag to zoom and rotate camera\n"
        "• Press 'R' to reset listener to center position\n"
        "• Press 'S' to toggle spatial indicators on/off\n"
        "• Press 'L' to toggle listener visualization on/off\n\n"
        "Audio sources are constrained within the defined room boundaries. "
        "Real-time spatial parameters (azimuth, elevation, distance) are "
        "calculated automatically and displayed when sources are selected.");
    instructionsText->SetExplicitMinSize(BSize(300, 150));
    
    // Layout the visualization tab
    BLayoutBuilder::Group<>(vizTab, B_VERTICAL, 10)
        .SetInsets(15, 15, 15, 15)
        .AddGroup(B_VERTICAL, 5)
            .Add(new BStringView("viz_label", "Visualization Options:"))
            .Add(fShowIndicatorsBox)
            .Add(fShowListenerBox)
            .Add(fShowRoomBoundsBox)
            .Add(fShowSpeakersBox)
        .End()
        .Add(new BSeparatorView(B_HORIZONTAL))
        .Add(instructionsText)
        .AddGlue();
    
    BTab* vizTabItem = new BTab(vizTab);
    fTabView->AddTab(vizTab, vizTabItem);
    vizTabItem->SetLabel("Visualization");
}

void SpatialControlPanel::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case MSG_SPATIAL_ENABLED:
        {
            bool enabled = (fSpatialEnabledBox->Value() == B_CONTROL_ON);
            printf("SpatialControlPanel: Spatial processing %s\n", 
                   enabled ? "enabled" : "disabled");
            // TODO: Enable/disable spatial processing in audio engine
            break;
        }
        
        case MSG_SPATIAL_MODE:
        {
            BMenuItem* item = nullptr;
            if (message->FindPointer("source", (void**)&item) == B_OK) {
                int32 index = fSpatialModeMenu->IndexOf(item);
                SurroundProcessor::SpatialMode mode = 
                    static_cast<SurroundProcessor::SpatialMode>(index);
                
                if (fSpatialView) {
                    fSpatialView->SetSpatialMode(mode);
                }
                
                printf("SpatialControlPanel: Spatial mode set to %d\n", (int)mode);
            }
            break;
        }
        
        case MSG_HRTF_ENABLED:
        {
            bool enabled = (fHRTFEnabledBox->Value() == B_CONTROL_ON);
            if (fAudioProcessor) {
                // Enable/disable HRTF in processor
                printf("SpatialControlPanel: HRTF processing %s\n",
                       enabled ? "enabled" : "disabled");
            }
            break;
        }
        
        case MSG_CROSSFEED:
        {
            int32 value = fCrossfeedSlider->Value();
            float crossfeed = value / 100.0f;  // Convert to 0.0-1.0 range
            if (fAudioProcessor) {
                fAudioProcessor->GetSurroundProcessor().EnableCrossfeed(true, crossfeed);
                printf("SpatialControlPanel: Crossfeed set to %.1f%%\n", crossfeed * 100.0f);
            }
            break;
        }
        
        case MSG_LOAD_HRTF:
        {
            // TODO: Implement HRTF file loading dialog
            BAlert* alert = new BAlert("HRTF Loading",
                "HRTF database loading will be implemented in a future version.\n"
                "Currently using built-in generic HRTF.",
                "OK", nullptr, nullptr, B_WIDTH_AS_USUAL, B_INFO_ALERT);
            alert->Go();
            break;
        }
        
        case MSG_ROOM_WIDTH:
        {
            int32 value = fRoomWidthSlider->Value();
            float width = value / 10.0f;  // Convert to meters
            if (fAudioProcessor) {
                // Update room width - need to get current height and depth
                ::VeniceDAW::DSP::Vector3D currentRoom = ::VeniceDAW::DSP::Vector3D(10.0f, 8.0f, 3.0f);  // TODO: Get from processor
                currentRoom.x = width;
                fAudioProcessor->GetSurroundProcessor().SetRoomSize(
                    currentRoom.x, currentRoom.y, currentRoom.z);
                printf("SpatialControlPanel: Room width set to %.1fm\n", width);
            }
            break;
        }
        
        case MSG_ROOM_HEIGHT:
        case MSG_ROOM_DEPTH:
        case MSG_REVERB_AMOUNT:
        case MSG_REVERB_DECAY:
        {
            // Similar handling for other sliders
            printf("SpatialControlPanel: Environmental parameter updated\n");
            break;
        }
        
        case MSG_AIR_ABSORPTION:
        {
            bool enabled = (fAirAbsorptionBox->Value() == B_CONTROL_ON);
            if (fAudioProcessor) {
                fAudioProcessor->GetSurroundProcessor().SetAirAbsorption(enabled);
                printf("SpatialControlPanel: Air absorption %s\n",
                       enabled ? "enabled" : "disabled");
            }
            break;
        }
        
        case MSG_DOPPLER:
        {
            bool enabled = (fDopplerBox->Value() == B_CONTROL_ON);
            if (fAudioProcessor) {
                fAudioProcessor->GetSurroundProcessor().SetDopplerEffect(enabled);
                printf("SpatialControlPanel: Doppler effect %s\n", 
                       enabled ? "enabled" : "disabled");
            }
            break;
        }
        
        case MSG_SHOW_INDICATORS:
        {
            bool show = (fShowIndicatorsBox->Value() == B_CONTROL_ON);
            if (fSpatialView) {
                fSpatialView->SetShowSpatialIndicators(show);
            }
            break;
        }
        
        case MSG_SHOW_LISTENER:
        {
            bool show = (fShowListenerBox->Value() == B_CONTROL_ON);
            if (fSpatialView) {
                fSpatialView->SetShowListenerVisualization(show);
            }
            break;
        }
        
        case MSG_SHOW_ROOM_BOUNDS:
        {
            bool show = (fShowRoomBoundsBox->Value() == B_CONTROL_ON);
            if (fSpatialView) {
                fSpatialView->SetShowRoomBoundaries(show);
            }
            break;
        }
        
        case MSG_SHOW_SPEAKERS:
        {
            // TODO: Implement speaker layout visualization toggle
            break;
        }
        
        default:
            BView::MessageReceived(message);
            break;
    }
}

void SpatialControlPanel::UpdateFromProcessor()
{
    if (!fAudioProcessor) return;
    
    const SurroundProcessor& processor = fAudioProcessor->GetSurroundProcessor();
    
    // Update spatial mode selection
    SurroundProcessor::SpatialMode currentMode = processor.GetSpatialMode();
    if (fSpatialModeMenu && (int)currentMode < fSpatialModeMenu->CountItems()) {
        // Clear all marks
        for (int32 i = 0; i < fSpatialModeMenu->CountItems(); i++) {
            fSpatialModeMenu->ItemAt(i)->SetMarked(false);
        }
        // Mark current mode
        fSpatialModeMenu->ItemAt((int)currentMode)->SetMarked(true);
    }
    
    // printf("SpatialControlPanel: Updated controls from processor state\n");  // Debug spam removed
}

// =====================================
// SpatialMixer3DWindow Implementation
// =====================================

SpatialMixer3DWindow::SpatialMixer3DWindow(SimpleHaikuEngine* engine, 
                                         AdvancedAudioProcessor* processor)
    : BWindow(BRect(100, 100, 1200, 800), "VeniceDAW Phase 5 - Spatial Audio with File Loading",
             B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
    , fEngine(engine)
    , fAudioProcessor(processor)
    , fSpatialView(nullptr)
    , fControlPanel(nullptr)
    , fUpdateRunner(nullptr)
    , fOpenFilePanel(nullptr)
{
    CreateInterface();
    
    // Start update timer for real-time visualization
    BMessage updateMsg(MSG_UPDATE_SPATIAL);
    fUpdateRunner = new BMessageRunner(this, &updateMsg, 33333);  // 30 FPS updates
    
    printf("SpatialMixer3DWindow: Professional spatial audio mixer window created\n");
}

SpatialMixer3DWindow::~SpatialMixer3DWindow()
{
    delete fUpdateRunner;
    delete fOpenFilePanel;
    printf("SpatialMixer3DWindow: Spatial mixer window destroyed\n");
}

void SpatialMixer3DWindow::CreateInterface()
{
    CreateMenuBar();
    
    // Create main horizontal layout: 3D view on left, controls on right
    BView* mainView = new BView("main_view", B_WILL_DRAW);
    mainView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    
    // Create spatial 3D view (takes 70% of width)
    BRect bounds = Bounds();
    bounds.top += fMenuBar->Frame().Height() + 1;
    BRect viewRect = bounds;
    viewRect.right = bounds.Width() * 0.7f;
    
    fSpatialView = new SpatialMixer3DView(viewRect, fEngine, fAudioProcessor);
    
    // Create control panel (takes 30% of width)
    BRect controlRect = bounds;
    controlRect.left = viewRect.right + 1;
    
    fControlPanel = new SpatialControlPanel(controlRect, fSpatialView, fAudioProcessor);
    
    // Add views to main layout
    BLayoutBuilder::Group<>(mainView, B_HORIZONTAL, 0)
        .Add(fSpatialView, 7.0f)  // 70% weight
        .Add(fControlPanel, 3.0f);  // 30% weight
    
    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .Add(fMenuBar)
        .Add(mainView);
}

void SpatialMixer3DWindow::CreateMenuBar()
{
    fMenuBar = new BMenuBar("menu_bar");
    
    // File menu
    BMenu* fileMenu = new BMenu("File");
    fileMenu->AddItem(new BMenuItem("Open Audio File...", new BMessage('oaf_')));
    fileMenu->AddItem(new BMenuItem("Open Multiple Files...", new BMessage('oamf')));
    fileMenu->AddSeparatorItem();
    fileMenu->AddItem(new BMenuItem("Save Spatial Configuration...", nullptr));
    fileMenu->AddItem(new BMenuItem("Load Spatial Configuration...", nullptr));
    fileMenu->AddSeparatorItem();
    fileMenu->AddItem(new BMenuItem("Export Spatial Audio Mix...", nullptr));
    fMenuBar->AddItem(fileMenu);
    
    // View menu  
    BMenu* viewMenu = new BMenu("View");
    viewMenu->AddItem(new BMenuItem("Zoom In", new BMessage('zmin'), '+'));
    viewMenu->AddItem(new BMenuItem("Zoom Out", new BMessage('zmot'), '-'));
    viewMenu->AddSeparatorItem();
    viewMenu->AddItem(new BMenuItem("Reset Camera", new BMessage('rset'), 'R'));
    viewMenu->AddItem(new BMenuItem("Toggle Fullscreen 3D", nullptr, 'F'));
    viewMenu->AddSeparatorItem();
    viewMenu->AddItem(new BMenuItem("Show Performance Metrics", nullptr));
    fMenuBar->AddItem(viewMenu);
    
    // Spatial menu
    BMenu* spatialMenu = new BMenu("Spatial");
    spatialMenu->AddItem(new BMenuItem("Reset All Positions", nullptr));
    spatialMenu->AddItem(new BMenuItem("Auto-Arrange Sources", nullptr));
    spatialMenu->AddSeparatorItem();
    spatialMenu->AddItem(new BMenuItem("Spatial Audio Settings...", nullptr));
    fMenuBar->AddItem(spatialMenu);
    
    // Help menu
    BMenu* helpMenu = new BMenu("Help");
    helpMenu->AddItem(new BMenuItem("Spatial Audio Guide", nullptr));
    helpMenu->AddItem(new BMenuItem("HRTF Information", nullptr));
    helpMenu->AddSeparatorItem();
    helpMenu->AddItem(new BMenuItem("About VeniceDAW Phase 4", nullptr));
    fMenuBar->AddItem(helpMenu);
}

bool SpatialMixer3DWindow::QuitRequested()
{
    printf("SpatialMixer3DWindow: Quit requested\n");
    be_app->PostMessage(B_QUIT_REQUESTED);
    return true;
}

void SpatialMixer3DWindow::MessageReceived(BMessage* message)
{
    // Define zoom message constants from Mixer3DWindow
    const uint32 MSG_ZOOM_IN = 'zmin';
    const uint32 MSG_ZOOM_OUT = 'zmot';
    const uint32 MSG_RESET_CAMERA = 'rset';
    
    switch (message->what) {
        case MSG_UPDATE_SPATIAL:
            UpdateSpatialVisualization();
            break;
            
        case MSG_ZOOM_IN:
            if (fSpatialView) {
                fSpatialView->ZoomCamera(-2.0f);  // Negative = zoom in (closer)
                printf("SpatialMixer3DWindow: Zoomed in\n");
            }
            break;
            
        case MSG_ZOOM_OUT:
            if (fSpatialView) {
                fSpatialView->ZoomCamera(3.0f);   // Positive = zoom out (farther)
                printf("SpatialMixer3DWindow: Zoomed out\n");
            }
            break;
            
        case MSG_RESET_CAMERA:
            if (fSpatialView) {
                fSpatialView->ResetCamera();
                printf("SpatialMixer3DWindow: Camera reset\n");
            }
            break;
            
        case MSG_OPEN_AUDIO_FILE:
        {
            // Create file panel if it doesn't exist
            if (!fOpenFilePanel) {
                BMessage* selectMsg = new BMessage(MSG_FILE_REFS);
                fOpenFilePanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), nullptr,
                                               B_FILE_NODE, false, selectMsg);
                
                // Set audio file filters
                fOpenFilePanel->SetButtonLabel(B_DEFAULT_BUTTON, "Load Audio");
            }
            
            fOpenFilePanel->Show();
            break;
        }
        
        case MSG_OPEN_MULTIPLE_FILES:
        {
            // Create file panel for multiple selection
            if (!fOpenFilePanel) {
                BMessage* selectMsg = new BMessage(MSG_FILE_REFS);
                fOpenFilePanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), nullptr,
                                               B_FILE_NODE, true, selectMsg);  // true = allow multiple
                
                fOpenFilePanel->SetButtonLabel(B_DEFAULT_BUTTON, "Load Audio Files");
            }
            
            fOpenFilePanel->Show();
            break;
        }
        
        case MSG_FILE_REFS:
        case B_REFS_RECEIVED:
        {
            entry_ref ref;
            int32 count = 0;
            
            // Count how many files we're loading
            while (message->FindRef("refs", count, &ref) == B_OK) {
                count++;
            }
            
            printf("SpatialMixer3DWindow: Loading %d audio file(s)...\n", (int)count);
            
            // Load each file as a new track
            int32 loadedCount = 0;
            int32 failedCount = 0;
            
            for (int32 i = 0; i < count; i++) {
                if (message->FindRef("refs", i, &ref) == B_OK) {
                    status_t status = fEngine->LoadAudioFileAsTrack(ref);
                    if (status == B_OK) {
                        loadedCount++;
                        printf("  ✓ Loaded: %s\n", ref.name);
                    } else {
                        failedCount++;
                        printf("  ✗ Failed: %s (%s)\n", ref.name, strerror(status));
                    }
                }
            }
            
            // Show result to user
            BString resultMsg;
            if (loadedCount > 0) {
                resultMsg.SetToFormat("Successfully loaded %d audio file(s) as spatial tracks!", 
                                     (int)loadedCount);
                if (failedCount > 0) {
                    resultMsg.Append(BString().SetToFormat("\n\n%d file(s) failed to load.", 
                                                          (int)failedCount));
                }
                
                BAlert* alert = new BAlert("Audio Loading", resultMsg.String(), "OK", 
                                          nullptr, nullptr, B_WIDTH_AS_USUAL, B_INFO_ALERT);
                alert->Go();
                
                // Update 3D view to show new tracks
                UpdateSpatialVisualization();
                
            } else if (failedCount > 0) {
                resultMsg.SetToFormat("Failed to load %d audio file(s).\n\n"
                                     "Supported formats: WAV, AIFF, MP3 (if available).", 
                                     (int)failedCount);
                
                BAlert* alert = new BAlert("Loading Error", resultMsg.String(), "OK", 
                                          nullptr, nullptr, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
                alert->Go();
            }
            
            break;
        }
            
        default:
            BWindow::MessageReceived(message);
            break;
    }
}

void SpatialMixer3DWindow::UpdateSpatialVisualization()
{
    if (fSpatialView && fSpatialView->LockLooper()) {
        // IMPORTANT: First update the base tracks from engine!
        fSpatialView->UpdateTracks();  // This populates f3DTracks with actual tracks
        // Then update spatial tracks from the base tracks
        fSpatialView->UpdateSpatialTracks();
        fSpatialView->Invalidate();
        fSpatialView->UnlockLooper();
    }
    
    if (fControlPanel && fControlPanel->LockLooper()) {
        fControlPanel->UpdateFromProcessor();
        fControlPanel->UnlockLooper();
    }
}

} // namespace HaikuDAW
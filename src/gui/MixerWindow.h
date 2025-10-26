/*
 * MixerWindow.h - Native Haiku mixer interface
 * Professional audio mixing console using BLayout
 */

#ifndef MIXER_WINDOW_H
#define MIXER_WINDOW_H

#include <Window.h>
#include <View.h>
#include <Box.h>
#include <Slider.h>
#include <Button.h>
#include <CheckBox.h>
#include <Control.h>
#include <StringView.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <GroupLayout.h>
#include <GridLayout.h>
#include <MessageRunner.h>
#include <FilePanel.h>
#include <Entry.h>
#include <Path.h>
#include <Alert.h>
#include <vector>

// 3dmix import support
#include "../audio/3dmix/3DMixProjectImporter.h"

namespace HaikuDAW {

// Forward declarations
class SimpleHaikuEngine;
class SimpleTrack;
class TrackInspectorPanel;

/*
 * Custom toggle button - looks like BButton but acts like BCheckBox
 */
class ToggleButton : public BButton {
public:
    ToggleButton(const char* name, const char* label, BMessage* message);
    virtual ~ToggleButton();
    
    virtual void MouseDown(BPoint where);
    virtual void Draw(BRect updateRect);
    
    void SetToggled(bool toggled);
    bool IsToggled() const { return fToggled; }
    
    void SetToggleColors(rgb_color normal, rgb_color pressed);
    
private:
    bool fToggled;
    rgb_color fNormalColor;
    rgb_color fPressedColor;
};

/*
 * Level meter - custom BView
 */
class LevelMeter : public BView {
public:
    LevelMeter();
    virtual ~LevelMeter();
    
    virtual void Draw(BRect updateRect);
    virtual void GetPreferredSize(float* width, float* height);
    
    void SetLevel(float peak, float rms);

private:
    float fPeakLevel;
    float fRMSLevel;
    bigtime_t fLastUpdate;
};

/*
 * Single channel strip - vertical layout
 */
class ChannelStrip : public BView {
public:
    ChannelStrip(SimpleTrack* track);
    virtual ~ChannelStrip();
    
    // BView interface
    virtual void AttachedToWindow();
    virtual void MessageReceived(BMessage* message);
    virtual void MouseDown(BPoint where);
    virtual void DragEnter(BMessage* message);
    virtual void DragLeave();
    virtual void Drop(BMessage* message, BPoint where);
    
    // Update from track data
    void UpdateLevels();
    void UpdateControls();
    
    // Track access
    SimpleTrack* GetTrack() const { return fTrack; }

    // Selection state
    void SetSelected(bool selected);
    bool IsSelected() const { return fSelected; }

    // Friend access for solo button updates
    friend class MixerWindow;

private:
    void CreateControls();

    SimpleTrack* fTrack;
    bool fSelected;
    
    // GUI controls
    BStringView* fTrackName;
    BSlider* fVolumeSlider;
    BSlider* fPanSlider;
    ToggleButton* fMuteButton;
    LevelMeter* fLevelMeter;
    
public:
    ToggleButton* fSoloButton;  // Public for MixerWindow access
    
    // Message constants
    enum {
        MSG_VOLUME_CHANGED = 'vol_',
        MSG_PAN_CHANGED = 'pan_',
        MSG_MUTE_TOGGLED = 'mut_',
        MSG_SOLO_TOGGLED = 'sol_',
        MSG_LOAD_AUDIO_TO_TRACK = 'latt',
        MSG_CLEAR_TRACK = 'cltr'
    };
};

/*
 * Main mixer window
 */
class MixerWindow : public BWindow {
public:
    MixerWindow(SimpleHaikuEngine* engine, int startTrack = 0, int maxTracks = kMaxTracksPerWindow);
    virtual ~MixerWindow();
    
    // BWindow interface
    virtual bool QuitRequested();
    virtual void MessageReceived(BMessage* message);
    virtual void DispatchMessage(BMessage* message, BHandler* handler);
    
    // Engine control
    void UpdateMeter();
    void RefreshControls();
    
    // Solo management
    void SetTrackSolo(int trackIndex, bool solo);
    void UpdateSoloButtons();
    
    // Track management
    void AddTrack();
    void RemoveTrack();
    bool CanAddTrack() const;
    bool CanRemoveTrack() const;

    // Audio file import
    void ImportAudioFile();
    void ImportMultipleFiles();
    void HandleDroppedFiles(BMessage* message);

    // 3dmix project import
    void Import3DMixProject();
    void Show3DMixImportDialog(const char* filePath);
    void Handle3DMixImportResult(const ::VeniceDAW::ImportResult& result);

    // Batch loading
    status_t LoadAudioFilesBatch(const std::vector<entry_ref>& files);
    void ShowBatchImportProgress(int32 current, int32 total, const char* filename);

    // Selective track loading (NEW)
    void LoadAudioFileToSpecificTrack(int32 trackIndex);
    void ShowTrackSelectionDialog();
    status_t LoadAudioFileToTrack(const entry_ref& ref, int32 trackIndex);
    ChannelStrip* FindChannelStripByTrackIndex(int32 trackIndex);
    
    // Window management
    void UpdateWindowsMenu();

    // Message constants (public for ChannelStrip access)
    enum {
        MSG_PLAY = 'play',
        MSG_STOP = 'stop',
        MSG_MASTER_VOLUME = 'mast',
        MSG_UPDATE_METERS = 'updt',
        MSG_SHOW_3D_MIXER = 'sh3d',
        MSG_SHOW_TIMELINE = 'shtl',
        MSG_ADD_TRACK = 'addt',
        MSG_REMOVE_TRACK = 'remt',
        MSG_IMPORT_AUDIO = 'impa',
        MSG_IMPORT_MULTIPLE = 'impm',
        MSG_IMPORT_3DMIX = 'im3d',
        MSG_3DMIX_IMPORT_COMPLETE = '3dic',
        MSG_REFRESH_WINDOWS = 'rfrw',
        MSG_SHOW_SUPER_MASTER = 'shsm',
        MSG_LOAD_AUDIO_TO_SPECIFIC_TRACK = 'last',
        MSG_SHOW_TRACK_SELECTION_DIALOG = 'stsd'
    };

private:
    void CreateMenuBar();
    void CreateMixerView();
    void CreateMasterSection();
    void CreateChannelStrips();
    
    // Solo button management
    void RefreshAllSoloButtons();
    
    SimpleHaikuEngine* fEngine;
    int fStartTrack;    // First track index for this window
    int fMaxTracks;     // Max tracks this window can show
    
    // GUI components
    BMenuBar* fMenuBar;
    BView* fMainView;
    BView* fChannelArea;
    BBox* fMasterSection;
    
    // Channel strips
    std::vector<ChannelStrip*> fChannelStrips;
    static const int kMinTracks = 1;
    static const int kMaxTracksPerWindow = 8;

    // Track inspector panel (right sidebar)
    TrackInspectorPanel* fInspectorPanel;
    
    // Master controls
    BSlider* fMasterVolume;
    BButton* fPlayButton;
    BButton* fStopButton;
    BStringView* fStatusDisplay;
    LevelMeter* fMasterLevelLeft;   // Left channel VU meter
    LevelMeter* fMasterLevelRight;  // Right channel VU meter
    
    // Update timer
    BMessageRunner* fUpdateRunner;

    // 3dmix import
    ::VeniceDAW::ThreeDMixProjectImporter* f3DMixImporter;
};

} // namespace HaikuDAW

#endif
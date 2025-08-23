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
#include <vector>

namespace HaikuDAW {

// Forward declarations
class SimpleHaikuEngine;
class SimpleTrack;

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
    
    // Update from track data
    void UpdateLevels();
    void UpdateControls();
    
    // Track access
    SimpleTrack* GetTrack() const { return fTrack; }
    
    // Friend access for solo button updates
    friend class MixerWindow;

private:
    void CreateControls();
    
    SimpleTrack* fTrack;
    
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
        MSG_SOLO_TOGGLED = 'sol_'
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
    
    // Window management
    void UpdateWindowsMenu();

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
    
    // Master controls
    BSlider* fMasterVolume;
    BButton* fPlayButton;
    BButton* fStopButton;
    BStringView* fStatusDisplay;
    LevelMeter* fMasterLevelLeft;   // Left channel VU meter
    LevelMeter* fMasterLevelRight;  // Right channel VU meter
    
    // Update timer
    BMessageRunner* fUpdateRunner;
    
    // Message constants
    enum {
        MSG_PLAY = 'play',
        MSG_STOP = 'stop',
        MSG_MASTER_VOLUME = 'mast',
        MSG_UPDATE_METERS = 'updt',
        MSG_SHOW_3D_MIXER = 'sh3d',
        MSG_ADD_TRACK = 'addt',
        MSG_REMOVE_TRACK = 'remt',
        MSG_REFRESH_WINDOWS = 'rfrw',
        MSG_SHOW_SUPER_MASTER = 'shsm'
    };
};

} // namespace HaikuDAW

#endif
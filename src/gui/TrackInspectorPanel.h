/*
 * TrackInspectorPanel.h - Unified track properties inspector
 * Displays all track parameters in one convenient panel
 */

#ifndef TRACK_INSPECTOR_PANEL_H
#define TRACK_INSPECTOR_PANEL_H

#include <interface/View.h>
#include <interface/StringView.h>
#include <interface/TextControl.h>
#include <interface/Slider.h>
#include <interface/CheckBox.h>
#include <interface/Button.h>
#include <interface/Box.h>
#include <support/String.h>

namespace HaikuDAW {

// Forward declaration
class SimpleTrack;

/*
 * TrackInspectorPanel - Unified inspector for track properties
 *
 * Displays and allows editing of:
 * - File information (path, duration, sample rate)
 * - Audio controls (volume, pan, mute, solo)
 * - 3D position (X, Y, Z coordinates)
 * - Real-time level meters
 * - Effects chain (future)
 *
 * Updates in real-time when track selection changes or
 * when track properties are modified elsewhere.
 */
class TrackInspectorPanel : public BView {
public:
    TrackInspectorPanel(BRect frame);
    virtual ~TrackInspectorPanel();

    // Track selection
    void SetTrack(SimpleTrack* track);
    SimpleTrack* GetTrack() const { return fSelectedTrack; }

    // Real-time updates (called from audio callback timer)
    void UpdateLevels();

    // BView overrides
    virtual void AttachedToWindow() override;
    virtual void MessageReceived(BMessage* message) override;

private:
    // Current track
    SimpleTrack* fSelectedTrack;

    // UI Sections (BBox groups)
    BBox* fFileInfoBox;
    BBox* fAudioControlsBox;
    BBox* fPositionBox;
    BBox* fLevelsBox;

    // File Info section
    BStringView* fFilePathLabel;
    BStringView* fDurationLabel;
    BStringView* fSampleRateLabel;
    BStringView* fChannelsLabel;

    // Audio Controls section
    BSlider* fVolumeSlider;
    BSlider* fPanSlider;
    BCheckBox* fMuteCheckbox;
    BCheckBox* fSoloCheckbox;

    // 3D Position section
    BTextControl* fPositionXControl;
    BTextControl* fPositionYControl;
    BTextControl* fPositionZControl;
    BButton* fResetPositionButton;
    BStringView* fDistanceLabel;

    // Levels section
    BView* fPeakMeterView;
    BView* fRMSMeterView;
    BStringView* fPeakValueLabel;
    BStringView* fRMSValueLabel;

    // Message constants
    enum {
        MSG_VOLUME_CHANGED = 'volc',
        MSG_PAN_CHANGED = 'panc',
        MSG_MUTE_CHANGED = 'mutc',
        MSG_SOLO_CHANGED = 'solc',
        MSG_POSITION_X_CHANGED = 'posx',
        MSG_POSITION_Y_CHANGED = 'posy',
        MSG_POSITION_Z_CHANGED = 'posz',
        MSG_RESET_POSITION = 'rstp'
    };

    // Internal methods
    void _CreateFileInfoSection(BRect frame);
    void _CreateAudioControlsSection(BRect frame);
    void _CreatePositionSection(BRect frame);
    void _CreateLevelsSection(BRect frame);
    void _UpdateFileInfo();
    void _UpdateAudioControls();
    void _UpdatePosition();
    void _ApplyVolumeChange(float volume);
    void _ApplyPanChange(float pan);
    void _ApplyMuteChange(bool muted);
    void _ApplySoloChange(bool solo);
    void _ApplyPositionChange(float x, float y, float z);
    void _DrawLevelMeter(BView* view, float level);
};

} // namespace HaikuDAW

#endif // TRACK_INSPECTOR_PANEL_H

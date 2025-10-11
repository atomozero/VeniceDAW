/*
 * AudioPreviewPanel.h - Audio file preview for VeniceDAW file dialogs
 * Provides real-time audio preview when selecting files
 */

#ifndef AUDIO_PREVIEW_PANEL_H
#define AUDIO_PREVIEW_PANEL_H

#include <View.h>
#include <Button.h>
#include <StringView.h>
#include <Slider.h>
#include <MessageRunner.h>
#include <storage/Entry.h>
#include <storage/Path.h>
#include <storage/FilePanel.h>
#include <StorageDefs.h>  // For file_panel_mode
#include <media/MediaFile.h>
#include <media/MediaTrack.h>
#include <media/SoundPlayer.h>
#include <interface/LayoutBuilder.h>

namespace VeniceDAW {

/*
 * Audio preview widget for file panels
 */
class AudioPreviewPanel : public BView {
public:
    AudioPreviewPanel(BRect frame);
    virtual ~AudioPreviewPanel();

    // BView interface
    virtual void AttachedToWindow() override;
    virtual void MessageReceived(BMessage* message) override;
    virtual void Draw(BRect updateRect) override;

    // Preview control
    status_t SetPreviewFile(const entry_ref& ref);
    void ClearPreview();
    bool IsPreviewActive() const { return fPreviewing; }

    // Playback control
    void StartPreview();
    void StopPreview();
    void SetPreviewVolume(float volume);

private:
    // Internal methods
    status_t LoadAudioFile(const entry_ref& ref);
    void UnloadAudioFile();
    status_t InitializePlayer();
    void CleanupPlayer();
    void UpdateDisplay();
    void UpdateTimeDisplay();

    // Audio callback
    static void AudioPlayFunc(void* cookie, void* buffer, size_t size, const media_raw_audio_format& format);
    void ProcessPreviewAudio(float* buffer, size_t frameCount);

    // GUI components
    BStringView* fFileInfoLabel;
    BStringView* fTimeDisplay;
    BButton* fPlayButton;
    BButton* fStopButton;
    BSlider* fVolumeSlider;
    BSlider* fPositionSlider;

    // Audio file info
    BMediaFile* fMediaFile;
    BMediaTrack* fMediaTrack;
    media_format fFileFormat;
    BString fFilePath;
    int64 fFileDuration;
    float fFileSampleRate;
    bool fFileLoaded;

    // Playback state
    BSoundPlayer* fSoundPlayer;
    bool fPreviewing;
    bool fPlayerInitialized;
    int64 fPlaybackPosition;
    float fPreviewVolume;

    // Audio buffer for preview
    float* fPreviewBuffer;
    size_t fPreviewBufferSize;
    size_t fPreviewBufferFrames;

    // Update timer
    BMessageRunner* fUpdateRunner;

    // Message constants
    enum {
        MSG_PLAY_PREVIEW = 'plpr',
        MSG_STOP_PREVIEW = 'stpr',
        MSG_VOLUME_CHANGED = 'volc',
        MSG_POSITION_CHANGED = 'posc',
        MSG_UPDATE_DISPLAY = 'updd'
    };
};

/*
 * Enhanced file panel with audio preview
 */
class AudioFilePanel : public BFilePanel {
public:
    AudioFilePanel(file_panel_mode mode = B_OPEN_PANEL,
                   BMessenger* target = nullptr,
                   const entry_ref* panel_directory = nullptr,
                   uint32 node_flavors = B_FILE_NODE,
                   bool allow_multiple_selection = false,
                   BMessage* message = nullptr,
                   BRefFilter* filter = nullptr,
                   bool modal = false,
                   bool hide_when_done = true);

    virtual ~AudioFilePanel();

    // BFilePanel interface
    virtual void SelectionChanged() override;
    virtual void Show();  // Not virtual in BFilePanel, so no override

private:
    void SetupPreviewPanel();
    void HandleSelectionChanged();

    AudioPreviewPanel* fPreviewPanel;
    bool fPreviewEnabled;
};

} // namespace VeniceDAW

#endif
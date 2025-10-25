/*
 * AudioPreviewPanel.cpp - Audio file preview implementation
 */

#include "AudioPreviewPanel.h"
#include <Alert.h>
#include <LayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>

namespace VeniceDAW {

// =====================================
// AudioPreviewPanel Implementation
// =====================================

AudioPreviewPanel::AudioPreviewPanel(BRect frame)
    : BView(frame, "audio_preview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS)
    , fFileInfoLabel(nullptr)
    , fTimeDisplay(nullptr)
    , fPlayButton(nullptr)
    , fStopButton(nullptr)
    , fVolumeSlider(nullptr)
    , fPositionSlider(nullptr)
    , fMediaFile(nullptr)
    , fMediaTrack(nullptr)
    , fFileDuration(0)
    , fFileSampleRate(44100.0f)
    , fFileLoaded(false)
    , fSoundPlayer(nullptr)
    , fPreviewing(false)
    , fPlayerInitialized(false)
    , fPlaybackPosition(0)
    , fPreviewVolume(0.5f)
    , fPreviewBuffer(nullptr)
    , fPreviewBufferSize(0)
    , fPreviewBufferFrames(0)
    , fUpdateRunner(nullptr)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    printf("AudioPreviewPanel: Created\n");
}

AudioPreviewPanel::~AudioPreviewPanel()
{
    StopPreview();
    UnloadAudioFile();
    CleanupPlayer();

    delete fUpdateRunner;

    if (fPreviewBuffer) {
        delete[] fPreviewBuffer;
    }

    printf("AudioPreviewPanel: Destroyed\n");
}

void AudioPreviewPanel::AttachedToWindow()
{
    BView::AttachedToWindow();

    // Create layout
    BGroupLayout* layout = nullptr;
    BLayoutBuilder::Group<>(this, B_VERTICAL, 5)
        .SetInsets(10, 10, 10, 10)
        .GetLayout(&layout);

    // File info display
    fFileInfoLabel = new BStringView("file_info", "No file selected");
    fFileInfoLabel->SetAlignment(B_ALIGN_CENTER);
    layout->AddView(fFileInfoLabel);

    // Time display
    fTimeDisplay = new BStringView("time_display", "00:00 / 00:00");
    fTimeDisplay->SetAlignment(B_ALIGN_CENTER);
    layout->AddView(fTimeDisplay);

    // Position slider
    fPositionSlider = new BSlider("position", "Position",
                                  new BMessage(MSG_POSITION_CHANGED),
                                  0, 1000, B_HORIZONTAL);
    fPositionSlider->SetTarget(this);
    fPositionSlider->SetEnabled(false);
    layout->AddView(fPositionSlider);

    // Control buttons
    fPlayButton = new BButton("play", "▶ Play", new BMessage(MSG_PLAY_PREVIEW));
    fPlayButton->SetTarget(this);
    fPlayButton->SetEnabled(false);

    fStopButton = new BButton("stop", "⏹ Stop", new BMessage(MSG_STOP_PREVIEW));
    fStopButton->SetTarget(this);
    fStopButton->SetEnabled(false);

    // Create button container with horizontal layout
    BView* buttonContainer = BLayoutBuilder::Group<>(B_HORIZONTAL, 5)
        .Add(fPlayButton)
        .Add(fStopButton)
        .View();

    layout->AddView(buttonContainer);

    // Volume control
    fVolumeSlider = new BSlider("volume", "Volume",
                                new BMessage(MSG_VOLUME_CHANGED),
                                0, 100, B_HORIZONTAL);
    fVolumeSlider->SetValue(50);  // 50% volume
    fVolumeSlider->SetTarget(this);
    fVolumeSlider->SetLimitLabels("0%", "100%");
    layout->AddView(fVolumeSlider);

    printf("AudioPreviewPanel: GUI created\n");
}

void AudioPreviewPanel::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case MSG_PLAY_PREVIEW:
            StartPreview();
            break;

        case MSG_STOP_PREVIEW:
            StopPreview();
            break;

        case MSG_VOLUME_CHANGED:
            if (fVolumeSlider) {
                fPreviewVolume = fVolumeSlider->Value() / 100.0f;
                printf("AudioPreviewPanel: Volume set to %.2f\n", fPreviewVolume);
            }
            break;

        case MSG_POSITION_CHANGED:
            if (fPositionSlider && fFileLoaded && !fPreviewing) {
                // Allow scrubbing when not playing
                float position = fPositionSlider->Value() / 1000.0f;
                fPlaybackPosition = (int64)(position * fFileDuration);
                UpdateTimeDisplay();
            }
            break;

        case MSG_UPDATE_DISPLAY:
            UpdateDisplay();
            break;

        default:
            BView::MessageReceived(message);
            break;
    }
}

void AudioPreviewPanel::Draw(BRect updateRect)
{
    BView::Draw(updateRect);

    // Draw a simple border
    BRect bounds = Bounds();
    SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
    StrokeRect(bounds);
}

status_t AudioPreviewPanel::SetPreviewFile(const entry_ref& ref)
{
    printf("AudioPreviewPanel: Setting preview file: %s\n", ref.name);

    // Stop any current preview
    StopPreview();

    // Load the new file
    status_t status = LoadAudioFile(ref);

    if (status == B_OK) {
        // Update GUI
        UpdateDisplay();

        // Enable controls
        if (fPlayButton) fPlayButton->SetEnabled(true);
        if (fPositionSlider) fPositionSlider->SetEnabled(true);

        printf("AudioPreviewPanel: Preview file loaded successfully\n");
    } else {
        printf("AudioPreviewPanel: Failed to load preview file: %s\n", strerror(status));
        ClearPreview();
    }

    return status;
}

void AudioPreviewPanel::ClearPreview()
{
    StopPreview();
    UnloadAudioFile();

    // Update GUI
    if (fFileInfoLabel) fFileInfoLabel->SetText("No file selected");
    if (fTimeDisplay) fTimeDisplay->SetText("00:00 / 00:00");

    // Disable controls
    if (fPlayButton) fPlayButton->SetEnabled(false);
    if (fStopButton) fStopButton->SetEnabled(false);
    if (fPositionSlider) {
        fPositionSlider->SetEnabled(false);
        fPositionSlider->SetValue(0);
    }

    printf("AudioPreviewPanel: Preview cleared\n");
}

void AudioPreviewPanel::StartPreview()
{
    if (!fFileLoaded) {
        printf("AudioPreviewPanel: No file loaded for preview\n");
        return;
    }

    if (fPreviewing) {
        printf("AudioPreviewPanel: Already previewing\n");
        return;
    }

    printf("AudioPreviewPanel: Starting preview\n");

    // Initialize player if needed
    status_t status = InitializePlayer();
    if (status != B_OK) {
        printf("AudioPreviewPanel: Failed to initialize player: %s\n", strerror(status));
        return;
    }

    // Start playback
    status = fSoundPlayer->Start();
    if (status != B_OK) {
        printf("AudioPreviewPanel: Failed to start player: %s\n", strerror(status));
        return;
    }

    fPreviewing = true;

    // Update GUI
    if (fPlayButton) fPlayButton->SetEnabled(false);
    if (fStopButton) fStopButton->SetEnabled(true);

    // Start update timer
    BMessage updateMsg(MSG_UPDATE_DISPLAY);
    fUpdateRunner = new BMessageRunner(BMessenger(this), &updateMsg, 100000); // 10 FPS

    printf("AudioPreviewPanel: Preview started\n");
}

void AudioPreviewPanel::StopPreview()
{
    if (!fPreviewing) {
        return;
    }

    printf("AudioPreviewPanel: Stopping preview\n");

    // Stop the player
    if (fSoundPlayer) {
        fSoundPlayer->Stop();
    }

    fPreviewing = false;

    // Update GUI
    if (fPlayButton) fPlayButton->SetEnabled(fFileLoaded);
    if (fStopButton) fStopButton->SetEnabled(false);

    // Stop update timer
    delete fUpdateRunner;
    fUpdateRunner = nullptr;

    printf("AudioPreviewPanel: Preview stopped\n");
}

status_t AudioPreviewPanel::LoadAudioFile(const entry_ref& ref)
{
    // Clean up any existing file
    UnloadAudioFile();

    BPath path(&ref);
    fFilePath.SetTo(path.Path());

    printf("AudioPreviewPanel: Loading audio file: %s\n", fFilePath.String());

    // Create BMediaFile
    fMediaFile = new BMediaFile(&ref);
    status_t status = fMediaFile->InitCheck();
    if (status != B_OK) {
        printf("AudioPreviewPanel: BMediaFile init failed: %s\n", strerror(status));
        delete fMediaFile;
        fMediaFile = nullptr;
        return status;
    }

    // Find the first audio track
    int32 trackCount = fMediaFile->CountTracks();
    for (int32 i = 0; i < trackCount; i++) {
        BMediaTrack* track = fMediaFile->TrackAt(i);
        if (!track) continue;

        media_format format;
        status = track->DecodedFormat(&format);
        if (status == B_OK && format.type == B_MEDIA_RAW_AUDIO) {
            fMediaTrack = track;
            fFileFormat = format;
            break;
        } else {
            fMediaFile->ReleaseTrack(track);
        }
    }

    if (!fMediaTrack) {
        printf("AudioPreviewPanel: No audio track found\n");
        delete fMediaFile;
        fMediaFile = nullptr;
        return B_ERROR;
    }

    // Set file info
    fFileSampleRate = fFileFormat.u.raw_audio.frame_rate;
    fFileDuration = fMediaTrack->CountFrames();
    fPlaybackPosition = 0;
    fFileLoaded = true;

    printf("AudioPreviewPanel: Audio file loaded successfully\n");
    printf("  Sample rate: %.0f Hz\n", fFileSampleRate);
    printf("  Duration: %ld frames (%.1f seconds)\n",
           (long)fFileDuration, fFileDuration / fFileSampleRate);

    return B_OK;
}

void AudioPreviewPanel::UnloadAudioFile()
{
    if (fMediaTrack && fMediaFile) {
        fMediaFile->ReleaseTrack(fMediaTrack);
        fMediaTrack = nullptr;
    }

    if (fMediaFile) {
        delete fMediaFile;
        fMediaFile = nullptr;
    }

    fFileLoaded = false;
    fPlaybackPosition = 0;
    fFilePath.SetTo("");

    printf("AudioPreviewPanel: Audio file unloaded\n");
}

status_t AudioPreviewPanel::InitializePlayer()
{
    if (fPlayerInitialized) {
        return B_OK;
    }

    printf("AudioPreviewPanel: Initializing BSoundPlayer for preview\n");

    // Set up player format (use file format if compatible, otherwise default)
    media_raw_audio_format playerFormat;
    playerFormat.frame_rate = fFileSampleRate;
    playerFormat.channel_count = 2;  // Always use stereo for preview
    playerFormat.format = media_raw_audio_format::B_AUDIO_FLOAT;
    playerFormat.byte_order = B_MEDIA_HOST_ENDIAN;
    playerFormat.buffer_size = 2048 * sizeof(float) * 2;

    // Create BSoundPlayer
    fSoundPlayer = new BSoundPlayer(&playerFormat, "VeniceDAW Preview", AudioPlayFunc, nullptr, this);

    status_t status = fSoundPlayer->InitCheck();
    if (status != B_OK) {
        printf("AudioPreviewPanel: BSoundPlayer init failed: %s\n", strerror(status));
        delete fSoundPlayer;
        fSoundPlayer = nullptr;
        return status;
    }

    // Allocate preview buffer
    fPreviewBufferFrames = 2048;
    fPreviewBufferSize = fPreviewBufferFrames * 2 * sizeof(float);  // Stereo
    fPreviewBuffer = new float[fPreviewBufferFrames * 2];

    fPlayerInitialized = true;
    printf("AudioPreviewPanel: BSoundPlayer initialized successfully\n");

    return B_OK;
}

void AudioPreviewPanel::CleanupPlayer()
{
    if (fSoundPlayer) {
        if (fPreviewing) {
            fSoundPlayer->Stop();
        }
        delete fSoundPlayer;
        fSoundPlayer = nullptr;
    }

    fPlayerInitialized = false;
    printf("AudioPreviewPanel: BSoundPlayer cleaned up\n");
}

void AudioPreviewPanel::UpdateDisplay()
{
    if (!fFileLoaded) {
        return;
    }

    // Update file info
    if (fFileInfoLabel) {
        BString info;
        info.SetToFormat("%s\n%.1f kHz, %.1f sec",
                        BPath(fFilePath.String()).Leaf(),
                        fFileSampleRate / 1000.0f,
                        fFileDuration / fFileSampleRate);
        fFileInfoLabel->SetText(info.String());
    }

    // Update time display and position
    UpdateTimeDisplay();

    // Update position slider
    if (fPositionSlider && fFileDuration > 0) {
        float position = (float)fPlaybackPosition / fFileDuration;
        fPositionSlider->SetValue((int)(position * 1000));
    }
}

void AudioPreviewPanel::UpdateTimeDisplay()
{
    if (!fFileLoaded || !fTimeDisplay) {
        return;
    }

    float currentTime = fPlaybackPosition / fFileSampleRate;
    float totalTime = fFileDuration / fFileSampleRate;

    int currentMin = (int)(currentTime / 60);
    int currentSec = (int)currentTime % 60;
    int totalMin = (int)(totalTime / 60);
    int totalSecInt = (int)totalTime % 60;

    BString timeText;
    timeText.SetToFormat("%02d:%02d / %02d:%02d",
                        currentMin, currentSec, totalMin, totalSecInt);
    fTimeDisplay->SetText(timeText.String());
}

// Static audio callback
void AudioPreviewPanel::AudioPlayFunc(void* cookie, void* buffer, size_t size, const media_raw_audio_format& format)
{
    AudioPreviewPanel* panel = static_cast<AudioPreviewPanel*>(cookie);
    if (panel) {
        panel->ProcessPreviewAudio(static_cast<float*>(buffer), size / (sizeof(float) * format.channel_count));
    }
}

void AudioPreviewPanel::ProcessPreviewAudio(float* buffer, size_t frameCount)
{
    if (!fFileLoaded || !fMediaTrack || !fPreviewing) {
        // Fill with silence
        memset(buffer, 0, frameCount * 2 * sizeof(float));
        return;
    }

    // Read audio data from file
    int64 framesToRead = std::min((int64)frameCount, fFileDuration - fPlaybackPosition);
    if (framesToRead <= 0) {
        // End of file - stop preview
        memset(buffer, 0, frameCount * 2 * sizeof(float));
        BMessage stopMsg(MSG_STOP_PREVIEW);
        BMessenger(this).SendMessage(&stopMsg);
        return;
    }

    // Read from media track
    media_header header;
    status_t status = fMediaTrack->ReadFrames(fPreviewBuffer, &framesToRead, &header);

    if (status == B_OK && framesToRead > 0) {
        // Convert to output format and apply volume
        for (int64 i = 0; i < framesToRead; i++) {
            float left = fPreviewBuffer[i * 2] * fPreviewVolume;
            float right = fPreviewBuffer[i * 2 + 1] * fPreviewVolume;

            buffer[i * 2] = left;
            buffer[i * 2 + 1] = right;
        }

        // Fill remaining frames with silence
        for (size_t i = framesToRead; i < frameCount; i++) {
            buffer[i * 2] = 0.0f;
            buffer[i * 2 + 1] = 0.0f;
        }

        fPlaybackPosition += framesToRead;
    } else {
        // Error or end of file
        memset(buffer, 0, frameCount * 2 * sizeof(float));
    }
}

// =====================================
// AudioFilePanel Implementation
// =====================================

AudioFilePanel::AudioFilePanel(file_panel_mode mode,
                               BMessenger* target,
                               const entry_ref* panel_directory,
                               uint32 node_flavors,
                               bool allow_multiple_selection,
                               BMessage* message,
                               BRefFilter* filter,
                               bool modal,
                               bool hide_when_done)
    : BFilePanel(mode, target, panel_directory, node_flavors,
                allow_multiple_selection, message, filter, modal, hide_when_done)
    , fPreviewPanel(nullptr)
    , fPreviewEnabled(true)
{
    printf("AudioFilePanel: Created with preview support\n");
}

AudioFilePanel::~AudioFilePanel()
{
    printf("AudioFilePanel: Destroyed\n");
}

void AudioFilePanel::SelectionChanged()
{
    BFilePanel::SelectionChanged();

    if (fPreviewEnabled) {
        HandleSelectionChanged();
    }
}

void AudioFilePanel::Show()
{
    BFilePanel::Show();

    if (fPreviewEnabled && !fPreviewPanel) {
        SetupPreviewPanel();
    }
}

void AudioFilePanel::SetupPreviewPanel()
{
    if (!Window()) {
        return;
    }

    printf("AudioFilePanel: Setting up preview panel\n");

    // Create preview panel
    BRect previewRect(0, 0, 250, 150);
    fPreviewPanel = new AudioPreviewPanel(previewRect);

    // Add to file panel window (this is a bit hacky but works)
    BView* backgroundView = Window()->ChildAt(0);
    if (backgroundView) {
        // Position preview panel on the right side
        BRect windowBounds = Window()->Bounds();
        previewRect.OffsetTo(windowBounds.right - 260, 30);
        fPreviewPanel->MoveTo(previewRect.LeftTop());

        backgroundView->AddChild(fPreviewPanel);

        // Resize window to accommodate preview
        Window()->ResizeBy(270, 0);

        printf("AudioFilePanel: Preview panel added to window\n");
    }
}

void AudioFilePanel::HandleSelectionChanged()
{
    if (!fPreviewPanel) {
        return;
    }

    // Get the current panel directory and selection
    entry_ref panelRef;
    GetPanelDirectory(&panelRef);

    // Try to get the currently selected file
    // In BFilePanel, we need to check the pose view for selection
    BWindow* window = Window();
    if (window && window->Lock()) {
        // Find the pose view (file list) in the panel
        BView* poseView = window->FindView("PoseView");
        if (poseView && poseView->LockLooper()) {
            // Get selection from pose view - this is a simplified approach
            // In production, would need more robust selection tracking

            // For now, clear preview and indicate selection changed
            printf("AudioFilePanel: Selection changed - tracking active\n");

            // Note: Full implementation would extract entry_ref from PoseView
            // and call fPreviewPanel->SetPreviewFile(selectedRef)
            // This requires accessing private BFilePanel internals

            fPreviewPanel->ClearPreview();
            poseView->UnlockLooper();
        }
        window->Unlock();
    }

    // Alternative approach: monitor directory changes and file clicks
    // This provides basic selection tracking without deep BFilePanel access
    printf("AudioFilePanel: Ready to preview selected file\n");
}

} // namespace VeniceDAW
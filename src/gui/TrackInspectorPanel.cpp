/*
 * TrackInspectorPanel.cpp - Implementation of track inspector
 */

#include "TrackInspectorPanel.h"
#include "../audio/SimpleHaikuEngine.h"
#include <interface/LayoutBuilder.h>
#include <stdio.h>
#include <stdlib.h>

namespace HaikuDAW {

TrackInspectorPanel::TrackInspectorPanel(BRect frame)
    : BView(frame, "track_inspector", B_FOLLOW_TOP_BOTTOM | B_FOLLOW_RIGHT, B_WILL_DRAW)
    , fSelectedTrack(nullptr)
    , fFileInfoBox(nullptr)
    , fAudioControlsBox(nullptr)
    , fPositionBox(nullptr)
    , fLevelsBox(nullptr)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

    // Use BGroupLayout for proper vertical stacking (no overlaps!)
    BGroupLayout* mainLayout = new BGroupLayout(B_VERTICAL);
    SetLayout(mainLayout);
    mainLayout->SetSpacing(5);
    mainLayout->SetInsets(5, 5, 5, 5);

    // Create sections with layout managers
    _CreateFileInfoSection(BRect());
    _CreateAudioControlsSection(BRect());
    _CreatePositionSection(BRect());
    _CreateLevelsSection(BRect());

    printf("TrackInspectorPanel: Created with proper layout\n");
}

TrackInspectorPanel::~TrackInspectorPanel()
{
    printf("TrackInspectorPanel: Destroyed\n");
}

void TrackInspectorPanel::AttachedToWindow()
{
    BView::AttachedToWindow();

    // Set message targets
    if (fVolumeSlider) fVolumeSlider->SetTarget(this);
    if (fPanSlider) fPanSlider->SetTarget(this);
    if (fMuteCheckbox) fMuteCheckbox->SetTarget(this);
    if (fSoloCheckbox) fSoloCheckbox->SetTarget(this);
    if (fResetPositionButton) fResetPositionButton->SetTarget(this);
}

void TrackInspectorPanel::SetTrack(SimpleTrack* track)
{
    fSelectedTrack = track;

    if (track) {
        _UpdateFileInfo();
        _UpdateAudioControls();
        _UpdatePosition();
        printf("TrackInspectorPanel: Displaying track '%s'\n", track->GetName());
    } else {
        // Clear display when no track selected
        if (fFilePathLabel) fFilePathLabel->SetText("(No track selected)");
        if (fDurationLabel) fDurationLabel->SetText("");
        if (fSampleRateLabel) fSampleRateLabel->SetText("");
        if (fChannelsLabel) fChannelsLabel->SetText("");
    }

    Invalidate();
}

void TrackInspectorPanel::UpdateLevels()
{
    if (!fSelectedTrack) return;

    // Update level meters
    float peak = fSelectedTrack->GetPeakLevel();
    float rms = fSelectedTrack->GetRMSLevel();

    // Update visual meters
    if (fPeakMeterView) {
        _DrawLevelMeter(fPeakMeterView, peak);
    }

    if (fRMSMeterView) {
        _DrawLevelMeter(fRMSMeterView, rms);
    }

    // Update numeric labels
    if (fPeakValueLabel) {
        float peakDB = peak > 0.0f ? 20.0f * log10f(peak) : -100.0f;
        BString peakStr;
        peakStr.SetToFormat("%.1f dB", peakDB);
        fPeakValueLabel->SetText(peakStr.String());
    }

    if (fRMSValueLabel) {
        float rmsDB = rms > 0.0f ? 20.0f * log10f(rms) : -100.0f;
        BString rmsStr;
        rmsStr.SetToFormat("%.1f dB", rmsDB);
        fRMSValueLabel->SetText(rmsStr.String());
    }
}

void TrackInspectorPanel::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case MSG_VOLUME_CHANGED:
            if (fVolumeSlider && fSelectedTrack) {
                float volume = fVolumeSlider->Value() / 100.0f;
                _ApplyVolumeChange(volume);
            }
            break;

        case MSG_PAN_CHANGED:
            if (fPanSlider && fSelectedTrack) {
                float pan = (fPanSlider->Value() - 50.0f) / 50.0f;  // -1 to +1
                _ApplyPanChange(pan);
            }
            break;

        case MSG_MUTE_CHANGED:
            if (fMuteCheckbox && fSelectedTrack) {
                bool muted = (fMuteCheckbox->Value() == B_CONTROL_ON);
                _ApplyMuteChange(muted);
            }
            break;

        case MSG_SOLO_CHANGED:
            if (fSoloCheckbox && fSelectedTrack) {
                bool solo = (fSoloCheckbox->Value() == B_CONTROL_ON);
                _ApplySoloChange(solo);
            }
            break;

        case MSG_RESET_POSITION:
            if (fSelectedTrack) {
                _ApplyPositionChange(0.0f, 0.0f, 0.0f);
                _UpdatePosition();
            }
            break;

        default:
            BView::MessageReceived(message);
            break;
    }
}

// Private methods

void TrackInspectorPanel::_CreateFileInfoSection(BRect frame)
{
    fFileInfoBox = new BBox("file_info_box");
    fFileInfoBox->SetLabel("File Information");

    // Use BGroupLayout for proper vertical stacking
    BGroupLayout* sectionLayout = new BGroupLayout(B_VERTICAL);
    fFileInfoBox->SetLayout(sectionLayout);
    sectionLayout->SetSpacing(3);
    sectionLayout->SetInsets(8, 18, 8, 8);

    // Create labels with layout manager (no MoveTo!)
    fFilePathLabel = new BStringView(BRect(0, 0, 150, 15),
                                     "file_path", "(No track selected)");
    fFilePathLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 15));
    sectionLayout->AddView(fFilePathLabel);

    fDurationLabel = new BStringView(BRect(0, 0, 150, 15),
                                     "duration", "");
    fDurationLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 15));
    sectionLayout->AddView(fDurationLabel);

    fSampleRateLabel = new BStringView(BRect(0, 0, 150, 15),
                                       "sample_rate", "");
    fSampleRateLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 15));
    sectionLayout->AddView(fSampleRateLabel);

    fChannelsLabel = new BStringView(BRect(0, 0, 150, 15),
                                     "channels", "");
    fChannelsLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 15));
    sectionLayout->AddView(fChannelsLabel);

    // Add to main layout
    GetLayout()->AddView(fFileInfoBox);
}

void TrackInspectorPanel::_CreateAudioControlsSection(BRect frame)
{
    fAudioControlsBox = new BBox("audio_controls_box");
    fAudioControlsBox->SetLabel("Audio Controls");

    // Use BGroupLayout for proper vertical stacking
    BGroupLayout* sectionLayout = new BGroupLayout(B_VERTICAL);
    fAudioControlsBox->SetLayout(sectionLayout);
    sectionLayout->SetSpacing(3);
    sectionLayout->SetInsets(8, 18, 8, 8);

    // Volume slider - compact
    fVolumeSlider = new BSlider(BRect(0, 0, 150, 30),
                                "volume_slider", "Volume:",
                                new BMessage(MSG_VOLUME_CHANGED),
                                0, 200, B_HORIZONTAL);  // 0-200%
    fVolumeSlider->SetHashMarks(B_HASH_MARKS_NONE);  // Clean look
    fVolumeSlider->SetLimitLabels("0%", "200%");
    fVolumeSlider->SetValue(100);
    fVolumeSlider->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 30));
    sectionLayout->AddView(fVolumeSlider);

    // Pan slider - compact
    fPanSlider = new BSlider(BRect(0, 0, 150, 30),
                             "pan_slider", "Pan:",
                             new BMessage(MSG_PAN_CHANGED),
                             0, 100, B_HORIZONTAL);  // 0=Left, 50=Center, 100=Right
    fPanSlider->SetHashMarks(B_HASH_MARKS_NONE);
    fPanSlider->SetLimitLabels("L", "R");
    fPanSlider->SetValue(50);
    fPanSlider->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 30));
    sectionLayout->AddView(fPanSlider);

    // Mute/Solo checkboxes in horizontal group
    BGroupView* checkboxGroup = new BGroupView(B_HORIZONTAL);
    checkboxGroup->GroupLayout()->SetSpacing(5);

    fMuteCheckbox = new BCheckBox(BRect(0, 0, 70, 20),
                                  "mute_check", "Mute",
                                  new BMessage(MSG_MUTE_CHANGED));
    checkboxGroup->AddChild(fMuteCheckbox);

    fSoloCheckbox = new BCheckBox(BRect(0, 0, 70, 20),
                                  "solo_check", "Solo",
                                  new BMessage(MSG_SOLO_CHANGED));
    checkboxGroup->AddChild(fSoloCheckbox);

    sectionLayout->AddView(checkboxGroup);

    // Add to main layout
    GetLayout()->AddView(fAudioControlsBox);
}

void TrackInspectorPanel::_CreatePositionSection(BRect frame)
{
    fPositionBox = new BBox("position_box");
    fPositionBox->SetLabel("3D Position");

    // Use BGroupLayout for proper vertical stacking
    BGroupLayout* sectionLayout = new BGroupLayout(B_VERTICAL);
    fPositionBox->SetLayout(sectionLayout);
    sectionLayout->SetSpacing(3);
    sectionLayout->SetInsets(8, 18, 8, 8);

    // Position controls - compact
    fPositionXControl = new BTextControl(BRect(0, 0, 150, 25),
                                         "pos_x", "X:", "0.0", nullptr);
    fPositionXControl->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 25));
    sectionLayout->AddView(fPositionXControl);

    fPositionYControl = new BTextControl(BRect(0, 0, 150, 25),
                                         "pos_y", "Y:", "0.0", nullptr);
    fPositionYControl->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 25));
    sectionLayout->AddView(fPositionYControl);

    fPositionZControl = new BTextControl(BRect(0, 0, 150, 25),
                                         "pos_z", "Z:", "0.0", nullptr);
    fPositionZControl->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 25));
    sectionLayout->AddView(fPositionZControl);

    // Reset button and distance in horizontal group
    BGroupView* resetGroup = new BGroupView(B_HORIZONTAL);
    resetGroup->GroupLayout()->SetSpacing(5);

    fResetPositionButton = new BButton(BRect(0, 0, 80, 25),
                                       "reset_pos", "Reset",
                                       new BMessage(MSG_RESET_POSITION));
    fResetPositionButton->SetExplicitMaxSize(BSize(80, 25));
    resetGroup->AddChild(fResetPositionButton);

    fDistanceLabel = new BStringView(BRect(0, 0, 80, 25),
                                     "distance", "Distance: 0.0");
    fDistanceLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 25));
    resetGroup->AddChild(fDistanceLabel);

    sectionLayout->AddView(resetGroup);

    // Add to main layout
    GetLayout()->AddView(fPositionBox);
}

void TrackInspectorPanel::_CreateLevelsSection(BRect frame)
{
    fLevelsBox = new BBox("levels_box");
    fLevelsBox->SetLabel("Real-time Levels");

    // Use BGroupLayout for proper vertical stacking
    BGroupLayout* sectionLayout = new BGroupLayout(B_VERTICAL);
    fLevelsBox->SetLayout(sectionLayout);
    sectionLayout->SetSpacing(5);
    sectionLayout->SetInsets(8, 18, 8, 8);

    // Peak meter row (horizontal: meter + value)
    BGroupView* peakRow = new BGroupView(B_HORIZONTAL);
    peakRow->GroupLayout()->SetSpacing(5);

    fPeakMeterView = new BView(BRect(0, 0, 100, 15), "peak_meter", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW);
    fPeakMeterView->SetViewColor(50, 50, 50);
    fPeakMeterView->SetExplicitMinSize(BSize(80, 15));
    fPeakMeterView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 15));
    peakRow->AddChild(fPeakMeterView);

    fPeakValueLabel = new BStringView(BRect(0, 0, 55, 15),
                                      "peak_value", "-inf dB");
    fPeakValueLabel->SetExplicitMinSize(BSize(55, 15));
    fPeakValueLabel->SetExplicitMaxSize(BSize(55, 15));
    peakRow->AddChild(fPeakValueLabel);

    sectionLayout->AddView(peakRow);

    // RMS meter row (horizontal: meter + value)
    BGroupView* rmsRow = new BGroupView(B_HORIZONTAL);
    rmsRow->GroupLayout()->SetSpacing(5);

    fRMSMeterView = new BView(BRect(0, 0, 100, 15), "rms_meter", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW);
    fRMSMeterView->SetViewColor(50, 50, 50);
    fRMSMeterView->SetExplicitMinSize(BSize(80, 15));
    fRMSMeterView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 15));
    rmsRow->AddChild(fRMSMeterView);

    fRMSValueLabel = new BStringView(BRect(0, 0, 55, 15),
                                     "rms_value", "-inf dB");
    fRMSValueLabel->SetExplicitMinSize(BSize(55, 15));
    fRMSValueLabel->SetExplicitMaxSize(BSize(55, 15));
    rmsRow->AddChild(fRMSValueLabel);

    sectionLayout->AddView(rmsRow);

    // Labels (Peak/RMS labels)
    BStringView* peakLabel = new BStringView(BRect(0, 0, 150, 15),
                                             "peak_label", "Peak:");
    peakLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 15));
    sectionLayout->AddView(peakLabel);

    BStringView* rmsLabel = new BStringView(BRect(0, 0, 150, 15),
                                            "rms_label", "RMS:");
    rmsLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 15));
    sectionLayout->AddView(rmsLabel);

    // Add to main layout
    GetLayout()->AddView(fLevelsBox);
}

void TrackInspectorPanel::_UpdateFileInfo()
{
    if (!fSelectedTrack) return;

    // File path
    if (fFilePathLabel) {
        const char* path = fSelectedTrack->GetFilePath();
        fFilePathLabel->SetText(path ? path : "(No file loaded)");
    }

    // Duration
    if (fDurationLabel) {
        int64 duration = fSelectedTrack->GetFileDuration();
        float sampleRate = fSelectedTrack->GetFileSampleRate();
        if (duration > 0 && sampleRate > 0) {
            float seconds = (float)duration / sampleRate;
            int mins = (int)(seconds / 60);
            float secs = seconds - (mins * 60);
            BString durStr;
            durStr.SetToFormat("Duration: %02d:%06.3f", mins, secs);
            fDurationLabel->SetText(durStr.String());
        } else {
            fDurationLabel->SetText("Duration: --:--");
        }
    }

    // Sample rate
    if (fSampleRateLabel) {
        float sampleRate = fSelectedTrack->GetFileSampleRate();
        BString srStr;
        srStr.SetToFormat("Sample Rate: %.0f Hz", sampleRate);
        fSampleRateLabel->SetText(srStr.String());
    }

    // Channels
    if (fChannelsLabel) {
        fChannelsLabel->SetText("Channels: 2 (Stereo)");
    }
}

void TrackInspectorPanel::_UpdateAudioControls()
{
    if (!fSelectedTrack) return;

    // Volume
    if (fVolumeSlider) {
        int32 volumePercent = (int32)(fSelectedTrack->GetVolume() * 100.0f);
        fVolumeSlider->SetValue(volumePercent);
    }

    // Pan
    if (fPanSlider) {
        float pan = fSelectedTrack->GetPan();  // -1 to +1
        int32 panValue = (int32)((pan + 1.0f) * 50.0f);  // 0 to 100
        fPanSlider->SetValue(panValue);
    }

    // Mute
    if (fMuteCheckbox) {
        fMuteCheckbox->SetValue(fSelectedTrack->IsMuted() ? B_CONTROL_ON : B_CONTROL_OFF);
    }

    // Solo
    if (fSoloCheckbox) {
        fSoloCheckbox->SetValue(fSelectedTrack->IsSolo() ? B_CONTROL_ON : B_CONTROL_OFF);
    }
}

void TrackInspectorPanel::_UpdatePosition()
{
    if (!fSelectedTrack) return;

    float x, y, z;
    fSelectedTrack->GetPosition(x, y, z);

    // Update text controls
    if (fPositionXControl) {
        BString xStr;
        xStr.SetToFormat("%.2f", x);
        fPositionXControl->SetText(xStr.String());
    }

    if (fPositionYControl) {
        BString yStr;
        yStr.SetToFormat("%.2f", y);
        fPositionYControl->SetText(yStr.String());
    }

    if (fPositionZControl) {
        BString zStr;
        zStr.SetToFormat("%.2f", z);
        fPositionZControl->SetText(zStr.String());
    }

    // Update distance
    if (fDistanceLabel) {
        float distance = sqrtf(x*x + y*y + z*z);
        BString distStr;
        distStr.SetToFormat("Distance: %.2f", distance);
        fDistanceLabel->SetText(distStr.String());
    }
}

void TrackInspectorPanel::_ApplyVolumeChange(float volume)
{
    if (fSelectedTrack) {
        fSelectedTrack->SetVolume(volume);
        printf("TrackInspectorPanel: Volume changed to %.2f\n", volume);
    }
}

void TrackInspectorPanel::_ApplyPanChange(float pan)
{
    if (fSelectedTrack) {
        fSelectedTrack->SetPan(pan);
        printf("TrackInspectorPanel: Pan changed to %.2f\n", pan);
    }
}

void TrackInspectorPanel::_ApplyMuteChange(bool muted)
{
    if (fSelectedTrack) {
        fSelectedTrack->SetMute(muted);
        printf("TrackInspectorPanel: Mute %s\n", muted ? "ON" : "OFF");
    }
}

void TrackInspectorPanel::_ApplySoloChange(bool solo)
{
    if (fSelectedTrack) {
        fSelectedTrack->SetSolo(solo);
        printf("TrackInspectorPanel: Solo %s\n", solo ? "ON" : "OFF");
    }
}

void TrackInspectorPanel::_ApplyPositionChange(float x, float y, float z)
{
    if (fSelectedTrack) {
        fSelectedTrack->SetPosition(x, y, z);
        printf("TrackInspectorPanel: Position changed to (%.2f, %.2f, %.2f)\n", x, y, z);
    }
}

void TrackInspectorPanel::_DrawLevelMeter(BView* view, float level)
{
    if (!view || !view->LockLooper()) return;

    BRect bounds = view->Bounds();

    // Clear background
    view->SetHighColor(50, 50, 50);
    view->FillRect(bounds);

    // Draw level bar
    if (level > 0.0f) {
        float width = bounds.Width() * level;

        // Color based on level
        if (level > 0.8f) {
            view->SetHighColor(255, 0, 0);  // Red (clipping)
        } else if (level > 0.5f) {
            view->SetHighColor(255, 255, 0);  // Yellow
        } else {
            view->SetHighColor(0, 255, 0);  // Green
        }

        view->FillRect(BRect(0, 0, width, bounds.Height()));
    }

    view->Sync();
    view->UnlockLooper();
}

} // namespace HaikuDAW

/*
 * MainWindow.cpp - Main application window implementation
 */

#include "MainWindow.h"
#include "../audio/AudioEngine.h"
#include <app/Application.h>
#include <interface/LayoutBuilder.h>
#include <interface/GroupLayout.h>
#include <interface/GroupView.h>
#include <interface/StringView.h>
#include <interface/Rect.h>
#include <interface/InterfaceDefs.h>
#include <stdio.h>

namespace HaikuDAW {

MainWindow::MainWindow()
    : BWindow(BRect(100, 100, 700, 500), "HaikuDAW - Audio Engine Demo", 
              B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE)
    , fStartButton(nullptr)
    , fStopButton(nullptr)
    , fAddTrackButton(nullptr)
    , fVolumeSlider(nullptr)
    , fStatusView(nullptr)
    , fMainView(nullptr)
    , fNextTrackId(1)
{
    printf("MainWindow: Creating demo interface\n");
    
    // Create audio engine
    fAudioEngine = std::make_unique<AudioEngine>("HaikuDAW Engine");
    
    BuildInterface();
    UpdateStatus();
    
    printf("MainWindow: Demo ready!\n");
}

MainWindow::~MainWindow()
{
    printf("MainWindow: Shutting down\n");
    
    if (fAudioEngine) {
        fAudioEngine->Stop();
    }
}

void MainWindow::BuildInterface()
{
    // Create main view
    fMainView = new BView("MainView", B_WILL_DRAW);
    fMainView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    
    // Create controls
    fStartButton = new BButton("StartButton", "Start Engine", 
                              new BMessage(MSG_START_ENGINE));
    
    fStopButton = new BButton("StopButton", "Stop Engine", 
                             new BMessage(MSG_STOP_ENGINE));
    fStopButton->SetEnabled(false);
    
    fAddTrackButton = new BButton("AddTrackButton", "Add Track", 
                                 new BMessage(MSG_ADD_TRACK));
    fAddTrackButton->SetEnabled(false);
    
    // Volume control
    fVolumeSlider = new BSlider("VolumeSlider", "Master Volume", 
                               new BMessage(MSG_VOLUME_CHANGED),
                               0, 100, B_HORIZONTAL);
    fVolumeSlider->SetValue(75); // Default 75%
    fVolumeSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
    fVolumeSlider->SetHashMarkCount(11);
    fVolumeSlider->SetEnabled(false);
    
    // Status view
    fStatusView = new BStringView("StatusView", "Audio Engine: Stopped");
    
    // Audio visualization area
    AudioView* audioView = new AudioView(BRect(0, 0, 300, 150));
    
    // Layout using BLayoutBuilder
    BLayoutBuilder::Group<>(fMainView, B_VERTICAL, 10)
        .SetInsets(10)
        .Add(new BStringView("TitleView", "HaikuDAW - Audio Engine Demo"))
        .AddGlue()
        .Add(audioView)
        .AddGlue()
        .AddGroup(B_HORIZONTAL, 5)
            .Add(fStartButton)
            .Add(fStopButton)
            .Add(fAddTrackButton)
            .AddGlue()
        .End()
        .Add(fVolumeSlider)
        .AddGlue()
        .Add(fStatusView)
    .End();
    
    AddChild(fMainView);
    
    printf("MainWindow: Interface built\n");
}

void MainWindow::UpdateStatus()
{
    if (!fStatusView || !fAudioEngine) {
        return;
    }
    
    BString status;
    
    if (fAudioEngine->IsRunning()) {
        status.SetToFormat("Engine: RUNNING | Tracks: %d | CPU: %.1f%% | Sample Rate: %.0f Hz",
                          fAudioEngine->GetTrackCount(),
                          fAudioEngine->GetCPUUsage(),
                          fAudioEngine->GetSampleRate());
        
        fStartButton->SetEnabled(false);
        fStopButton->SetEnabled(true);
        fAddTrackButton->SetEnabled(true);
        fVolumeSlider->SetEnabled(true);
    } else {
        status.SetTo("Audio Engine: STOPPED - Click 'Start Engine' to begin");
        
        fStartButton->SetEnabled(true);
        fStopButton->SetEnabled(false);
        fAddTrackButton->SetEnabled(false);
        fVolumeSlider->SetEnabled(false);
    }
    
    fStatusView->SetText(status);
    
    // Schedule next update
    BMessage updateMsg(MSG_UPDATE_STATUS);
    MessageReceived(&updateMsg);
}

bool MainWindow::QuitRequested()
{
    printf("MainWindow: Quit requested\n");
    be_app->PostMessage(B_QUIT_REQUESTED);
    return true;
}

void MainWindow::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case MSG_START_ENGINE:
        {
            printf("MainWindow: Starting audio engine\n");
            status_t result = fAudioEngine->Start();
            if (result == B_OK) {
                printf("MainWindow: Audio engine started successfully\n");
            } else {
                printf("MainWindow: Failed to start audio engine: %s\n", strerror(result));
            }
            UpdateStatus();
            break;
        }
        
        case MSG_STOP_ENGINE:
        {
            printf("MainWindow: Stopping audio engine\n");
            fAudioEngine->Stop();
            UpdateStatus();
            break;
        }
        
        case MSG_ADD_TRACK:
        {
            BString trackName;
            trackName.SetToFormat("Track %d", fNextTrackId);
            
            auto track = std::make_shared<AudioTrack>(fNextTrackId, trackName.String());
            
            status_t result = fAudioEngine->AddTrack(track);
            if (result == B_OK) {
                printf("MainWindow: Added %s\n", trackName.String());
                fNextTrackId++;
            } else {
                printf("MainWindow: Failed to add track: %s\n", strerror(result));
            }
            
            UpdateStatus();
            break;
        }
        
        case MSG_VOLUME_CHANGED:
        {
            if (fVolumeSlider && fAudioEngine) {
                float volume = fVolumeSlider->Value() / 100.0f;
                fAudioEngine->SetMasterVolume(volume);
                printf("MainWindow: Volume changed to %.0f%%\n", volume * 100.0f);
            }
            break;
        }
        
        case MSG_UPDATE_STATUS:
        {
            UpdateStatus();
            
            // Schedule next update in 500ms
            BMessage* nextUpdate = new BMessage(MSG_UPDATE_STATUS);
            MessageReceived(nextUpdate);
            delete nextUpdate;
            break;
        }
        
        default:
            BWindow::MessageReceived(message);
            break;
    }
}

// ========================================
// AudioView Implementation  
// ========================================

AudioView::AudioView(BRect frame)
    : BView(frame, "AudioView", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
    , fLeftLevel(0.0f)
    , fRightLevel(0.0f)
{
    SetViewColor(0, 0, 0); // Black background
    SetPulseRate(50000);   // 20 FPS updates
}

AudioView::~AudioView()
{
}

void AudioView::Draw(BRect updateRect)
{
    // Clear background
    SetHighColor(0, 0, 0);
    FillRect(updateRect);
    
    BRect bounds = Bounds();
    
    // Draw title
    SetHighColor(255, 255, 255);
    SetFontSize(12);
    DrawString("Audio Visualization", BPoint(10, 20));
    
    // Draw level meters
    BRect leftMeter(20, 40, 40, bounds.bottom - 20);
    BRect rightMeter(60, 40, 80, bounds.bottom - 20);
    
    // Meter backgrounds
    SetHighColor(50, 50, 50);
    FillRect(leftMeter);
    FillRect(rightMeter);
    
    // Meter levels
    SetHighColor(0, 255, 0); // Green
    
    float leftHeight = leftMeter.Height() * fLeftLevel;
    float rightHeight = rightMeter.Height() * fRightLevel;
    
    BRect leftLevel(leftMeter.left, leftMeter.bottom - leftHeight,
                   leftMeter.right, leftMeter.bottom);
    BRect rightLevel(rightMeter.left, rightMeter.bottom - rightHeight,
                    rightMeter.right, rightMeter.bottom);
    
    FillRect(leftLevel);
    FillRect(rightLevel);
    
    // Labels
    SetHighColor(200, 200, 200);
    SetFontSize(10);
    DrawString("L", BPoint(25, leftMeter.bottom + 15));
    DrawString("R", BPoint(65, rightMeter.bottom + 15));
    
    // Demo waveform (sine wave)
    SetHighColor(100, 150, 255);
    BPoint lastPoint(100, bounds.Height() / 2);
    
    for (int x = 100; x < bounds.right - 10; x += 2) {
        float t = (x - 100) / 50.0f;
        float y = bounds.Height() / 2 + sin(t) * 30 * (fLeftLevel + fRightLevel) / 2;
        
        BPoint currentPoint(x, y);
        StrokeLine(lastPoint, currentPoint);
        lastPoint = currentPoint;
    }
}

void AudioView::Pulse()
{
    // Simulate audio levels for demo
    static float time = 0;
    time += 0.1f;
    
    fLeftLevel = (sin(time) + 1.0f) / 4.0f;   // 0-0.5 range
    fRightLevel = (cos(time * 1.1f) + 1.0f) / 4.0f;
    
    Invalidate();
}

void AudioView::SetLevel(float left, float right)
{
    fLeftLevel = left;
    fRightLevel = right;
    Invalidate();
}

} // namespace HaikuDAW
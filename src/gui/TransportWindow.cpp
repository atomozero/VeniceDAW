/*
 * TransportWindow.cpp - Professional transport controls implementation
 */

#include "TransportWindow.h"
#include <Application.h>
#include <LayoutBuilder.h>
#include <GroupLayout.h>
#include <StringFormat.h>
#include <Screen.h>
#include <stdio.h>
#include <cmath>

namespace VeniceDAW {

// TransportView implementation
TransportView::TransportView(BRect frame)
    : BView(frame, "TransportView", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED),
      fEngine(nullptr),
      fUpdateRunner(nullptr),
      fIsPlaying(false),
      fPlayheadPosition(0.0f)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

TransportView::~TransportView()
{
    delete fUpdateRunner;
}

void TransportView::AttachedToWindow()
{
    BView::AttachedToWindow();
    
    // Create transport controls
    
    // Play button
    BRect playRect(10, 10, 60, 40);
    fPlayButton = new BButton(playRect, "play", "Play", new BMessage(MSG_PLAY));
    AddChild(fPlayButton);
    fPlayButton->SetTarget(this);
    
    // Stop button  
    BRect stopRect(70, 10, 120, 40);
    fStopButton = new BButton(stopRect, "stop", "Stop", new BMessage(MSG_STOP));
    AddChild(fStopButton);
    fStopButton->SetTarget(this);
    
    // Reset button
    BRect resetRect(130, 10, 180, 40);
    fResetButton = new BButton(resetRect, "reset", "Reset", new BMessage(MSG_RESET));
    AddChild(fResetButton);
    fResetButton->SetTarget(this);
    
    // Volume slider
    BRect volumeRect(200, 15, 350, 35);
    fVolumeSlider = new BSlider(volumeRect, "volume", "Volume:", 
                               new BMessage(MSG_VOLUME), 0, 100);
    fVolumeSlider->SetValue(70); // 70% default
    AddChild(fVolumeSlider);
    fVolumeSlider->SetTarget(this);
    
    // Time display
    BRect timeRect(10, 50, 200, 70);
    fTimeDisplay = new BStringView(timeRect, "time", "00:00.0 / 00:00.0");
    AddChild(fTimeDisplay);
    
    // Track info
    BRect infoRect(10, 75, 400, 95);
    fTrackInfo = new BStringView(infoRect, "info", "No tracks loaded");
    AddChild(fTrackInfo);
    
    // Start update timer (60 FPS for smooth playhead)
    BMessage updateMsg(MSG_UPDATE);
    fUpdateRunner = new BMessageRunner(this, &updateMsg, 16667); // ~60 FPS
    
    UpdateDisplay();
}

void TransportView::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case MSG_PLAY:
            if (fEngine) {
                if (fIsPlaying) {
                    // Pause (stop for now)
                    fEngine->Stop();
                    fIsPlaying = false;
                    fPlayButton->SetLabel("Play");
                } else {
                    // Play
                    status_t status = fEngine->Start();
                    if (status == B_OK) {
                        fIsPlaying = true;
                        fPlayButton->SetLabel("Pause");
                    }
                }
            }
            break;
            
        case MSG_STOP:
            if (fEngine && fIsPlaying) {
                fEngine->Stop();
                fIsPlaying = false;
                fPlayButton->SetLabel("Play");
                fPlayheadPosition = 0.0f;
            }
            break;
            
        case MSG_RESET:
            if (fEngine) {
                fEngine->ResetAllTracks();
                fPlayheadPosition = 0.0f;
                Invalidate();
            }
            break;
            
        case MSG_VOLUME:
            if (fEngine) {
                float volume = fVolumeSlider->Value() / 100.0f;
                fEngine->SetMasterVolume(volume);
            }
            break;
            
        case MSG_UPDATE:
            UpdateDisplay();
            break;
            
        default:
            BView::MessageReceived(message);
            break;
    }
}

void TransportView::Draw(BRect updateRect)
{
    BView::Draw(updateRect);
    
    // Draw waveform area
    BRect waveRect = Bounds();
    waveRect.top = 100;
    waveRect.bottom = waveRect.bottom - 20;
    waveRect.left += 10;
    waveRect.right -= 10;
    
    if (waveRect.IsValid() && waveRect.Intersects(updateRect)) {
        DrawWaveform(waveRect);
        DrawPlayhead(waveRect);
    }
    
    // Draw level meters
    BRect meterRect = Bounds();
    meterRect.top = meterRect.bottom - 15;
    meterRect.left += 10;
    meterRect.right -= 10;
    
    if (fEngine && meterRect.IsValid()) {
        // Draw master level meters
        SetHighColor(0, 150, 0); // Green
        
        float leftLevel = fEngine->GetMasterPeakLeft();
        float rightLevel = fEngine->GetMasterPeakRight();
        
        // Clamp levels
        leftLevel = std::min(1.0f, std::max(0.0f, leftLevel));
        rightLevel = std::min(1.0f, std::max(0.0f, rightLevel));
        
        // Draw left meter
        BRect leftMeter = meterRect;
        leftMeter.bottom = leftMeter.top + 6;
        leftMeter.right = leftMeter.left + leftLevel * (meterRect.Width() * 0.45f);
        FillRect(leftMeter);
        
        // Draw right meter  
        BRect rightMeter = meterRect;
        rightMeter.top = rightMeter.bottom - 6;
        rightMeter.right = rightMeter.left + rightLevel * (meterRect.Width() * 0.45f);
        FillRect(rightMeter);
        
        // Draw meter borders
        SetHighColor(100, 100, 100);
        BRect leftBorder = meterRect;
        leftBorder.bottom = leftBorder.top + 6;
        leftBorder.right = leftBorder.left + meterRect.Width() * 0.45f;
        StrokeRect(leftBorder);
        
        BRect rightBorder = meterRect;
        rightBorder.top = rightBorder.bottom - 6; 
        rightBorder.right = rightBorder.left + meterRect.Width() * 0.45f;
        StrokeRect(rightBorder);
    }
}

void TransportView::DrawWaveform(BRect rect)
{
    // Simple waveform visualization
    SetHighColor(50, 50, 50);
    StrokeRect(rect);
    
    if (!fEngine || fEngine->GetTrackCount() == 0) {
        SetHighColor(100, 100, 100);
        DrawString("No audio loaded", BPoint(rect.left + 10, rect.top + 20));
        return;
    }
    
    // Draw simple waveform representation for each track
    float trackHeight = rect.Height() / std::max(1, fEngine->GetTrackCount());
    
    for (int i = 0; i < fEngine->GetTrackCount(); i++) {
        HaikuDAW::SimpleTrack* track = fEngine->GetTrack(i);
        if (!track || !track->HasFile()) continue;
        
        BRect trackRect = rect;
        trackRect.top = rect.top + i * trackHeight;
        trackRect.bottom = trackRect.top + trackHeight - 2;
        
        // Track background
        SetHighColor(20, 20, 30);
        FillRect(trackRect);
        
        // Simple waveform simulation
        SetHighColor(0, 150, 255);
        float duration = (float)track->GetFileDuration() / track->GetFileSampleRate();
        
        for (int x = 0; x < (int)trackRect.Width(); x += 2) {
            float time = (x / trackRect.Width()) * duration;
            float amplitude = sin(time * 10.0f) * 0.3f + sin(time * 30.0f) * 0.1f;
            amplitude *= track->GetVolume();
            
            float centerY = trackRect.top + trackRect.Height() / 2;
            float waveY = centerY + amplitude * trackRect.Height() * 0.4f;
            
            StrokeLine(BPoint(trackRect.left + x, centerY),
                      BPoint(trackRect.left + x, waveY));
        }
        
        // Track name
        SetHighColor(255, 255, 255);
        DrawString(track->GetName(), BPoint(trackRect.left + 5, trackRect.top + 15));
    }
}

void TransportView::DrawPlayhead(BRect rect)
{
    if (fPlayheadPosition <= 0.0f) return;
    
    SetHighColor(255, 100, 100); // Red playhead
    float x = rect.left + fPlayheadPosition * rect.Width();
    StrokeLine(BPoint(x, rect.top), BPoint(x, rect.bottom));
    
    // Draw playhead triangle
    BPoint triangle[3];
    triangle[0] = BPoint(x - 5, rect.top - 5);
    triangle[1] = BPoint(x + 5, rect.top - 5);  
    triangle[2] = BPoint(x, rect.top);
    FillPolygon(triangle, 3);
}

void TransportView::SetEngine(HaikuDAW::SimpleHaikuEngine* engine)
{
    fEngine = engine;
    UpdateDisplay();
}

void TransportView::UpdateDisplay()
{
    if (!fEngine) return;
    
    UpdateTimeDisplay();
    
    // Update track info
    int trackCount = fEngine->GetTrackCount();
    BString info;
    info.SetToFormat("%d tracks loaded", trackCount);
    
    if (trackCount > 0) {
        float totalDuration = 0;
        for (int i = 0; i < trackCount; i++) {
            HaikuDAW::SimpleTrack* track = fEngine->GetTrack(i);
            if (track && track->HasFile()) {
                float duration = (float)track->GetFileDuration() / track->GetFileSampleRate();
                totalDuration = std::max(totalDuration, duration);
            }
        }
        
        int minutes = (int)(totalDuration / 60);
        float seconds = totalDuration - minutes * 60;
        info << BString().SetToFormat(", longest: %02d:%04.1f", minutes, seconds);
    }
    
    fTrackInfo->SetText(info.String());
    
    // Trigger redraw for waveform and meters
    Invalidate();
}

void TransportView::UpdateTimeDisplay()
{
    // Simple time display - would need actual playback position from engine
    BString timeStr("00:00.0 / 00:00.0");
    fTimeDisplay->SetText(timeStr.String());
}

// TransportWindow implementation
TransportWindow::TransportWindow(HaikuDAW::SimpleHaikuEngine* engine)
    : BWindow(BRect(100, 100, 700, 400), "VeniceDAW Transport", 
              B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS),
      fEngine(engine)
{
    // Create transport view
    fTransportView = new TransportView(Bounds());
    fTransportView->SetEngine(engine);
    AddChild(fTransportView);
    
    // Center window on screen
    BScreen screen;
    BRect screenFrame = screen.Frame();
    BRect windowFrame = Frame();
    MoveTo((screenFrame.Width() - windowFrame.Width()) / 2,
           (screenFrame.Height() - windowFrame.Height()) / 2);
}

TransportWindow::~TransportWindow()
{
}

bool TransportWindow::QuitRequested()
{
    be_app->PostMessage(B_QUIT_REQUESTED);
    return true;
}

void TransportWindow::MessageReceived(BMessage* message)
{
    BWindow::MessageReceived(message);
}

} // namespace VeniceDAW
/*
 * MainWindow.h - Main application window
 * 
 * Simple UI to test and demo the audio engine
 */

#ifndef HAIKU_DAW_MAIN_WINDOW_H
#define HAIKU_DAW_MAIN_WINDOW_H

#include <interface/Window.h>
#include <interface/View.h>
#include <interface/Button.h>
#include <interface/Slider.h>
#include <interface/StringView.h>
#include <memory>

namespace HaikuDAW {

// Forward declarations
class AudioEngine;

class MainWindow : public BWindow {
public:
    MainWindow();
    virtual ~MainWindow();
    
    // BWindow overrides
    virtual bool QuitRequested();
    virtual void MessageReceived(BMessage* message);
    
private:
    void BuildInterface();
    void UpdateStatus();
    
    // Message constants
    enum {
        MSG_START_ENGINE = 'strt',
        MSG_STOP_ENGINE = 'stop',
        MSG_ADD_TRACK = 'addt',
        MSG_VOLUME_CHANGED = 'volc',
        MSG_UPDATE_STATUS = 'upst'
    };
    
    // UI elements
    BButton*        fStartButton;
    BButton*        fStopButton;
    BButton*        fAddTrackButton;
    BSlider*        fVolumeSlider;
    BStringView*    fStatusView;
    BView*          fMainView;
    
    // Audio engine
    std::unique_ptr<AudioEngine> fAudioEngine;
    
    // State
    int32           fNextTrackId;
};

/*
 * Simple view to show audio visualization
 */
class AudioView : public BView {
public:
    AudioView(BRect frame);
    virtual ~AudioView();
    
    virtual void Draw(BRect updateRect);
    virtual void Pulse();
    
    void SetLevel(float left, float right);
    
private:
    float fLeftLevel;
    float fRightLevel;
};

} // namespace HaikuDAW

#endif // HAIKU_DAW_MAIN_WINDOW_H
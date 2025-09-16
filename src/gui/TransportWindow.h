/*
 * TransportWindow.h - Professional transport controls for VeniceDAW
 * Play, pause, stop, seek controls with timeline display
 */

#ifndef TRANSPORT_WINDOW_H
#define TRANSPORT_WINDOW_H

#include <Window.h>
#include <View.h>
#include <Button.h>
#include <StringView.h>
#include <MessageRunner.h>
#include <Slider.h>

#include "../audio/SimpleHaikuEngine.h"

namespace VeniceDAW {

class TransportView : public BView {
public:
    TransportView(BRect frame);
    virtual ~TransportView();
    
    virtual void AttachedToWindow() override;
    virtual void MessageReceived(BMessage* message) override;
    virtual void Draw(BRect updateRect) override;
    
    void SetEngine(HaikuDAW::SimpleHaikuEngine* engine);
    void UpdateDisplay();
    
private:
    void DrawWaveform(BRect rect);
    void DrawPlayhead(BRect rect);
    void UpdateTimeDisplay();
    
    HaikuDAW::SimpleHaikuEngine* fEngine;
    BButton* fPlayButton;
    BButton* fStopButton;
    BButton* fResetButton;
    BStringView* fTimeDisplay;
    BStringView* fTrackInfo;
    BSlider* fVolumeSlider;
    BMessageRunner* fUpdateRunner;
    
    bool fIsPlaying;
    float fPlayheadPosition; // 0.0 to 1.0
    
    // Messages
    enum {
        MSG_PLAY = 'play',
        MSG_STOP = 'stop', 
        MSG_RESET = 'rest',
        MSG_VOLUME = 'volu',
        MSG_UPDATE = 'updt'
    };
};

class TransportWindow : public BWindow {
public:
    TransportWindow(HaikuDAW::SimpleHaikuEngine* engine);
    virtual ~TransportWindow();
    
    virtual bool QuitRequested() override;
    virtual void MessageReceived(BMessage* message) override;
    
private:
    TransportView* fTransportView;
    HaikuDAW::SimpleHaikuEngine* fEngine;
};

} // namespace VeniceDAW

#endif
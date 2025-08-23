/*
 * SuperMasterWindow.h - Global master control for multiple mixer windows
 */

#ifndef SUPER_MASTER_WINDOW_H
#define SUPER_MASTER_WINDOW_H

#include <Window.h>
#include <View.h>
#include <Box.h>
#include <Slider.h>
#include <Button.h>
#include <StringView.h>
#include <GroupLayout.h>
#include <MessageRunner.h>

namespace HaikuDAW {

// Forward declarations
class SimpleHaikuEngine;
class LevelMeter;

/*
 * Super Master Window - Global control for multiple mixer windows
 */
class SuperMasterWindow : public BWindow {
public:
    SuperMasterWindow(SimpleHaikuEngine* engine);
    virtual ~SuperMasterWindow();
    
    // BWindow interface
    virtual bool QuitRequested();
    virtual void MessageReceived(BMessage* message);
    
    // Control
    void UpdateMeter();
    void SetWindowCount(int count);
    void StartUpdateTimer();  // Start the update timer safely

private:
    void CreateControls();
    
    SimpleHaikuEngine* fEngine;
    
    // GUI components
    BView* fMainView;
    BBox* fControlSection;
    
    // Global controls
    BSlider* fGlobalVolume;
    BButton* fGlobalPlayButton;
    BButton* fGlobalStopButton;
    BStringView* fWindowCountDisplay;
    BStringView* fStatusDisplay;
    LevelMeter* fGlobalLevelLeft;
    LevelMeter* fGlobalLevelRight;
    
    // Update timer
    BMessageRunner* fUpdateRunner;
    
    // Message constants
    enum {
        MSG_GLOBAL_PLAY = 'gply',
        MSG_GLOBAL_STOP = 'gstp',
        MSG_GLOBAL_VOLUME = 'gvol',
        MSG_UPDATE_GLOBAL = 'uglo'
    };
};

} // namespace HaikuDAW

#endif
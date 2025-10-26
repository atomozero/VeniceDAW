/*
 * UnifiedWindow.h - Unified DAW window with tab-based view system
 * Combines Mixer, Timeline, and 3D views in one organized interface
 */

#ifndef UNIFIED_WINDOW_H
#define UNIFIED_WINDOW_H

#include <Window.h>
#include <TabView.h>
#include <View.h>
#include <MenuBar.h>
#include <MessageRunner.h>

namespace HaikuDAW {

// Forward declarations
class SimpleHaikuEngine;
class MixerWindow;
class TimelineWindow;
class Mixer3DWindow;

/*
 * UnifiedWindow - Main DAW window with tabbed interface
 *
 * Provides unified access to:
 * - Mixer view (traditional channel strips)
 * - Timeline view (arranger/editor)
 * - 3D Mixer view (spatial positioning)
 *
 * Benefits:
 * - Single window reduces clutter
 * - Easy switching between views
 * - Consistent keyboard shortcuts
 * - Better screen space utilization
 */
class UnifiedWindow : public BWindow {
public:
    UnifiedWindow(SimpleHaikuEngine* engine);
    virtual ~UnifiedWindow();

    virtual bool QuitRequested() override;
    virtual void MessageReceived(BMessage* message) override;

    // Tab management
    void SwitchToMixer();
    void SwitchToTimeline();
    void Switch3DMixer();

    int32 GetCurrentTab() const;
    void SetCurrentTab(int32 index);

    // Message constants
    enum {
        MSG_TAB_CHANGED = 'tch',
        MSG_SWITCH_MIXER = 'swmx',
        MSG_SWITCH_TIMELINE = 'swtl',
        MSG_SWITCH_3D = 'sw3d'
    };

    // Tab indices
    enum TabIndex {
        TAB_MIXER = 0,
        TAB_TIMELINE = 1,
        TAB_3D_MIXER = 2
    };

private:
    void CreateMenuBar();
    void CreateTabView();
    void CreateMixerTab();
    void CreateTimelineTab();
    void Create3DMixerTab();

    SimpleHaikuEngine* fEngine;

    BMenuBar* fMenuBar;
    BTabView* fTabView;

    // Tab content views
    BView* fMixerView;
    BView* fTimelineView;
    BView* f3DMixerView;

    // Embedded mixer components
    // (We'll embed simplified versions of existing windows)
    BView* fMixerContainer;
    BView* fTimelineContainer;
    BView* f3DMixerContainer;
};

} // namespace HaikuDAW

#endif // UNIFIED_WINDOW_H

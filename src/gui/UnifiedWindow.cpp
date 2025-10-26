/*
 * UnifiedWindow.cpp - Unified window implementation
 */

#include "UnifiedWindow.h"
#include "../audio/SimpleHaikuEngine.h"
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Tab.h>
#include <StringView.h>
#include <stdio.h>

namespace HaikuDAW {

UnifiedWindow::UnifiedWindow(SimpleHaikuEngine* engine)
    : BWindow(BRect(100, 100, 1000, 700), "VeniceDAW - Professional Audio Workstation",
              B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS | B_QUIT_ON_WINDOW_CLOSE)
    , fEngine(engine)
    , fMenuBar(nullptr)
    , fTabView(nullptr)
    , fMixerView(nullptr)
    , fTimelineView(nullptr)
    , f3DMixerView(nullptr)
    , fMixerContainer(nullptr)
    , fTimelineContainer(nullptr)
    , f3DMixerContainer(nullptr)
{
    printf("UnifiedWindow: Creating unified DAW interface\n");

    CreateMenuBar();
    CreateTabView();

    // Set minimum size
    SetSizeLimits(800, 2000, 600, 1400);

    printf("UnifiedWindow: Created successfully\n");
}

UnifiedWindow::~UnifiedWindow()
{
    printf("UnifiedWindow: Destroyed\n");
}

void UnifiedWindow::CreateMenuBar()
{
    fMenuBar = new BMenuBar("menubar");

    // File menu
    BMenu* fileMenu = new BMenu("File");
    fileMenu->AddItem(new BMenuItem("New Project", new BMessage('new_'), 'N'));
    fileMenu->AddItem(new BMenuItem("Open Project...", new BMessage('open'), 'O'));
    fileMenu->AddSeparatorItem();
    fileMenu->AddItem(new BMenuItem("Save Project", new BMessage('save'), 'S'));
    fileMenu->AddItem(new BMenuItem("Save Project As...", new BMessage('svas')));
    fileMenu->AddSeparatorItem();
    fileMenu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
    fMenuBar->AddItem(fileMenu);

    // View menu
    BMenu* viewMenu = new BMenu("View");
    viewMenu->AddItem(new BMenuItem("Mixer", new BMessage(MSG_SWITCH_MIXER), '1'));
    viewMenu->AddItem(new BMenuItem("Timeline", new BMessage(MSG_SWITCH_TIMELINE), '2'));
    viewMenu->AddItem(new BMenuItem("3D Mixer", new BMessage(MSG_SWITCH_3D), '3'));
    fMenuBar->AddItem(viewMenu);

    // Transport menu
    BMenu* transportMenu = new BMenu("Transport");
    transportMenu->AddItem(new BMenuItem("Play", new BMessage('play'), ' '));
    transportMenu->AddItem(new BMenuItem("Stop", new BMessage('stop'), '.'));
    fMenuBar->AddItem(transportMenu);

    // Help menu
    BMenu* helpMenu = new BMenu("Help");
    helpMenu->AddItem(new BMenuItem("Keyboard Shortcuts", new BMessage('keys')));
    helpMenu->AddSeparatorItem();
    helpMenu->AddItem(new BMenuItem("About VeniceDAW", new BMessage('abou')));
    fMenuBar->AddItem(helpMenu);
}

void UnifiedWindow::CreateTabView()
{
    // Create main vertical layout
    BGroupLayout* mainLayout = new BGroupLayout(B_VERTICAL);
    SetLayout(mainLayout);
    mainLayout->SetSpacing(0);

    mainLayout->AddView(fMenuBar);

    // Create tab view
    BRect tabRect(0, 0, 900, 600);
    fTabView = new BTabView(tabRect, "main_tabs");
    fTabView->SetTabWidth(B_WIDTH_FROM_LABEL);

    // Create tabs
    CreateMixerTab();
    CreateTimelineTab();
    Create3DMixerTab();

    mainLayout->AddView(fTabView);

    // Default to mixer tab
    fTabView->Select(TAB_MIXER);
}

void UnifiedWindow::CreateMixerTab()
{
    // Create mixer container
    fMixerContainer = new BView("mixer_container", B_WILL_DRAW);
    fMixerContainer->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

    BGroupLayout* layout = new BGroupLayout(B_VERTICAL);
    fMixerContainer->SetLayout(layout);
    layout->SetInsets(10, 10, 10, 10);

    // Add placeholder for mixer content
    BStringView* label = new BStringView("mixer_label",
                                         "MIXER VIEW\n\n"
                                         "Channel strips, volume controls, and level meters\n"
                                         "appear here.\n\n"
                                         "This tab provides traditional mixer layout similar\n"
                                         "to hardware mixing consoles.");
    label->SetAlignment(B_ALIGN_CENTER);
    layout->AddView(label);

    // Create tab
    BTab* mixerTab = new BTab();
    fTabView->AddTab(fMixerContainer, mixerTab);
    mixerTab->SetLabel("Mixer");
}

void UnifiedWindow::CreateTimelineTab()
{
    // Create timeline container
    fTimelineContainer = new BView("timeline_container", B_WILL_DRAW);
    fTimelineContainer->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

    BGroupLayout* layout = new BGroupLayout(B_VERTICAL);
    fTimelineContainer->SetLayout(layout);
    layout->SetInsets(10, 10, 10, 10);

    // Add placeholder for timeline content
    BStringView* label = new BStringView("timeline_label",
                                         "TIMELINE VIEW\n\n"
                                         "Multi-track arranger with audio clips and waveforms\n"
                                         "appears here.\n\n"
                                         "This tab provides non-destructive audio editing\n"
                                         "similar to Logic Pro and Ableton Live.");
    label->SetAlignment(B_ALIGN_CENTER);
    layout->AddView(label);

    // Create tab
    BTab* timelineTab = new BTab();
    fTabView->AddTab(fTimelineContainer, timelineTab);
    timelineTab->SetLabel("Timeline");
}

void UnifiedWindow::Create3DMixerTab()
{
    // Create 3D mixer container
    f3DMixerContainer = new BView("3dmixer_container", B_WILL_DRAW);
    f3DMixerContainer->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

    BGroupLayout* layout = new BGroupLayout(B_VERTICAL);
    f3DMixerContainer->SetLayout(layout);
    layout->SetInsets(10, 10, 10, 10);

    // Add placeholder for 3D mixer content
    BStringView* label = new BStringView("3dmixer_label",
                                         "3D MIXER VIEW\n\n"
                                         "Interactive 3D spatial audio positioning with OpenGL\n"
                                         "visualization appears here.\n\n"
                                         "This tab provides revolutionary 3D mixing capabilities\n"
                                         "unique to VeniceDAW.");
    label->SetAlignment(B_ALIGN_CENTER);
    layout->AddView(label);

    // Create tab
    BTab* mixerTab = new BTab();
    fTabView->AddTab(f3DMixerContainer, mixerTab);
    mixerTab->SetLabel("3D Mixer");
}

bool UnifiedWindow::QuitRequested()
{
    printf("UnifiedWindow: Quit requested\n");
    be_app->PostMessage(B_QUIT_REQUESTED);
    return true;
}

void UnifiedWindow::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case MSG_SWITCH_MIXER:
            SwitchToMixer();
            break;

        case MSG_SWITCH_TIMELINE:
            SwitchToTimeline();
            break;

        case MSG_SWITCH_3D:
            Switch3DMixer();
            break;

        case MSG_TAB_CHANGED:
        {
            int32 selection = fTabView->Selection();
            printf("UnifiedWindow: Tab changed to %d\n", (int)selection);
            break;
        }

        case 'abou':
        {
            BAlert* alert = new BAlert("About",
                "VeniceDAW v1.0\n"
                "Professional Audio Workstation for Haiku OS\n\n"
                "Unified Interface Design:\n"
                "• Mixer - Traditional channel strip layout\n"
                "• Timeline - Non-destructive audio editing\n"
                "• 3D Mixer - Spatial audio positioning\n\n"
                "Built with 100% native Haiku APIs",
                "Cool!");
            alert->Go();
            break;
        }

        default:
            BWindow::MessageReceived(message);
            break;
    }
}

void UnifiedWindow::SwitchToMixer()
{
    SetCurrentTab(TAB_MIXER);
}

void UnifiedWindow::SwitchToTimeline()
{
    SetCurrentTab(TAB_TIMELINE);
}

void UnifiedWindow::Switch3DMixer()
{
    SetCurrentTab(TAB_3D_MIXER);
}

int32 UnifiedWindow::GetCurrentTab() const
{
    if (!fTabView) return -1;
    return fTabView->Selection();
}

void UnifiedWindow::SetCurrentTab(int32 index)
{
    if (fTabView && index >= 0 && index < fTabView->CountTabs()) {
        fTabView->Select(index);
        printf("UnifiedWindow: Switched to tab %d\n", (int)index);
    }
}

} // namespace HaikuDAW

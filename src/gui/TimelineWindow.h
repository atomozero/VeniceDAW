/*
 * TimelineWindow.h - Professional timeline/arranger view
 * Non-destructive multi-track audio editing with clip management
 */

#ifndef TIMELINE_WINDOW_H
#define TIMELINE_WINDOW_H

#include <Window.h>
#include <View.h>
#include <MenuBar.h>
#include <Slider.h>
#include <StringView.h>
#include <ScrollView.h>
#include <Entry.h>
#include <String.h>
#include <vector>

namespace HaikuDAW {

// Forward declarations
class SimpleHaikuEngine;
class TimelineView;
class TimeRulerView;
class TrackLaneView;

/*
 * AudioClip - Represents a clip on the timeline
 * Non-destructive: references original file with offset/length
 */
struct AudioClip {
    entry_ref fileRef;          // Original audio file
    BString name;               // Clip name (default: filename)
    int64 startFrame;           // Position on timeline (in frames)
    int64 length;               // Duration in frames
    int64 fileOffset;           // Offset into original file
    float gain;                 // Clip gain (0.0 - 2.0)
    int trackIndex;             // Which track this clip belongs to
    bool selected;              // Selection state
    rgb_color color;            // Visual color (from track color)

    AudioClip()
        : startFrame(0)
        , length(0)
        , fileOffset(0)
        , gain(1.0f)
        , trackIndex(0)
        , selected(false)
    {
        color = {180, 180, 180, 255};
    }
};

/*
 * TimeRulerView - Time axis with markers and playhead
 */
class TimeRulerView : public BView {
public:
    TimeRulerView(BRect frame);
    virtual ~TimeRulerView();

    virtual void Draw(BRect updateRect) override;
    virtual void MouseDown(BPoint where) override;

    // Time display
    void SetTimeRange(int64 startFrame, int64 endFrame);
    void SetSampleRate(float sampleRate);
    void SetPlayheadPosition(int64 frame);

    // Conversion helpers
    float FrameToPixel(int64 frame) const;
    int64 PixelToFrame(float pixel) const;

private:
    int64 fStartFrame;
    int64 fEndFrame;
    float fSampleRate;
    int64 fPlayheadFrame;
    float fPixelsPerSecond;  // Zoom level
};

/*
 * TrackLaneView - Single track lane showing clips
 */
class TrackLaneView : public BView {
public:
    TrackLaneView(BRect frame, int trackIndex);
    virtual ~TrackLaneView();

    virtual void Draw(BRect updateRect) override;
    virtual void MouseDown(BPoint where) override;
    virtual void MouseMoved(BPoint where, uint32 transit, const BMessage* message) override;
    virtual void MouseUp(BPoint where) override;

    // Clip management
    void AddClip(const AudioClip& clip);
    void RemoveClip(int index);
    void ClearClips();
    const std::vector<AudioClip>& GetClips() const { return fClips; }

    // Track properties
    void SetTrackIndex(int index) { fTrackIndex = index; }
    int GetTrackIndex() const { return fTrackIndex; }

    void SetTrackName(const char* name);
    const char* GetTrackName() const { return fTrackName.String(); }

    void SetTrackColor(rgb_color color) { fTrackColor = color; }

    // Zoom/scroll support
    void SetTimeRange(int64 startFrame, int64 endFrame);
    void SetSampleRate(float sampleRate);

private:
    void DrawClip(const AudioClip& clip, BRect clipRect);
    BRect GetClipRect(const AudioClip& clip) const;
    int FindClipAtPoint(BPoint point) const;

    int fTrackIndex;
    BString fTrackName;
    rgb_color fTrackColor;
    std::vector<AudioClip> fClips;

    // Time/zoom state
    int64 fStartFrame;
    int64 fEndFrame;
    float fSampleRate;
    float fPixelsPerSecond;

    // Interaction state
    int fDraggingClipIndex;
    BPoint fDragStartPoint;
    int64 fDragStartFrame;
};

/*
 * TimelineView - Container for all track lanes
 */
class TimelineView : public BView {
public:
    TimelineView(BRect frame, SimpleHaikuEngine* engine);
    virtual ~TimelineView();

    virtual void AttachedToWindow() override;
    virtual void Draw(BRect updateRect) override;

    // Track lane management
    void CreateTrackLanes();
    void UpdateTrackCount();
    TrackLaneView* GetTrackLane(int index);

    // Zoom controls
    void SetZoom(float pixelsPerSecond);
    float GetZoom() const { return fPixelsPerSecond; }
    void ZoomIn();
    void ZoomOut();
    void ZoomToFit();

    // Playback position
    void SetPlayheadPosition(int64 frame);
    int64 GetPlayheadPosition() const { return fPlayheadFrame; }

private:
    SimpleHaikuEngine* fEngine;
    std::vector<TrackLaneView*> fTrackLanes;

    float fPixelsPerSecond;  // Zoom level (default 100 pixels = 1 second)
    int64 fPlayheadFrame;    // Current playback position
    float fSampleRate;       // Audio sample rate
};

/*
 * TimelineWindow - Main timeline/arranger window
 */
class TimelineWindow : public BWindow {
public:
    TimelineWindow(SimpleHaikuEngine* engine);
    virtual ~TimelineWindow();

    virtual bool QuitRequested() override;
    virtual void MessageReceived(BMessage* message) override;

    // Playhead update (called from timer)
    void UpdatePlayhead();

    // Message constants
    enum {
        MSG_ZOOM_IN = 'zmin',
        MSG_ZOOM_OUT = 'zmot',
        MSG_ZOOM_FIT = 'zmft',
        MSG_UPDATE_PLAYHEAD = 'upph'
    };

private:
    void CreateMenuBar();
    void CreateTimelineView();
    void CreateTransportBar();

    SimpleHaikuEngine* fEngine;

    BMenuBar* fMenuBar;
    TimeRulerView* fTimeRuler;
    TimelineView* fTimelineView;
    BScrollView* fScrollView;

    // Transport controls
    BView* fTransportBar;
    BStringView* fTimeDisplay;
    BSlider* fZoomSlider;

    // Update timer
    BMessageRunner* fUpdateRunner;
};

} // namespace HaikuDAW

#endif // TIMELINE_WINDOW_H

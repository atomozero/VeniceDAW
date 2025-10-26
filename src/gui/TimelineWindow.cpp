/*
 * TimelineWindow.cpp - Implementation of timeline/arranger view
 */

#include "TimelineWindow.h"
#include "../audio/SimpleHaikuEngine.h"
#include "TrackColors.h"
#include "WaveformView.h"
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <stdio.h>
#include <algorithm>

namespace HaikuDAW {

// =====================================
// TimeRulerView Implementation
// =====================================

TimeRulerView::TimeRulerView(BRect frame)
    : BView(frame, "time_ruler", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW)
    , fStartFrame(0)
    , fEndFrame(441000)  // 10 seconds at 44.1kHz
    , fSampleRate(44100.0f)
    , fPlayheadFrame(0)
    , fPixelsPerSecond(100.0f)
{
    SetViewColor(40, 40, 40);
}

TimeRulerView::~TimeRulerView()
{
}

void TimeRulerView::Draw(BRect updateRect)
{
    BRect bounds = Bounds();

    // Background
    SetHighColor(40, 40, 40);
    FillRect(bounds);

    // Draw time markers every second
    SetHighColor(200, 200, 200);
    font_height fh;
    GetFontHeight(&fh);

    float durationSeconds = (fEndFrame - fStartFrame) / fSampleRate;
    int32 numMarkers = (int32)(durationSeconds) + 1;

    for (int32 i = 0; i <= numMarkers; i++) {
        int64 frame = (int64)(i * fSampleRate);
        float x = FrameToPixel(frame);

        if (x >= bounds.left && x <= bounds.right) {
            // Major marker line
            SetHighColor(200, 200, 200);
            StrokeLine(BPoint(x, bounds.bottom - 15), BPoint(x, bounds.bottom));

            // Time label
            BString timeStr;
            timeStr.SetToFormat("%d:00", i);
            DrawString(timeStr.String(), BPoint(x + 2, bounds.bottom - 5));

            // Minor markers (half seconds)
            if (i < numMarkers) {
                float halfX = FrameToPixel((int64)((i + 0.5) * fSampleRate));
                SetHighColor(150, 150, 150);
                StrokeLine(BPoint(halfX, bounds.bottom - 8), BPoint(halfX, bounds.bottom));
            }
        }
    }

    // Draw playhead
    float playheadX = FrameToPixel(fPlayheadFrame);
    if (playheadX >= bounds.left && playheadX <= bounds.right) {
        SetHighColor(255, 100, 100);  // Red playhead
        StrokeLine(BPoint(playheadX, bounds.top), BPoint(playheadX, bounds.bottom),
                   B_SOLID_HIGH);
    }
}

void TimeRulerView::MouseDown(BPoint where)
{
    // Click to seek
    int64 clickedFrame = PixelToFrame(where.x);
    SetPlayheadPosition(clickedFrame);
    Invalidate();
}

void TimeRulerView::SetTimeRange(int64 startFrame, int64 endFrame)
{
    fStartFrame = startFrame;
    fEndFrame = endFrame;
    Invalidate();
}

void TimeRulerView::SetSampleRate(float sampleRate)
{
    fSampleRate = sampleRate;
    Invalidate();
}

void TimeRulerView::SetPlayheadPosition(int64 frame)
{
    fPlayheadFrame = frame;
    Invalidate();
}

float TimeRulerView::FrameToPixel(int64 frame) const
{
    float seconds = (float)(frame - fStartFrame) / fSampleRate;
    return seconds * fPixelsPerSecond;
}

int64 TimeRulerView::PixelToFrame(float pixel) const
{
    float seconds = pixel / fPixelsPerSecond;
    return fStartFrame + (int64)(seconds * fSampleRate);
}

// =====================================
// TrackLaneView Implementation
// =====================================

TrackLaneView::TrackLaneView(BRect frame, int trackIndex)
    : BView(frame, "track_lane", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW)
    , fTrackIndex(trackIndex)
    , fTrackColor({180, 180, 180, 255})
    , fStartFrame(0)
    , fEndFrame(441000)
    , fSampleRate(44100.0f)
    , fPixelsPerSecond(100.0f)
    , fDraggingClipIndex(-1)
{
    BString name;
    name << "Track " << (trackIndex + 1);
    fTrackName = name;

    SetViewColor(B_TRANSPARENT_COLOR);
}

TrackLaneView::~TrackLaneView()
{
}

void TrackLaneView::Draw(BRect updateRect)
{
    BRect bounds = Bounds();

    // Background
    SetHighColor(60, 60, 60);
    FillRect(bounds);

    // Track separator line
    SetHighColor(40, 40, 40);
    StrokeLine(bounds.LeftBottom(), bounds.RightBottom());

    // Draw track name
    SetHighColor(200, 200, 200);
    BString displayName;
    displayName << fTrackIndex + 1 << ": " << fTrackName;
    DrawString(displayName.String(), BPoint(5, bounds.Height() / 2 + 5));

    // Draw clips
    for (const AudioClip& clip : fClips) {
        BRect clipRect = GetClipRect(clip);
        if (clipRect.Intersects(updateRect)) {
            DrawClip(clip, clipRect);
        }
    }
}

void TrackLaneView::DrawClip(const AudioClip& clip, BRect clipRect)
{
    // Clip background
    rgb_color bgColor = clip.selected ?
                        tint_color(clip.color, B_LIGHTEN_1_TINT) : clip.color;
    SetHighColor(bgColor);
    FillRect(clipRect);

    // Draw simplified waveform representation
    // (In a full implementation, this would use actual audio data)
    PushState();
    BRegion clipRegion(clipRect);
    ConstrainClippingRegion(&clipRegion);

    // Draw waveform-like pattern
    SetHighColor(tint_color(bgColor, B_DARKEN_2_TINT));
    float centerY = clipRect.top + (clipRect.Height() / 2.0f);
    float amplitude = clipRect.Height() * 0.3f;

    // Simple sine wave visualization
    for (float x = clipRect.left; x < clipRect.right; x += 2.0f) {
        float t = (x - clipRect.left) / clipRect.Width();
        float wave = sinf(t * 20.0f * M_PI) * amplitude;
        float y1 = centerY - wave;
        float y2 = centerY + wave;
        StrokeLine(BPoint(x, y1), BPoint(x, y2));
    }

    PopState();

    // Clip border
    SetHighColor(0, 0, 0);
    StrokeRect(clipRect);

    // Clip name
    SetHighColor(255, 255, 255);  // White text
    SetFont(be_plain_font);
    font_height fh;
    GetFontHeight(&fh);
    float textY = clipRect.top + 12 + fh.ascent;
    DrawString(clip.name.String(), BPoint(clipRect.left + 5, textY));
}

BRect TrackLaneView::GetClipRect(const AudioClip& clip) const
{
    BRect bounds = Bounds();

    float startX = (float)(clip.startFrame - fStartFrame) / fSampleRate * fPixelsPerSecond;
    float width = (float)clip.length / fSampleRate * fPixelsPerSecond;

    // Vertical positioning within track lane
    float margin = 2.0f;
    return BRect(startX, bounds.top + margin,
                 startX + width, bounds.bottom - margin);
}

int TrackLaneView::FindClipAtPoint(BPoint point) const
{
    for (size_t i = 0; i < fClips.size(); i++) {
        BRect clipRect = GetClipRect(fClips[i]);
        if (clipRect.Contains(point)) {
            return (int)i;
        }
    }
    return -1;
}

void TrackLaneView::MouseDown(BPoint where)
{
    int clipIndex = FindClipAtPoint(where);
    if (clipIndex >= 0) {
        fDraggingClipIndex = clipIndex;
        fDragStartPoint = where;
        fDragStartFrame = fClips[clipIndex].startFrame;

        // Select this clip
        for (auto& clip : fClips) {
            clip.selected = false;
        }
        fClips[clipIndex].selected = true;
        Invalidate();
    }
}

void TrackLaneView::MouseMoved(BPoint where, uint32 transit, const BMessage* message)
{
    if (fDraggingClipIndex >= 0) {
        // Drag clip
        float deltaX = where.x - fDragStartPoint.x;
        float deltaSeconds = deltaX / fPixelsPerSecond;
        int64 deltaFrames = (int64)(deltaSeconds * fSampleRate);

        fClips[fDraggingClipIndex].startFrame = fDragStartFrame + deltaFrames;

        // Clamp to valid range
        if (fClips[fDraggingClipIndex].startFrame < 0) {
            fClips[fDraggingClipIndex].startFrame = 0;
        }

        Invalidate();
    }
}

void TrackLaneView::MouseUp(BPoint where)
{
    fDraggingClipIndex = -1;
}

void TrackLaneView::AddClip(const AudioClip& clip)
{
    fClips.push_back(clip);
    Invalidate();
}

void TrackLaneView::RemoveClip(int index)
{
    if (index >= 0 && index < (int)fClips.size()) {
        fClips.erase(fClips.begin() + index);
        Invalidate();
    }
}

void TrackLaneView::ClearClips()
{
    fClips.clear();
    Invalidate();
}

void TrackLaneView::SetTrackName(const char* name)
{
    fTrackName = name;
    Invalidate();
}

void TrackLaneView::SetTimeRange(int64 startFrame, int64 endFrame)
{
    fStartFrame = startFrame;
    fEndFrame = endFrame;
    Invalidate();
}

void TrackLaneView::SetSampleRate(float sampleRate)
{
    fSampleRate = sampleRate;
}

// =====================================
// TimelineView Implementation
// =====================================

TimelineView::TimelineView(BRect frame, SimpleHaikuEngine* engine)
    : BView(frame, "timeline_view", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS)
    , fEngine(engine)
    , fPixelsPerSecond(100.0f)
    , fPlayheadFrame(0)
    , fSampleRate(44100.0f)
{
    SetViewColor(50, 50, 50);
}

TimelineView::~TimelineView()
{
}

void TimelineView::AttachedToWindow()
{
    BView::AttachedToWindow();
    CreateTrackLanes();
}

void TimelineView::Draw(BRect updateRect)
{
    BView::Draw(updateRect);

    // Draw playhead line across all tracks
    float playheadX = (float)fPlayheadFrame / fSampleRate * fPixelsPerSecond;
    SetHighColor(255, 100, 100);
    StrokeLine(BPoint(playheadX, 0), BPoint(playheadX, Bounds().Height()));
}

void TimelineView::CreateTrackLanes()
{
    if (!fEngine) return;

    // Clear existing lanes
    for (auto* lane : fTrackLanes) {
        RemoveChild(lane);
        delete lane;
    }
    fTrackLanes.clear();

    // Create vertical layout
    BGroupLayout* layout = new BGroupLayout(B_VERTICAL);
    SetLayout(layout);
    layout->SetSpacing(0);

    // Create lane for each track
    int trackCount = fEngine->GetTrackCount();
    const float kLaneHeight = 80.0f;

    for (int i = 0; i < trackCount; i++) {
        BRect laneRect(0, i * kLaneHeight, Bounds().Width(), (i + 1) * kLaneHeight);
        TrackLaneView* lane = new TrackLaneView(laneRect, i);

        // Get track properties
        SimpleTrack* track = fEngine->GetTrack(i);
        if (track) {
            lane->SetTrackName(track->GetName());
            const TrackColor& trackColor = TrackColorManager::GetColorByIndex(track->GetColorIndex());
            lane->SetTrackColor(trackColor.normal);
        }

        lane->SetSampleRate(fSampleRate);
        lane->SetTimeRange(0, (int64)(10.0f * fSampleRate));  // 10 seconds visible

        fTrackLanes.push_back(lane);
        layout->AddView(lane);
    }
}

void TimelineView::UpdateTrackCount()
{
    CreateTrackLanes();
}

TrackLaneView* TimelineView::GetTrackLane(int index)
{
    if (index >= 0 && index < (int)fTrackLanes.size()) {
        return fTrackLanes[index];
    }
    return nullptr;
}

void TimelineView::SetZoom(float pixelsPerSecond)
{
    fPixelsPerSecond = pixelsPerSecond;

    // Update all lanes
    for (auto* lane : fTrackLanes) {
        lane->SetTimeRange(0, (int64)(Bounds().Width() / pixelsPerSecond * fSampleRate));
    }

    Invalidate();
}

void TimelineView::ZoomIn()
{
    SetZoom(fPixelsPerSecond * 1.5f);
}

void TimelineView::ZoomOut()
{
    SetZoom(fPixelsPerSecond / 1.5f);
}

void TimelineView::ZoomToFit()
{
    // Find longest clip duration
    // For now, just reset to default
    SetZoom(100.0f);
}

void TimelineView::SetPlayheadPosition(int64 frame)
{
    fPlayheadFrame = frame;
    Invalidate();
}

// =====================================
// TimelineWindow Implementation
// =====================================

TimelineWindow::TimelineWindow(SimpleHaikuEngine* engine)
    : BWindow(BRect(100, 100, 900, 600), "Timeline - VeniceDAW",
              B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
    , fEngine(engine)
    , fMenuBar(nullptr)
    , fTimeRuler(nullptr)
    , fTimelineView(nullptr)
    , fScrollView(nullptr)
    , fTransportBar(nullptr)
    , fTimeDisplay(nullptr)
    , fZoomSlider(nullptr)
    , fUpdateRunner(nullptr)
{
    CreateMenuBar();
    CreateTransportBar();
    CreateTimelineView();

    // Start update timer for playhead
    BMessage updateMsg(MSG_UPDATE_PLAYHEAD);
    fUpdateRunner = new BMessageRunner(BMessenger(this), &updateMsg, 50000);  // 20 FPS
}

TimelineWindow::~TimelineWindow()
{
    delete fUpdateRunner;
}

void TimelineWindow::CreateMenuBar()
{
    fMenuBar = new BMenuBar("menubar");

    // File menu
    BMenu* fileMenu = new BMenu("File");
    fileMenu->AddItem(new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W'));
    fMenuBar->AddItem(fileMenu);

    // View menu
    BMenu* viewMenu = new BMenu("View");
    viewMenu->AddItem(new BMenuItem("Zoom In", new BMessage(MSG_ZOOM_IN), '+'));
    viewMenu->AddItem(new BMenuItem("Zoom Out", new BMessage(MSG_ZOOM_OUT), '-'));
    viewMenu->AddItem(new BMenuItem("Zoom to Fit", new BMessage(MSG_ZOOM_FIT), 'F'));
    fMenuBar->AddItem(viewMenu);
}

void TimelineWindow::CreateTransportBar()
{
    fTransportBar = new BView("transport", B_WILL_DRAW);
    fTransportBar->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

    BGroupLayout* layout = new BGroupLayout(B_HORIZONTAL);
    fTransportBar->SetLayout(layout);
    layout->SetInsets(10, 5, 10, 5);

    // Time display
    fTimeDisplay = new BStringView("time_display", "00:00:00.000");
    layout->AddView(fTimeDisplay);

    // Zoom slider
    fZoomSlider = new BSlider("zoom", "Zoom", nullptr, 10, 500, B_HORIZONTAL);
    fZoomSlider->SetValue(100);
    fZoomSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
    fZoomSlider->SetHashMarkCount(10);
    fZoomSlider->SetLimitLabels("10%", "500%");
    fZoomSlider->SetExplicitMaxSize(BSize(200, B_SIZE_UNSET));
    layout->AddView(fZoomSlider);
}

void TimelineWindow::CreateTimelineView()
{
    // Create main vertical layout
    BGroupLayout* mainLayout = new BGroupLayout(B_VERTICAL);
    SetLayout(mainLayout);
    mainLayout->SetSpacing(0);

    mainLayout->AddView(fMenuBar);
    mainLayout->AddView(fTransportBar);

    // Time ruler
    BRect rulerRect(0, 0, 800, 30);
    fTimeRuler = new TimeRulerView(rulerRect);
    mainLayout->AddView(fTimeRuler);

    // Timeline view with scroll support
    BRect timelineRect(0, 0, 800, 400);
    fTimelineView = new TimelineView(timelineRect, fEngine);

    fScrollView = new BScrollView("timeline_scroll", fTimelineView,
                                  B_FOLLOW_ALL, 0, true, true);
    mainLayout->AddView(fScrollView);
}

bool TimelineWindow::QuitRequested()
{
    Hide();
    return false;  // Don't actually quit, just hide
}

void TimelineWindow::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case MSG_ZOOM_IN:
            if (fTimelineView) {
                fTimelineView->ZoomIn();
            }
            break;

        case MSG_ZOOM_OUT:
            if (fTimelineView) {
                fTimelineView->ZoomOut();
            }
            break;

        case MSG_ZOOM_FIT:
            if (fTimelineView) {
                fTimelineView->ZoomToFit();
            }
            break;

        case MSG_UPDATE_PLAYHEAD:
            UpdatePlayhead();
            break;

        default:
            BWindow::MessageReceived(message);
            break;
    }
}

void TimelineWindow::UpdatePlayhead()
{
    if (!fEngine || !fEngine->IsRunning()) {
        return;
    }

    // Update playhead position
    // For now, just increment (will integrate with actual playback later)
    if (fTimelineView && fTimeRuler) {
        int64 currentFrame = fTimelineView->GetPlayheadPosition();
        currentFrame += 2205;  // ~50ms at 44.1kHz

        fTimelineView->SetPlayheadPosition(currentFrame);
        fTimeRuler->SetPlayheadPosition(currentFrame);

        // Update time display
        if (fTimeDisplay) {
            float seconds = (float)currentFrame / 44100.0f;
            int minutes = (int)(seconds / 60.0f);
            int secs = (int)seconds % 60;
            int millis = (int)((seconds - (int)seconds) * 1000.0f);

            BString timeStr;
            timeStr.SetToFormat("%02d:%02d.%03d", minutes, secs, millis);
            fTimeDisplay->SetText(timeStr.String());
        }
    }
}

} // namespace HaikuDAW

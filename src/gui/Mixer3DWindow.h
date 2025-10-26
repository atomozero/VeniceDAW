/*
 * Mixer3DWindow.h - Revolutionary 3D mixer for Haiku OS
 * First native 3D audio interface using BGL (OpenGL)
 */

#ifndef MIXER_3D_WINDOW_H
#define MIXER_3D_WINDOW_H

#include <interface/Window.h>
#include <opengl/GLView.h>
#include <interface/GroupLayout.h>
#include <interface/MenuBar.h>
#include <interface/MenuItem.h>
#include <interface/Button.h>
#include <interface/StringView.h>
#include <app/MessageRunner.h>
#include <support/Locker.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <vector>
#include <math.h>
#include "AudioParticleSystem.h"

namespace HaikuDAW {

// Forward declarations
class SimpleHaikuEngine;
class SimpleTrack;

/*
 * 3D Track representation in OpenGL space
 */
struct Track3D {
    SimpleTrack* track;
    float x, y, z;          // 3D position
    float scale;            // Size based on volume
    float rotation;         // Rotation animation
    float levelHeight;      // Height based on audio level
    float color[3];         // RGB color
    bool selected;          // Selected for editing

    Track3D(SimpleTrack* t) : track(t), x(0), y(0), z(0), scale(1.0f),
                             rotation(0), levelHeight(0), selected(false) {
        // BeOS-inspired vibrant color palette (3dmix heritage)
        static const float beosColors[][3] = {
            {1.0f, 0.85f, 0.2f},   // BeOS signature yellow
            {1.0f, 0.55f, 0.15f},  // Warm orange
            {0.3f, 0.7f, 1.0f},    // Sky blue
            {0.2f, 0.95f, 0.85f},  // Cyan-aqua
            {0.9f, 0.3f, 0.65f},   // Pink-magenta
            {0.55f, 0.9f, 0.35f},  // Lime green
            {0.75f, 0.4f, 1.0f},   // Purple
            {1.0f, 0.65f, 0.35f}   // Peach
        };

        // Assign colors cyclically from BeOS palette
        static int colorIndex = 0;
        int idx = colorIndex % 8;
        color[0] = beosColors[idx][0];
        color[1] = beosColors[idx][1];
        color[2] = beosColors[idx][2];
        colorIndex++;
    }
};

/*
 * 3D OpenGL View for mixer visualization
 */
class Mixer3DView : public BGLView {
public:
    Mixer3DView(BRect frame, SimpleHaikuEngine* engine);
    virtual ~Mixer3DView();
    
    // BGLView interface
    virtual void AttachedToWindow();
    virtual void DetachedFromWindow();
    virtual void FrameResized(float width, float height);
    virtual void Draw(BRect updateRect);
    
    // Mouse interaction
    virtual void MouseDown(BPoint where);
    virtual void MouseUp(BPoint where);
    virtual void MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage);
    
    // Keyboard interaction
    virtual void KeyDown(const char* bytes, int32 numBytes);
    
    // 3D scene management
    void InitGL();
    void RenderScene();
    void UpdateTracks();
    void AnimateScene();
    
    // Camera controls
    void SetCameraAngle(float angleX, float angleY);
    void ZoomCamera(float zoom);
    void ResetCamera();

    // Particle system controls
    void SetParticlesEnabled(bool enabled);
    bool AreParticlesEnabled() const;

protected:
    // Engine reference (protected for SpatialMixer3DView access)
    SimpleHaikuEngine* fEngine;
    
    // 3D tracks (protected for SpatialMixer3DView access)
    std::vector<Track3D> f3DTracks;
    
    // Camera state (protected for SpatialMixer3DView access)
    float fCameraAngleX;
    float fCameraAngleY;
    float fCameraDistance;
    float fCameraTarget[3];
    
    // OpenGL helpers (protected for SpatialMixer3DView access)
    void DrawTrack3D(const Track3D& track);
    void DrawTrackNumber(int trackNumber, float x, float y, float z);
    void ProjectPoint(float x, float y, float z, BPoint& screen);

    // Animation (protected for SpatialMixer3DView access)
    float fAnimationTime;

    // Camera calculation cache (protected for SpatialMixer3DView access)
    bool fCameraDirty;
    float fCachedCameraX;
    float fCachedCameraY;
    float fCachedCameraZ;
    void UpdateCameraCache();

    // Drawing helpers (protected for SpatialMixer3DView access)
    void DrawGrid();

private:
    // Mouse interaction (private)
    BPoint fLastMousePos;
    bool fMouseDown;
    int fSelectedTrack;
    bool fDraggingTrack;  // NEW: Track if dragging a track vs rotating camera
    BPoint fDragStartPos;  // NEW: Track drag start position

    // Thread safety (private)
    BLocker fGLLocker;

    // Particle system
    AudioParticleSystem fParticleSystem;
    bigtime_t fLastUpdateTime;

    // Private OpenGL helpers
    void DrawAxisLabels();
    Track3D* GetTrackAt(BPoint point);
};

// Forward declare for spatial mode
class SpatialMixer3DView;
class SpatialControlPanel;

} // End namespace HaikuDAW temporarily to forward declare from global namespace

// Forward declare from global VeniceDAW namespace
namespace VeniceDAW {
    class AdvancedAudioProcessor;
}

namespace HaikuDAW {

/*
 * Unified 3D Mixer Window - Combines simple and spatial modes
 * (3dmix heritage with modern spatial audio)
 */
class Mixer3DWindow : public BWindow {
public:
    // Constructor supports optional spatial mode
    Mixer3DWindow(SimpleHaikuEngine* engine, ::VeniceDAW::AdvancedAudioProcessor* processor = nullptr);
    virtual ~Mixer3DWindow();

    // BWindow interface
    virtual bool QuitRequested();
    virtual void MessageReceived(BMessage* message);

    // Update from engine
    void UpdateVisualization();

    // Mode switching
    void SetSpatialMode(bool enabled);
    bool IsSpatialMode() const { return fSpatialMode; }

    // Public access for benchmarking
    Mixer3DView* GetView() { return f3DView; }
    SpatialMixer3DView* GetSpatialView();

private:
    void CreateMenuBar();
    void CreateControlsPanel();
    void Create3DView();
    void SwitchToSpatialMode();
    void SwitchToSimpleMode();

    SimpleHaikuEngine* fEngine;
    ::VeniceDAW::AdvancedAudioProcessor* fAudioProcessor;  // Optional for spatial mode

    // Mode state
    bool fSpatialMode;

    // GUI components
    BMenuBar* fMenuBar;
    Mixer3DView* f3DView;  // Can be Mixer3DView or SpatialMixer3DView (polymorphic)
    BView* fControlsPanel;
    SpatialControlPanel* fSpatialControlPanel;  // Only in spatial mode

    // Controls
    BButton* fPlayButton;
    BButton* fStopButton;
    BButton* fResetCameraButton;
    BButton* fModeSwitchButton;  // NEW: Switch between simple/spatial
    BStringView* fInfoDisplay;

    // Update timer
    BMessageRunner* fUpdateRunner;

    // Message constants
    enum {
        MSG_PLAY = 'play',
        MSG_STOP = 'stop',
        MSG_RESET_CAMERA = 'rset',
        MSG_ZOOM_IN = 'zmin',
        MSG_ZOOM_OUT = 'zmot',
        MSG_TOGGLE_PARTICLES = 'tpar',
        MSG_UPDATE_3D = 'up3d',
        MSG_TOGGLE_SPATIAL_MODE = 'tspm'  // NEW: Toggle spatial/simple mode
    };
};

} // namespace HaikuDAW

#endif
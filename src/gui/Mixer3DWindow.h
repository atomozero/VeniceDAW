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
        // Random colors for each track
        color[0] = 0.3f + (rand() % 70) / 100.0f;
        color[1] = 0.3f + (rand() % 70) / 100.0f; 
        color[2] = 0.3f + (rand() % 70) / 100.0f;
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
    void ProjectPoint(float x, float y, float z, BPoint& screen);

private:
    // Mouse interaction (private)
    BPoint fLastMousePos;
    bool fMouseDown;
    int fSelectedTrack;

    // Animation (private)
    float fAnimationTime;

    // Thread safety (private)
    BLocker fGLLocker;

    // Private OpenGL helpers
    void DrawGrid();
    void DrawAxisLabels();
    Track3D* GetTrackAt(BPoint point);
};

/*
 * 3D Mixer Window - Revolutionary interface
 */
class Mixer3DWindow : public BWindow {
public:
    Mixer3DWindow(SimpleHaikuEngine* engine);
    virtual ~Mixer3DWindow();
    
    // BWindow interface
    virtual bool QuitRequested();
    virtual void MessageReceived(BMessage* message);
    
    // Update from engine
    void UpdateVisualization();
    
    // Public access for benchmarking
    Mixer3DView* GetView() { return f3DView; }

private:
    void CreateMenuBar();
    void CreateControlsPanel();
    void Create3DView();
    
    SimpleHaikuEngine* fEngine;
    
    // GUI components
    BMenuBar* fMenuBar;
    Mixer3DView* f3DView;
    BView* fControlsPanel;
    
    // Controls
    BButton* fPlayButton;
    BButton* fStopButton;
    BButton* fResetCameraButton;
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
        MSG_UPDATE_3D = 'up3d'
    };
};

} // namespace HaikuDAW

#endif
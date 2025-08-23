/*
 * Mixer3DView.h - Hardware-accelerated 3D mixer for HaikuDAW
 * 
 * Modern OpenGL-based visualization using BGLView
 * Clean implementation, no legacy code
 */

#ifndef HAIKU_DAW_MIXER_3D_VIEW_H
#define HAIKU_DAW_MIXER_3D_VIEW_H

#include <GLView.h>
#include <Point.h>
#include <Rect.h>
#include <Message.h>
#include <String.h>
#include <vector>
#include <memory>
#include <atomic>

namespace HaikuDAW {

// Forward declarations
class AudioEngine;
class AudioTrack;

/*
 * Modern 3D mixer using hardware acceleration
 * 
 * Features:
 * - OpenGL ES 2.0 / OpenGL 3.3 rendering
 * - Smooth 60fps animation
 * - Interactive mouse/touch control
 * - Real-time audio visualization
 * - GPU shaders for effects
 */
class Mixer3DView : public BGLView {
public:
    Mixer3DView(BRect frame, const char* name);
    virtual ~Mixer3DView();
    
    // BGLView overrides
    virtual void    AttachedToWindow();
    virtual void    DetachedFromWindow();
    virtual void    Draw(BRect updateRect);
    virtual void    FrameResized(float width, float height);
    
    // Mouse interaction
    virtual void    MouseDown(BPoint where);
    virtual void    MouseMoved(BPoint where, uint32 transit, const BMessage* message);
    virtual void    MouseUp(BPoint where);
    virtual void    ScrollWheel(BPoint where, float deltaX, float deltaY);
    
    // Keyboard control
    virtual void    KeyDown(const char* bytes, int32 numBytes);
    virtual void    KeyUp(const char* bytes, int32 numBytes);
    
    // Audio engine connection
    void            SetAudioEngine(AudioEngine* engine);
    void            UpdateFromEngine();
    
    // Channel visualization
    struct Channel3D {
        int32       id;
        BString     name;
        float       x, y, z;           // 3D position
        float       volume;             // Current volume level
        float       peakLevel;          // Peak meter
        rgb_color   color;              // Channel color
        bool        selected;
        bool        muted;
        bool        solo;
        
        // Animation
        float       targetX, targetY, targetZ;
        float       animationSpeed;
    };
    
    void            AddChannel(const Channel3D& channel);
    void            RemoveChannel(int32 channelId);
    void            UpdateChannel(int32 channelId, const Channel3D& channel);
    Channel3D*      GetChannel(int32 channelId);
    
    // Camera control
    void            SetCameraPosition(float x, float y, float z);
    void            SetCameraTarget(float x, float y, float z);
    void            SetCameraFOV(float fov);
    void            ResetCamera();
    void            AnimateCameraTo(float x, float y, float z, bigtime_t duration);
    
    // Rendering options
    void            SetRenderMode(uint32 mode);
    void            SetShowGrid(bool show);
    void            SetShowLabels(bool show);
    void            SetShowMeters(bool show);
    void            SetUseShaders(bool use);
    
    // Themes
    void            ApplyTheme(const BMessage* theme);
    void            SetBackgroundColor(rgb_color color);
    void            SetGridColor(rgb_color color);
    
    // Performance
    float           GetFPS() const;
    int32           GetTriangleCount() const;
    
    // Constants
    enum RenderMode {
        RENDER_MODE_SIMPLE = 0,
        RENDER_MODE_SHADED,
        RENDER_MODE_WIREFRAME,
        RENDER_MODE_PARTICLES
    };
    
protected:
    // OpenGL initialization
    status_t        InitOpenGL();
    void            CleanupOpenGL();
    
    // Rendering
    void            RenderFrame();
    void            RenderChannels();
    void            RenderChannel(const Channel3D& channel);
    void            RenderGrid();
    void            RenderLabels();
    void            RenderMeters();
    void            RenderSelection();
    
    // Shaders
    status_t        LoadShaders();
    uint32          CompileShader(uint32 type, const char* source);
    uint32          CreateShaderProgram(const char* vertexSource, const char* fragmentSource);
    
    // Interaction
    Channel3D*      GetChannelAt(BPoint screenPoint);
    BPoint          Project3DTo2D(float x, float y, float z);
    void            Unproject2DTo3D(BPoint screenPoint, float& x, float& y, float& z);
    
    // Animation
    static int32    AnimationThread(void* data);
    void            AnimationLoop();
    void            UpdateAnimations();
    
private:
    // OpenGL state
    bool            fGLInitialized;
    uint32          fShaderProgram;
    uint32          fVertexBuffer;
    uint32          fIndexBuffer;
    
    // Channels
    std::vector<Channel3D> fChannels;
    mutable BLocker fChannelLock;
    
    // Camera
    struct Camera {
        float       posX, posY, posZ;
        float       targetX, targetY, targetZ;
        float       fov;
        float       nearPlane, farPlane;
        
        // Animation
        float       animTargetX, animTargetY, animTargetZ;
        bigtime_t   animStartTime;
        bigtime_t   animDuration;
        bool        animating;
    } fCamera;
    
    // Interaction state
    bool            fIsDragging;
    BPoint          fLastMousePos;
    int32           fSelectedChannel;
    float           fZoomLevel;
    
    // Rendering options
    uint32          fRenderMode;
    bool            fShowGrid;
    bool            fShowLabels;
    bool            fShowMeters;
    bool            fUseShaders;
    
    // Theme
    rgb_color       fBackgroundColor;
    rgb_color       fGridColor;
    rgb_color       fLabelColor;
    
    // Performance monitoring
    std::atomic<float> fFPS;
    std::atomic<int32> fTriangleCount;
    bigtime_t       fLastFrameTime;
    int32           fFrameCount;
    
    // Audio engine reference
    AudioEngine*    fAudioEngine;
    
    // Animation thread
    thread_id       fAnimationThread;
    std::atomic<bool> fAnimationRunning;
    
    // Constants
    static const float kGridSize;
    static const float kChannelSize;
    static const float kLabelHeight;
    static const char* kVertexShaderSource;
    static const char* kFragmentShaderSource;
};

} // namespace HaikuDAW

#endif // HAIKU_DAW_MIXER_3D_VIEW_H
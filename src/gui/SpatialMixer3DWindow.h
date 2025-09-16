/*
 * SpatialMixer3DWindow.h - Phase 4 Spatial Audio Integration
 * 
 * Professional 3D spatial audio mixer extending VeniceDAW's existing 3D GUI system
 * with real-time spatial positioning, HRTF processing, and surround sound controls.
 */

#ifndef SPATIAL_MIXER_3D_WINDOW_H
#define SPATIAL_MIXER_3D_WINDOW_H

#include "Mixer3DWindow.h"
#include "../audio/AdvancedAudioProcessor.h"
#include <interface/TabView.h>
#include <interface/CheckBox.h>
#include <interface/Slider.h>
#include <interface/MenuField.h>
#include <interface/PopUpMenu.h>
#include <interface/TextControl.h>
#include <storage/FilePanel.h>
#include <interface/Alert.h>
#include <atomic>
#include <mutex>
#include <queue>

namespace HaikuDAW {

// Forward declarations
namespace VeniceDAW {
    class AdvancedAudioProcessor;
    class SurroundProcessor;
    namespace DSP {
        struct Vector3D;
    }
}

/*
 * Enhanced 3D Track with spatial audio parameters
 */
struct SpatialTrack3D : public Track3D {
    // Spatial audio parameters  
    ::VeniceDAW::DSP::Vector3D spatialPosition{0.0f, 0.0f, 0.0f};
    ::VeniceDAW::DSP::Vector3D velocity{0.0f, 0.0f, 0.0f};
    float azimuth{0.0f};           // Calculated spherical coordinate
    float elevation{0.0f};         // Calculated spherical coordinate  
    float distance{1.0f};          // Distance from listener
    float dopplerShift{1.0f};      // Current doppler effect
    float airAbsorption{1.0f};     // High frequency attenuation
    bool spatialEnabled{true};     // Enable spatial processing for this track
    
    // Visual feedback for spatial parameters
    float spatialIndicatorAlpha{0.0f};  // Fade in/out for parameter indicators
    bool showParameters{false};          // Show parameter overlay
    
    SpatialTrack3D(SimpleTrack* t) : Track3D(t) {
        // Enhanced color coding for spatial tracks
        color[0] = 0.4f + (rand() % 60) / 100.0f;  // Slightly brighter colors
        color[1] = 0.4f + (rand() % 60) / 100.0f;
        color[2] = 0.4f + (rand() % 60) / 100.0f;
    }
    
    void UpdateSpatialParameters(const ::VeniceDAW::SurroundProcessor& processor);
};

/*
 * Thread-safe parameter update queue for <10ms audio latency
 */
struct SpatialParameterUpdate {
    enum Type { POSITION, VELOCITY, LISTENER_POS, LISTENER_ORIENT, ROOM_SIZE, HRTF_CONFIG };
    
    Type type;
    int trackIndex{-1};  // -1 for global parameters
    ::VeniceDAW::DSP::Vector3D vector;
    ::VeniceDAW::DSP::Vector3D vector2;  // For orientation (forward, up)
    float value{0.0f};
    bool boolValue{false};
    
    static SpatialParameterUpdate Position(int track, const ::VeniceDAW::DSP::Vector3D& pos) {
        SpatialParameterUpdate update;
        update.type = POSITION;
        update.trackIndex = track;
        update.vector = pos;
        return update;
    }
    
    static SpatialParameterUpdate ListenerPosition(const ::VeniceDAW::DSP::Vector3D& pos) {
        SpatialParameterUpdate update;
        update.type = LISTENER_POS;
        update.trackIndex = -1;
        update.vector = pos;
        return update;
    }
    
    static SpatialParameterUpdate ListenerOrientation(const ::VeniceDAW::DSP::Vector3D& forward, 
                                                     const ::VeniceDAW::DSP::Vector3D& up) {
        SpatialParameterUpdate update;
        update.type = LISTENER_ORIENT;
        update.trackIndex = -1;
        update.vector = forward;
        update.vector2 = up;
        return update;
    }
};

/*
 * Professional spatial audio 3D view extending Mixer3DView
 */
class SpatialMixer3DView : public Mixer3DView {
public:
    SpatialMixer3DView(BRect frame, SimpleHaikuEngine* engine, ::VeniceDAW::AdvancedAudioProcessor* processor);
    virtual ~SpatialMixer3DView();
    
    // Enhanced 3D rendering with spatial visualization
    virtual void Draw(BRect updateRect) override;
    virtual void MouseDown(BPoint where) override;
    virtual void MouseUp(BPoint where) override; 
    virtual void MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage) override;
    virtual void KeyDown(const char* bytes, int32 numBytes) override;
    
    // Spatial audio integration
    void UpdateSpatialTracks();
    void ProcessParameterUpdates();  // Called from audio thread
    void SetSpatialMode(::VeniceDAW::SurroundProcessor::SpatialMode mode);
    ::VeniceDAW::SurroundProcessor::SpatialMode GetSpatialMode() const { return fCurrentSpatialMode; }
    
    // Listener controls
    void SetListenerPosition(const ::VeniceDAW::DSP::Vector3D& position);
    void SetListenerOrientation(const ::VeniceDAW::DSP::Vector3D& forward, const ::VeniceDAW::DSP::Vector3D& up);
    ::VeniceDAW::DSP::Vector3D GetListenerPosition() const { return fListenerPosition; }
    
    // Track spatial positioning
    void SetTrackSpatialPosition(int trackIndex, const ::VeniceDAW::DSP::Vector3D& position);
    ::VeniceDAW::DSP::Vector3D GetTrackSpatialPosition(int trackIndex) const;
    
    // Visualization controls
    void SetShowSpatialIndicators(bool show) { fShowSpatialIndicators = show; Invalidate(); }
    void SetShowListenerVisualization(bool show) { fShowListener = show; Invalidate(); }
    void SetShowRoomBoundaries(bool show) { fShowRoomBounds = show; Invalidate(); }
    
protected:
    // Enhanced OpenGL rendering
    void RenderSpatialScene();
    void DrawSpatialTrack(const SpatialTrack3D& track);
    void DrawListenerVisualization();
    void DrawSpatialIndicators();
    void DrawRoomBoundaries();
    void DrawSurroundSpeakerLayout();
    void DrawHRTFVisualization();
    
    // Spatial parameter calculations
    void UpdateTrackSpatialParameters(SpatialTrack3D& track);
    void CalculateSphericalCoordinates(SpatialTrack3D& track);
    
    // Mouse interaction for spatial positioning
    SpatialTrack3D* GetSpatialTrackAt(BPoint point);
    void BeginTrackPositioning(SpatialTrack3D* track, BPoint startPoint);
    void UpdateTrackPositioning(BPoint currentPoint);
    void EndTrackPositioning();
    
    // Parameter update system
    void QueueParameterUpdate(const SpatialParameterUpdate& update);
    
private:
    // Audio processor reference
    ::VeniceDAW::AdvancedAudioProcessor* fAudioProcessor;
    
    // Enhanced spatial tracks
    std::vector<SpatialTrack3D> fSpatialTracks;
    
    // Listener representation
    ::VeniceDAW::DSP::Vector3D fListenerPosition{0.0f, 0.0f, 0.0f};
    ::VeniceDAW::DSP::Vector3D fListenerForward{0.0f, 1.0f, 0.0f}; 
    ::VeniceDAW::DSP::Vector3D fListenerUp{0.0f, 0.0f, 1.0f};
    
    // Room/environment parameters
    ::VeniceDAW::DSP::Vector3D fRoomSize{10.0f, 8.0f, 3.0f};
    
    // Spatial processing mode
    ::VeniceDAW::SurroundProcessor::SpatialMode fCurrentSpatialMode{::VeniceDAW::SurroundProcessor::SpatialMode::SPATIAL_3D};
    
    // Visualization settings
    bool fShowSpatialIndicators{true};
    bool fShowListener{true};
    bool fShowRoomBounds{true};
    bool fShowSpeakerLayout{false};
    
    // Mouse interaction state
    SpatialTrack3D* fDraggingTrack{nullptr};
    BPoint fDragStartPoint;
    ::VeniceDAW::DSP::Vector3D fDragStartPosition;
    bool fDraggingListener{false};
    
    // Thread-safe parameter updates
    mutable std::mutex fParameterMutex;
    std::queue<SpatialParameterUpdate> fParameterQueue;
    std::atomic<bool> fHasParameterUpdates{false};
    
    // Performance monitoring
    std::atomic<float> fRenderTime{0.0f};
    bigtime_t fLastRenderTime{0};
};

/*
 * Control panels for spatial audio parameters
 */
class SpatialControlPanel : public BView {
public:
    SpatialControlPanel(BRect frame, SpatialMixer3DView* spatialView, 
                       ::VeniceDAW::AdvancedAudioProcessor* processor);
    virtual ~SpatialControlPanel();
    
    virtual void AttachedToWindow() override;
    virtual void MessageReceived(BMessage* message) override;
    
    void UpdateFromProcessor();
    
private:
    void CreateSpatialModeTab();
    void CreateHRTFTab();
    void CreateEnvironmentTab();
    void CreateVisualizationTab();
    
    SpatialMixer3DView* fSpatialView;
    ::VeniceDAW::AdvancedAudioProcessor* fAudioProcessor;
    
    // UI components
    BTabView* fTabView;
    
    // Spatial Mode tab
    BMenuField* fSpatialModeField;
    BPopUpMenu* fSpatialModeMenu;
    BCheckBox* fSpatialEnabledBox;
    
    // HRTF tab  
    BCheckBox* fHRTFEnabledBox;
    BSlider* fCrossfeedSlider;
    BButton* fLoadHRTFButton;
    BStringView* fHRTFStatusView;
    
    // Environment tab
    BSlider* fRoomWidthSlider;
    BSlider* fRoomHeightSlider; 
    BSlider* fRoomDepthSlider;
    BSlider* fReverbAmountSlider;
    BSlider* fReverbDecaySlider;
    BCheckBox* fAirAbsorptionBox;
    BCheckBox* fDopplerBox;
    
    // Visualization tab
    BCheckBox* fShowIndicatorsBox;
    BCheckBox* fShowListenerBox;
    BCheckBox* fShowRoomBoundsBox;
    BCheckBox* fShowSpeakersBox;
    
    // Message constants
    enum {
        MSG_SPATIAL_MODE = 'spmd',
        MSG_SPATIAL_ENABLED = 'spen',
        MSG_HRTF_ENABLED = 'hrte',
        MSG_CROSSFEED = 'cros',
        MSG_LOAD_HRTF = 'lhrt',
        MSG_ROOM_WIDTH = 'rmwd',
        MSG_ROOM_HEIGHT = 'rmht', 
        MSG_ROOM_DEPTH = 'rmdp',
        MSG_REVERB_AMOUNT = 'ramt',
        MSG_REVERB_DECAY = 'rday',
        MSG_AIR_ABSORPTION = 'aabs',
        MSG_DOPPLER = 'dopl',
        MSG_SHOW_INDICATORS = 'sind',
        MSG_SHOW_LISTENER = 'slis',
        MSG_SHOW_ROOM_BOUNDS = 'srbh',
        MSG_SHOW_SPEAKERS = 'sspk'
    };
};

/*
 * Main spatial mixer window integrating all components
 */
class SpatialMixer3DWindow : public BWindow {
public:
    SpatialMixer3DWindow(SimpleHaikuEngine* engine, ::VeniceDAW::AdvancedAudioProcessor* processor);
    virtual ~SpatialMixer3DWindow();
    
    // BWindow interface
    virtual bool QuitRequested() override;
    virtual void MessageReceived(BMessage* message) override;
    
    // Update system
    void UpdateSpatialVisualization();
    
    // Public access
    SpatialMixer3DView* GetSpatialView() { return fSpatialView; }
    SpatialControlPanel* GetControlPanel() { return fControlPanel; }

private:
    void CreateInterface();
    void CreateMenuBar();
    
    SimpleHaikuEngine* fEngine;
    ::VeniceDAW::AdvancedAudioProcessor* fAudioProcessor;
    
    // GUI components
    BMenuBar* fMenuBar;
    SpatialMixer3DView* fSpatialView;
    SpatialControlPanel* fControlPanel;
    
    // Update timer
    BMessageRunner* fUpdateRunner;
    
    // File loading
    BFilePanel* fOpenFilePanel;
    
    // Message constants
    enum {
        MSG_UPDATE_SPATIAL = 'upsp',
        MSG_OPEN_AUDIO_FILE = 'oaf_',
        MSG_OPEN_MULTIPLE_FILES = 'oamf',
        MSG_FILE_REFS = 'refs'
    };
};

} // namespace HaikuDAW

#endif // SPATIAL_MIXER_3D_WINDOW_H
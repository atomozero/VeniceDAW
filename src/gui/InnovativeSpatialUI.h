/*
 * InnovativeSpatialUI.h - Revolutionary Synaptic Spatial Audio Interface
 * 
 * The world's first organic, living spatial audio interface that breaks all
 * conventional DAW patterns. This interface behaves like a living organism,
 * with particle systems, fluid dynamics, neural network layouts, and 
 * gesture-based magnetic controls that redefine how musicians interact with spatial audio.
 * 
 * Key Innovations:
 * - Particle cloud audio sources that pulse with sound
 * - Synaptic neural connections between related sources  
 * - Fluid mercury-like control surfaces that morph dynamically
 * - Magnetic field gesture control with visual feedback
 * - Real-time environmental physics simulation
 * - Context-aware interface that adapts its entire structure to workflow
 * - Organic color schemes that convey spatial information through living gradients
 */

#ifndef INNOVATIVE_SPATIAL_UI_H
#define INNOVATIVE_SPATIAL_UI_H

#include "SpatialMixer3DWindow.h"
#include <GLView.h>
#include <Bitmap.h>
#include <Region.h>
#include <MessageRunner.h>
#include <vector>
#include <memory>
#include <random>
#include <atomic>
#include <chrono>

namespace HaikuDAW {

// Forward declarations
class DynamicControlSurface;
class FluidAudioVisualizer;
class NeuralNetworkLayout;

/*
 * Particle representing a piece of an audio source in 3D space
 */
struct AudioParticle {
    float x, y, z;              // Current position
    float vx, vy, vz;           // Velocity vector
    float life;                 // Particle life (0.0 to 1.0)
    float size;                 // Visual size
    float intensity;            // Brightness based on audio level
    float r, g, b, a;           // Color with transparency
    float magnetic_charge;      // For magnetic field interactions
    
    AudioParticle() : x(0), y(0), z(0), vx(0), vy(0), vz(0), 
                     life(1.0f), size(1.0f), intensity(1.0f),
                     r(1.0f), g(1.0f), b(1.0f), a(1.0f), magnetic_charge(0.0f) {}
};

/*
 * Particle cloud representing a complete audio source
 * This replaces the traditional sphere representation
 */
struct OrganicAudioSource {
    std::vector<AudioParticle> particles;
    VeniceDAW::DSP::Vector3D center_position;    // Cloud center
    VeniceDAW::DSP::Vector3D target_position;    // Where it's moving to
    VeniceDAW::DSP::Vector3D velocity;           // Overall movement
    
    // Audio-responsive properties
    float audio_level;           // Current audio level (0.0 to 1.0)
    float frequency_response[8]; // 8-band frequency analysis
    float spatial_resonance;     // How much it resonates with other sources
    
    // Visual properties
    float base_hue;              // Base color hue
    float saturation_pulse;      // Pulsing saturation based on audio
    float cloud_density;         // How tightly packed particles are
    float cloud_radius;          // Current cloud size
    float target_radius;         // Target cloud size
    
    // Interaction state
    bool is_selected;            // Currently selected for manipulation
    bool is_magnetically_active; // Responding to magnetic gestures
    float selection_glow;        // Glow intensity when selected
    
    // Neural connections to other sources
    std::vector<int> connected_sources; // Indices of connected sources
    std::vector<float> connection_strengths; // Connection strength values
    
    OrganicAudioSource() : center_position(0, 0, 0), target_position(0, 0, 0), 
                          velocity(0, 0, 0), audio_level(0.0f), spatial_resonance(0.0f),
                          base_hue(0.0f), saturation_pulse(0.0f), cloud_density(1.0f),
                          cloud_radius(1.0f), target_radius(1.0f), is_selected(false),
                          is_magnetically_active(false), selection_glow(0.0f) {
        // Initialize frequency response
        for (int i = 0; i < 8; i++) frequency_response[i] = 0.0f;
    }
    
    void UpdateParticles(float deltaTime, const VeniceDAW::DSP::Vector3D& magnetic_field);
    void RegenerateParticles(int particle_count);
    void UpdateFromAudioData(float level, const float* freq_data);
};

/*
 * Synaptic connection between audio sources
 * Visualized as living neural pathways that pulse with correlation
 */
struct SynapticConnection {
    int source_a_index;
    int source_b_index;
    float correlation_strength;   // How correlated the audio sources are
    float visual_intensity;       // Current visual brightness
    float pulse_phase;           // Phase of pulsing animation
    
    // Visual representation  
    std::vector<VeniceDAW::DSP::Vector3D> connection_points; // Points along the connection
    std::vector<float> point_intensities;                   // Brightness at each point
    
    SynapticConnection(int a, int b) : source_a_index(a), source_b_index(b),
                                      correlation_strength(0.0f), visual_intensity(0.0f),
                                      pulse_phase(0.0f) {}
    
    void UpdateConnection(const OrganicAudioSource& a, const OrganicAudioSource& b, float deltaTime);
    void GenerateConnectionPath(const VeniceDAW::DSP::Vector3D& pos_a, const VeniceDAW::DSP::Vector3D& pos_b);
};

/*
 * Magnetic field generator for gesture-based control
 * Users move their hand/cursor to create magnetic fields that sources respond to
 */
struct MagneticField {
    VeniceDAW::DSP::Vector3D position;
    float strength;              // Field strength
    float radius;                // Effective radius
    bool is_attracting;          // True for attraction, false for repulsion
    float visual_ripple_phase;   // Phase of visual ripple effect
    
    MagneticField() : position(0, 0, 0), strength(0.0f), radius(5.0f), 
                     is_attracting(true), visual_ripple_phase(0.0f) {}
    
    VeniceDAW::DSP::Vector3D GetFieldAt(const VeniceDAW::DSP::Vector3D& point) const;
};

/*
 * Environmental physics simulation for the spatial interface
 * Creates a living ecosystem where audio sources interact naturally
 */
class SpatialEcosystem {
public:
    SpatialEcosystem();
    ~SpatialEcosystem();
    
    // Physics simulation
    void UpdatePhysics(float deltaTime);
    void AddGravityWell(const VeniceDAW::DSP::Vector3D& position, float strength);
    void AddTurbulence(const VeniceDAW::DSP::Vector3D& center, float intensity);
    
    // Environmental effects
    void SetAmbientFlow(const VeniceDAW::DSP::Vector3D& flow_direction, float strength);
    void UpdateFromRoomAcoustics(float room_width, float room_height, float room_depth, float reverb);
    
    // Get environmental forces at a position
    VeniceDAW::DSP::Vector3D GetEnvironmentalForce(const VeniceDAW::DSP::Vector3D& position) const;
    
private:
    VeniceDAW::DSP::Vector3D ambient_flow;
    std::vector<VeniceDAW::DSP::Vector3D> gravity_wells;
    std::vector<float> gravity_strengths;
    std::vector<VeniceDAW::DSP::Vector3D> turbulence_centers;
    std::vector<float> turbulence_intensities;
    float environmental_viscosity;
};

/*
 * Revolutionary Synaptic Spatial Interface View
 * This completely reimagines spatial audio control as a living organism
 */
class InnovativeSpatialView : public BGLView {
public:
    InnovativeSpatialView(BRect frame, SimpleHaikuEngine* engine, VeniceDAW::AdvancedAudioProcessor* processor);
    virtual ~InnovativeSpatialView();
    
    // BGLView interface with revolutionary enhancements
    virtual void AttachedToWindow() override;
    virtual void DetachedFromWindow() override;
    virtual void FrameResized(float width, float height) override;
    virtual void Draw(BRect updateRect) override;
    
    // Revolutionary gesture-based interaction
    virtual void MouseDown(BPoint where) override;
    virtual void MouseUp(BPoint where) override;
    virtual void MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage) override;
    virtual void KeyDown(const char* bytes, int32 numBytes) override;
    
    // Organic interface modes
    enum InterfaceMode {
        MODE_PARTICLE_SCULPTOR = 0,  // Sculpt with magnetic fields
        MODE_NEURAL_CONNECTOR,       // Create neural connections
        MODE_ECOSYSTEM_DESIGNER,     // Design environmental physics
        MODE_SYNAPTIC_ANALYZER       // Analyze spatial relationships
    };
    
    void SetInterfaceMode(InterfaceMode mode);
    InterfaceMode GetInterfaceMode() const { return current_mode; }
    
    // Living organism interface updates
    void UpdateOrganicSources();
    void UpdateSynapticConnections();
    void UpdateMagneticFields();
    void UpdateEcosystemPhysics();
    
    // Audio integration
    void ProcessAudioData(const float* audio_levels, const float** frequency_data, int source_count);
    void UpdateSpatialPositions();
    
    // Visual customization
    void SetOrganicColorScheme(int scheme_id);
    void SetParticleDensity(float density);
    void SetNeuralConnectionSensitivity(float sensitivity);
    void SetEnvironmentalPhysicsStrength(float strength);
    
    // Interface adaptation  
    void AdaptToWorkflow(const std::string& workflow_context);
    void MorphInterfaceFor(const std::string& task);
    
protected:
    // Revolutionary rendering system
    void InitializeOrganicGL();
    void RenderOrganicEcosystem();
    void RenderAudioParticles();
    void RenderSynapticConnections();
    void RenderMagneticFields();
    void RenderEnvironmentalEffects();
    
    // Particle system management
    void CreateParticleSystemFor(int source_index);
    void UpdateParticlePhysics(float deltaTime);
    void RegenerateParticlesFor(int source_index);
    
    // Neural network visualization
    void AnalyzeSpatialCorrelations();
    void UpdateNeuralConnections();
    void RenderNeuralPathways();
    
    // Magnetic field interaction
    void HandleMagneticGesture(BPoint screen_pos, bool is_attracting);
    void UpdateMagneticFieldVisualization();
    VeniceDAW::DSP::Vector3D ScreenToWorld(BPoint screen_pos) const;
    
    // Ecosystem physics
    void UpdateEcosystemForces();
    void ApplyEnvironmentalForces();
    void SimulateFluidDynamics();
    
private:
    // Core audio engine
    SimpleHaikuEngine* audio_engine;
    VeniceDAW::AdvancedAudioProcessor* audio_processor;
    
    // Revolutionary data structures
    std::vector<OrganicAudioSource> organic_sources;
    std::vector<SynapticConnection> synaptic_connections;
    std::vector<MagneticField> magnetic_fields;
    std::unique_ptr<SpatialEcosystem> ecosystem;
    
    // Interface state
    InterfaceMode current_mode;
    bool interface_is_adapting;
    float adaptation_progress;
    
    // Gesture interaction
    BPoint last_mouse_pos;
    bool is_creating_magnetic_field;
    bool is_drawing_neural_connection;
    int source_being_connected;
    float gesture_strength;
    
    // Visual customization
    int current_color_scheme;
    float particle_density_multiplier;
    float neural_sensitivity;
    float physics_strength;
    
    // Animation and timing
    std::chrono::high_resolution_clock::time_point last_update_time;
    float global_animation_time;
    std::mt19937 random_generator;
    
    // Performance optimization
    std::atomic<bool> needs_particle_regeneration;
    std::atomic<bool> needs_connection_analysis;
    BBitmap* offscreen_buffer;
    BRegion dirty_regions;
    
    // Context adaptation
    std::string current_workflow_context;
    float interface_morph_progress;
    
    // Revolutionary helper methods
    void InitializeOrganicColorSchemes();
    void AnimateInterfaceMorphing(float deltaTime);
    void OptimizeParticleRendering();
    void UpdateDirtyRegions();
    rgb_color GetOrganicColor(float hue, float saturation, float brightness, float audio_intensity) const;
    void CreateRippleEffect(const VeniceDAW::DSP::Vector3D& center, float strength);
    
    // Spatial audio integration
    void UpdateSpatialProcessorFromSources();
    void SynchronizeWithTraditionalInterface();
};

/*
 * Revolutionary Control Surface that flows like liquid mercury
 * Replaces static tabs and sliders with organic, context-aware controls
 */
class OrganicControlSurface : public BView {
public:
    OrganicControlSurface(BRect frame, InnovativeSpatialView* spatial_view);
    virtual ~OrganicControlSurface();
    
    // BView interface with organic enhancements
    virtual void AttachedToWindow() override;
    virtual void Draw(BRect updateRect) override;
    virtual void MouseDown(BPoint where) override;
    virtual void MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage) override;
    virtual void FrameResized(float width, float height) override;
    
    // Organic control adaptation
    void AdaptToMode(InnovativeSpatialView::InterfaceMode mode);
    void MorphControlsFor(const std::string& context);
    
    // Revolutionary control methods
    void SetOrganicParameter(const std::string& parameter, float value);
    float GetOrganicParameter(const std::string& parameter) const;
    
private:
    void RenderFluidControls(BView* view);
    void UpdateControlFlow();
    void CreateMorphingControls();
    
    InnovativeSpatialView* spatial_view;
    std::map<std::string, float> organic_parameters;
    float control_morph_progress;
    BBitmap* control_surface_buffer;
};

/*
 * Main Revolutionary Spatial Interface Window
 * Houses the complete organic ecosystem interface
 */
class InnovativeSpatialWindow : public BWindow {
public:
    InnovativeSpatialWindow(SimpleHaikuEngine* engine, VeniceDAW::AdvancedAudioProcessor* processor);
    virtual ~InnovativeSpatialWindow();
    
    // BWindow interface
    virtual bool QuitRequested() override;
    virtual void MessageReceived(BMessage* message) override;
    
    // Revolutionary interface management
    void StartOrganicInterface();
    void StopOrganicInterface();
    void SwitchToMode(InnovativeSpatialView::InterfaceMode mode);
    
    // Public access
    InnovativeSpatialView* GetInnovativeView() { return innovative_view; }
    OrganicControlSurface* GetOrganicControls() { return organic_controls; }
    
private:
    void CreateRevolutionaryInterface();
    void CreateOrganicMenuSystem();
    void UpdateOrganicVisualization();
    
    SimpleHaikuEngine* audio_engine;
    VeniceDAW::AdvancedAudioProcessor* audio_processor;
    
    // Revolutionary interface components
    InnovativeSpatialView* innovative_view;
    OrganicControlSurface* organic_controls;
    BMenuBar* organic_menu_bar;
    
    // Real-time updates
    BMessageRunner* organic_update_runner;
    std::atomic<bool> interface_is_active;
    
    // Message constants for organic interface
    enum {
        MSG_UPDATE_ORGANIC = 'upog',
        MSG_SWITCH_MODE = 'swmd',
        MSG_ADAPT_INTERFACE = 'adif',
        MSG_MORPH_CONTROLS = 'morc'
    };
};

} // namespace HaikuDAW

#endif // INNOVATIVE_SPATIAL_UI_H
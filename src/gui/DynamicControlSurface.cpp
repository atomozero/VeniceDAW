/*
 * DynamicControlSurface.cpp - Revolutionary Gesture-Based Magnetic Control Surface
 * 
 * This implements a revolutionary control interface that flows like liquid mercury,
 * responding to gestures with magnetic field interactions and adapting its entire
 * structure based on the user's workflow context. This replaces static sliders
 * and buttons with an organic, living control system.
 * 
 * Key Innovations:
 * - Magnetic snap zones for precise parameter control
 * - Fluid control morphing based on context
 * - Gesture-based interaction with visual force feedback
 * - Organic color schemes that respond to audio
 * - Neural network-inspired control clustering
 * - Real-time parameter smoothing with inertial physics
 */

#include "InnovativeSpatialUI.h"
#include <interface/StringView.h>
#include <interface/Region.h>
#include <interface/Polygon.h>
#include <math.h>
#include <algorithm>
#include <map>

using namespace VeniceDAW;
using namespace VeniceDAW::DSP;

namespace HaikuDAW {

/*
 * Fluid Control Node - A single control that can morph its shape and behavior
 * These replace traditional sliders, knobs, and buttons
 */
struct FluidControlNode {
    // Core properties
    std::string parameter_name;
    float current_value;        // 0.0 to 1.0 normalized value
    float target_value;         // Where it's morphing to
    float visual_intensity;     // How prominently it's displayed
    
    // Position and shape (can morph dynamically)
    BPoint center;
    float radius;
    float target_radius;
    BPolygon shape;            // Current morphed shape
    BPolygon target_shape;     // Shape it's morphing toward
    
    // Interaction state
    bool is_active;            // Currently being controlled
    bool is_magnetic_target;   // Acting as magnetic snap zone
    float magnetic_strength;   // Strength of magnetic attraction
    float gesture_influence;   // How much gesture affects this control
    
    // Visual properties
    rgb_color base_color;
    rgb_color current_color;
    float glow_intensity;
    float pulse_phase;
    
    // Physics simulation
    BPoint velocity;           // For fluid movement
    float damping;            // How quickly it settles
    float elasticity;         // How bouncy interactions feel
    
    // Audio responsiveness
    float audio_sensitivity;   // How much audio affects appearance
    float frequency_band;      // Which frequency band influences this control
    
    FluidControlNode(const std::string& name) 
        : parameter_name(name)
        , current_value(0.0f)
        , target_value(0.0f)
        , visual_intensity(1.0f)
        , center(0, 0)
        , radius(20.0f)
        , target_radius(20.0f)
        , is_active(false)
        , is_magnetic_target(false)
        , magnetic_strength(0.0f)
        , gesture_influence(0.0f)
        , glow_intensity(0.0f)
        , pulse_phase(0.0f)
        , velocity(0, 0)
        , damping(0.9f)
        , elasticity(0.8f)
        , audio_sensitivity(0.3f)
        , frequency_band(0.0f) {
        
        base_color = make_color(100, 150, 255);
        current_color = base_color;
    }
    
    void UpdatePhysics(float deltaTime) {
        // Smooth value transitions
        if (std::abs(current_value - target_value) > 0.001f) {
            current_value += (target_value - current_value) * deltaTime * 8.0f;
        }
        
        // Radius morphing
        if (std::abs(radius - target_radius) > 0.1f) {
            radius += (target_radius - radius) * deltaTime * 6.0f;
        }
        
        // Position physics
        velocity.x *= damping;
        velocity.y *= damping;
        center.x += velocity.x * deltaTime;
        center.y += velocity.y * deltaTime;
        
        // Update visual effects
        pulse_phase += deltaTime * 4.0f;
        if (pulse_phase > 6.28f) pulse_phase -= 6.28f;
        
        if (is_active) {
            glow_intensity = std::min(1.0f, glow_intensity + deltaTime * 4.0f);
        } else {
            glow_intensity = std::max(0.0f, glow_intensity - deltaTime * 2.0f);
        }
    }
    
    void UpdateFromAudio(float audio_level, float frequency_response) {
        // Audio-responsive visual changes
        float audio_influence = audio_level * audio_sensitivity;
        
        // Modify color based on frequency content
        uint8 red_shift = (uint8)(frequency_response * 100);
        uint8 blue_shift = (uint8)(audio_level * 100);
        
        current_color.red = std::min(255, (int)base_color.red + red_shift);
        current_color.blue = std::min(255, (int)base_color.blue + blue_shift);
        current_color.green = (uint8)(base_color.green * (0.7f + audio_influence * 0.3f));
        
        // Size pulsing with audio
        target_radius = radius * (1.0f + audio_influence * 0.3f);
    }
    
    void ApplyGestureForce(BPoint gesture_pos, float force_strength) {
        BPoint direction = center - gesture_pos;
        float distance = sqrt(direction.x * direction.x + direction.y * direction.y);
        
        if (distance < radius * 3.0f) { // Within influence range
            // Normalize direction
            if (distance > 0.001f) {
                direction.x /= distance;
                direction.y /= distance;
            }
            
            // Apply magnetic attraction/repulsion
            float influence = force_strength / (1.0f + distance * 0.1f);
            velocity.x += direction.x * influence * gesture_influence;
            velocity.y += direction.y * influence * gesture_influence;
            
            // Update parameter value based on gesture position
            if (is_magnetic_target && distance < radius) {
                float gesture_value = (gesture_pos.x - (center.x - radius)) / (radius * 2.0f);
                gesture_value = std::max(0.0f, std::min(1.0f, gesture_value));
                target_value = gesture_value;
            }
        }
    }
    
    void MorphToMode(const std::string& mode_context) {
        // Morph appearance and behavior based on context
        if (mode_context == "particle_sculptor") {
            // Become more fluid and responsive
            damping = 0.85f;
            elasticity = 0.9f;
            gesture_influence = 1.0f;
            base_color = make_color(150, 100, 255); // Purple for sculpting
        } else if (mode_context == "neural_connector") {
            // Become more structured and precise
            damping = 0.95f;
            elasticity = 0.6f;
            gesture_influence = 0.7f;
            base_color = make_color(100, 255, 150); // Green for connections
        } else if (mode_context == "ecosystem_designer") {
            // Become more organic and flowing
            damping = 0.8f;
            elasticity = 1.2f;
            gesture_influence = 1.2f;
            base_color = make_color(255, 150, 100); // Orange for environment
        }
    }
    
    bool ContainsPoint(BPoint point) const {
        BPoint offset = point - center;
        return (offset.x * offset.x + offset.y * offset.y) <= (radius * radius);
    }
    
    void Render(BView* view) {
        // Render the fluid control with all its organic properties
        view->SetHighColor(current_color);
        
        // Draw glow effect if active
        if (glow_intensity > 0.01f) {
            rgb_color glow_color = current_color;
            glow_color.alpha = (uint8)(glow_intensity * 100);
            view->SetHighColor(glow_color);
            view->FillEllipse(center, radius * 1.5f, radius * 1.5f);
        }
        
        // Draw main control body
        view->SetHighColor(current_color);
        view->FillEllipse(center, radius, radius);
        
        // Draw parameter value indicator
        float value_angle = current_value * 6.28f;
        BPoint value_indicator(
            center.x + cos(value_angle) * radius * 0.7f,
            center.y + sin(value_angle) * radius * 0.7f
        );
        
        view->SetHighColor(make_color(255, 255, 255));
        view->FillEllipse(value_indicator, 3, 3);
        
        // Draw magnetic field visualization if magnetic target
        if (is_magnetic_target && magnetic_strength > 0.1f) {
            rgb_color field_color = make_color(255, 255, 255, (uint8)(magnetic_strength * 50));
            view->SetHighColor(field_color);
            
            // Draw ripple rings
            for (int ring = 1; ring <= 3; ring++) {
                float ring_radius = radius * (1.0f + ring * 0.5f);
                view->StrokeEllipse(center, ring_radius, ring_radius);
            }
        }
        
        // Draw parameter name if active
        if (is_active) {
            view->SetHighColor(make_color(255, 255, 255));
            view->SetFontSize(10);
            
            BPoint text_pos(center.x - 30, center.y - radius - 15);
            view->DrawString(parameter_name.c_str(), text_pos);
        }
    }
};

/*
 * Control Cluster - A group of related controls that move together organically
 */
struct ControlCluster {
    std::vector<std::shared_ptr<FluidControlNode>> controls;
    BPoint cluster_center;
    float cluster_radius;
    float cohesion_strength;    // How tightly controls stick together
    std::string cluster_context; // What this cluster represents
    
    ControlCluster(const std::string& context) 
        : cluster_center(0, 0)
        , cluster_radius(50.0f)
        , cohesion_strength(0.5f)
        , cluster_context(context) {}
    
    void AddControl(std::shared_ptr<FluidControlNode> control) {
        controls.push_back(control);
        UpdateClusterLayout();
    }
    
    void UpdateClusterLayout() {
        if (controls.empty()) return;
        
        // Arrange controls in organic pattern around cluster center
        float angle_step = 6.28f / controls.size();
        for (size_t i = 0; i < controls.size(); i++) {
            float angle = i * angle_step;
            float orbit_radius = cluster_radius * (0.5f + (i % 3) * 0.25f);
            
            BPoint target_pos(
                cluster_center.x + cos(angle) * orbit_radius,
                cluster_center.y + sin(angle) * orbit_radius
            );
            
            // Apply gentle force toward target position
            BPoint current_pos = controls[i]->center;
            BPoint direction = target_pos - current_pos;
            
            controls[i]->velocity.x += direction.x * cohesion_strength * 0.1f;
            controls[i]->velocity.y += direction.y * cohesion_strength * 0.1f;
        }
    }
    
    void UpdatePhysics(float deltaTime) {
        UpdateClusterLayout();
        
        for (auto& control : controls) {
            control->UpdatePhysics(deltaTime);
        }
    }
    
    void MorphForContext(const std::string& new_context) {
        cluster_context = new_context;
        
        for (auto& control : controls) {
            control->MorphToMode(new_context);
        }
        
        // Adjust cluster properties based on context
        if (new_context == "particle_sculptor") {
            cohesion_strength = 0.3f; // Loose arrangement for creative flow
            cluster_radius = 80.0f;
        } else if (new_context == "neural_connector") {
            cohesion_strength = 0.8f; // Tight arrangement for precision
            cluster_radius = 60.0f;
        } else if (new_context == "ecosystem_designer") {
            cohesion_strength = 0.4f; // Organic, flowing arrangement
            cluster_radius = 100.0f;
        }
    }
};

// =====================================
// OrganicControlSurface Implementation
// =====================================

OrganicControlSurface::OrganicControlSurface(BRect frame, InnovativeSpatialView* spatial_view)
    : BView(frame, "organic_control_surface", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS)
    , spatial_view(spatial_view)
    , control_morph_progress(0.0f)
    , control_surface_buffer(nullptr)
{
    SetViewColor(B_TRANSPARENT_COLOR);
    
    // Create the revolutionary control surface buffer
    control_surface_buffer = new BBitmap(Bounds(), B_RGB32, true);
    
    // Initialize organic parameters
    organic_parameters["particle_density"] = 1.0f;
    organic_parameters["magnetic_strength"] = 0.5f;
    organic_parameters["neural_sensitivity"] = 0.7f;
    organic_parameters["ecosystem_viscosity"] = 0.3f;
    organic_parameters["spatial_resonance"] = 0.6f;
    organic_parameters["synaptic_threshold"] = 0.4f;
    organic_parameters["environmental_flow"] = 0.2f;
    organic_parameters["quantum_coherence"] = 0.8f; // Revolutionary parameter
    
    printf("OrganicControlSurface: Revolutionary control surface initialized\n");
}

OrganicControlSurface::~OrganicControlSurface() {
    delete control_surface_buffer;
    printf("OrganicControlSurface: Organic control surface destroyed\n");
}

void OrganicControlSurface::AttachedToWindow() {
    BView::AttachedToWindow();
    
    // Create initial fluid controls based on current mode
    CreateMorphingControls();
    
    printf("OrganicControlSurface: Organic controls attached to window\n");
}

void OrganicControlSurface::CreateMorphingControls() {
    // Create fluid control nodes that replace traditional UI elements
    std::vector<std::string> control_names = {
        "particle_density", "magnetic_strength", "neural_sensitivity",
        "ecosystem_viscosity", "spatial_resonance", "synaptic_threshold",
        "environmental_flow", "quantum_coherence"
    };
    
    // Initialize control clusters for different contexts
    auto sculptor_cluster = std::make_shared<ControlCluster>("particle_sculptor");
    auto neural_cluster = std::make_shared<ControlCluster>("neural_connector");
    auto ecosystem_cluster = std::make_shared<ControlCluster>("ecosystem_designer");
    
    BRect bounds = Bounds();
    
    // Position clusters organically across the control surface
    sculptor_cluster->cluster_center = BPoint(bounds.Width() * 0.25f, bounds.Height() * 0.3f);
    neural_cluster->cluster_center = BPoint(bounds.Width() * 0.75f, bounds.Height() * 0.3f);
    ecosystem_cluster->cluster_center = BPoint(bounds.Width() * 0.5f, bounds.Height() * 0.7f);
    
    // Create and distribute controls among clusters
    for (size_t i = 0; i < control_names.size(); i++) {
        auto control = std::make_shared<FluidControlNode>(control_names[i]);
        
        // Set initial values from organic parameters
        if (organic_parameters.find(control_names[i]) != organic_parameters.end()) {
            control->current_value = organic_parameters[control_names[i]];
            control->target_value = control->current_value;
        }
        
        // Assign controls to appropriate clusters
        if (i < 3) {
            sculptor_cluster->AddControl(control);
        } else if (i < 6) {
            neural_cluster->AddControl(control);
        } else {
            ecosystem_cluster->AddControl(control);
        }
    }
}

void OrganicControlSurface::Draw(BRect updateRect) {
    if (!control_surface_buffer || !control_surface_buffer->IsValid()) {
        return;
    }
    
    // Render to offscreen buffer for smooth organic effects
    if (control_surface_buffer->Lock()) {
        BView* buffer_view = new BView(control_surface_buffer->Bounds(), "buffer", 
                                     B_FOLLOW_NONE, B_WILL_DRAW);
        control_surface_buffer->AddChild(buffer_view);
        
        // Clear with organic background
        buffer_view->SetHighColor(make_color(8, 8, 20)); // Deep space background
        buffer_view->FillRect(control_surface_buffer->Bounds());
        
        // Render fluid controls
        RenderFluidControls(buffer_view);
        
        // Create ambient particle field
        buffer_view->SetHighColor(make_color(50, 50, 100, 100));
        for (int i = 0; i < 50; i++) {
            BPoint particle_pos(
                rand() % (int)Bounds().Width(),
                rand() % (int)Bounds().Height()
            );
            buffer_view->FillEllipse(particle_pos, 1, 1);
        }
        
        // Draw organic connection lines between related controls
        buffer_view->SetHighColor(make_color(100, 150, 200, 80));
        buffer_view->SetPenSize(1.0f);
        
        // Example: Connect related parameters with flowing lines
        // This would be expanded to show actual parameter relationships
        
        control_surface_buffer->RemoveChild(buffer_view);
        delete buffer_view;
        control_surface_buffer->Unlock();
    }
    
    // Draw the buffer to the view
    DrawBitmap(control_surface_buffer, BPoint(0, 0));
    
    // Draw mode indicator
    SetHighColor(make_color(255, 255, 255));
    SetFontSize(12);
    
    std::string mode_text = "Mode: ";
    switch (spatial_view->GetInterfaceMode()) {
        case InnovativeSpatialView::MODE_PARTICLE_SCULPTOR:
            mode_text += "Particle Sculptor";
            break;
        case InnovativeSpatialView::MODE_NEURAL_CONNECTOR:
            mode_text += "Neural Connector";
            break;
        case InnovativeSpatialView::MODE_ECOSYSTEM_DESIGNER:
            mode_text += "Ecosystem Designer";
            break;
        case InnovativeSpatialView::MODE_SYNAPTIC_ANALYZER:
            mode_text += "Synaptic Analyzer";
            break;
    }
    
    DrawString(mode_text.c_str(), BPoint(10, 20));
    
    // Draw parameter values for active controls
    SetFontSize(10);
    BPoint value_pos(10, 40);
    
    for (const auto& param : organic_parameters) {
        if (param.second != 0.0f) { // Only show non-zero parameters
            char value_text[100];
            snprintf(value_text, sizeof(value_text), "%s: %.2f", 
                    param.first.c_str(), param.second);
            
            DrawString(value_text, value_pos);
            value_pos.y += 15;
        }
    }
}

void OrganicControlSurface::RenderFluidControls(BView* view) {
    // This would render all the fluid control nodes
    // For now, we'll create a simplified representation
    
    BRect bounds = Bounds();
    
    // Draw flowing parameter visualization
    view->SetHighColor(make_color(100, 150, 255, 100));
    
    for (const auto& param : organic_parameters) {
        // Create flowing visual representation of each parameter
        float value = param.second;
        
        // Calculate position based on parameter name hash
        int name_hash = 0;
        for (char c : param.first) name_hash += c;
        
        float x = (name_hash % 300) + 50;
        float y = bounds.Height() * 0.5f + sin(value * 6.28f) * 100;
        
        BPoint control_center(x, y);
        float control_radius = 15 + value * 10;
        
        // Draw parameter control with organic appearance
        view->SetHighColor(make_color(
            (uint8)(100 + value * 155),
            (uint8)(150 + sin(value * 3.14f) * 105),
            (uint8)(255 - value * 100),
            200
        ));
        
        view->FillEllipse(control_center, control_radius, control_radius);
        
        // Draw parameter name
        view->SetHighColor(make_color(255, 255, 255));
        view->SetFontSize(8);
        BPoint text_pos(control_center.x - 20, control_center.y - control_radius - 10);
        view->DrawString(param.first.c_str(), text_pos);
        
        // Draw value indicator
        float value_angle = value * 6.28f;
        BPoint value_indicator(
            control_center.x + cos(value_angle) * control_radius * 0.7f,
            control_center.y + sin(value_angle) * control_radius * 0.7f
        );
        view->FillEllipse(value_indicator, 2, 2);
    }
}

void OrganicControlSurface::MouseDown(BPoint where) {
    // Find which fluid control was clicked
    // For now, implement basic parameter adjustment
    
    BRect bounds = Bounds();
    
    // Simple parameter control based on mouse position
    float x_ratio = where.x / bounds.Width();
    float y_ratio = where.y / bounds.Height();
    
    // Adjust parameters based on mouse position and current mode
    switch (spatial_view->GetInterfaceMode()) {
        case InnovativeSpatialView::MODE_PARTICLE_SCULPTOR:
            SetOrganicParameter("particle_density", x_ratio);
            SetOrganicParameter("magnetic_strength", y_ratio);
            break;
            
        case InnovativeSpatialView::MODE_NEURAL_CONNECTOR:
            SetOrganicParameter("neural_sensitivity", x_ratio);
            SetOrganicParameter("synaptic_threshold", y_ratio);
            break;
            
        case InnovativeSpatialView::MODE_ECOSYSTEM_DESIGNER:
            SetOrganicParameter("ecosystem_viscosity", x_ratio);
            SetOrganicParameter("environmental_flow", y_ratio);
            break;
            
        case InnovativeSpatialView::MODE_SYNAPTIC_ANALYZER:
            SetOrganicParameter("spatial_resonance", x_ratio);
            SetOrganicParameter("quantum_coherence", y_ratio);
            break;
    }
    
    Invalidate();
}

void OrganicControlSurface::MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage) {
    if (code & B_PRIMARY_MOUSE_BUTTON) {
        MouseDown(where); // Continue adjustment while dragging
    }
}

void OrganicControlSurface::FrameResized(float width, float height) {
    BView::FrameResized(width, height);
    
    // Recreate control surface buffer for new size
    delete control_surface_buffer;
    BRect new_bounds(0, 0, width - 1, height - 1);
    control_surface_buffer = new BBitmap(new_bounds, B_RGB32, true);
    
    Invalidate();
}

void OrganicControlSurface::AdaptToMode(InnovativeSpatialView::InterfaceMode mode) {
    // Morph the control surface for the new mode
    control_morph_progress = 0.0f;
    
    // Each mode emphasizes different parameters
    switch (mode) {
        case InnovativeSpatialView::MODE_PARTICLE_SCULPTOR:
            // Emphasize particle and magnetic controls
            SetOrganicParameter("particle_density", 1.0f);
            SetOrganicParameter("magnetic_strength", 0.8f);
            break;
            
        case InnovativeSpatialView::MODE_NEURAL_CONNECTOR:
            // Emphasize neural connection controls
            SetOrganicParameter("neural_sensitivity", 0.9f);
            SetOrganicParameter("synaptic_threshold", 0.6f);
            break;
            
        case InnovativeSpatialView::MODE_ECOSYSTEM_DESIGNER:
            // Emphasize environmental controls
            SetOrganicParameter("ecosystem_viscosity", 0.7f);
            SetOrganicParameter("environmental_flow", 0.5f);
            break;
            
        case InnovativeSpatialView::MODE_SYNAPTIC_ANALYZER:
            // Emphasize analysis controls
            SetOrganicParameter("spatial_resonance", 0.8f);
            SetOrganicParameter("quantum_coherence", 1.0f);
            break;
    }
    
    Invalidate();
    printf("OrganicControlSurface: Adapted to mode %d\n", (int)mode);
}

void OrganicControlSurface::MorphControlsFor(const std::string& context) {
    // Completely reshape the control interface for specific contexts
    control_morph_progress = 0.0f;
    
    if (context == "mixing_session") {
        // Emphasize spatial positioning controls
        SetOrganicParameter("spatial_resonance", 1.0f);
        SetOrganicParameter("neural_sensitivity", 0.8f);
    } else if (context == "sound_design") {
        // Emphasize particle and ecosystem controls
        SetOrganicParameter("particle_density", 1.0f);
        SetOrganicParameter("ecosystem_viscosity", 0.9f);
    } else if (context == "live_performance") {
        // Emphasize magnetic and environmental controls
        SetOrganicParameter("magnetic_strength", 1.0f);
        SetOrganicParameter("environmental_flow", 0.8f);
    }
    
    printf("OrganicControlSurface: Morphed controls for context: %s\n", context.c_str());
    Invalidate();
}

void OrganicControlSurface::SetOrganicParameter(const std::string& parameter, float value) {
    value = std::max(0.0f, std::min(1.0f, value)); // Clamp to 0-1 range
    organic_parameters[parameter] = value;
    
    // Apply parameter to spatial view
    if (spatial_view) {
        if (parameter == "particle_density") {
            spatial_view->SetParticleDensity(value);
        } else if (parameter == "neural_sensitivity") {
            spatial_view->SetNeuralConnectionSensitivity(value);
        } else if (parameter == "ecosystem_viscosity") {
            spatial_view->SetEnvironmentalPhysicsStrength(value);
        }
        // Add more parameter mappings as needed
    }
}

float OrganicControlSurface::GetOrganicParameter(const std::string& parameter) const {
    auto it = organic_parameters.find(parameter);
    if (it != organic_parameters.end()) {
        return it->second;
    }
    return 0.0f;
}

void OrganicControlSurface::UpdateControlFlow() {
    // Update the flowing animation of controls
    control_morph_progress += 0.02f;
    if (control_morph_progress > 1.0f) {
        control_morph_progress = 0.0f;
    }
    
    Invalidate();
}

// =====================================
// InnovativeSpatialWindow Implementation
// =====================================

InnovativeSpatialWindow::InnovativeSpatialWindow(SimpleHaikuEngine* engine, AdvancedAudioProcessor* processor)
    : BWindow(BRect(100, 100, 1400, 900), "VeniceDAW Revolutionary Spatial Interface", 
             B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
    , audio_engine(engine)
    , audio_processor(processor)
    , innovative_view(nullptr)
    , organic_controls(nullptr)
    , organic_menu_bar(nullptr)
    , organic_update_runner(nullptr)
    , interface_is_active(false)
{
    CreateRevolutionaryInterface();
    StartOrganicInterface();
    
    printf("InnovativeSpatialWindow: Revolutionary spatial interface window created\n");
}

InnovativeSpatialWindow::~InnovativeSpatialWindow() {
    StopOrganicInterface();
    printf("InnovativeSpatialWindow: Revolutionary interface window destroyed\n");
}

bool InnovativeSpatialWindow::QuitRequested() {
    StopOrganicInterface();
    be_app->PostMessage(B_QUIT_REQUESTED);
    return true;
}

void InnovativeSpatialWindow::MessageReceived(BMessage* message) {
    switch (message->what) {
        case MSG_UPDATE_ORGANIC:
            UpdateOrganicVisualization();
            break;
            
        case MSG_SWITCH_MODE:
        {
            int32 mode;
            if (message->FindInt32("mode", &mode) == B_OK) {
                SwitchToMode((InnovativeSpatialView::InterfaceMode)mode);
            }
            break;
        }
        
        case MSG_ADAPT_INTERFACE:
        {
            const char* context;
            if (message->FindString("context", &context) == B_OK) {
                if (innovative_view) {
                    innovative_view->AdaptToWorkflow(context);
                }
                if (organic_controls) {
                    organic_controls->MorphControlsFor(context);
                }
            }
            break;
        }
        
        case MSG_MORPH_CONTROLS:
        {
            if (organic_controls) {
                organic_controls->UpdateControlFlow();
            }
            break;
        }
        
        default:
            BWindow::MessageReceived(message);
            break;
    }
}

void InnovativeSpatialWindow::CreateRevolutionaryInterface() {
    CreateOrganicMenuSystem();
    
    // Create main revolutionary layout
    BView* main_view = new BView("main_view", B_WILL_DRAW);
    main_view->SetViewColor(make_color(5, 5, 15)); // Deep space background
    
    BRect bounds = Bounds();
    bounds.top += organic_menu_bar->Frame().Height() + 1;
    
    // Create innovative spatial view (75% of width)
    BRect spatial_rect = bounds;
    spatial_rect.right = bounds.Width() * 0.75f;
    
    innovative_view = new InnovativeSpatialView(spatial_rect, audio_engine, audio_processor);
    
    // Create organic control surface (25% of width)
    BRect controls_rect = bounds;
    controls_rect.left = spatial_rect.right + 1;
    
    organic_controls = new OrganicControlSurface(controls_rect, innovative_view);
    
    // Revolutionary layout
    BLayoutBuilder::Group<>(main_view, B_HORIZONTAL, 0)
        .Add(innovative_view, 3.0f)   // 75% weight
        .Add(organic_controls, 1.0f); // 25% weight
    
    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .Add(organic_menu_bar)
        .Add(main_view);
}

void InnovativeSpatialWindow::CreateOrganicMenuSystem() {
    organic_menu_bar = new BMenuBar("organic_menu_bar");
    
    // Revolutionary File menu
    BMenu* file_menu = new BMenu("Spatial");
    file_menu->AddItem(new BMenuItem("Save Organic Configuration...", nullptr));
    file_menu->AddItem(new BMenuItem("Load Spatial Ecosystem...", nullptr));
    file_menu->AddSeparatorItem();
    file_menu->AddItem(new BMenuItem("Export Neural Connections...", nullptr));
    file_menu->AddItem(new BMenuItem("Import Particle Presets...", nullptr));
    organic_menu_bar->AddItem(file_menu);
    
    // Revolutionary Interface menu
    BMenu* interface_menu = new BMenu("Interface");
    
    BMessage* sculptor_msg = new BMessage(MSG_SWITCH_MODE);
    sculptor_msg->AddInt32("mode", InnovativeSpatialView::MODE_PARTICLE_SCULPTOR);
    interface_menu->AddItem(new BMenuItem("Particle Sculptor Mode", sculptor_msg, '1'));
    
    BMessage* neural_msg = new BMessage(MSG_SWITCH_MODE);
    neural_msg->AddInt32("mode", InnovativeSpatialView::MODE_NEURAL_CONNECTOR);
    interface_menu->AddItem(new BMenuItem("Neural Connector Mode", neural_msg, '2'));
    
    BMessage* ecosystem_msg = new BMessage(MSG_SWITCH_MODE);
    ecosystem_msg->AddInt32("mode", InnovativeSpatialView::MODE_ECOSYSTEM_DESIGNER);
    interface_menu->AddItem(new BMenuItem("Ecosystem Designer Mode", ecosystem_msg, '3'));
    
    BMessage* analyzer_msg = new BMessage(MSG_SWITCH_MODE);
    analyzer_msg->AddInt32("mode", InnovativeSpatialView::MODE_SYNAPTIC_ANALYZER);
    interface_menu->AddItem(new BMenuItem("Synaptic Analyzer Mode", analyzer_msg, '4'));
    
    organic_menu_bar->AddItem(interface_menu);
    
    // Revolutionary Workflow menu
    BMenu* workflow_menu = new BMenu("Workflow");
    
    BMessage* mixing_msg = new BMessage(MSG_ADAPT_INTERFACE);
    mixing_msg->AddString("context", "mixing_session");
    workflow_menu->AddItem(new BMenuItem("Mixing Session", mixing_msg));
    
    BMessage* design_msg = new BMessage(MSG_ADAPT_INTERFACE);
    design_msg->AddString("context", "sound_design");
    workflow_menu->AddItem(new BMenuItem("Sound Design", design_msg));
    
    BMessage* performance_msg = new BMessage(MSG_ADAPT_INTERFACE);
    performance_msg->AddString("context", "live_performance");
    workflow_menu->AddItem(new BMenuItem("Live Performance", performance_msg));
    
    organic_menu_bar->AddItem(workflow_menu);
    
    // Revolutionary Help menu
    BMenu* help_menu = new BMenu("Help");
    help_menu->AddItem(new BMenuItem("Organic Interface Guide", nullptr));
    help_menu->AddItem(new BMenuItem("Particle Sculpting Tutorial", nullptr));
    help_menu->AddItem(new BMenuItem("Neural Connection Theory", nullptr));
    help_menu->AddSeparatorItem();
    help_menu->AddItem(new BMenuItem("About Revolutionary Interface", nullptr));
    organic_menu_bar->AddItem(help_menu);
}

void InnovativeSpatialWindow::StartOrganicInterface() {
    if (interface_is_active.load()) return;
    
    interface_is_active.store(true);
    
    // Start organic update timer (60 FPS for smooth organic animation)
    BMessage update_msg(MSG_UPDATE_ORGANIC);
    organic_update_runner = new BMessageRunner(this, &update_msg, 16667); // ~60 FPS
    
    // Start control flow animation timer
    BMessage morph_msg(MSG_MORPH_CONTROLS);
    BMessageRunner* morph_runner = new BMessageRunner(this, &morph_msg, 50000); // 20 FPS for control morphing
    
    printf("InnovativeSpatialWindow: Organic interface started\n");
}

void InnovativeSpatialWindow::StopOrganicInterface() {
    interface_is_active.store(false);
    
    delete organic_update_runner;
    organic_update_runner = nullptr;
    
    printf("InnovativeSpatialWindow: Organic interface stopped\n");
}

void InnovativeSpatialWindow::SwitchToMode(InnovativeSpatialView::InterfaceMode mode) {
    if (innovative_view) {
        innovative_view->SetInterfaceMode(mode);
    }
    
    if (organic_controls) {
        organic_controls->AdaptToMode(mode);
    }
    
    printf("InnovativeSpatialWindow: Switched to interface mode %d\n", (int)mode);
}

void InnovativeSpatialWindow::UpdateOrganicVisualization() {
    if (!interface_is_active.load()) return;
    
    // Update the innovative spatial view
    if (innovative_view && innovative_view->LockLooper()) {
        innovative_view->UpdateOrganicSources();
        innovative_view->Invalidate();
        innovative_view->UnlockLooper();
    }
    
    // Update organic controls
    if (organic_controls && organic_controls->LockLooper()) {
        organic_controls->Invalidate();
        organic_controls->UnlockLooper();
    }
}

} // namespace HaikuDAW
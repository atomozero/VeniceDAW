/*
 * SpatialMixer3DWindow.cpp - Phase 4 Spatial Audio Integration Implementation
 * 
 * Professional spatial audio integration for VeniceDAW's 3D mixer interface.
 * Maintains <10ms audio latency while providing real-time visual feedback.
 */

#include "SpatialMixer3DWindow.h"
#include "../audio/SimpleHaikuEngine.h"
#include "../benchmark/PerformanceStation.h"
#include <app/Application.h>
#include <interface/Alert.h>
#include <interface/LayoutBuilder.h>
#include <interface/GroupLayoutBuilder.h>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>

using namespace VeniceDAW;
using namespace VeniceDAW::DSP;

namespace HaikuDAW {

// =====================================
// SpatialTrack3D Implementation
// =====================================

void SpatialTrack3D::UpdateSpatialParameters(const SurroundProcessor& processor) {
    // Get current spatial parameters from processor
    spatialPosition = processor.GetSourcePosition();
    distance = processor.GetDistance();
    azimuth = processor.GetAzimuth();
    elevation = processor.GetElevation();
    
    // Update visual position to match spatial coordinates
    // Convert spatial coordinates to 3D scene coordinates
    x = spatialPosition.x;
    y = spatialPosition.z;  // Z becomes Y in OpenGL scene
    z = spatialPosition.y;  // Y becomes Z (depth) in OpenGL scene
    
    // Update visual scale based on distance (closer = larger)
    scale = std::max(0.5f, 2.0f / (1.0f + distance * 0.3f));
    
    // Color intensity based on distance and activity
    float intensity = std::max(0.3f, 1.0f - distance * 0.1f);
    for (int i = 0; i < 3; i++) {
        color[i] = std::min(1.0f, color[i] * intensity);
    }
}

// =====================================
// SpatialMixer3DView Implementation
// =====================================

SpatialMixer3DView::SpatialMixer3DView(BRect frame, SimpleHaikuEngine* engine, 
                                     AdvancedAudioProcessor* processor)
    : Mixer3DView(frame, engine)
    , fAudioProcessor(processor)
    , fLastRenderTime(system_time())
{
    printf("SpatialMixer3DView: Initializing Phase 4 spatial audio integration\n");
    
    // Set initial listener position at origin
    fListenerPosition = Vector3D(0.0f, 0.0f, 0.0f);
    fListenerForward = Vector3D(0.0f, 1.0f, 0.0f);
    fListenerUp = Vector3D(0.0f, 0.0f, 1.0f);
    
    // Initialize room size to reasonable defaults
    fRoomSize = Vector3D(10.0f, 8.0f, 3.0f);
    
    // IMPORTANT: First populate base tracks from engine
    UpdateTracks();  // This creates f3DTracks from engine tracks
    UpdateSpatialTracks();  // Then create spatial tracks from f3DTracks
}

SpatialMixer3DView::~SpatialMixer3DView()
{
    printf("SpatialMixer3DView: Cleaning up spatial audio integration\n");
    
    // Process any remaining parameter updates before destruction
    ProcessParameterUpdates();
    
    // Clear parameter queue
    {
        std::lock_guard<std::mutex> lock(fParameterMutex);
        while (!fParameterQueue.empty()) {
            fParameterQueue.pop();
        }
    }
}

void SpatialMixer3DView::Draw(BRect updateRect) 
{
    bigtime_t startTime = system_time();
    
    // Process any pending parameter updates from audio thread
    if (fHasParameterUpdates.load()) {
        ProcessParameterUpdates();
    }
    
    // Update spatial parameters for all tracks
    UpdateSpatialTracks();
    
    // Do NOT call parent Draw() - we handle everything ourselves
    // Mixer3DView::Draw(updateRect);  // REMOVED - was causing double rendering
    
    // Render our complete spatial scene
    LockGL();
    RenderSpatialScene();  // This renders everything we need
    SwapBuffers();
    UnlockGL();
    
    // Update performance monitoring
    bigtime_t endTime = system_time();
    fRenderTime.store((endTime - startTime) / 1000.0f); // Convert to milliseconds
    fLastRenderTime = endTime;
}

void SpatialMixer3DView::RenderSpatialScene()
{
    // Clear and setup 3D scene (parent class handles basic setup)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Set up camera (enhanced for spatial visualization)
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Position camera for optimal spatial visualization
    gluLookAt(fCameraDistance * cos(fCameraAngleX) * cos(fCameraAngleY),
              fCameraDistance * sin(fCameraAngleX),
              fCameraDistance * sin(fCameraAngleY),
              fCameraTarget[0], fCameraTarget[1], fCameraTarget[2],
              0.0f, 0.0f, 1.0f);
    
    // Draw room boundaries if enabled
    if (fShowRoomBounds) {
        DrawRoomBoundaries();
    }
    
    // Draw speaker layout for surround modes
    if (fShowSpeakerLayout) {
        DrawSurroundSpeakerLayout();
    }
    
    // Draw listener visualization
    if (fShowListener) {
        DrawListenerVisualization();
    }
    
    // Draw spatial tracks with enhanced visualization
    for (const auto& track : fSpatialTracks) {
        DrawSpatialTrack(track);
    }
    
    // Draw spatial parameter indicators
    if (fShowSpatialIndicators) {
        DrawSpatialIndicators();
    }
    
    // Draw HRTF processing indicator if enabled
    if (fAudioProcessor && fAudioProcessor->GetSurroundProcessor().IsHRTFEnabled()) {
        DrawHRTFVisualization();
    }
}

void SpatialMixer3DView::DrawSpatialTrack(const SpatialTrack3D& track)
{
    if (!track.spatialEnabled) {
        // Draw as regular track if spatial processing disabled
        DrawTrack3D(track);
        return;
    }
    
    glPushMatrix();
    
    // Position in 3D space - debug print to verify
    // printf("Drawing track at (%.2f, %.2f, %.2f)\n", track.x, track.y, track.z);
    glTranslatef(track.x, track.y, track.z);
    glScalef(track.scale, track.scale, track.scale);
    glRotatef(track.rotation, 0.0f, 0.0f, 1.0f);
    
    // Enhanced color coding for spatial tracks
    float alpha = track.selected ? 1.0f : 0.8f;
    glColor4f(track.color[0], track.color[1], track.color[2], alpha);
    
    // Draw main track representation as sphere for better 3D positioning
    GLUquadric* quadric = gluNewQuadric();
    gluSphere(quadric, 0.5, 16, 16);
    gluDeleteQuadric(quadric);
    
    // Draw spatial indicator rings showing effective range
    if (track.showParameters || track.selected) {
        glColor4f(1.0f, 1.0f, 1.0f, 0.3f);
        glPushMatrix();
        glScalef(track.distance * 0.5f, track.distance * 0.5f, track.distance * 0.5f);
        GLUquadric* wireQuadric = gluNewQuadric();
        gluQuadricDrawStyle(wireQuadric, GLU_LINE);
        gluSphere(wireQuadric, 1.0, 12, 12);
        gluDeleteQuadric(wireQuadric);
        glPopMatrix();
        
        // Draw directional indicator for azimuth
        glBegin(GL_LINES);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        float azimuthX = cos(track.azimuth) * 1.5f;
        float azimuthY = sin(track.azimuth) * 1.5f;
        glVertex3f(azimuthX, azimuthY, 0.0f);
        glEnd();
    }
    
    // Draw level visualization (height based on audio level)
    if (track.levelHeight > 0.01f) {
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, track.levelHeight * 2.0f);
        glColor4f(track.color[0] * 0.5f, track.color[1] * 0.5f, track.color[2] * 0.5f, 0.6f);
        glScalef(0.3f, 0.3f, track.levelHeight);
        // Draw a simple cube using GL_QUADS
        glBegin(GL_QUADS);
        // Front face
        glVertex3f(-0.5f, -0.5f,  0.5f);
        glVertex3f( 0.5f, -0.5f,  0.5f);
        glVertex3f( 0.5f,  0.5f,  0.5f);
        glVertex3f(-0.5f,  0.5f,  0.5f);
        // Back face
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(-0.5f,  0.5f, -0.5f);
        glVertex3f( 0.5f,  0.5f, -0.5f);
        glVertex3f( 0.5f, -0.5f, -0.5f);
        // Top face
        glVertex3f(-0.5f,  0.5f, -0.5f);
        glVertex3f(-0.5f,  0.5f,  0.5f);
        glVertex3f( 0.5f,  0.5f,  0.5f);
        glVertex3f( 0.5f,  0.5f, -0.5f);
        // Bottom face
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f( 0.5f, -0.5f, -0.5f);
        glVertex3f( 0.5f, -0.5f,  0.5f);
        glVertex3f(-0.5f, -0.5f,  0.5f);
        // Right face
        glVertex3f( 0.5f, -0.5f, -0.5f);
        glVertex3f( 0.5f,  0.5f, -0.5f);
        glVertex3f( 0.5f,  0.5f,  0.5f);
        glVertex3f( 0.5f, -0.5f,  0.5f);
        // Left face
        glVertex3f(-0.5f, -0.5f, -0.5f);
        glVertex3f(-0.5f, -0.5f,  0.5f);
        glVertex3f(-0.5f,  0.5f,  0.5f);
        glVertex3f(-0.5f,  0.5f, -0.5f);
        glEnd();
        glPopMatrix();
    }
    
    glPopMatrix();
}

void SpatialMixer3DView::DrawListenerVisualization()
{
    glPushMatrix();
    
    // Position listener
    glTranslatef(fListenerPosition.x, fListenerPosition.y, fListenerPosition.z);
    
    // Draw listener head representation
    glColor3f(0.0f, 1.0f, 0.0f);  // Green for listener
    GLUquadric* listenerQuadric = gluNewQuadric();
    gluSphere(listenerQuadric, 0.3, 12, 12);
    gluDeleteQuadric(listenerQuadric);
    
    // Draw orientation indicators
    glBegin(GL_LINES);
    
    // Forward direction (red line)
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(fListenerForward.x * 2.0f, fListenerForward.y * 2.0f, fListenerForward.z * 2.0f);
    
    // Up direction (blue line)
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(fListenerUp.x * 1.5f, fListenerUp.y * 1.5f, fListenerUp.z * 1.5f);
    
    glEnd();
    
    glPopMatrix();
}

void SpatialMixer3DView::DrawRoomBoundaries()
{
    // Draw room as wireframe box
    glColor4f(0.5f, 0.5f, 0.5f, 0.3f);
    glLineWidth(1.0f);
    
    float halfWidth = fRoomSize.x * 0.5f;
    float halfHeight = fRoomSize.y * 0.5f; 
    float halfDepth = fRoomSize.z * 0.5f;
    
    glBegin(GL_LINES);
    
    // Bottom face
    glVertex3f(-halfWidth, -halfHeight, -halfDepth);
    glVertex3f( halfWidth, -halfHeight, -halfDepth);
    glVertex3f( halfWidth, -halfHeight, -halfDepth);
    glVertex3f( halfWidth,  halfHeight, -halfDepth);
    glVertex3f( halfWidth,  halfHeight, -halfDepth);
    glVertex3f(-halfWidth,  halfHeight, -halfDepth);
    glVertex3f(-halfWidth,  halfHeight, -halfDepth);
    glVertex3f(-halfWidth, -halfHeight, -halfDepth);
    
    // Top face
    glVertex3f(-halfWidth, -halfHeight,  halfDepth);
    glVertex3f( halfWidth, -halfHeight,  halfDepth);
    glVertex3f( halfWidth, -halfHeight,  halfDepth);
    glVertex3f( halfWidth,  halfHeight,  halfDepth);
    glVertex3f( halfWidth,  halfHeight,  halfDepth);
    glVertex3f(-halfWidth,  halfHeight,  halfDepth);
    glVertex3f(-halfWidth,  halfHeight,  halfDepth);
    glVertex3f(-halfWidth, -halfHeight,  halfDepth);
    
    // Vertical edges
    glVertex3f(-halfWidth, -halfHeight, -halfDepth);
    glVertex3f(-halfWidth, -halfHeight,  halfDepth);
    glVertex3f( halfWidth, -halfHeight, -halfDepth);
    glVertex3f( halfWidth, -halfHeight,  halfDepth);
    glVertex3f( halfWidth,  halfHeight, -halfDepth);
    glVertex3f( halfWidth,  halfHeight,  halfDepth);
    glVertex3f(-halfWidth,  halfHeight, -halfDepth);
    glVertex3f(-halfWidth,  halfHeight,  halfDepth);
    
    glEnd();
}

void SpatialMixer3DView::DrawSurroundSpeakerLayout()
{
    // Draw standard 5.1 or 7.1 speaker positions
    ChannelConfiguration config = fAudioProcessor->GetSurroundProcessor().GetChannelConfiguration();
    
    glColor3f(1.0f, 1.0f, 0.0f);  // Yellow for speakers
    
    if (config == ChannelConfiguration::SURROUND_5_1) {
        // Standard 5.1 layout
        float radius = 3.0f;
        float angles[] = { 0.0f, 30.0f, -30.0f, 110.0f, -110.0f };  // Front L/R, Rear L/R
        
        for (int i = 0; i < 5; i++) {
            float angle = angles[i] * M_PI / 180.0f;
            float x = radius * cos(angle);
            float y = radius * sin(angle);
            
            glPushMatrix();
            glTranslatef(x, y, 0.0f);
            GLUquadric* coneQuadric = gluNewQuadric();
            gluCylinder(coneQuadric, 0.2, 0.0, 0.5, 8, 2);  // Cone approximation
            gluDeleteQuadric(coneQuadric);
            glPopMatrix();
        }
        
        // Subwoofer (center, lower)
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, -1.0f);
        glScalef(0.4f, 0.4f, 0.4f);
        // Draw subwoofer cube
        glBegin(GL_QUADS);
        // Front, back, top, bottom, right, left faces
        float size = 0.5f;
        // Front
        glVertex3f(-size, -size,  size); glVertex3f( size, -size,  size);
        glVertex3f( size,  size,  size); glVertex3f(-size,  size,  size);
        // Back
        glVertex3f(-size, -size, -size); glVertex3f(-size,  size, -size);
        glVertex3f( size,  size, -size); glVertex3f( size, -size, -size);
        // Top
        glVertex3f(-size,  size, -size); glVertex3f(-size,  size,  size);
        glVertex3f( size,  size,  size); glVertex3f( size,  size, -size);
        // Bottom
        glVertex3f(-size, -size, -size); glVertex3f( size, -size, -size);
        glVertex3f( size, -size,  size); glVertex3f(-size, -size,  size);
        // Right
        glVertex3f( size, -size, -size); glVertex3f( size,  size, -size);
        glVertex3f( size,  size,  size); glVertex3f( size, -size,  size);
        // Left
        glVertex3f(-size, -size, -size); glVertex3f(-size, -size,  size);
        glVertex3f(-size,  size,  size); glVertex3f(-size,  size, -size);
        glEnd();
        glPopMatrix();
    }
}

void SpatialMixer3DView::DrawHRTFVisualization()
{
    // Draw HRTF processing indicator - head representation at listener position
    glPushMatrix();
    glTranslatef(fListenerPosition.x, fListenerPosition.y, fListenerPosition.z);
    
    // Draw head as slightly flattened sphere with HRTF active indication
    glColor4f(0.2f, 0.8f, 1.0f, 0.7f);  // Cyan color for HRTF active
    GLUquadric* headQuadric = gluNewQuadric();
    gluQuadricDrawStyle(headQuadric, GLU_FILL);
    
    // Head sphere (slightly flattened)
    glPushMatrix();
    glScalef(0.4f, 0.3f, 0.4f);
    gluSphere(headQuadric, 1.0, 16, 12);
    glPopMatrix();
    
    // Draw ears as small spheres to indicate HRTF processing points
    glColor4f(1.0f, 0.8f, 0.2f, 0.8f);  // Golden color for ears
    
    // Left ear
    glPushMatrix();
    glTranslatef(-0.45f, 0.0f, 0.0f);
    glScalef(0.1f, 0.1f, 0.1f);
    gluSphere(headQuadric, 1.0, 8, 6);
    glPopMatrix();
    
    // Right ear
    glPushMatrix();
    glTranslatef(0.45f, 0.0f, 0.0f);
    glScalef(0.1f, 0.1f, 0.1f);
    gluSphere(headQuadric, 1.0, 8, 6);
    glPopMatrix();
    
    gluDeleteQuadric(headQuadric);
    
    // Draw HRTF effect visualization - sound waves from sources
    glLineWidth(1.0f);
    glColor4f(0.2f, 1.0f, 0.8f, 0.3f);  // Translucent cyan
    
    // Draw lines from each track to each ear showing HRTF processing
    for (const auto& track : fSpatialTracks) {
        if (track.spatialEnabled) {
            glBegin(GL_LINES);
            
            // Line to left ear (with slight curve effect)
            glVertex3f(track.x, track.y, track.z);
            glVertex3f(fListenerPosition.x - 0.45f, fListenerPosition.y, fListenerPosition.z);
            
            // Line to right ear  
            glVertex3f(track.x, track.y, track.z);
            glVertex3f(fListenerPosition.x + 0.45f, fListenerPosition.y, fListenerPosition.z);
            
            glEnd();
        }
    }
    
    glPopMatrix();
    
    // Draw HRTF status text overlay (top-right corner)
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, Bounds().Width(), 0, Bounds().Height(), -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glColor3f(0.2f, 1.0f, 0.8f);
    glRasterPos2f(Bounds().Width() - 150, Bounds().Height() - 30);
    
    // Simple text rendering (placeholder - in real implementation would use proper text rendering)
    // BString hrtfText = "HRTF ACTIVE";
    // DrawGLText(hrtfText.String());  // Would need proper implementation
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void SpatialMixer3DView::DrawSpatialIndicators()
{
    // Draw coordinate system axes
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    
    // X axis (red)
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-5.0f, 0.0f, 0.0f);
    glVertex3f( 5.0f, 0.0f, 0.0f);
    
    // Y axis (green) 
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, -5.0f, 0.0f);
    glVertex3f(0.0f,  5.0f, 0.0f);
    
    // Z axis (blue)
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, -3.0f);
    glVertex3f(0.0f, 0.0f,  3.0f);
    
    glEnd();
    glLineWidth(1.0f);
}

// =====================================
// Parameter Updates and Thread Safety
// =====================================

void SpatialMixer3DView::UpdateSpatialTracks()
{
    // Update spatial tracks from current engine state
    if (!fEngine) return;
    
    // Resize spatial tracks vector if needed
    size_t trackCount = f3DTracks.size();  // From parent class
    
    if (fSpatialTracks.size() != trackCount) {
        fSpatialTracks.clear();
        for (size_t i = 0; i < trackCount; i++) {
            fSpatialTracks.emplace_back(f3DTracks[i].track);
            fSpatialTracks[i].Track3D::operator=(f3DTracks[i]);  // Copy base Track3D data
            
            // IMPORTANT: Set spatialPosition to match the 3D coordinates
            fSpatialTracks[i].spatialPosition.x = f3DTracks[i].x;
            fSpatialTracks[i].spatialPosition.y = f3DTracks[i].z;  // OpenGL Y->Z mapping
            fSpatialTracks[i].spatialPosition.z = f3DTracks[i].y;  // OpenGL Z->Y mapping
        }
    }
    
    // Update spatial parameters for each track
    for (size_t i = 0; i < fSpatialTracks.size(); i++) {
        UpdateTrackSpatialParameters(fSpatialTracks[i]);
    }
}

void SpatialMixer3DView::UpdateTrackSpatialParameters(SpatialTrack3D& track)
{
    // DON'T update from processor - it overwrites our unique positions!
    // Just keep the positions we set in UpdateTracks()
    
    // We can still calculate spherical coordinates if needed
    if (track.spatialEnabled) {
        CalculateSphericalCoordinates(track);
    }
}

void SpatialMixer3DView::CalculateSphericalCoordinates(SpatialTrack3D& track)
{
    // Calculate relative position from listener
    Vector3D relative = track.spatialPosition - fListenerPosition;
    
    // Convert to spherical coordinates
    track.distance = relative.Magnitude();
    
    if (track.distance > 0.001f) {
        track.azimuth = atan2f(relative.y, relative.x);
        track.elevation = asinf(relative.z / track.distance);
    } else {
        track.azimuth = 0.0f;
        track.elevation = 0.0f;
    }
}

void SpatialMixer3DView::ProcessParameterUpdates()
{
    std::lock_guard<std::mutex> lock(fParameterMutex);
    
    while (!fParameterQueue.empty()) {
        const SpatialParameterUpdate& update = fParameterQueue.front();
        
        // Apply parameter update to audio processor
        if (fAudioProcessor) {
            SurroundProcessor& processor = fAudioProcessor->GetSurroundProcessor();
            
            switch (update.type) {
                case SpatialParameterUpdate::POSITION:
                    if (update.trackIndex >= 0 && update.trackIndex < (int)fSpatialTracks.size()) {
                        processor.SetSourcePosition(update.vector);
                        fSpatialTracks[update.trackIndex].spatialPosition = update.vector;
                    }
                    break;
                    
                case SpatialParameterUpdate::LISTENER_POS:
                    processor.SetListenerPosition(update.vector);
                    fListenerPosition = update.vector;
                    break;
                    
                case SpatialParameterUpdate::LISTENER_ORIENT:
                    processor.SetListenerOrientation(update.vector, update.vector2);
                    fListenerForward = update.vector;
                    fListenerUp = update.vector2;
                    break;
                    
                case SpatialParameterUpdate::ROOM_SIZE:
                    processor.SetRoomSize(update.vector.x, update.vector.y, update.vector.z);
                    fRoomSize = update.vector;
                    break;
                    
                default:
                    break;
            }
        }
        
        fParameterQueue.pop();
    }
    
    fHasParameterUpdates.store(false);
}

void SpatialMixer3DView::QueueParameterUpdate(const SpatialParameterUpdate& update)
{
    std::lock_guard<std::mutex> lock(fParameterMutex);
    fParameterQueue.push(update);
    fHasParameterUpdates.store(true);
}

// =====================================
// Mouse Interaction for Spatial Positioning  
// =====================================

void SpatialMixer3DView::MouseDown(BPoint where)
{
    // Check if clicking on a spatial track for positioning
    SpatialTrack3D* track = GetSpatialTrackAt(where);
    
    if (track) {
        BeginTrackPositioning(track, where);
        return;
    }
    
    // Check if clicking near listener for repositioning
    BPoint listenerScreen;
    ProjectPoint(fListenerPosition.x, fListenerPosition.y, fListenerPosition.z, listenerScreen);
    
    float listenerDist = sqrt(pow(where.x - listenerScreen.x, 2) + pow(where.y - listenerScreen.y, 2));
    if (listenerDist < 30.0f) {  // 30 pixel tolerance
        fDraggingListener = true;
        fDragStartPoint = where;
        return;
    }
    
    // Fall back to parent class behavior (camera control)
    Mixer3DView::MouseDown(where);
}

void SpatialMixer3DView::MouseUp(BPoint where)
{
    if (fDraggingTrack) {
        EndTrackPositioning();
        return;
    }
    
    if (fDraggingListener) {
        fDraggingListener = false;
        Invalidate();
        return;
    }
    
    Mixer3DView::MouseUp(where);
}

void SpatialMixer3DView::MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
{
    if (fDraggingTrack && (code & B_PRIMARY_MOUSE_BUTTON)) {
        UpdateTrackPositioning(where);
        return;
    }
    
    if (fDraggingListener && (code & B_PRIMARY_MOUSE_BUTTON)) {
        // Move listener based on mouse movement
        BPoint delta = where - fDragStartPoint;
        
        // Convert screen movement to world coordinates (simplified)
        Vector3D newPos = fListenerPosition;
        newPos.x += delta.x * 0.02f;  // Scale factor for reasonable movement
        newPos.y += delta.y * 0.02f;
        
        QueueParameterUpdate(SpatialParameterUpdate::ListenerPosition(newPos));
        fDragStartPoint = where;
        Invalidate();
        return;
    }
    
    Mixer3DView::MouseMoved(where, code, dragMessage);
}

void SpatialMixer3DView::KeyDown(const char* bytes, int32 numBytes)
{
    if (numBytes > 0) {
        switch (bytes[0]) {
            case '+':
            case '=':
                // Zoom in (handled by parent)
                Mixer3DView::KeyDown(bytes, numBytes);
                break;
                
            case '-':
            case '_':
                // Zoom out (handled by parent)
                Mixer3DView::KeyDown(bytes, numBytes);
                break;
                
            case 'r':
            case 'R':
                // Reset camera AND listener to origin
                ResetCamera();  // Reset camera view
                QueueParameterUpdate(SpatialParameterUpdate::ListenerPosition(Vector3D(0.0f, 0.0f, 0.0f)));
                Invalidate();
                break;
                
            case 's':
            case 'S':
                // Toggle spatial indicators
                fShowSpatialIndicators = !fShowSpatialIndicators;
                Invalidate();
                break;
                
            case 'l':
            case 'L':
                // Toggle listener visualization
                fShowListener = !fShowListener;
                Invalidate();
                break;
                
            default:
                Mixer3DView::KeyDown(bytes, numBytes);
                break;
        }
    }
}

SpatialTrack3D* SpatialMixer3DView::GetSpatialTrackAt(BPoint point)
{
    // Project all track positions to screen and find closest
    float minDistance = 50.0f;  // Maximum click tolerance in pixels
    SpatialTrack3D* closestTrack = nullptr;
    
    for (auto& track : fSpatialTracks) {
        BPoint screenPos;
        ProjectPoint(track.x, track.y, track.z, screenPos);
        
        float distance = sqrt(pow(point.x - screenPos.x, 2) + pow(point.y - screenPos.y, 2));
        if (distance < minDistance) {
            minDistance = distance;
            closestTrack = &track;
        }
    }
    
    return closestTrack;
}

void SpatialMixer3DView::BeginTrackPositioning(SpatialTrack3D* track, BPoint startPoint)
{
    fDraggingTrack = track;
    fDragStartPoint = startPoint;
    fDragStartPosition = track->spatialPosition;
    track->selected = true;
    track->showParameters = true;
    
    printf("SpatialMixer3DView: Started positioning track at (%.2f, %.2f, %.2f)\n", 
           track->spatialPosition.x, track->spatialPosition.y, track->spatialPosition.z);
           
    Invalidate();
}

void SpatialMixer3DView::UpdateTrackPositioning(BPoint currentPoint)
{
    if (!fDraggingTrack) return;
    
    // Calculate movement delta
    BPoint delta = currentPoint - fDragStartPoint;
    
    // Convert screen movement to 3D world coordinates
    // This is simplified - in a full implementation you'd use proper screen-to-world projection
    Vector3D newPosition = fDragStartPosition;
    newPosition.x += delta.x * 0.02f;  // Scale factors for reasonable movement speed
    newPosition.y += delta.y * 0.02f;
    
    // Constrain to room boundaries
    float halfWidth = fRoomSize.x * 0.5f;
    float halfHeight = fRoomSize.y * 0.5f;
    float halfDepth = fRoomSize.z * 0.5f;
    
    newPosition.x = std::max(-halfWidth, std::min(halfWidth, newPosition.x));
    newPosition.y = std::max(-halfHeight, std::min(halfHeight, newPosition.y));
    newPosition.z = std::max(-halfDepth, std::min(halfDepth, newPosition.z));
    
    // Update track position
    fDraggingTrack->spatialPosition = newPosition;
    fDraggingTrack->x = newPosition.x;
    fDraggingTrack->y = newPosition.z;  // Swap Y/Z for OpenGL coordinates
    fDraggingTrack->z = newPosition.y;
    
    // Queue parameter update for audio thread
    int trackIndex = fDraggingTrack - &fSpatialTracks[0];  // Get index
    QueueParameterUpdate(SpatialParameterUpdate::Position(trackIndex, newPosition));
    
    Invalidate();
}

void SpatialMixer3DView::EndTrackPositioning()
{
    if (!fDraggingTrack) return;
    
    fDraggingTrack->selected = false;
    fDraggingTrack->showParameters = false;
    
    printf("SpatialMixer3DView: Finished positioning track at (%.2f, %.2f, %.2f)\n",
           fDraggingTrack->spatialPosition.x, fDraggingTrack->spatialPosition.y, fDraggingTrack->spatialPosition.z);
    
    fDraggingTrack = nullptr;
    Invalidate();
}

// =====================================
// Public Interface Methods
// =====================================

void SpatialMixer3DView::SetSpatialMode(SurroundProcessor::SpatialMode mode)
{
    fCurrentSpatialMode = mode;
    if (fAudioProcessor) {
        fAudioProcessor->GetSurroundProcessor().SetSpatialMode(mode);
    }
    
    // Update visualization based on mode
    fShowSpeakerLayout = (mode == SurroundProcessor::SpatialMode::BASIC_SURROUND);
    
    Invalidate();
}

void SpatialMixer3DView::SetListenerPosition(const Vector3D& position)
{
    QueueParameterUpdate(SpatialParameterUpdate::ListenerPosition(position));
    Invalidate();
}

void SpatialMixer3DView::SetListenerOrientation(const Vector3D& forward, const Vector3D& up)
{
    QueueParameterUpdate(SpatialParameterUpdate::ListenerOrientation(forward, up));
    Invalidate();
}

void SpatialMixer3DView::SetTrackSpatialPosition(int trackIndex, const Vector3D& position)
{
    if (trackIndex >= 0 && trackIndex < (int)fSpatialTracks.size()) {
        QueueParameterUpdate(SpatialParameterUpdate::Position(trackIndex, position));
        Invalidate();
    }
}

Vector3D SpatialMixer3DView::GetTrackSpatialPosition(int trackIndex) const
{
    if (trackIndex >= 0 && trackIndex < (int)fSpatialTracks.size()) {
        return fSpatialTracks[trackIndex].spatialPosition;
    }
    return Vector3D(0.0f, 0.0f, 0.0f);
}

} // namespace HaikuDAW
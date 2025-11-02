/*
 * Mixer3DWindow.cpp - Unified 3D mixer implementation
 * BeOS 3dmix heritage with optional spatial audio mode
 */

#include "Mixer3DWindow.h"
#include "SpatialMixer3DWindow.h"  // For spatial mode
#include "VeniceTheme.h"
#include "../audio/SimpleHaikuEngine.h"
#include "../audio/AdvancedAudioProcessor.h"
#include "../benchmark/PerformanceStation.h"  // Per i RAII guards
#include <Application.h>
#include <Alert.h>
#include <Autolock.h>
#include <stdio.h>
#include <stdlib.h>

namespace HaikuDAW {

// =====================================
// Mixer3DView Implementation
// =====================================

Mixer3DView::Mixer3DView(BRect frame, SimpleHaikuEngine* engine)
    : BGLView(frame, "3d_mixer", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, BGL_RGB | BGL_DEPTH | BGL_DOUBLE)
    , fEngine(engine)
    , fCameraAngleX(30.0f)
    , fCameraAngleY(45.0f)
    , fCameraDistance(20.0f)  // Increased to see all spheres initially
    , fMouseDown(false)
    , fSelectedTrack(-1)
    , fDraggingTrack(false)  // NEW: Initialize drag state
    , fAnimationTime(0.0f)
    , fGLLocker("3D Mixer GL Lock")
    , fCameraDirty(true)  // Initial cache computation needed
    , fLastUpdateTime(system_time())
    , fGridDisplayList(0)  // OpenGL optimization: will be created in InitGL()
{
    fCameraTarget[0] = 0.0f;
    fCameraTarget[1] = 0.0f;
    fCameraTarget[2] = 0.0f;

    // Particle system starts disabled (can be enabled via menu)
    fParticleSystem.SetEnabled(false);

    printf("Mixer3DView: Created 3D OpenGL view with thread-safe locking\n");
    printf("Mixer3DView: Particle system initialized (disabled by default)\n");
}

Mixer3DView::~Mixer3DView()
{
    printf("Mixer3DView: Starting destruction...\n");

    // Lock to ensure no concurrent OpenGL operations
    BAutolock locker(fGLLocker);

    // Properly cleanup OpenGL context
    if (Window()) {
        // Disable rendering updates first
        Window()->DisableUpdates();

        // Use RAII guard for safe OpenGL cleanup
        WindowLockGuard windowGuard(Window());
        if (windowGuard) {
            GLContextGuard glGuard(this);
            if (glGuard) {
                // Delete display lists for static geometry
                if (fGridDisplayList != 0) {
                    glDeleteLists(fGridDisplayList, 1);
                    fGridDisplayList = 0;
                    printf("Mixer3DView: Grid display list deleted\n");
                }

                // Ensure all GL commands are completed
                glFinish();

                // Unbind any active textures
                glBindTexture(GL_TEXTURE_2D, 0);

                // Clear errors if any
                while (glGetError() != GL_NO_ERROR) {
                    // Clear error queue
                }
            }
        }

        // Re-enable updates before window cleanup
        Window()->EnableUpdates();
    }

    // BAutolock ensures synchronization without sleep
    printf("Mixer3DView: Destroyed safely with proper synchronization\n");
}

void Mixer3DView::AttachedToWindow()
{
    BGLView::AttachedToWindow();

    // Make the view focusable to receive keyboard events
    MakeFocus(true);

    LockGL();
    InitGL();
    UpdateTracks();

    // **CRITICAL**: Initialize camera cache to avoid NaN coordinates
    UpdateCameraCache();
    printf("Mixer3DView: Camera cache initialized (%.1f, %.1f, %.1f)\n",
           fCachedCameraX, fCachedCameraY, fCachedCameraZ);

    UnlockGL();

    printf("Mixer3DView: OpenGL initialized and tracks loaded\n");
}

void Mixer3DView::DetachedFromWindow()
{
    // Clean up OpenGL context before detaching using RAII
    {
        GLContextGuard glGuard(this);
        if (glGuard) {
            // Cleanup OpenGL resources
            glFinish();  // Ensure all GL commands are completed
            glFlush();   // Flush command buffer
        }
    }
    
    BGLView::DetachedFromWindow();
    printf("Mixer3DView: Detached from window\n");
}

void Mixer3DView::InitGL()
{
    printf("Mixer3DView: Initializing OpenGL (3dmix modern style)...\n");

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Enable blending for transparency and glow effects
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Enable lighting with modern multi-light setup
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);  // Main key light
    glEnable(GL_LIGHT1);  // Fill light

    // Key light (from upper front-right) - bright BeOS-style
    GLfloat light0Pos[] = {8.0f, 10.0f, 8.0f, 1.0f};
    GLfloat light0Ambient[] = {0.2f, 0.2f, 0.25f, 1.0f};
    GLfloat light0Diffuse[] = {0.9f, 0.9f, 0.95f, 1.0f};
    GLfloat light0Specular[] = {1.0f, 1.0f, 1.0f, 1.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light0Ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0Diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0Specular);

    // Fill light (from back-left) - subtle blue tint for depth
    GLfloat light1Pos[] = {-5.0f, 6.0f, -5.0f, 1.0f};
    GLfloat light1Ambient[] = {0.1f, 0.1f, 0.15f, 1.0f};
    GLfloat light1Diffuse[] = {0.3f, 0.35f, 0.5f, 1.0f};

    glLightfv(GL_LIGHT1, GL_POSITION, light1Pos);
    glLightfv(GL_LIGHT1, GL_AMBIENT, light1Ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1Diffuse);

    // Material properties - enable color and specular highlights
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Smooth shading for spheres
    glShadeModel(GL_SMOOTH);

    // Enable anti-aliasing hints
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

    // Background: Deep blue-grey gradient (3dmix heritage)
    glClearColor(0.08f, 0.1f, 0.15f, 1.0f);

    printf("Mixer3DView: 3dmix-inspired OpenGL setup complete\n");
}

void Mixer3DView::FrameResized(float width, float height)
{
    BGLView::FrameResized(width, height);
    
    LockGL();
    
    glViewport(0, 0, (GLint)width, (GLint)height);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, width/height, 1.0, 100.0);
    
    glMatrixMode(GL_MODELVIEW);
    
    UnlockGL();
}

void Mixer3DView::UpdateTracks()
{
    // Lock for thread-safe track updates
    BAutolock locker(fGLLocker);

    f3DTracks.clear();

    if (!fEngine) return;

    // Get tracks from engine and create 3D representations
    int trackCount = fEngine->GetTrackCount();
    
    // Position tracks in a circle using REAL tracks from engine
    for (int i = 0; i < trackCount; i++) {
        // Get the actual track from the engine (not a dummy!)
        SimpleTrack* realTrack = fEngine->GetTrack(i);
        if (!realTrack) continue;
        
        Track3D track3D(realTrack);
        
        // Position in circle with larger radius for visibility
        float angle = (i / (float)trackCount) * 2.0f * M_PI;
        track3D.x = cos(angle) * 8.0f;  // Increased radius
        track3D.z = sin(angle) * 8.0f;
        track3D.y = 0.0f;
        
        // Vary scale slightly for visual distinction
        track3D.scale = 0.8f + (i * 0.1f);
        
        f3DTracks.push_back(track3D);
    }
}

void Mixer3DView::Draw(BRect updateRect)
{
    LockGL();
    RenderScene();
    SwapBuffers();
    UnlockGL();
}

void Mixer3DView::RenderScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Update camera cache if needed (avoids expensive trig calculations every frame)
    if (fCameraDirty) {
        UpdateCameraCache();
    }

    // Set up camera using cached values
    gluLookAt(
        fCachedCameraX, fCachedCameraY, fCachedCameraZ,
        fCameraTarget[0], fCameraTarget[1], fCameraTarget[2],
        0.0f, 1.0f, 0.0f
    );

    // **FIX**: Update light positions AFTER camera setup so they move with the view
    // This makes the lighting follow the camera rotation for consistent illumination
    GLfloat light0Pos[] = {8.0f, 10.0f, 8.0f, 1.0f};    // Upper front-right
    GLfloat light1Pos[] = {-5.0f, 6.0f, -5.0f, 1.0f};   // Back-left fill
    glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);
    glLightfv(GL_LIGHT1, GL_POSITION, light1Pos);

    // Draw grid
    DrawGrid();

    // Animate and draw tracks
    AnimateScene();
    int trackIndex = 0;
    int culledTracks = 0;  // Performance metric
    for (const Track3D& track : f3DTracks) {
        // Frustum culling optimization: Skip tracks outside view frustum
        // Sphere radius = track scale (0.6-1.0) + animation float (0.15) + glow margin (0.5)
        float trackRadius = 1.0f + track.scale + 0.65f;  // Conservative estimate

        if (!IsTrackVisible(track.x, track.y, track.z, trackRadius)) {
            culledTracks++;
            trackIndex++;
            continue;  // Skip rendering, track is outside frustum
        }

        DrawTrack3D(track);
        // Draw track number next to sphere (1-based for user display)
        DrawTrackNumber(trackIndex + 1, track.x, track.y, track.z);
        trackIndex++;
    }

    // Performance debug info (only print occasionally to avoid spam)
    static int frameCount = 0;
    if (culledTracks > 0 && (frameCount++ % 120 == 0)) {
        printf("[Frustum Culling] Skipped %d/%d tracks this frame (%.1f%% saved)\n",
               culledTracks, (int)f3DTracks.size(),
               (culledTracks * 100.0f) / f3DTracks.size());
    }

    // Update and render particle system
    bigtime_t currentTime = system_time();
    float deltaTime = (currentTime - fLastUpdateTime) / 1000000.0f;  // Convert to seconds
    fLastUpdateTime = currentTime;

    fParticleSystem.Update(deltaTime);
    fParticleSystem.Render();

    fAnimationTime += 0.02f;  // Animation speed
}

void Mixer3DView::DrawGrid()
{
    // OpenGL Optimization: Use display list for static grid geometry
    // This pre-compiles the grid rendering commands for reuse each frame
    if (fGridDisplayList == 0) {
        // First time: Create display list and compile grid geometry
        fGridDisplayList = glGenLists(1);
        if (fGridDisplayList == 0) {
            printf("[OpenGL] Warning: Failed to create grid display list, falling back to immediate mode\n");
            // Fall through to direct rendering below
        } else {
            glNewList(fGridDisplayList, GL_COMPILE_AND_EXECUTE);

            glDisable(GL_LIGHTING);

            // Draw subtle "glass floor" plane with gradient (3dmix heritage)
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glBegin(GL_QUADS);
            // Center brighter (BeOS yellow tint), edges darker
            glColor4f(0.15f, 0.15f, 0.2f, 0.3f);  // Edge color (darker blue)
            glVertex3f(-10, -0.01f, -10);
            glVertex3f(10, -0.01f, -10);

            glColor4f(0.2f, 0.22f, 0.28f, 0.5f);  // Center (lighter)
            glVertex3f(10, -0.01f, 10);
            glVertex3f(-10, -0.01f, 10);
            glEnd();

            // Draw elegant grid lines (3dmix style - sparse and clean)
            glLineWidth(1.0f);
            glBegin(GL_LINES);

            // Outer grid - faint lines
            for (int i = -10; i <= 10; i += 2) {
                // Distance-based alpha for depth effect
                float alpha = 1.0f - (abs(i) / 12.0f);
                glColor4f(0.25f, 0.28f, 0.35f, alpha * 0.4f);

                glVertex3f(i, 0, -10); glVertex3f(i, 0, 10);  // X lines
                glVertex3f(-10, 0, i); glVertex3f(10, 0, i);  // Z lines
            }
            glEnd();

            // Center cross - brighter (BeOS signature yellow-orange)
            glLineWidth(2.0f);
            glBegin(GL_LINES);
            glColor4f(0.9f, 0.7f, 0.3f, 0.8f);  // BeOS yellow
            glVertex3f(-10, 0, 0); glVertex3f(10, 0, 0);   // X axis
            glVertex3f(0, 0, -10); glVertex3f(0, 0, 10);   // Z axis
            glEnd();

            // Axis indicators (subtle 3dmix style)
            glLineWidth(3.0f);
            glBegin(GL_LINES);
            // X axis - warm orange-red (BeOS heritage)
            glColor4f(1.0f, 0.5f, 0.2f, 0.9f);
            glVertex3f(0, 0, 0); glVertex3f(3, 0, 0);

            // Y axis - bright yellow-green
            glColor4f(0.7f, 1.0f, 0.3f, 0.9f);
            glVertex3f(0, 0, 0); glVertex3f(0, 3, 0);

            // Z axis - cool blue-cyan
            glColor4f(0.2f, 0.6f, 1.0f, 0.9f);
            glVertex3f(0, 0, 0); glVertex3f(0, 0, 3);
            glEnd();

            glLineWidth(1.0f);  // Reset
            glEnable(GL_LIGHTING);

            glEndList();
            printf("[OpenGL] Grid display list created (ID: %u) - static geometry cached\n", fGridDisplayList);
            return;  // Grid already drawn by GL_COMPILE_AND_EXECUTE
        }
    }

    // Fast path: Reuse compiled display list (huge performance gain!)
    if (fGridDisplayList != 0) {
        glCallList(fGridDisplayList);
        return;
    }

    // Fallback path: Direct rendering (only if display list creation failed)
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBegin(GL_QUADS);
    glColor4f(0.15f, 0.15f, 0.2f, 0.3f);
    glVertex3f(-10, -0.01f, -10);
    glVertex3f(10, -0.01f, -10);
    glColor4f(0.2f, 0.22f, 0.28f, 0.5f);
    glVertex3f(10, -0.01f, 10);
    glVertex3f(-10, -0.01f, 10);
    glEnd();

    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int i = -10; i <= 10; i += 2) {
        float alpha = 1.0f - (abs(i) / 12.0f);
        glColor4f(0.25f, 0.28f, 0.35f, alpha * 0.4f);
        glVertex3f(i, 0, -10); glVertex3f(i, 0, 10);
        glVertex3f(-10, 0, i); glVertex3f(10, 0, i);
    }
    glEnd();

    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor4f(0.9f, 0.7f, 0.3f, 0.8f);
    glVertex3f(-10, 0, 0); glVertex3f(10, 0, 0);
    glVertex3f(0, 0, -10); glVertex3f(0, 0, 10);
    glEnd();

    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glColor4f(1.0f, 0.5f, 0.2f, 0.9f);
    glVertex3f(0, 0, 0); glVertex3f(3, 0, 0);
    glColor4f(0.7f, 1.0f, 0.3f, 0.9f);
    glVertex3f(0, 0, 0); glVertex3f(0, 3, 0);
    glColor4f(0.2f, 0.6f, 1.0f, 0.9f);
    glVertex3f(0, 0, 0); glVertex3f(0, 0, 3);
    glEnd();

    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}

// Frustum culling optimization: Test if track sphere is visible
bool Mixer3DView::IsTrackVisible(float x, float y, float z, float radius) const
{
    // Calculate vector from camera to track
    float dx = x - fCachedCameraX;
    float dy = y - fCachedCameraY;
    float dz = z - fCachedCameraZ;
    float distanceSquared = dx*dx + dy*dy + dz*dz;

    // Far plane culling: Skip objects too far from camera
    // Maximum distance = camera distance + generous margin
    float maxDistance = fCameraDistance * 2.5f;
    if (distanceSquared > maxDistance * maxDistance) {
        return false;  // Too far, not visible
    }

    // Near plane culling: Skip objects too close
    float minDistance = 0.5f;
    if (distanceSquared < minDistance * minDistance) {
        return false;  // Too close, not visible
    }

    // Simplified frustum test: Check if track is roughly within view cone
    // FOV is approximately 45 degrees, use cosine for angle test
    float distance = sqrt(distanceSquared);

    // Direction from camera to track (normalized)
    float dirX = dx / distance;
    float dirY = dy / distance;
    float dirZ = dz / distance;

    // Camera forward direction (from gluLookAt setup)
    float camDirX = fCameraTarget[0] - fCachedCameraX;
    float camDirY = fCameraTarget[1] - fCachedCameraY;
    float camDirZ = fCameraTarget[2] - fCachedCameraZ;
    float camDirLength = sqrt(camDirX*camDirX + camDirY*camDirY + camDirZ*camDirZ);

    if (camDirLength < 0.001f) {
        return true;  // Degenerate case, render to be safe
    }

    camDirX /= camDirLength;
    camDirY /= camDirLength;
    camDirZ /= camDirLength;

    // Dot product to find angle between camera direction and track direction
    float dotProduct = dirX * camDirX + dirY * camDirY + dirZ * camDirZ;

    // cos(70°) ≈ 0.34 - generous frustum cone (wider than actual 45° FOV)
    // This ensures we don't accidentally cull visible objects at screen edges
    float minCosAngle = 0.25f;  // Very generous cone

    // Account for sphere radius in frustum test
    float sinHalfAngle = radius / distance;
    float adjustedMinCosAngle = minCosAngle - sinHalfAngle;

    if (dotProduct < adjustedMinCosAngle) {
        return false;  // Outside frustum cone
    }

    return true;  // Track is visible, render it
}

void Mixer3DView::DrawTrackNumber(int trackNumber, float x, float y, float z)
{
    // Draw track number next to sphere (billboard style - always faces camera)
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);  // Draw on top for maximum visibility

    glPushMatrix();

    // Position next to sphere - ABOVE it for better visibility
    glTranslatef(x, y + 2.0f, z);  // 2 units above sphere center

    // **PERFECT BILLBOARD**: Calculate vector from number to camera
    // This ensures the number ALWAYS faces the camera exactly
    float dx = fCachedCameraX - x;
    float dy = fCachedCameraY - (y + 2.0f);
    float dz = fCachedCameraZ - z;

    // Calculate angle to rotate around Y axis (horizontal rotation)
    float angleY = atan2(dx, dz) * 180.0f / M_PI;

    // Calculate angle to rotate around X axis (vertical tilt)
    float distXZ = sqrt(dx*dx + dz*dz);
    float angleX = atan2(dy, distXZ) * 180.0f / M_PI;

    // Apply rotations: first Y (horizontal), then X (vertical tilt)
    glRotatef(angleY, 0, 1, 0);   // Face camera horizontally
    glRotatef(-angleX, 1, 0, 0);  // Tilt to face camera vertically

    // MUCH larger background for visibility
    float bgSize = 1.2f;  // Increased from 0.6f

    // Draw semi-transparent dark background for visibility
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.05f, 0.05f, 0.1f, 0.85f);  // Darker, more opaque

    glBegin(GL_QUADS);
    glVertex3f(-bgSize, -bgSize, 0);
    glVertex3f(bgSize, -bgSize, 0);
    glVertex3f(bgSize, bgSize, 0);
    glVertex3f(-bgSize, bgSize, 0);
    glEnd();

    // Draw track number in bright BeOS yellow
    glColor4f(1.0f, 0.9f, 0.3f, 1.0f);  // Brighter yellow
    glLineWidth(5.0f);  // MUCH thicker lines (was 3.0f)

    float scale = 0.8f;  // DOUBLED from 0.4f for visibility
    glScalef(scale, scale, scale);

    // Draw digits using GL_LINES (stroke font style)
    // Custom simple digit patterns for numbers 1-8
    glBegin(GL_LINES);

    switch (trackNumber) {
        case 1:  // "1" - vertical line with cap
            glVertex3f(0, -1, 0); glVertex3f(0, 1, 0);
            glVertex3f(-0.3f, 0.7f, 0); glVertex3f(0, 1, 0);
            break;

        case 2:  // "2" - S curve with base
            glVertex3f(-0.5f, 0.7f, 0); glVertex3f(0.5f, 0.7f, 0);
            glVertex3f(0.5f, 0.7f, 0); glVertex3f(0.5f, 0, 0);
            glVertex3f(0.5f, 0, 0); glVertex3f(-0.5f, 0, 0);
            glVertex3f(-0.5f, 0, 0); glVertex3f(-0.5f, -0.7f, 0);
            glVertex3f(-0.5f, -0.7f, 0); glVertex3f(0.5f, -0.7f, 0);
            break;

        case 3:  // "3" - Two humps
            glVertex3f(-0.5f, 0.7f, 0); glVertex3f(0.5f, 0.7f, 0);
            glVertex3f(0.5f, 0.7f, 0); glVertex3f(0.5f, 0, 0);
            glVertex3f(0.5f, 0, 0); glVertex3f(-0.3f, 0, 0);
            glVertex3f(0.5f, 0, 0); glVertex3f(0.5f, -0.7f, 0);
            glVertex3f(0.5f, -0.7f, 0); glVertex3f(-0.5f, -0.7f, 0);
            break;

        case 4:  // "4" - Open box with vertical
            glVertex3f(-0.5f, 0.7f, 0); glVertex3f(-0.5f, 0, 0);
            glVertex3f(-0.5f, 0, 0); glVertex3f(0.5f, 0, 0);
            glVertex3f(0.5f, 0.7f, 0); glVertex3f(0.5f, -0.7f, 0);
            break;

        case 5:  // "5" - Reverse S
            glVertex3f(0.5f, 0.7f, 0); glVertex3f(-0.5f, 0.7f, 0);
            glVertex3f(-0.5f, 0.7f, 0); glVertex3f(-0.5f, 0, 0);
            glVertex3f(-0.5f, 0, 0); glVertex3f(0.5f, 0, 0);
            glVertex3f(0.5f, 0, 0); glVertex3f(0.5f, -0.7f, 0);
            glVertex3f(0.5f, -0.7f, 0); glVertex3f(-0.5f, -0.7f, 0);
            break;

        case 6:  // "6" - Lower loop
            glVertex3f(0.5f, 0.7f, 0); glVertex3f(-0.5f, 0.7f, 0);
            glVertex3f(-0.5f, 0.7f, 0); glVertex3f(-0.5f, -0.7f, 0);
            glVertex3f(-0.5f, -0.7f, 0); glVertex3f(0.5f, -0.7f, 0);
            glVertex3f(0.5f, -0.7f, 0); glVertex3f(0.5f, 0, 0);
            glVertex3f(0.5f, 0, 0); glVertex3f(-0.5f, 0, 0);
            break;

        case 7:  // "7" - Top with diagonal
            glVertex3f(-0.5f, 0.7f, 0); glVertex3f(0.5f, 0.7f, 0);
            glVertex3f(0.5f, 0.7f, 0); glVertex3f(0, -0.7f, 0);
            break;

        case 8:  // "8" - Two boxes
            glVertex3f(-0.5f, 0.7f, 0); glVertex3f(0.5f, 0.7f, 0);
            glVertex3f(0.5f, 0.7f, 0); glVertex3f(0.5f, -0.7f, 0);
            glVertex3f(0.5f, -0.7f, 0); glVertex3f(-0.5f, -0.7f, 0);
            glVertex3f(-0.5f, -0.7f, 0); glVertex3f(-0.5f, 0.7f, 0);
            glVertex3f(-0.5f, 0, 0); glVertex3f(0.5f, 0, 0);
            break;
    }

    glEnd();
    glLineWidth(1.0f);
    glPopMatrix();

    glEnable(GL_DEPTH_TEST);  // Re-enable depth test
    glEnable(GL_LIGHTING);
}

void Mixer3DView::DrawTrack3D(const Track3D& track)
{
    glPushMatrix();

    // Position
    glTranslatef(track.x, track.y, track.z);

    // Gentle floating animation (3dmix heritage)
    float floatOffset = sin(fAnimationTime * 1.5f + track.x) * 0.15f;
    glTranslatef(0, floatOffset, 0);

    // Scale based on volume
    float scale = 0.6f + track.scale * 0.4f;

    // === ADAPTIVE LOD SYSTEM: Calculate distance from camera ===
    // Reduces geometry complexity for distant tracks to improve performance
    float dx = track.x - fCachedCameraX;
    float dy = track.y - fCachedCameraY;
    float dz = track.z - fCachedCameraZ;
    float distanceFromCamera = sqrt(dx*dx + dy*dy + dz*dz);

    // Determine LOD level based on distance
    // LOD 0 (Close): < 15 units - Full quality (32x32 sphere, all effects)
    // LOD 1 (Medium): 15-25 units - Medium quality (20x20 sphere, reduced effects)
    // LOD 2 (Far): > 25 units - Low quality (12x12 sphere, minimal effects)
    int lodLevel = 0;
    int sphereSlices = 32;  // Default high quality
    int sphereStacks = 32;
    bool enableGlow = true;
    bool enableDetails = true;

    if (distanceFromCamera > 25.0f) {
        lodLevel = 2;  // Far
        sphereSlices = 12;
        sphereStacks = 12;
        enableGlow = false;
        enableDetails = false;
    } else if (distanceFromCamera > 15.0f) {
        lodLevel = 1;  // Medium
        sphereSlices = 20;
        sphereStacks = 20;
        enableGlow = false;  // Skip glow for medium distance
        enableDetails = true;
    }

    // Determine track state and colors
    bool isMuted = (track.track && track.track->IsMuted());
    float baseColor[3];
    float glowIntensity = 0.0f;

    if (isMuted) {
        // Muted: dark desaturated sphere
        baseColor[0] = 0.25f;
        baseColor[1] = 0.25f;
        baseColor[2] = 0.3f;
        glowIntensity = 0.0f;
    } else {
        // Active: vibrant BeOS-inspired colors with glow
        baseColor[0] = track.color[0];
        baseColor[1] = track.color[1];
        baseColor[2] = track.color[2];
        glowIntensity = track.levelHeight * 0.7f;  // Glow based on audio level
    }

    // === SELECTION RING (if track is selected) ===
    if (track.selected) {
        glDisable(GL_LIGHTING);
        glLineWidth(4.0f);
        glColor4f(1.0f, 1.0f, 0.0f, 1.0f);  // Bright yellow selection ring

        glPushMatrix();
        glScalef(scale * 1.3f, scale * 1.3f, scale * 1.3f);
        glRotatef(90, 1, 0, 0);

        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 48; i++) {
            float angle = (i / 48.0f) * 2.0f * M_PI;
            glVertex3f(cos(angle), sin(angle), 0);
        }
        glEnd();

        glPopMatrix();
        glLineWidth(1.0f);
        glEnable(GL_LIGHTING);
    }

    // === OUTER GLOW HALO (3dmix signature effect) ===
    // LOD Optimization: Skip glow for medium/far distances to save GPU
    if (!isMuted && glowIntensity > 0.05f && enableGlow) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive blending for glow

        // Draw multiple expanding halos for bloom effect
        for (int halo = 3; halo >= 1; halo--) {
            float haloScale = scale * (1.0f + halo * 0.15f);
            float haloAlpha = glowIntensity * (0.2f / halo);

            glPushMatrix();
            glScalef(haloScale, haloScale, haloScale);

            glColor4f(baseColor[0], baseColor[1], baseColor[2], haloAlpha);

            // Draw glow sphere (low detail for performance)
            GLUquadric* glowQuad = gluNewQuadric();
            gluQuadricDrawStyle(glowQuad, GLU_FILL);
            gluQuadricNormals(glowQuad, GLU_SMOOTH);
            gluSphere(glowQuad, 1.0, 12, 12);
            gluDeleteQuadric(glowQuad);

            glPopMatrix();
        }

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Reset blend
        glEnable(GL_LIGHTING);
    }

    // === MAIN SPHERE (3dmix iconic look) ===
    glPushMatrix();
    glScalef(scale, scale, scale);

    // Material properties for glossy sphere
    GLfloat matAmbient[] = {baseColor[0] * 0.3f, baseColor[1] * 0.3f, baseColor[2] * 0.3f, 1.0f};
    GLfloat matDiffuse[] = {baseColor[0], baseColor[1], baseColor[2], 1.0f};
    GLfloat matSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat matShininess[] = {80.0f};  // High shininess for glass-like look

    glMaterialfv(GL_FRONT, GL_AMBIENT, matAmbient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
    glMaterialfv(GL_FRONT, GL_SHININESS, matShininess);

    // Draw main sphere with LOD-based tessellation
    // LOD 0 (close): 32x32 triangles (high detail)
    // LOD 1 (medium): 20x20 triangles (medium detail)
    // LOD 2 (far): 12x12 triangles (low detail)
    GLUquadric* quadric = gluNewQuadric();
    gluQuadricDrawStyle(quadric, GLU_FILL);
    gluQuadricNormals(quadric, GLU_SMOOTH);
    gluQuadricTexture(quadric, GL_FALSE);
    gluSphere(quadric, 1.0, sphereSlices, sphereStacks);  // Adaptive LOD tessellation
    gluDeleteQuadric(quadric);

    glPopMatrix();

    // === EQUATOR RING (visual detail) ===
    // LOD Optimization: Skip equator ring for far distances
    if (!isMuted && enableDetails) {
        glDisable(GL_LIGHTING);
        glLineWidth(2.0f);
        glColor4f(baseColor[0] * 1.2f, baseColor[1] * 1.2f, baseColor[2] * 1.2f, 0.6f);

        glPushMatrix();
        glScalef(scale * 1.05f, scale * 1.05f, scale * 1.05f);
        glRotatef(90, 1, 0, 0);  // Horizontal ring

        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 32; i++) {
            float angle = (i / 32.0f) * 2.0f * M_PI;
            glVertex3f(cos(angle), sin(angle), 0);
        }
        glEnd();

        glPopMatrix();
        glLineWidth(1.0f);
        glEnable(GL_LIGHTING);
    }

    // === VERTICAL LEVEL INDICATOR (3dmix style) ===
    // LOD Optimization: Skip level indicator for far distances
    if (!isMuted && track.levelHeight > 0.1f && enableDetails) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);

        float levelHeight = track.levelHeight * 2.0f;
        float levelWidth = 0.12f;

        // Vertical bar above sphere
        glPushMatrix();
        glTranslatef(0, scale + levelHeight * 0.5f, 0);

        // Gradient: green to yellow to red based on level
        float r = (levelHeight > 1.0f) ? 1.0f : levelHeight;
        float g = (levelHeight < 1.0f) ? 1.0f : (2.0f - levelHeight);
        float b = 0.2f;

        glBegin(GL_QUADS);
        glColor4f(r * 0.7f, g * 0.7f, b, 0.8f);
        glVertex3f(-levelWidth, -levelHeight * 0.5f, 0);
        glVertex3f(levelWidth, -levelHeight * 0.5f, 0);

        glColor4f(r, g, b, 0.95f);
        glVertex3f(levelWidth, levelHeight * 0.5f, 0);
        glVertex3f(-levelWidth, levelHeight * 0.5f, 0);
        glEnd();

        glPopMatrix();
        glEnable(GL_LIGHTING);
    }

    // === MUTE INDICATOR ===
    if (isMuted) {
        glDisable(GL_LIGHTING);
        glLineWidth(3.0f);
        glColor4f(0.9f, 0.2f, 0.2f, 0.9f);

        glPushMatrix();
        glTranslatef(0, scale + 0.8f, 0);
        glScalef(0.4f, 0.4f, 0.4f);

        // Draw X
        glBegin(GL_LINES);
        glVertex3f(-1, -1, 0); glVertex3f(1, 1, 0);
        glVertex3f(-1, 1, 0); glVertex3f(1, -1, 0);
        glEnd();

        // Draw circle around X
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 24; i++) {
            float angle = (i / 24.0f) * 2.0f * M_PI;
            glVertex3f(cos(angle) * 1.3f, sin(angle) * 1.3f, 0);
        }
        glEnd();

        glPopMatrix();
        glLineWidth(1.0f);
        glEnable(GL_LIGHTING);
    }

    // LOD debug output (optional, only print occasionally for performance monitoring)
    #ifdef DEBUG_LOD_STATS
    static int lodFrameCount = 0;
    if (lodLevel > 0 && (lodFrameCount++ % 180 == 0)) {
        const char* lodNames[] = {"High (32x32)", "Medium (20x20)", "Low (12x12)"};
        printf("[Adaptive LOD] Track at distance %.1f using LOD %d (%s)%s%s\n",
               distanceFromCamera, lodLevel, lodNames[lodLevel],
               enableGlow ? "" : ", glow disabled",
               enableDetails ? "" : ", details disabled");
    }
    #endif

    glPopMatrix();
}

void Mixer3DView::AnimateScene()
{
    // Lock for thread-safe animation updates
    BAutolock locker(fGLLocker);

    // Animate track properties using REAL audio data
    for (size_t i = 0; i < f3DTracks.size(); i++) {
        Track3D& track3d = f3DTracks[i];

        if (fEngine && (int)i < fEngine->GetTrackCount()) {
            SimpleTrack* audioTrack = fEngine->GetTrack(i);
            if (audioTrack) {
                if (audioTrack->IsMuted()) {
                    // Muted track - minimal scale, no level, no rotation
                    track3d.scale = 0.3f;  // Smaller when muted
                    track3d.levelHeight = 0.0f;  // No level meter
                    // track3d.rotation stays the same (frozen)
                } else {
                    // Active track - normal behavior
                    track3d.scale = 0.5f + audioTrack->GetVolume() * 0.5f;
                    track3d.levelHeight = audioTrack->GetPeakLevel() * 2.0f;
                    track3d.rotation += audioTrack->GetRMSLevel() * 50.0f;

                    // Emit particles from active tracks
                    float audioLevel = audioTrack->GetRMSLevel();
                    if (audioLevel > 0.05f) {  // Only emit for audible signal
                        fParticleSystem.EmitFromTrack(i, track3d.x, track3d.y + 1.0f, track3d.z, audioLevel);
                    }
                }
            }
        } else {
            // Fallback animation if no engine data
            track3d.scale = 0.8f + 0.4f * sin(fAnimationTime * 3.0f + track3d.x);
            track3d.levelHeight = 0.5f + 0.5f * sin(fAnimationTime * 5.0f + track3d.z);
            track3d.rotation = fAnimationTime * 10.0f;
        }
    }
}

void Mixer3DView::MouseDown(BPoint where)
{
    fMouseDown = true;
    fLastMousePos = where;
    fDragStartPos = where;

    // Check if clicking on a track using 3D picking
    Track3D* clickedTrack = GetTrackAt(where);

    if (clickedTrack) {
        // Find track index
        for (size_t i = 0; i < f3DTracks.size(); i++) {
            if (&f3DTracks[i] == clickedTrack) {
                fSelectedTrack = i;
                fDraggingTrack = true;

                // Mark track as selected (visual feedback)
                for (auto& t : f3DTracks) t.selected = false;  // Deselect all
                clickedTrack->selected = true;  // Select clicked

                printf("Mixer3DView: Selected Track %d for dragging\n", fSelectedTrack + 1);

                if (Window() && Window()->LockLooper()) {
                    Invalidate();
                    Window()->UnlockLooper();
                }
                break;
            }
        }
    } else {
        // Clicking on empty space - prepare for camera rotation
        fSelectedTrack = -1;
        fDraggingTrack = false;
        printf("Mixer3DView: Camera rotation mode\n");
    }
}

void Mixer3DView::MouseUp(BPoint where)
{
    fMouseDown = false;
    fDraggingTrack = false;

    if (fSelectedTrack >= 0) {
        printf("Mixer3DView: Track %d final position: (%.2f, %.2f, %.2f)\n",
               fSelectedTrack + 1,
               f3DTracks[fSelectedTrack].x,
               f3DTracks[fSelectedTrack].y,
               f3DTracks[fSelectedTrack].z);
    }
}

void Mixer3DView::MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
{
    if (fMouseDown) {
        if (fDraggingTrack && fSelectedTrack >= 0) {
            // **DRAG TRACK IN 3D SPACE**
            Track3D& track = f3DTracks[fSelectedTrack];

            float deltaX = where.x - fLastMousePos.x;
            float deltaY = where.y - fLastMousePos.y;

            // Get keyboard modifiers to determine drag mode
            uint32 modifiers = ::modifiers();

            if (modifiers & B_CONTROL_KEY) {
                // CTRL + Drag = Move track vertically (Y axis)
                track.y -= deltaY * 0.05f;  // Invert for natural feel

                // Clamp Y position to reasonable range
                if (track.y < -5.0f) track.y = -5.0f;
                if (track.y > 10.0f) track.y = 10.0f;

                printf("Track %d Y: %.2f\n", fSelectedTrack + 1, track.y);

            } else {
                // Normal Drag = Move track on horizontal plane (XZ)
                // Convert screen delta to world space movement

                float sensitivity = 0.05f;  // Movement speed

                // Camera-relative movement (more intuitive)
                float angleYRad = fCameraAngleY * M_PI / 180.0f;

                // Right vector (perpendicular to camera forward on XZ plane)
                float rightX = cos(angleYRad);
                float rightZ = -sin(angleYRad);

                // Forward vector
                float forwardX = sin(angleYRad);
                float forwardZ = cos(angleYRad);

                // Apply movement
                track.x += (deltaX * rightX - deltaY * forwardX) * sensitivity;
                track.z += (deltaX * rightZ - deltaY * forwardZ) * sensitivity;

                printf("Track %d XZ: (%.2f, %.2f)\n", fSelectedTrack + 1, track.x, track.z);
            }

            fLastMousePos = where;

            // Trigger redraw
            if (Window() && Window()->LockLooper()) {
                Invalidate();
                Window()->UnlockLooper();
            }

        } else {
            // **ROTATE CAMERA** (no track selected)
            float deltaX = where.x - fLastMousePos.x;
            float deltaY = where.y - fLastMousePos.y;

            fCameraAngleY += deltaX * 0.5f;
            fCameraAngleX -= deltaY * 0.5f;

            // Clamp X angle
            if (fCameraAngleX > 89.0f) fCameraAngleX = 89.0f;
            if (fCameraAngleX < -89.0f) fCameraAngleX = -89.0f;

            fCameraDirty = true;  // Invalidate cache

            fLastMousePos = where;

            // Trigger redraw
            if (Window() && Window()->LockLooper()) {
                Invalidate();
                Window()->UnlockLooper();
            }
        }
    }
}

void Mixer3DView::SetCameraAngle(float angleX, float angleY)
{
    fCameraAngleX = angleX;
    fCameraAngleY = angleY;
    fCameraDirty = true;  // Invalidate cache

    if (Window() && Window()->LockLooper()) {
        Invalidate();
        Window()->UnlockLooper();
    }
}

void Mixer3DView::ZoomCamera(float zoom)
{
    fCameraDistance += zoom;

    // Limit zoom range
    if (fCameraDistance < 2.0f) fCameraDistance = 2.0f;
    if (fCameraDistance > 50.0f) fCameraDistance = 50.0f;

    fCameraDirty = true;  // Invalidate cache

    if (Window() && Window()->LockLooper()) {
        Invalidate();
        Window()->UnlockLooper();
    }

    printf("Mixer3DView: Camera distance: %.1f\n", fCameraDistance);
}

void Mixer3DView::ResetCamera()
{
    fCameraAngleX = 30.0f;
    fCameraAngleY = 45.0f;
    fCameraDistance = 20.0f;  // Increased from 10.0f to see all spheres

    fCameraDirty = true;  // Invalidate cache

    if (Window() && Window()->LockLooper()) {
        Invalidate();
        Window()->UnlockLooper();
    }

    printf("Mixer3DView: Camera reset to distance %.1f\n", fCameraDistance);
}

void Mixer3DView::KeyDown(const char* bytes, int32 numBytes)
{
    if (numBytes > 0) {
        switch (bytes[0]) {
            case '+':
            case '=':
                // Zoom in
                ZoomCamera(-2.0f);
                break;
                
            case '-':
            case '_':
                // Zoom out
                ZoomCamera(3.0f);
                break;
                
            case 'r':
            case 'R':
                // Reset camera
                ResetCamera();
                break;
                
            default:
                // Pass unhandled keys to parent
                BGLView::KeyDown(bytes, numBytes);
                break;
        }
    }
}

// =====================================
// Mixer3DWindow Implementation
// =====================================

Mixer3DWindow::Mixer3DWindow(SimpleHaikuEngine* engine, ::VeniceDAW::AdvancedAudioProcessor* processor)
    : BWindow(BRect(150, 150, 950, 650), "VeniceDAW - 3D Mixer (3dmix heritage)",
              B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
    , fEngine(engine)
    , fAudioProcessor(processor)
    , fSpatialMode(processor != nullptr)  // Auto-detect spatial mode if processor provided
    , fMenuBar(nullptr)
    , f3DView(nullptr)
    , fControlsPanel(nullptr)
    , fSpatialControlPanel(nullptr)
    , fPlayButton(nullptr)
    , fStopButton(nullptr)
    , fResetCameraButton(nullptr)
    , fModeSwitchButton(nullptr)
    , fInfoDisplay(nullptr)
    , fUpdateRunner(nullptr)
{
    CreateMenuBar();
    Create3DView();
    CreateControlsPanel();

    // Start update timer for smooth animation
    BMessage updateMsg(MSG_UPDATE_3D);
    fUpdateRunner = new BMessageRunner(BMessenger(this), &updateMsg, 50000); // 20 FPS - optimized

    printf("Mixer3DWindow: Unified 3dmix-inspired mixer created!\n");
    printf("✨ Mode: %s\n", fSpatialMode ? "Spatial Audio (advanced)" : "Simple 3D (classic 3dmix)");
    printf("✨ BeOS 3dmix heritage meets modern Haiku design! ✨\n");
}

Mixer3DWindow::~Mixer3DWindow()
{
    delete fUpdateRunner;
    printf("Mixer3DWindow: 3D mixer destroyed\n");
}

void Mixer3DWindow::CreateMenuBar()
{
    fMenuBar = new BMenuBar("3d_menubar");
    
    // File menu
    BMenu* fileMenu = new BMenu("File");
    fileMenu->AddItem(new BMenuItem("Export 3D Scene", new BMessage('exp3')));
    fileMenu->AddSeparatorItem();
    fileMenu->AddItem(new BMenuItem("Close 3D View", new BMessage(B_QUIT_REQUESTED), 'W'));
    fMenuBar->AddItem(fileMenu);
    
    // View menu
    BMenu* viewMenu = new BMenu("View");
    viewMenu->AddItem(new BMenuItem("Zoom In", new BMessage(MSG_ZOOM_IN), '+'));
    viewMenu->AddItem(new BMenuItem("Zoom Out", new BMessage(MSG_ZOOM_OUT), '-'));
    viewMenu->AddSeparatorItem();
    viewMenu->AddItem(new BMenuItem("Reset Camera", new BMessage(MSG_RESET_CAMERA), 'R'));
    viewMenu->AddSeparatorItem();
    BMenuItem* particleToggle = new BMenuItem("Enable Particles", new BMessage(MSG_TOGGLE_PARTICLES), 'P');
    particleToggle->SetMarked(false);  // Start disabled
    viewMenu->AddItem(particleToggle);
    viewMenu->AddSeparatorItem();
    viewMenu->AddItem(new BMenuItem("Fullscreen 3D", new BMessage('full')));
    fMenuBar->AddItem(viewMenu);
    
    // Help menu
    BMenu* helpMenu = new BMenu("Help");
    helpMenu->AddItem(new BMenuItem("3D Controls", new BMessage('help')));
    helpMenu->AddItem(new BMenuItem("About 3D Mixer", new BMessage('ab3d')));
    fMenuBar->AddItem(helpMenu);
}

void Mixer3DWindow::Create3DView()
{
    // Main 3D view - large professional viewport (60% of window)
    BRect viewRect(0, 0, 900, 600);

    // Create appropriate view based on mode
    if (fSpatialMode && fAudioProcessor) {
        // Advanced spatial mode with HRTF
        f3DView = new SpatialMixer3DView(viewRect, fEngine, fAudioProcessor);
        printf("Mixer3DWindow: Created SpatialMixer3DView (advanced)\n");
    } else {
        // Classic 3dmix-inspired mode
        f3DView = new Mixer3DView(viewRect, fEngine);
        printf("Mixer3DWindow: Created Mixer3DView (classic 3dmix)\n");
    }

    // Set explicit size to ensure 3D viewport dominates the window
    f3DView->SetExplicitMinSize(BSize(800, 500));
    f3DView->SetExplicitPreferredSize(BSize(900, 600));
}

void Mixer3DWindow::CreateControlsPanel()
{
    // Control panel at bottom with VeniceTheme
    fControlsPanel = new BView("controls", B_WILL_DRAW);
    fControlsPanel->SetViewColor(::VeniceDAW::VeniceTheme::PanelBackground());

    BGroupLayout* controlsLayout = new BGroupLayout(B_HORIZONTAL);
    fControlsPanel->SetLayout(controlsLayout);
    controlsLayout->SetInsets(::VeniceDAW::VeniceTheme::MARGIN, ::VeniceDAW::VeniceTheme::PADDING,
                              ::VeniceDAW::VeniceTheme::MARGIN, ::VeniceDAW::VeniceTheme::PADDING);
    controlsLayout->SetSpacing(::VeniceDAW::VeniceTheme::SPACING);

    // Transport controls - compact
    fPlayButton = new BButton("3d_play", "▶ Play 3D", new BMessage(MSG_PLAY));
    fPlayButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, ::VeniceDAW::VeniceTheme::BUTTON_HEIGHT));
    fStopButton = new BButton("3d_stop", "⏹ Stop", new BMessage(MSG_STOP));
    fStopButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, ::VeniceDAW::VeniceTheme::BUTTON_HEIGHT));

    controlsLayout->AddView(fPlayButton);
    controlsLayout->AddView(fStopButton);

    // Camera controls - compact
    fResetCameraButton = new BButton("reset_cam", "📷 Reset", new BMessage(MSG_RESET_CAMERA));
    fResetCameraButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, ::VeniceDAW::VeniceTheme::BUTTON_HEIGHT));
    controlsLayout->AddView(fResetCameraButton);

    // Mode switcher button (if processor available)
    if (fAudioProcessor) {
        const char* modeLabel = fSpatialMode ? "🎚️ Simple Mode" : "🌐 Spatial Mode";
        fModeSwitchButton = new BButton("mode_switch", modeLabel, new BMessage(MSG_TOGGLE_SPATIAL_MODE));
        fModeSwitchButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, ::VeniceDAW::VeniceTheme::BUTTON_HEIGHT));
        controlsLayout->AddView(fModeSwitchButton);
    }

    // Info display
    const char* infoText = fSpatialMode ?
        "🌐 Spatial 3D Mixer - Drag tracks for 3D positioning" :
        "🎵 Classic 3D Mixer - Drag to rotate camera";
    fInfoDisplay = new BStringView("3d_info", infoText);
    controlsLayout->AddView(fInfoDisplay);
    
    // Window layout
    BGroupLayout* windowLayout = new BGroupLayout(B_VERTICAL);
    SetLayout(windowLayout);
    windowLayout->SetSpacing(0);
    
    windowLayout->AddView(fMenuBar);
    windowLayout->AddView(f3DView);
    windowLayout->AddView(fControlsPanel);
}

bool Mixer3DWindow::QuitRequested()
{
    printf("Mixer3DWindow: QuitRequested called\n");
    
    // Disable updates first
    DisableUpdates();
    
    // Clean up OpenGL view properly to avoid deadlock
    if (f3DView) {
        // Stop rendering
        f3DView->EnableDirectMode(false);
        
        // Use RAII guard for safe OpenGL cleanup
        {
            GLContextGuard glGuard(f3DView);
            if (glGuard) {
                // Force OpenGL to finish all pending operations
                glFinish();
                glFlush();
                
                // Disable all OpenGL features that might be using threads
                glDisable(GL_LIGHTING);
                glDisable(GL_DEPTH_TEST);
                glDisable(GL_TEXTURE_2D);
                glDisable(GL_BLEND);
                
                // Clear any bound textures
                glBindTexture(GL_TEXTURE_2D, 0);
                
                printf("Mixer3DWindow: OpenGL context cleaned\n");
            }
        } // GL context automatically unlocked here
        
        // Now safely remove and delete view (no locks held)
        RemoveChild(f3DView);
        
        // Delete the view explicitly
        delete f3DView;
        f3DView = nullptr;
        
        // Increased wait time based on agent analysis
        snooze(500000); // 500ms for llvmpipe thread termination
        printf("Mixer3DWindow: View deleted and threads cleaned\n");
    }
    
    // Notify app that 3D window is closing (only if app exists)
    if (be_app) {
        be_app->PostMessage(new BMessage('3dcl'));  // 3D window closed
    }
    
    printf("Mixer3DWindow: Ready to quit\n");
    
    // Allow window to close
    return true;
}

void Mixer3DWindow::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case MSG_PLAY:
            if (fEngine) {
                fEngine->Start();
                fInfoDisplay->SetText("🎵 Playing 3D Audio - Watch the visualization!");
            }
            break;
            
        case MSG_STOP:
            if (fEngine) {
                fEngine->Stop();
                fInfoDisplay->SetText("⏸ Stopped - 3D visualization paused");
            }
            break;
            
        case MSG_ZOOM_IN:
            if (f3DView) {
                f3DView->ZoomCamera(-2.0f);  // Negative = zoom in (closer)
                fInfoDisplay->SetText("🔍 Zoomed in");
            }
            break;
            
        case MSG_ZOOM_OUT:
            if (f3DView) {
                f3DView->ZoomCamera(3.0f);   // Positive = zoom out (farther)
                fInfoDisplay->SetText("🔍 Zoomed out - should see all spheres!");
            }
            break;
            
        case MSG_RESET_CAMERA:
            if (f3DView) {
                f3DView->ResetCamera();
                fInfoDisplay->SetText("📷 Camera reset to see all spheres");
            }
            break;

        case MSG_TOGGLE_PARTICLES:
            if (f3DView) {
                bool currentState = f3DView->AreParticlesEnabled();
                f3DView->SetParticlesEnabled(!currentState);

                // Update menu item checkmark
                BMenuItem* item = nullptr;
                if (message->FindPointer("source", (void**)&item) == B_OK && item) {
                    item->SetMarked(!currentState);
                }

                fInfoDisplay->SetText(!currentState ? "✨ Particles enabled!" : "Particles disabled");
            }
            break;

        case MSG_UPDATE_3D:
            if (f3DView && f3DView->LockLooper()) {
                f3DView->Invalidate();
                f3DView->UnlockLooper();
            }
            break;

        case MSG_TOGGLE_SPATIAL_MODE:
            // Toggle between simple and spatial modes
            SetSpatialMode(!fSpatialMode);
            break;

        case 'ab3d':
        {
            BAlert* alert = new BAlert("About 3D Mixer",
                "VeniceDAW 3D Mixer v2.0\n"
                "A tribute to BeOS 3dmix by Benoit Schillings\n\n"
                "✨ Modern reinterpretation of 3dmix heritage\n"
                "🎵 Real-time spherical track visualization\n"
                "💫 Glow effects and glass floor reflections\n"
                "🎨 BeOS signature color palette\n"
                "🎚️ Live audio level indicators\n\n"
                "Built with native Haiku BGL (OpenGL)\n"
                "Hardware accelerated graphics\n\n"
                "Keeping the BeOS spirit alive in Haiku! 🚀",
                "Beautiful!");
            alert->Go();
            break;
        }
        
        case 'help':
        {
            BAlert* help = new BAlert("3D Controls",
                "🖱️ MOUSE CONTROLS:\n"
                "• Click sphere: Select track\n"
                "• Drag sphere: Move track horizontally (XZ plane)\n"
                "• CTRL + Drag sphere: Move track vertically (Y axis)\n"
                "• Drag empty space: Rotate camera\n\n"
                "⌨️ KEYBOARD:\n"
                "• R: Reset camera\n"
                "• +/-: Zoom in/out\n\n"
                "🎯 3D POSITIONING:\n"
                "• Tracks can be moved freely in 3D space\n"
                "• Yellow ring shows selected track\n"
                "• Position affects spatial audio mixing\n"
                "• Real-time visual feedback\n\n"
                "Welcome to the future of audio mixing! 🚀",
                "Got it!");
            help->Go();
            break;
        }
        
        default:
            BWindow::MessageReceived(message);
            break;
    }
}

void Mixer3DWindow::UpdateVisualization()
{
    // Update 3D scene with current engine state
    if (f3DView) {
        // In full implementation, would sync with actual audio engine data
    }
}

// =====================================
// Missing Mixer3DView Methods
// =====================================

void Mixer3DView::ProjectPoint(float x, float y, float z, BPoint& screen)
{
    // Project 3D world coordinates to 2D screen coordinates
    // This is a simplified implementation for the spatial mixer
    
    GLdouble modelMatrix[16];
    GLdouble projMatrix[16];
    GLint viewport[4];
    GLdouble winX, winY, winZ;
    
    // Get current OpenGL matrices
    glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    // Project the 3D point to screen coordinates
    if (gluProject(x, y, z, modelMatrix, projMatrix, viewport, &winX, &winY, &winZ) == GL_TRUE) {
        screen.x = (float)winX;
        screen.y = (float)(viewport[3] - winY); // Flip Y coordinate
    } else {
        // Fallback if projection fails
        screen.x = Bounds().Width() / 2.0f + x * 50.0f;
        screen.y = Bounds().Height() / 2.0f - y * 50.0f;
    }
}

void Mixer3DView::UpdateCameraCache()
{
    // Pre-compute expensive trigonometric calculations for camera position
    // This optimization reduces CPU usage when camera is static
    float angleXRad = fCameraAngleX * M_PI / 180.0f;
    float angleYRad = fCameraAngleY * M_PI / 180.0f;

    float cosX = cos(angleXRad);
    float sinX = sin(angleXRad);
    float cosY = cos(angleYRad);
    float sinY = sin(angleYRad);

    fCachedCameraX = fCameraTarget[0] + fCameraDistance * cosY * cosX;
    fCachedCameraY = fCameraTarget[1] + fCameraDistance * sinX;
    fCachedCameraZ = fCameraTarget[2] + fCameraDistance * sinY * cosX;

    fCameraDirty = false;  // Mark cache as valid
}

void Mixer3DView::SetParticlesEnabled(bool enabled)
{
    fParticleSystem.SetEnabled(enabled);
    printf("Mixer3DView: Particle system %s\n", enabled ? "enabled" : "disabled");
}

bool Mixer3DView::AreParticlesEnabled() const
{
    return fParticleSystem.IsEnabled();
}

Track3D* Mixer3DView::GetTrackAt(BPoint screenPoint)
{
    // **3D PICKING**: Ray casting from screen point into 3D scene
    // Returns the track clicked by the user, or nullptr if none

    LockGL();

    // Get OpenGL viewport, projection, and modelview matrices
    GLint viewport[4];
    GLdouble modelMatrix[16];
    GLdouble projMatrix[16];

    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);

    // Convert screen coordinates (flip Y for OpenGL)
    float winX = screenPoint.x;
    float winY = viewport[3] - screenPoint.y;  // Flip Y axis

    // Un-project to get ray in 3D space
    GLdouble nearX, nearY, nearZ;  // Ray start (near plane)
    GLdouble farX, farY, farZ;     // Ray end (far plane)

    gluUnProject(winX, winY, 0.0, modelMatrix, projMatrix, viewport, &nearX, &nearY, &nearZ);
    gluUnProject(winX, winY, 1.0, modelMatrix, projMatrix, viewport, &farX, &farY, &farZ);

    // Ray direction
    float rayDirX = farX - nearX;
    float rayDirY = farY - nearY;
    float rayDirZ = farZ - nearZ;

    // Normalize ray direction
    float rayLength = sqrt(rayDirX*rayDirX + rayDirY*rayDirY + rayDirZ*rayDirZ);
    rayDirX /= rayLength;
    rayDirY /= rayLength;
    rayDirZ /= rayLength;

    // Ray origin
    float rayOriginX = nearX;
    float rayOriginY = nearY;
    float rayOriginZ = nearZ;

    UnlockGL();

    // Test ray against all tracks (sphere collision)
    Track3D* closestTrack = nullptr;
    float closestDistance = 1000000.0f;

    for (auto& track : f3DTracks) {
        // Sphere center
        float sphereX = track.x;
        float sphereY = track.y;
        float sphereZ = track.z;
        float sphereRadius = 1.0f * track.scale;  // Sphere radius

        // Vector from ray origin to sphere center
        float ocX = rayOriginX - sphereX;
        float ocY = rayOriginY - sphereY;
        float ocZ = rayOriginZ - sphereZ;

        // Ray-sphere intersection using quadratic formula
        float a = rayDirX*rayDirX + rayDirY*rayDirY + rayDirZ*rayDirZ;
        float b = 2.0f * (ocX*rayDirX + ocY*rayDirY + ocZ*rayDirZ);
        float c = ocX*ocX + ocY*ocY + ocZ*ocZ - sphereRadius*sphereRadius;

        float discriminant = b*b - 4*a*c;

        if (discriminant >= 0) {
            // Ray hits sphere!
            float t = (-b - sqrt(discriminant)) / (2*a);  // Nearest intersection

            if (t > 0 && t < closestDistance) {
                closestDistance = t;
                closestTrack = &track;
            }
        }
    }

    if (closestTrack) {
        printf("Mixer3DView: 3D picking hit track at distance %.2f\n", closestDistance);
    }

    return closestTrack;
}

// =====================================
// Mixer3DWindow Mode Switching
// =====================================

SpatialMixer3DView* Mixer3DWindow::GetSpatialView()
{
    // Safe cast - returns nullptr if not in spatial mode
    return dynamic_cast<SpatialMixer3DView*>(f3DView);
}

void Mixer3DWindow::SetSpatialMode(bool enabled)
{
    if (enabled == fSpatialMode) return;  // Already in requested mode

    if (enabled && !fAudioProcessor) {
        printf("Mixer3DWindow: Cannot enable spatial mode - no audio processor!\n");
        return;
    }

    if (enabled) {
        SwitchToSpatialMode();
    } else {
        SwitchToSimpleMode();
    }
}

void Mixer3DWindow::SwitchToSpatialMode()
{
    printf("Mixer3DWindow: Switching to Spatial Mode...\n");

    if (!fAudioProcessor) {
        printf("ERROR: Cannot switch to spatial mode without audio processor!\n");
        return;
    }

    // Remove old view
    if (f3DView) {
        RemoveChild(f3DView);
        delete f3DView;
        f3DView = nullptr;
    }

    // Create new spatial view
    BRect viewRect(0, 0, 900, 600);
    f3DView = new SpatialMixer3DView(viewRect, fEngine, fAudioProcessor);
    f3DView->SetExplicitMinSize(BSize(800, 500));
    f3DView->SetExplicitPreferredSize(BSize(900, 600));

    // Re-add to layout
    BGroupLayout* layout = dynamic_cast<BGroupLayout*>(GetLayout());
    if (layout) {
        layout->AddView(1, f3DView);  // Position 1 (between menubar and controls)
    }

    fSpatialMode = true;

    // Update button label
    if (fModeSwitchButton) {
        fModeSwitchButton->SetLabel("🎚️ Simple Mode");
    }

    // Update info text
    if (fInfoDisplay) {
        fInfoDisplay->SetText("🌐 Spatial 3D Mixer - Drag tracks for 3D positioning");
    }

    printf("Mixer3DWindow: ✅ Switched to Spatial Mode\n");
}

void Mixer3DWindow::SwitchToSimpleMode()
{
    printf("Mixer3DWindow: Switching to Simple 3dmix Mode...\n");

    // Remove old view
    if (f3DView) {
        RemoveChild(f3DView);
        delete f3DView;
        f3DView = nullptr;
    }

    // Create classic 3dmix view
    BRect viewRect(0, 0, 900, 600);
    f3DView = new Mixer3DView(viewRect, fEngine);
    f3DView->SetExplicitMinSize(BSize(800, 500));
    f3DView->SetExplicitPreferredSize(BSize(900, 600));

    // Re-add to layout
    BGroupLayout* layout = dynamic_cast<BGroupLayout*>(GetLayout());
    if (layout) {
        layout->AddView(1, f3DView);  // Position 1 (between menubar and controls)
    }

    fSpatialMode = false;

    // Update button label
    if (fModeSwitchButton) {
        fModeSwitchButton->SetLabel("🌐 Spatial Mode");
    }

    // Update info text
    if (fInfoDisplay) {
        fInfoDisplay->SetText("🎵 Classic 3D Mixer - Drag to rotate camera");
    }

    printf("Mixer3DWindow: ✅ Switched to Simple 3dmix Mode\n");
}

} // namespace HaikuDAW
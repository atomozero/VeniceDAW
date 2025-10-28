/*
 * demo_3dmix_viewer.cpp - Educational 3DMix Visualization Demo
 *
 * Demonstrates 3D spatial audio positioning from BeOS 3dmix files
 * WITHOUT playing audio - pure visualization for learning purposes
 *
 * Usage: ./demo_3dmix_viewer /path/to/project.3dmix
 */

#include <Application.h>
#include <Window.h>
#include <GLView.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <MessageRunner.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <String.h>
#include <vector>
#include <Bitmap.h>
#include <View.h>
#include <Font.h>

// Include our 3dmix parser
#include "audio/3dmix/3DMixFormat.h"
#include "audio/3dmix/3DMixParser.h"

struct AudioSource {
    BString name;
    float x, y, z;  // 3D position
    rgb_color color;
};

class DemoGL3DView : public BGLView {
public:
    DemoGL3DView(BRect frame, const char* name)
        : BGLView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS,
                  BGL_RGB | BGL_DOUBLE | BGL_DEPTH)
        , fRotationY(180.0f)  // Rotate 180 degrees
        , fZoom(-25.0f)  // Increased zoom to see all tracks
        , fProjectName("")
        , fTrackCount(0)
        , fAutoRotate(false)  // Disable auto-rotation
        , fMouseX(0)
        , fMouseY(0)
        , fHoveredTrackIndex(-1)
    {
        SetEventMask(B_POINTER_EVENTS, 0);
    }

    virtual void AttachedToWindow() override {
        BGLView::AttachedToWindow();
        LockGL();
        InitGL();
        UnlockGL();
    }

    virtual void FrameResized(float width, float height) override {
        BGLView::FrameResized(width, height);
        LockGL();
        glViewport(0, 0, (GLint)width, (GLint)height);
        SetupProjection(width, height);
        UnlockGL();
    }

    void InitGL() {
        glClearColor(0.08f, 0.08f, 0.1f, 1.0f);  // Dark blue-gray
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glShadeModel(GL_SMOOTH);

        BRect bounds = Bounds();
        SetupProjection(bounds.Width(), bounds.Height());
    }

    void SetupProjection(float width, float height) {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0, width / height, 0.1, 100.0);
        glMatrixMode(GL_MODELVIEW);
    }

    void AddAudioSource(const char* name, float x, float y, float z) {
        AudioSource source;
        source.name = name;
        source.x = x;
        source.y = y;
        source.z = z;

        // Generate color based on name hash
        int hash = 0;
        for (int i = 0; name[i]; i++) hash += name[i];
        source.color.red = 100 + (hash * 17) % 155;
        source.color.green = 100 + (hash * 31) % 155;
        source.color.blue = 100 + (hash * 47) % 155;
        source.color.alpha = 255;

        fSources.push_back(source);
    }

    virtual void Draw(BRect updateRect) override {
        LockGL();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        // Camera setup - bird's eye view
        glTranslatef(0.0f, 0.0f, fZoom);
        glRotatef(fRotationY, 0.0f, 1.0f, 0.0f);
        glRotatef(-45.0f, 1.0f, 0.0f, 0.0f);  // 45° tilt for isometric view

        // Draw ground plane/grid
        DrawGroundPlane();

        // Draw listener (center) on the plane
        DrawListener();

        // Draw audio sources on the plane
        for (const auto& source : fSources) {
            DrawAudioSource(source);
        }

        // Draw project info overlay (2D text)
        DrawProjectInfo();

        SwapBuffers();
        UnlockGL();
    }

    void DrawGroundPlane() {
        // Draw a grid plane where tracks are positioned
        glLineWidth(1.0f);
        glColor4f(0.3f, 0.3f, 0.4f, 0.5f);

        const float gridSize = 12.0f;
        const float gridStep = 2.0f;

        // Draw grid lines
        glBegin(GL_LINES);
        for (float i = -gridSize; i <= gridSize; i += gridStep) {
            // Lines parallel to X axis
            glVertex3f(-gridSize, 0.0f, i);
            glVertex3f(gridSize, 0.0f, i);

            // Lines parallel to Z axis
            glVertex3f(i, 0.0f, -gridSize);
            glVertex3f(i, 0.0f, gridSize);
        }
        glEnd();

        // Draw center axes more prominently
        glLineWidth(2.0f);
        glBegin(GL_LINES);

        // X axis - Red
        glColor3f(0.8f, 0.2f, 0.2f);
        glVertex3f(-gridSize, 0.0f, 0.0f);
        glVertex3f(gridSize, 0.0f, 0.0f);

        // Z axis - Blue
        glColor3f(0.2f, 0.2f, 0.8f);
        glVertex3f(0.0f, 0.0f, -gridSize);
        glVertex3f(0.0f, 0.0f, gridSize);

        glEnd();
    }

    void DrawListener() {
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.0f);

        // Draw listener icon on the ground (ears/head from above)
        glColor3f(1.0f, 1.0f, 0.0f);

        // Draw main circle for head (top view)
        GLUquadric* quad = gluNewQuadric();
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluDisk(quad, 0.0, 0.5, 20, 1);
        glPopMatrix();

        // Draw "ears" as small cylinders
        glColor3f(0.9f, 0.9f, 0.0f);
        glPushMatrix();
        glTranslatef(-0.5f, 0.0f, 0.0f);
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        gluCylinder(quad, 0.15, 0.15, 0.3, 12, 1);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.2f, 0.0f, 0.0f);
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        gluCylinder(quad, 0.15, 0.15, 0.3, 12, 1);
        glPopMatrix();

        gluDeleteQuadric(quad);
        glPopMatrix();
    }

    void DrawAudioSource(const AudioSource& source) {
        glPushMatrix();
        glTranslatef(source.x, 0.0f, source.z);  // Always on Y=0 plane

        // Draw vertical cylinder/pole
        glColor3ub(source.color.red, source.color.green, source.color.blue);
        GLUquadric* quad = gluNewQuadric();

        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);  // Point cylinder upward
        gluCylinder(quad, 0.1, 0.1, 0.8, 12, 1);  // Thin pole
        glPopMatrix();

        // Draw sphere on top of pole
        glTranslatef(0.0f, 0.8f, 0.0f);
        gluSphere(quad, 0.3, 16, 16);
        gluDeleteQuadric(quad);

        // Draw base circle on ground
        glPushMatrix();
        glTranslatef(0.0f, -0.8f, 0.0f);
        glColor4f(source.color.red / 255.0f,
                  source.color.green / 255.0f,
                  source.color.blue / 255.0f, 0.3f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0.0f, 0.01f, 0.0f);
        for (int i = 0; i <= 20; i++) {
            float angle = (i / 20.0f) * 2.0f * M_PI;
            glVertex3f(cos(angle) * 0.4f, 0.01f, sin(angle) * 0.4f);
        }
        glEnd();
        glPopMatrix();

        glPopMatrix();
    }

    virtual void MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage) override {
        fMouseX = where.x;
        fMouseY = where.y;
        UpdateHoveredTrack();
    }

    void UpdateHoveredTrack() {
        // Project 3D track positions to screen space and compare with mouse
        fHoveredTrackIndex = -1;
        float minDist = 50.0f;  // Proximity threshold in pixels

        GLint viewport[4];
        GLdouble modelview[16], projection[16];

        LockGL();
        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
        glGetDoublev(GL_PROJECTION_MATRIX, projection);

        for (size_t i = 0; i < fSources.size(); i++) {
            const AudioSource& source = fSources[i];

            // Project 3D position (top of pole) to screen coordinates
            GLdouble screenX, screenY, screenZ;
            gluProject(source.x, 0.8, source.z,  // 0.8 = top of pole
                      modelview, projection, viewport,
                      &screenX, &screenY, &screenZ);

            // Convert screen Y (OpenGL is bottom-up)
            screenY = viewport[3] - screenY;

            // Calculate distance in screen space (pixels)
            float dx = fMouseX - screenX;
            float dy = fMouseY - screenY;
            float dist = sqrt(dx*dx + dy*dy);

            if (dist < minDist) {
                minDist = dist;
                fHoveredTrackIndex = (int)i;
            }
        }

        UnlockGL();
    }

    void Pulse() {
        if (fAutoRotate) {
            fRotationY += 0.5f;
            if (fRotationY >= 360.0f) fRotationY -= 360.0f;
        }
        Draw(Bounds());
    }

    void ZoomIn() { fZoom += 1.0f; if (fZoom > -2.0f) fZoom = -2.0f; }
    void ZoomOut() { fZoom -= 1.0f; if (fZoom < -30.0f) fZoom = -30.0f; }

    void DrawChar(float x, float y, char c) {
        // Simple bitmap font using lines (7x12 pixel grid)
        glLineWidth(1.5f);
        glBegin(GL_LINES);

        switch (c) {
            // Lowercase letters
            case 'a': case 'A':
                glVertex2f(x, y+12); glVertex2f(x, y+4);
                glVertex2f(x, y+4); glVertex2f(x+6, y+4);
                glVertex2f(x+6, y+4); glVertex2f(x+6, y+12);
                glVertex2f(x, y+8); glVertex2f(x+6, y+8);
                break;
            case 'b': case 'B':
                glVertex2f(x, y); glVertex2f(x, y+12);
                glVertex2f(x, y); glVertex2f(x+5, y);
                glVertex2f(x+5, y); glVertex2f(x+5, y+6);
                glVertex2f(x+5, y+6); glVertex2f(x, y+6);
                glVertex2f(x, y+6); glVertex2f(x+5, y+6);
                glVertex2f(x+5, y+6); glVertex2f(x+5, y+12);
                glVertex2f(x+5, y+12); glVertex2f(x, y+12);
                break;
            case 'c': case 'C':
                glVertex2f(x+6, y+4); glVertex2f(x, y+4);
                glVertex2f(x, y+4); glVertex2f(x, y+12);
                glVertex2f(x, y+12); glVertex2f(x+6, y+12);
                break;
            case 'd': case 'D':
                glVertex2f(x, y); glVertex2f(x, y+12);
                glVertex2f(x, y); glVertex2f(x+5, y+2);
                glVertex2f(x+5, y+2); glVertex2f(x+5, y+10);
                glVertex2f(x+5, y+10); glVertex2f(x, y+12);
                break;
            case 'e': case 'E':
                glVertex2f(x, y); glVertex2f(x, y+12);
                glVertex2f(x, y); glVertex2f(x+6, y);
                glVertex2f(x, y+6); glVertex2f(x+5, y+6);
                glVertex2f(x, y+12); glVertex2f(x+6, y+12);
                break;
            case 'f': case 'F':
                glVertex2f(x, y); glVertex2f(x, y+12);
                glVertex2f(x, y); glVertex2f(x+6, y);
                glVertex2f(x, y+6); glVertex2f(x+5, y+6);
                break;
            case 'g': case 'G':
                glVertex2f(x+6, y+4); glVertex2f(x, y+4);
                glVertex2f(x, y+4); glVertex2f(x, y+12);
                glVertex2f(x, y+12); glVertex2f(x+6, y+12);
                glVertex2f(x+6, y+12); glVertex2f(x+6, y+8);
                glVertex2f(x+6, y+8); glVertex2f(x+3, y+8);
                break;
            case 'h': case 'H':
                glVertex2f(x, y); glVertex2f(x, y+12);
                glVertex2f(x+6, y); glVertex2f(x+6, y+12);
                glVertex2f(x, y+6); glVertex2f(x+6, y+6);
                break;
            case 'i': case 'I':
                glVertex2f(x+3, y); glVertex2f(x+3, y+12);
                glVertex2f(x, y); glVertex2f(x+6, y);
                glVertex2f(x, y+12); glVertex2f(x+6, y+12);
                break;
            case 'j': case 'J':
                glVertex2f(x+6, y); glVertex2f(x+6, y+10);
                glVertex2f(x+6, y+10); glVertex2f(x+3, y+12);
                glVertex2f(x+3, y+12); glVertex2f(x, y+10);
                break;
            case 'k': case 'K':
                glVertex2f(x, y); glVertex2f(x, y+12);
                glVertex2f(x+6, y); glVertex2f(x, y+6);
                glVertex2f(x, y+6); glVertex2f(x+6, y+12);
                break;
            case 'l': case 'L':
                glVertex2f(x, y); glVertex2f(x, y+12);
                glVertex2f(x, y+12); glVertex2f(x+6, y+12);
                break;
            case 'm': case 'M':
                glVertex2f(x, y+12); glVertex2f(x, y);
                glVertex2f(x, y); glVertex2f(x+3, y+6);
                glVertex2f(x+3, y+6); glVertex2f(x+6, y);
                glVertex2f(x+6, y); glVertex2f(x+6, y+12);
                break;
            case 'n': case 'N':
                glVertex2f(x, y+12); glVertex2f(x, y);
                glVertex2f(x, y); glVertex2f(x+6, y+12);
                glVertex2f(x+6, y+12); glVertex2f(x+6, y);
                break;
            case 'o': case 'O':
                glVertex2f(x, y+4); glVertex2f(x, y+12);
                glVertex2f(x, y+12); glVertex2f(x+6, y+12);
                glVertex2f(x+6, y+12); glVertex2f(x+6, y+4);
                glVertex2f(x+6, y+4); glVertex2f(x, y+4);
                break;
            case 'p': case 'P':
                glVertex2f(x, y+12); glVertex2f(x, y);
                glVertex2f(x, y); glVertex2f(x+6, y);
                glVertex2f(x+6, y); glVertex2f(x+6, y+6);
                glVertex2f(x+6, y+6); glVertex2f(x, y+6);
                break;
            case 'q': case 'Q':
                glVertex2f(x, y+4); glVertex2f(x, y+12);
                glVertex2f(x, y+12); glVertex2f(x+6, y+12);
                glVertex2f(x+6, y+12); glVertex2f(x+6, y+4);
                glVertex2f(x+6, y+4); glVertex2f(x, y+4);
                glVertex2f(x+4, y+10); glVertex2f(x+7, y+13);
                break;
            case 'r': case 'R':
                glVertex2f(x, y+12); glVertex2f(x, y);
                glVertex2f(x, y); glVertex2f(x+6, y);
                glVertex2f(x+6, y); glVertex2f(x+6, y+6);
                glVertex2f(x+6, y+6); glVertex2f(x, y+6);
                glVertex2f(x, y+6); glVertex2f(x+6, y+12);
                break;
            case 's': case 'S':
                glVertex2f(x+6, y+4); glVertex2f(x, y+4);
                glVertex2f(x, y+4); glVertex2f(x, y+8);
                glVertex2f(x, y+8); glVertex2f(x+6, y+8);
                glVertex2f(x+6, y+8); glVertex2f(x+6, y+12);
                glVertex2f(x+6, y+12); glVertex2f(x, y+12);
                break;
            case 't': case 'T':
                glVertex2f(x, y); glVertex2f(x+6, y);
                glVertex2f(x+3, y); glVertex2f(x+3, y+12);
                break;
            case 'u': case 'U':
                glVertex2f(x, y); glVertex2f(x, y+12);
                glVertex2f(x, y+12); glVertex2f(x+6, y+12);
                glVertex2f(x+6, y+12); glVertex2f(x+6, y);
                break;
            case 'v': case 'V':
                glVertex2f(x, y); glVertex2f(x+3, y+12);
                glVertex2f(x+3, y+12); glVertex2f(x+6, y);
                break;
            case 'w': case 'W':
                glVertex2f(x, y); glVertex2f(x+1.5, y+12);
                glVertex2f(x+1.5, y+12); glVertex2f(x+3, y+6);
                glVertex2f(x+3, y+6); glVertex2f(x+4.5, y+12);
                glVertex2f(x+4.5, y+12); glVertex2f(x+6, y);
                break;
            case 'x': case 'X':
                glVertex2f(x, y); glVertex2f(x+6, y+12);
                glVertex2f(x+6, y); glVertex2f(x, y+12);
                break;
            case 'y': case 'Y':
                glVertex2f(x, y); glVertex2f(x+3, y+6);
                glVertex2f(x+6, y); glVertex2f(x+3, y+6);
                glVertex2f(x+3, y+6); glVertex2f(x+3, y+12);
                break;
            case 'z': case 'Z':
                glVertex2f(x, y); glVertex2f(x+6, y);
                glVertex2f(x+6, y); glVertex2f(x, y+12);
                glVertex2f(x, y+12); glVertex2f(x+6, y+12);
                break;
            // Numbers
            case '0':
                glVertex2f(x, y); glVertex2f(x, y+12);
                glVertex2f(x, y+12); glVertex2f(x+6, y+12);
                glVertex2f(x+6, y+12); glVertex2f(x+6, y);
                glVertex2f(x+6, y); glVertex2f(x, y);
                break;
            case '1':
                glVertex2f(x+3, y); glVertex2f(x+3, y+12);
                glVertex2f(x+3, y); glVertex2f(x+1, y+2);
                break;
            case '2':
                glVertex2f(x, y); glVertex2f(x+6, y);
                glVertex2f(x+6, y); glVertex2f(x+6, y+6);
                glVertex2f(x+6, y+6); glVertex2f(x, y+6);
                glVertex2f(x, y+6); glVertex2f(x, y+12);
                glVertex2f(x, y+12); glVertex2f(x+6, y+12);
                break;
            case '3':
                glVertex2f(x, y); glVertex2f(x+6, y);
                glVertex2f(x+6, y); glVertex2f(x+6, y+12);
                glVertex2f(x+6, y+12); glVertex2f(x, y+12);
                glVertex2f(x+6, y+6); glVertex2f(x+2, y+6);
                break;
            case '4':
                glVertex2f(x, y); glVertex2f(x, y+6);
                glVertex2f(x, y+6); glVertex2f(x+6, y+6);
                glVertex2f(x+6, y); glVertex2f(x+6, y+12);
                break;
            case '5':
                glVertex2f(x+6, y); glVertex2f(x, y);
                glVertex2f(x, y); glVertex2f(x, y+6);
                glVertex2f(x, y+6); glVertex2f(x+6, y+6);
                glVertex2f(x+6, y+6); glVertex2f(x+6, y+12);
                glVertex2f(x+6, y+12); glVertex2f(x, y+12);
                break;
            case '6':
                glVertex2f(x+6, y); glVertex2f(x, y);
                glVertex2f(x, y); glVertex2f(x, y+12);
                glVertex2f(x, y+12); glVertex2f(x+6, y+12);
                glVertex2f(x+6, y+12); glVertex2f(x+6, y+6);
                glVertex2f(x+6, y+6); glVertex2f(x, y+6);
                break;
            case '7':
                glVertex2f(x, y); glVertex2f(x+6, y);
                glVertex2f(x+6, y); glVertex2f(x+3, y+12);
                break;
            case '8':
                glVertex2f(x, y); glVertex2f(x+6, y);
                glVertex2f(x+6, y); glVertex2f(x+6, y+12);
                glVertex2f(x+6, y+12); glVertex2f(x, y+12);
                glVertex2f(x, y+12); glVertex2f(x, y);
                glVertex2f(x, y+6); glVertex2f(x+6, y+6);
                break;
            case '9':
                glVertex2f(x, y+12); glVertex2f(x+6, y+12);
                glVertex2f(x+6, y+12); glVertex2f(x+6, y);
                glVertex2f(x+6, y); glVertex2f(x, y);
                glVertex2f(x, y); glVertex2f(x, y+6);
                glVertex2f(x, y+6); glVertex2f(x+6, y+6);
                break;
            // Special characters
            case '-':
                glVertex2f(x, y+6); glVertex2f(x+6, y+6);
                break;
            case '_':
                glVertex2f(x, y+12); glVertex2f(x+6, y+12);
                break;
            case '.':
                glVertex2f(x+2, y+11); glVertex2f(x+3, y+11);
                glVertex2f(x+3, y+11); glVertex2f(x+3, y+12);
                glVertex2f(x+3, y+12); glVertex2f(x+2, y+12);
                glVertex2f(x+2, y+12); glVertex2f(x+2, y+11);
                break;
            case ' ':
                // Space - draw nothing
                break;
            default:
                // Unknown character - draw a box
                glVertex2f(x, y); glVertex2f(x+6, y);
                glVertex2f(x+6, y); glVertex2f(x+6, y+12);
                glVertex2f(x+6, y+12); glVertex2f(x, y+12);
                glVertex2f(x, y+12); glVertex2f(x, y);
                break;
        }

        glEnd();
    }

    void SetProjectInfo(const char* name, int trackCount) {
        fProjectName = name;
        fTrackCount = trackCount;
    }

    void DrawProjectInfo() {
        // Only draw track name if hovering over a track
        if (fHoveredTrackIndex < 0 || fHoveredTrackIndex >= (int)fSources.size())
            return;

        const AudioSource& hoveredSource = fSources[fHoveredTrackIndex];

        // Switch to 2D overlay mode
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();

        BRect bounds = Bounds();
        glOrtho(0, bounds.Width(), bounds.Height(), 0, -1, 1);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glDisable(GL_DEPTH_TEST);

        // Calculate text size (approximate)
        const char* trackName = hoveredSource.name.String();
        int textLen = strlen(trackName);
        float textWidth = textLen * 8.0f + 20.0f;  // Approximate character width
        float boxHeight = 30.0f;

        // Position near mouse cursor
        float boxX = fMouseX + 15.0f;
        float boxY = fMouseY - 15.0f;

        // Keep box within screen bounds
        if (boxX + textWidth > bounds.Width() - 10)
            boxX = bounds.Width() - textWidth - 10;
        if (boxY < 10)
            boxY = 10;
        if (boxY + boxHeight > bounds.Height() - 10)
            boxY = bounds.Height() - boxHeight - 10;

        // Draw semi-transparent background
        glColor4f(0.0f, 0.0f, 0.0f, 0.9f);
        glBegin(GL_QUADS);
        glVertex2f(boxX, boxY);
        glVertex2f(boxX + textWidth, boxY);
        glVertex2f(boxX + textWidth, boxY + boxHeight);
        glVertex2f(boxX, boxY + boxHeight);
        glEnd();

        // Draw colored border
        glLineWidth(2.0f);
        glColor3ub(hoveredSource.color.red, hoveredSource.color.green, hoveredSource.color.blue);
        glBegin(GL_LINE_LOOP);
        glVertex2f(boxX, boxY);
        glVertex2f(boxX + textWidth, boxY);
        glVertex2f(boxX + textWidth, boxY + boxHeight);
        glVertex2f(boxX, boxY + boxHeight);
        glEnd();

        // Draw text using simple bitmap font
        glColor3f(1.0f, 1.0f, 1.0f);
        float charX = boxX + 10.0f;
        float charY = boxY + 10.0f;

        for (int i = 0; trackName[i] != '\0'; i++) {
            DrawChar(charX, charY, trackName[i]);
            charX += 8.0f;
        }

        glEnable(GL_DEPTH_TEST);

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }

private:
    std::vector<AudioSource> fSources;
    float fRotationY;
    float fZoom;
    BString fProjectName;
    int fTrackCount;
    bool fAutoRotate;
    float fMouseX;
    float fMouseY;
    int fHoveredTrackIndex;
};

// ============================================================================
// MODERN TIMELINE VIEW - Professional DAW-style Timeline
// ============================================================================

// Time Ruler View - Shows time and beat markers
class TimeRulerView : public BView {
public:
    TimeRulerView(BRect frame, const VeniceDAW::Project3DMix& project, float pixelsPerSecond)
        : BView(frame, "time_ruler", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW)
        , fProject(project)
        , fPixelsPerSecond(pixelsPerSecond)
    {
        SetViewColor(45, 45, 50);
    }

    void SetPixelsPerSecond(float pps) {
        fPixelsPerSecond = pps;
        Invalidate();
    }

    virtual void Draw(BRect updateRect) override {
        BRect bounds = Bounds();

        // Background
        SetHighColor(45, 45, 50);
        FillRect(bounds);

        // Time markers
        float duration = fProject.CalculateTotalDuration();
        float width = bounds.Width();

        SetFont(be_plain_font);
        font_height fh;
        be_plain_font->GetHeight(&fh);

        // Draw major time markers (every second or appropriate interval)
        float interval = 1.0f;  // 1 second
        if (fPixelsPerSecond < 20) interval = 5.0f;
        if (fPixelsPerSecond < 10) interval = 10.0f;

        SetHighColor(180, 180, 185);
        for (float t = 0; t <= duration; t += interval) {
            float x = t * fPixelsPerSecond;
            if (x > width) break;

            // Draw tick mark
            StrokeLine(BPoint(x, bounds.bottom - 15), BPoint(x, bounds.bottom));

            // Draw time label
            int mins = (int)(t / 60);
            int secs = (int)t % 60;
            BString timeStr;
            timeStr << mins << ":" << (secs < 10 ? "0" : "") << secs;

            SetHighColor(200, 200, 205);
            DrawString(timeStr.String(), BPoint(x + 3, bounds.bottom - 3));
        }

        // Draw beat markers
        float bpm = fProject.TimelineBeatPM();
        int beatsPerMeasure = fProject.TimelineBeatPerMeasure();

        if (bpm > 0) {
            float secondsPerBeat = 60.0f / bpm;

            for (int beat = 0; beat * secondsPerBeat <= duration; beat++) {
                float beatTime = beat * secondsPerBeat;
                float x = beatTime * fPixelsPerSecond;
                if (x > width) break;

                bool isMeasure = (beat % beatsPerMeasure) == 0;

                if (isMeasure) {
                    // Measure marker - taller
                    SetHighColor(120, 140, 255);
                    StrokeLine(BPoint(x, bounds.bottom - 25), BPoint(x, bounds.bottom - 15));
                } else {
                    // Beat marker - shorter
                    SetHighColor(80, 100, 180);
                    StrokeLine(BPoint(x, bounds.bottom - 20), BPoint(x, bounds.bottom - 15));
                }
            }
        }

        // Bottom border
        SetHighColor(30, 30, 35);
        StrokeLine(BPoint(0, bounds.bottom), BPoint(bounds.right, bounds.bottom));
    }

private:
    const VeniceDAW::Project3DMix& fProject;
    float fPixelsPerSecond;
};

// Track Lane View - Shows individual tracks as horizontal lanes
class TrackLanesView : public BView {
public:
    TrackLanesView(BRect frame, const VeniceDAW::Project3DMix& project, float pixelsPerSecond)
        : BView(frame, "track_lanes", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW)
        , fProject(project)
        , fPixelsPerSecond(pixelsPerSecond)
        , fPlayheadPosition(0.0f)
    {
        SetViewColor(35, 35, 40);
    }

    void SetPixelsPerSecond(float pps) {
        fPixelsPerSecond = pps;
        Invalidate();
    }

    void SetPlayheadPosition(float seconds) {
        fPlayheadPosition = seconds;
        Invalidate();
    }

    virtual void Draw(BRect updateRect) override {
        BRect bounds = Bounds();

        // Background
        SetHighColor(35, 35, 40);
        FillRect(bounds);

        int trackCount = fProject.CountTracks();
        float laneHeight = 60.0f;
        float trackNameWidth = 150.0f;

        SetFont(be_plain_font);
        font_height fh;
        be_plain_font->GetHeight(&fh);

        // Draw each track lane
        for (int i = 0; i < trackCount; i++) {
            VeniceDAW::Track3DMix* track = fProject.TrackAt(i);
            if (!track) continue;

            float y = i * laneHeight;
            BRect laneRect(0, y, bounds.right, y + laneHeight - 1);

            // Alternating lane background
            if (i % 2 == 0) {
                SetHighColor(40, 40, 45);
            } else {
                SetHighColor(35, 35, 40);
            }
            FillRect(laneRect);

            // Track name area background
            SetHighColor(50, 50, 55);
            FillRect(BRect(0, y, trackNameWidth, y + laneHeight - 1));

            // Track name
            SetHighColor(200, 200, 205);
            BString trackName = track->TrackName();
            if (trackName.Length() == 0) {
                trackName = "Track ";
                trackName << (i + 1);
            }

            // Truncate if too long
            if (be_plain_font->StringWidth(trackName.String()) > trackNameWidth - 10) {
                BString testStr;
                do {
                    trackName.Truncate(trackName.Length() - 1);
                    testStr = trackName;
                    testStr << "...";
                } while (be_plain_font->StringWidth(testStr.String()) > trackNameWidth - 10 && trackName.Length() > 3);
                trackName = testStr;
            }

            DrawString(trackName.String(), BPoint(10, y + (laneHeight / 2) + (fh.ascent / 2)));

            // Track color indicator from 3D position
            VeniceDAW::Coordinate3D pos = track->Position();
            rgb_color trackColor;
            trackColor.red = 100 + (int)(fabs(pos.x) * 10) % 155;
            trackColor.green = 100 + (int)(fabs(pos.y) * 10) % 155;
            trackColor.blue = 100 + (int)(fabs(pos.z) * 10) % 155;
            trackColor.alpha = 255;

            SetHighColor(trackColor);
            FillRect(BRect(5, y + 5, 8, y + laneHeight - 6));

            // Track region (audio block representation)
            // Each track has its own start/end position in samples
            const VeniceDAW::AudioFormat3DMix& format = track->GetAudioFormat();
            int32 startSample = track->StartPosition();
            int32 endSample = track->EndPosition();

            // Convert samples to seconds
            float sampleRate = format.sampleRate > 0 ? format.sampleRate : 44100.0f;
            float startTime = startSample / sampleRate;
            float endTime = endSample / sampleRate;

            // If no explicit end position, calculate from file size
            if (endSample == 0 && format.fileSize > 0 && format.channels > 0 && format.bitDepth > 0) {
                int32 bytesPerSample = (format.bitDepth + 7) / 8;
                int32 totalSamples = format.fileSize / (format.channels * bytesPerSample);
                endTime = startTime + (totalSamples / sampleRate);
            }

            // If still no end time, use project duration as fallback
            if (endTime <= startTime) {
                endTime = fProject.CalculateTotalDuration();
            }

            float trackDuration = endTime - startTime;

            // Debug logging on first track only
            if (i == 0) {
                printf("[TrackLanes] Track '%s': start=%.2fs, end=%.2fs, duration=%.2fs, BlockWidth=%.1fpx\n",
                       track->TrackName().String(), startTime, endTime, trackDuration, trackDuration * fPixelsPerSecond);
            }

            if (trackDuration > 0) {
                // Convert times to horizontal positions
                float startX = trackNameWidth + 10 + (startTime * fPixelsPerSecond);
                float endX = trackNameWidth + 10 + (endTime * fPixelsPerSecond);
                BRect blockRect(startX, y + 5, endX, y + laneHeight - 6);

                // Audio block - BRIGHTER colors for visibility
                SetHighColor(trackColor.red * 0.6, trackColor.green * 0.6, trackColor.blue * 0.6);
                FillRect(blockRect);

                // Audio block border - BRIGHT
                SetHighColor(trackColor.red * 1.0, trackColor.green * 1.0, trackColor.blue * 1.0);
                StrokeRect(blockRect);

                // Simplified waveform visualization - BRIGHTER
                SetHighColor(trackColor.red * 0.8, trackColor.green * 0.8, trackColor.blue * 0.8, 200);
                float centerY = y + laneHeight / 2;
                for (float x = blockRect.left; x < blockRect.right && x < bounds.right; x += 4) {
                    float waveHeight = 10 + (int)(x * 137) % 15;  // Pseudo-random
                    StrokeLine(BPoint(x, centerY - waveHeight), BPoint(x, centerY + waveHeight));
                }
            } else {
                // If no duration, draw a placeholder block
                BRect placeholderRect(trackNameWidth + 10, y + 5, trackNameWidth + 500, y + laneHeight - 6);
                SetHighColor(80, 80, 90);
                FillRect(placeholderRect);
                SetHighColor(120, 120, 130);
                StrokeRect(placeholderRect);
            }

            // Lane separator
            SetHighColor(25, 25, 30);
            StrokeLine(BPoint(0, y + laneHeight - 1), BPoint(bounds.right, y + laneHeight - 1));

            // Track name separator
            SetHighColor(30, 30, 35);
            StrokeLine(BPoint(trackNameWidth, y), BPoint(trackNameWidth, y + laneHeight - 1));
        }

        // Draw playhead (red vertical line)
        if (fPlayheadPosition >= 0) {
            float playheadX = 150 + fPlayheadPosition * fPixelsPerSecond;

            SetHighColor(255, 60, 60);
            SetPenSize(2.0);
            StrokeLine(BPoint(playheadX, 0), BPoint(playheadX, bounds.bottom));
            SetPenSize(1.0);

            // Playhead triangle at top
            SetHighColor(255, 60, 60);
            BPoint tri[3];
            tri[0] = BPoint(playheadX, 0);
            tri[1] = BPoint(playheadX - 6, 10);
            tri[2] = BPoint(playheadX + 6, 10);
            FillPolygon(tri, 3);
        }
    }

private:
    const VeniceDAW::Project3DMix& fProject;
    float fPixelsPerSecond;
    float fPlayheadPosition;
};

// Main Timeline Content View - Container for ruler and lanes
class TimelineContentView : public BView {
public:
    TimelineContentView(BRect frame, const VeniceDAW::Project3DMix& project)
        : BView(frame, "timeline_content", B_FOLLOW_ALL, B_WILL_DRAW)
        , fProject(project)
        , fPixelsPerSecond(50.0f)  // Initial zoom
        , fPlayheadPosition(0.0f)
        , fIsPlaying(false)
    {
        SetViewColor(30, 30, 35);

        // Create ruler view
        BRect rulerFrame = Bounds();
        rulerFrame.bottom = 40;
        fRulerView = new TimeRulerView(rulerFrame, project, fPixelsPerSecond);
        AddChild(fRulerView);

        // Create track lanes view
        BRect lanesFrame = Bounds();
        lanesFrame.top = 41;
        fLanesView = new TrackLanesView(lanesFrame, project, fPixelsPerSecond);
        AddChild(fLanesView);
    }

    void ZoomIn() {
        fPixelsPerSecond *= 1.5f;
        if (fPixelsPerSecond > 200.0f) fPixelsPerSecond = 200.0f;
        UpdateZoom();
    }

    void ZoomOut() {
        fPixelsPerSecond /= 1.5f;
        if (fPixelsPerSecond < 10.0f) fPixelsPerSecond = 10.0f;
        UpdateZoom();
    }

    void UpdateZoom() {
        fRulerView->SetPixelsPerSecond(fPixelsPerSecond);
        fLanesView->SetPixelsPerSecond(fPixelsPerSecond);
    }

    void TogglePlayback() {
        fIsPlaying = !fIsPlaying;
        // Note: This is visualization only, no actual audio playback

        // Update window title to show play state
        BWindow* window = Window();
        if (window) {
            BString title = "Timeline View - Modern DAW Style";
            if (fIsPlaying) {
                title << " [PLAYING]";
            } else {
                title << " [PAUSED]";
            }
            window->SetTitle(title.String());
        }

        printf("[Timeline] Playback %s\n", fIsPlaying ? "STARTED" : "PAUSED");
    }

    void UpdatePlayhead(float deltaSeconds) {
        if (fIsPlaying) {
            fPlayheadPosition += deltaSeconds;
            float duration = fProject.CalculateTotalDuration();
            if (fPlayheadPosition > duration) {
                fPlayheadPosition = 0.0f;  // Loop
            }
            fLanesView->SetPlayheadPosition(fPlayheadPosition);
        }
    }

    void ResetPlayhead() {
        fPlayheadPosition = 0.0f;
        fLanesView->SetPlayheadPosition(fPlayheadPosition);
    }

    virtual void AttachedToWindow() override {
        BView::AttachedToWindow();
        MakeFocus(true);
    }

    virtual void KeyDown(const char* bytes, int32 numBytes) override {
        char key = bytes[0];

        printf("[Timeline] KeyDown: key='%c' (0x%02x)\n", (key >= 32 && key < 127) ? key : '?', (unsigned char)key);

        switch (key) {
            case '+':
            case '=':
                printf("[Timeline] Zoom IN\n");
                ZoomIn();
                break;
            case '-':
            case '_':
                printf("[Timeline] Zoom OUT\n");
                ZoomOut();
                break;
            case ' ':  // Spacebar
                printf("[Timeline] SPACEBAR detected - toggling playback\n");
                TogglePlayback();
                break;
            case 'r':
            case 'R':
                printf("[Timeline] Reset playhead\n");
                ResetPlayhead();
                break;
            default:
                BView::KeyDown(bytes, numBytes);
                break;
        }
    }

private:
    const VeniceDAW::Project3DMix& fProject;
    TimeRulerView* fRulerView;
    TrackLanesView* fLanesView;
    float fPixelsPerSecond;
    float fPlayheadPosition;
    bool fIsPlaying;
};

// Timeline visualization window
class TimelineWindow : public BWindow {
public:
    TimelineWindow(const VeniceDAW::Project3DMix& project)
        : BWindow(BRect(100, 100, 1400, 850), "Timeline View - Modern DAW Style",
                  B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
        , fProject(project)
        , fContentView(nullptr)
        , fUpdateRunner(nullptr)
    {
        fContentView = new TimelineContentView(Bounds(), project);
        AddChild(fContentView);

        // Create animation timer (30 FPS)
        BMessage pulse('Tpls');
        fUpdateRunner = new BMessageRunner(this, &pulse, 33333);  // 33ms = ~30 FPS
    }

    ~TimelineWindow() {
        delete fUpdateRunner;
    }

    virtual void MessageReceived(BMessage* message) override {
        switch (message->what) {
            case 'Tpls':  // Timeline pulse
                if (fContentView) {
                    fContentView->UpdatePlayhead(0.033f);  // 33ms in seconds
                }
                break;
            default:
                BWindow::MessageReceived(message);
                break;
        }
    }

    virtual bool QuitRequested() override {
        // Don't quit the app, just hide this window
        Hide();
        return false;
    }

private:
    const VeniceDAW::Project3DMix& fProject;
    TimelineContentView* fContentView;
    BMessageRunner* fUpdateRunner;
};

class DemoWindow : public BWindow {
public:
    DemoWindow(const char* projectPath)
        : BWindow(BRect(100, 100, 900, 700), "3DMix Visualization Demo - Educational Only",
                  B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
        , fGLView(nullptr)
        , fUpdateRunner(nullptr)
        , fTimelineWindow(nullptr)
        , fProject(nullptr)
    {
        // Create GL view
        BRect bounds = Bounds();
        fGLView = new DemoGL3DView(bounds, "gl_view");
        AddChild(fGLView);

        // Load and parse 3dmix file
        LoadProject(projectPath);

        // Start animation
        BMessage pulse(MSG_PULSE);
        fUpdateRunner = new BMessageRunner(this, &pulse, 33333);  // 30 FPS
    }

    ~DemoWindow() {
        delete fUpdateRunner;

        // Properly close TimelineWindow if it exists
        if (fTimelineWindow && fTimelineWindow->Lock()) {
            fTimelineWindow->Quit();
            // Don't delete - Quit() will handle cleanup
        }

        delete fProject;
    }

    void LoadProject(const char* path) {
        printf("Loading 3DMix project: %s\n", path);

        VeniceDAW::Legacy3DMixLoader loader;

        if (loader.LoadProject(path) != B_OK) {
            printf("Failed to parse 3DMix file: %s\n", loader.GetLastError().String());
            return;
        }

        // Save project for timeline window
        fProject = loader.DetachProject();
        if (!fProject) {
            printf("Failed to get project data\n");
            return;
        }

        const VeniceDAW::Project3DMix& project = *fProject;

        printf("Project: %s\n", project.ProjectName().String());
        printf("Tracks: %d\n", (int)project.CountTracks());

        // Set project info for display in 3D view
        fGLView->SetProjectInfo(project.ProjectName().String(), project.CountTracks());

        // Update window title
        char windowTitle[512];
        snprintf(windowTitle, sizeof(windowTitle), "3DMix Viewer - %s (%d tracks)",
                 project.ProjectName().String(), project.CountTracks());
        SetTitle(windowTitle);

        printf("\n3D Audio Sources (Legend - match #numbers in 3D view):\n");
        printf("#   %-20s X      Y      Z      Color (RGB)\n", "Track Name");
        printf("----------------------------------------------------------------\n");

        for (int32 i = 0; i < project.CountTracks(); i++) {
            VeniceDAW::Track3DMix* track = project.TrackAt(i);
            if (!track) continue;

            const VeniceDAW::Coordinate3D& pos = track->Position();

            // Add to GL view first to get the color
            fGLView->AddAudioSource(track->TrackName().String(),
                                    pos.x, pos.y, pos.z);

            // Get the color that was assigned (hash-based)
            const char* name = track->TrackName().String();
            int hash = 0;
            for (int j = 0; name[j]; j++) hash += name[j];
            int r = 100 + (hash * 17) % 155;
            int g = 100 + (hash * 31) % 155;
            int b = 100 + (hash * 47) % 155;

            printf("%-3d %-20s %.2f  %.2f  %.2f  RGB(%d,%d,%d)\n",
                   (i + 1),
                   track->TrackName().String(),
                   pos.x, pos.y, pos.z, r, g, b);
        }

        printf("\nVisualization:\n");
        printf("- Yellow disk at center = Listener (you)\n");
        printf("- Colored spheres on poles = Audio sources\n");
        printf("- Grid shows the 2D mixing plane\n");
        printf("- Red/Blue axes = X/Z coordinates\n");
        printf("\nControls:\n");
        printf("- Move mouse over a track to see its name\n");
        printf("- Press + to zoom in, - to zoom out\n");
        printf("- Press T to open Timeline window\n");
        printf("- View is fixed for easy track identification\n");
    }

    void OpenTimelineWindow() {
        if (!fProject) {
            printf("No project loaded\n");
            return;
        }

        if (!fTimelineWindow) {
            fTimelineWindow = new TimelineWindow(*fProject);
        }

        if (fTimelineWindow->IsHidden()) {
            fTimelineWindow->Show();
        } else {
            fTimelineWindow->Activate();
        }
    }

    virtual void MessageReceived(BMessage* message) override {
        switch (message->what) {
            case MSG_PULSE:
                if (fGLView) {
                    fGLView->Pulse();
                }
                break;
            case B_KEY_DOWN: {
                const char* bytes;
                if (message->FindString("bytes", &bytes) == B_OK) {
                    char key = bytes[0];
                    if (key == 't' || key == 'T') {
                        OpenTimelineWindow();
                        return;
                    }
                }
                break;
            }
            default:
                BWindow::MessageReceived(message);
        }
    }

    virtual bool QuitRequested() override {
        be_app->PostMessage(B_QUIT_REQUESTED);
        return true;
    }

private:
    static const uint32 MSG_PULSE = 'puls';
    DemoGL3DView* fGLView;
    BMessageRunner* fUpdateRunner;
    TimelineWindow* fTimelineWindow;
    VeniceDAW::Project3DMix* fProject;
};

class DemoApp : public BApplication {
public:
    DemoApp(const char* projectPath)
        : BApplication("application/x-vnd.VeniceDAW-3DMixDemo")
    {
        fWindow = new DemoWindow(projectPath);
        fWindow->Show();
    }

private:
    DemoWindow* fWindow;
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("3DMix Visualization Demo - Educational Tool\n");
        printf("============================================\n\n");
        printf("Usage: %s <path-to-3dmix-file>\n\n", argv[0]);
        printf("Example:\n");
        printf("  %s /boot/home/Desktop/3D_Mixes/she-loves-it/she-loves-it-3dmix/she-loves-it.3dmix\n\n", argv[0]);
        printf("This is a DIDACTIC DEMONSTRATION that shows 3D spatial audio positioning\n");
        printf("WITHOUT playing any sound. Perfect for learning about spatial audio!\n");
        return 1;
    }

    DemoApp app(argv[1]);
    app.Run();
    return 0;
}

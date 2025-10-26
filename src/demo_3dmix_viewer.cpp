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
        , fRotationY(0.0f)
        , fZoom(-15.0f)
    {
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

        // Camera setup
        glTranslatef(0.0f, 0.0f, fZoom);
        glRotatef(fRotationY, 0.0f, 1.0f, 0.0f);
        glRotatef(-20.0f, 1.0f, 0.0f, 0.0f);  // Tilt for better view

        // Draw coordinate axes
        DrawAxes();

        // Draw listener (center)
        DrawListener();

        // Draw audio sources
        for (const auto& source : fSources) {
            DrawAudioSource(source);
        }

        SwapBuffers();
        UnlockGL();
    }

    void DrawAxes() {
        glLineWidth(2.0f);
        glBegin(GL_LINES);

        // X axis - Red
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(-5.0f, 0.0f, 0.0f);
        glVertex3f(5.0f, 0.0f, 0.0f);

        // Y axis - Green
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, -5.0f, 0.0f);
        glVertex3f(0.0f, 5.0f, 0.0f);

        // Z axis - Blue
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, -5.0f);
        glVertex3f(0.0f, 0.0f, 5.0f);

        glEnd();
    }

    void DrawListener() {
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.0f);

        // Draw head as yellow sphere
        glColor3f(1.0f, 1.0f, 0.0f);
        GLUquadric* quad = gluNewQuadric();
        gluSphere(quad, 0.3, 16, 16);
        gluDeleteQuadric(quad);

        glPopMatrix();
    }

    void DrawAudioSource(const AudioSource& source) {
        glPushMatrix();
        glTranslatef(source.x, source.y, source.z);

        // Draw sphere with source color
        glColor3ub(source.color.red, source.color.green, source.color.blue);
        GLUquadric* quad = gluNewQuadric();
        gluSphere(quad, 0.2, 12, 12);
        gluDeleteQuadric(quad);

        // Draw line to center
        glLineWidth(1.0f);
        glColor4f(source.color.red / 255.0f,
                  source.color.green / 255.0f,
                  source.color.blue / 255.0f, 0.3f);
        glBegin(GL_LINES);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(-source.x, -source.y, -source.z);
        glEnd();

        glPopMatrix();
    }

    void Pulse() {
        fRotationY += 0.5f;
        if (fRotationY >= 360.0f) fRotationY -= 360.0f;
        Draw(Bounds());
    }

    void ZoomIn() { fZoom += 1.0f; if (fZoom > -2.0f) fZoom = -2.0f; }
    void ZoomOut() { fZoom -= 1.0f; if (fZoom < -30.0f) fZoom = -30.0f; }

private:
    std::vector<AudioSource> fSources;
    float fRotationY;
    float fZoom;
};

class DemoWindow : public BWindow {
public:
    DemoWindow(const char* projectPath)
        : BWindow(BRect(100, 100, 900, 700), "3DMix Visualization Demo - Educational Only",
                  B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
        , fGLView(nullptr)
        , fUpdateRunner(nullptr)
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
    }

    void LoadProject(const char* path) {
        printf("Loading 3DMix project: %s\n", path);

        VeniceDAW::Legacy3DMixLoader loader;

        if (loader.LoadProject(path) != B_OK) {
            printf("Failed to parse 3DMix file: %s\n", loader.GetLastError().String());
            return;
        }

        const VeniceDAW::Project3DMix& project = loader.GetProject();

        printf("Project: %s\n", project.ProjectName().String());
        printf("Tracks: %d\n", (int)project.CountTracks());
        printf("\n3D Audio Sources:\n");
        printf("%-20s X      Y      Z\n", "Track Name");
        printf("----------------------------------------\n");

        for (int32 i = 0; i < project.CountTracks(); i++) {
            VeniceDAW::Track3DMix* track = project.TrackAt(i);
            if (!track) continue;

            const VeniceDAW::Coordinate3D& pos = track->Position();
            printf("%-20s %.2f  %.2f  %.2f\n",
                   track->TrackName().String(),
                   pos.x, pos.y, pos.z);

            fGLView->AddAudioSource(track->TrackName().String(),
                                    pos.x, pos.y, pos.z);
        }

        printf("\nVisualization:\n");
        printf("- Yellow sphere = Listener (you)\n");
        printf("- Colored spheres = Audio sources\n");
        printf("- Lines show spatial relationships\n");
        printf("- Scene rotates automatically\n");
        printf("\nPress + to zoom in, - to zoom out\n");
    }

    virtual void MessageReceived(BMessage* message) override {
        switch (message->what) {
            case MSG_PULSE:
                if (fGLView) {
                    fGLView->Pulse();
                }
                break;
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

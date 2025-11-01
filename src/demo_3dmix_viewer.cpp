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
#include <map>
#include <Bitmap.h>
#include <View.h>
#include <Font.h>
#include <Button.h>

// Include our 3dmix parser
#include "audio/3dmix/3DMixFormat.h"
#include "audio/3dmix/3DMixParser.h"
#include "audio/3dmix/AudioPathResolver.h"
#include <MediaFile.h>
#include <SoundPlayer.h>
#include <MediaDefs.h>
#include <MediaTrack.h>
#include <Path.h>
#include <Entry.h>

struct AudioSource {
    BString name;
    float x, y, z;  // 3D position
    rgb_color color;
};

// Audio sample cache - stores ALL audio samples (loaded once)
// Inspired by R6's SampleCache with on-the-fly GetSample() method
struct AudioSampleCache {
    std::vector<float> samples;      // All audio samples in memory
    float sampleRate;                // Sample rate (e.g., 44100)
    int channels;                    // Number of channels
    bool isValid;

    AudioSampleCache() : sampleRate(0.0f), channels(1), isValid(false) {}

    // R6-style GetSample: Calculate min/max ON-THE-FLY for a time range
    // This is called once per pixel during rendering
    void GetSample(float time, float duration, float* outMin, float* outMax) const {
        *outMin = 0.0f;
        *outMax = 0.0f;

        if (!isValid || samples.empty() || time < 0) {
            return;
        }

        // Convert time to sample indices
        int startIdx = (int)(time * sampleRate);
        int numSamples = (int)(duration * sampleRate);

        if (numSamples == 0) numSamples = 1;
        if (startIdx >= (int)samples.size()) return;

        int endIdx = startIdx + numSamples;
        if (endIdx > (int)samples.size()) {
            endIdx = samples.size();
        }

        // Adaptive step like R6: step = 1 + (numSamples >> 1) for fast display
        // This skips samples when zoomed out (many samples per pixel)
        int step = 1 + (numSamples >> 1);  // Same as R6's fast_display mode
        if (step < 1) step = 1;

        float minVal = 0.0f;
        float maxVal = 0.0f;

        // Scan only this pixel's range
        for (int i = startIdx; i < endIdx && i < (int)samples.size(); i += step) {
            float v = samples[i];
            if (v < minVal) minVal = v;
            if (v > maxVal) maxVal = v;
        }

        *outMin = minVal;
        *outMax = maxVal;
    }
};

// Waveform cache - R6 approach: only cache samples, calculate peaks on-the-fly
class WaveformCache {
public:
    static WaveformCache& Instance() {
        static WaveformCache instance;
        return instance;
    }

    // Get audio sample cache (loads once, then returns cached version)
    // Rendering will call GetSample() on the returned cache
    const AudioSampleCache* GetAudioCache(const char* filePath, const VeniceDAW::AudioFormat3DMix* rawFormat = nullptr) {
        // Check if already loaded
        std::string key(filePath);
        auto it = fSamplesCache.find(key);
        if (it != fSamplesCache.end()) {
            return &it->second;  // Already loaded!
        }

        // Load audio samples ONCE
        AudioSampleCache samples = LoadAudioSamples(filePath, rawFormat);
        fSamplesCache[key] = samples;
        return &fSamplesCache[key];
    }

private:
    WaveformCache() {}

    // Load ALL audio samples once (zoom-independent!)
    AudioSampleCache LoadAudioSamples(const char* filePath, const VeniceDAW::AudioFormat3DMix* rawFormat) {
        AudioSampleCache cache;

        printf("[AudioCache] Loading audio samples: '%s'\n", filePath);

        // Try BMediaFile first
        entry_ref ref;
        if (get_ref_for_path(filePath, &ref) == B_OK) {
            BMediaFile mediaFile(&ref);
            if (mediaFile.InitCheck() == B_OK) {
                // Get first audio track
                BMediaTrack* track = nullptr;
                for (int32 i = 0; i < mediaFile.CountTracks(); i++) {
                    BMediaTrack* t = mediaFile.TrackAt(i);
                    media_format format;
                    if (t && t->EncodedFormat(&format) == B_OK) {
                        if (format.type == B_MEDIA_RAW_AUDIO || format.type == B_MEDIA_ENCODED_AUDIO) {
                            track = t;
                            break;
                        }
                    }
                    if (t && !track) {
                        mediaFile.ReleaseTrack(t);
                    }
                }

                if (track) {
                    // Decode to raw audio
                    media_format format;
                    format.type = B_MEDIA_RAW_AUDIO;
                    format.u.raw_audio = media_raw_audio_format::wildcard;
                    format.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
                    format.u.raw_audio.channel_count = 2;

                    if (track->DecodedFormat(&format) == B_OK) {
                        int64 frameCount = track->CountFrames();
                        if (frameCount > 0) {
                            cache.sampleRate = format.u.raw_audio.frame_rate;
                            cache.channels = format.u.raw_audio.channel_count;

                            // OPTIMIZATION: For very long files, decimate during loading
                            // to reduce memory and improve performance
                            int decimation = 1;
                            if (frameCount > 5000000) decimation = 2;  // >2min: keep every 2nd sample
                            if (frameCount > 10000000) decimation = 4; // >4min: keep every 4th sample

                            cache.samples.reserve(frameCount / decimation);

                            // Read samples with optional decimation
                            const int bufferSize = 8192;
                            float buffer[bufferSize * 2];
                            int64 framesRead = 0;
                            int decimationCounter = 0;

                            while (framesRead < frameCount) {
                                int64 frames = 0;
                                if (track->ReadFrames(buffer, &frames) != B_OK || frames == 0) {
                                    break;
                                }

                                // Convert stereo to mono and store (with decimation)
                                for (int64 i = 0; i < frames; i++) {
                                    if (decimationCounter++ % decimation == 0) {
                                        float mono = (buffer[i * 2] + buffer[i * 2 + 1]) * 0.5f;
                                        cache.samples.push_back(mono);
                                    }
                                }

                                framesRead += frames;
                            }

                            // Adjust sample rate if decimated
                            if (decimation > 1) {
                                cache.sampleRate /= decimation;
                            }

                            mediaFile.ReleaseTrack(track);
                            cache.isValid = true;
                            printf("[AudioCache] ✓ Loaded %zu samples at %.0f Hz\n",
                                   cache.samples.size(), cache.sampleRate);
                            return cache;
                        }
                    }
                    mediaFile.ReleaseTrack(track);
                }
            }
        }

        // Try RAW format if BMediaFile failed and we have format info
        if (!cache.isValid && rawFormat && rawFormat->isRawFormat) {
            cache = LoadRawAudioSamples(filePath, *rawFormat);
        }

        return cache;
    }

    AudioSampleCache LoadRawAudioSamples(const char* filePath, const VeniceDAW::AudioFormat3DMix& format) {
        AudioSampleCache cache;

        printf("[AudioCache] Loading RAW PCM: '%s'\n", filePath);
        printf("[AudioCache] RAW format from track: bitDepth=%d, channels=%d, sampleRate=%d\n",
               format.bitDepth, format.channels, format.sampleRate);

        // Initialize sample rate from format if available
        if (format.sampleRate > 0) {
            cache.sampleRate = (float)format.sampleRate;
            printf("[AudioCache] Using sample rate from track format: %.0f Hz\n", cache.sampleRate);
        }

        BFile file(filePath, B_READ_ONLY);
        if (file.InitCheck() != B_OK) {
            return cache;
        }

        off_t fileSize;
        if (file.GetSize(&fileSize) != B_OK || fileSize == 0) {
            return cache;
        }

        // Check for BeOS Track Object header (!TRK marker)
        off_t headerSkip = 0;
        char marker[5] = {0};
        file.Read(marker, 4);
        if (strcmp(marker, "!TRK") == 0) {
            // This is a BeOS Track Object file with header - skip to audio data
            // The header contains: !TRK, track name, SIMPRAW_, filename, etc.
            // Audio data typically starts around offset 96, but we'll search for it

            // Read more header to find audio start
            uint8 headerBuf[512];
            file.Seek(0, SEEK_SET);
            ssize_t headerRead = file.Read(headerBuf, 512);

            // BeOS Track Object files typically have a 96-byte header
            // Header structure: !TRK, track name, SIMPRAW_, filename, various metadata
            // Let's try to find the sample rate in the header

            // Common sample rates to check for
            float detectedRate = 44100.0f;  // default

            // Scan header for int32 values that match common sample rates
            int32 commonRates[] = {11025, 22050, 44100, 48000, 88200, 96000};
            for (int offset = 4; offset < 90; offset += 4) {
                // Try reading as big-endian int32
                uint8* ptr = headerBuf + offset;
                int32 beValue = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];

                // Check if it matches a common sample rate
                for (int i = 0; i < 6; i++) {
                    if (beValue == commonRates[i]) {
                        detectedRate = (float)beValue;
                        printf("[AudioCache] Found sample rate in header at offset %d: %.0f Hz\n", offset, detectedRate);
                        goto found_rate;
                    }
                }
            }
found_rate:

            headerSkip = 96;
            printf("[AudioCache] Detected BeOS Track Object header, skip=%lld bytes, rate=%.0f Hz\n",
                   headerSkip, detectedRate);

            file.Seek(headerSkip, SEEK_SET);
            fileSize -= headerSkip;

            // Verify we're at the right position - read first few bytes to check
            uint8 verifyBuf[8];
            ssize_t verified = file.Read(verifyBuf, 8);
            if (verified == 8) {
                printf("[AudioCache] First 8 bytes after header: %02x %02x %02x %02x %02x %02x %02x %02x\n",
                       verifyBuf[0], verifyBuf[1], verifyBuf[2], verifyBuf[3],
                       verifyBuf[4], verifyBuf[5], verifyBuf[6], verifyBuf[7]);
                // Seek back to start of audio
                file.Seek(headerSkip, SEEK_SET);
            }

            // BeOS Track Object files were saved with resampled audio at 44100 Hz,
            // but were originally recorded at 22050 Hz. Force half sample rate for correct playback.
            // This matches the original BeOS 3D Mixer behavior where it resampled on load.
            if (detectedRate == 44100.0f) {
                cache.sampleRate = 22050.0f;
                printf("[AudioCache] BeOS Track Object: Using corrected sample rate: %.0f Hz (was detecting %.0f Hz)\n",
                       cache.sampleRate, detectedRate);
            } else if (detectedRate > 0) {
                cache.sampleRate = detectedRate;
            }
        } else {
            // Pure RAW file, start from beginning
            file.Seek(0, SEEK_SET);
        }

        // Calculate total frames
        int32 bytesPerSample = (format.bitDepth + 7) / 8;
        int32 bytesPerFrame = bytesPerSample * format.channels;
        int64 totalFrames = fileSize / bytesPerFrame;

        printf("[AudioCache] RAW calculation: fileSize=%lld, bytesPerFrame=%d, totalFrames=%lld\n",
               fileSize, bytesPerFrame, totalFrames);

        if (totalFrames <= 0) {
            return cache;
        }

        // Use detected sample rate from header if available, otherwise use format or default
        if (cache.sampleRate == 0) {
            cache.sampleRate = format.sampleRate > 0 ? format.sampleRate : 44100.0f;
        }
        cache.channels = format.channels;
        cache.samples.reserve(totalFrames);

        // Read ALL samples in large chunks
        const int chunkFrames = 131072;  // 128K frames per chunk
        const int chunkSize = chunkFrames * bytesPerFrame;
        uint8* chunkBuffer = new uint8[chunkSize];

        while (cache.samples.size() < (size_t)totalFrames) {
            ssize_t bytesRead = file.Read(chunkBuffer, chunkSize);
            if (bytesRead <= 0) break;

            int framesInChunk = bytesRead / bytesPerFrame;

            for (int i = 0; i < framesInChunk; i++) {
                uint8* frameData = chunkBuffer + (i * bytesPerFrame);

                // Convert to mono float
                float sample = 0.0f;
                if (format.bitDepth == 16) {
                    // BeOS Track Object files are BIG-ENDIAN!
                    // Need to swap bytes on little-endian Haiku
                    int16* samples = (int16*)frameData;

                    // Debug: log first few raw values (BEFORE swap)
                    if (cache.samples.size() < 5) {
                        printf("[AudioCache] Sample %zu: raw int16 values (LE) = [", cache.samples.size());
                        for (int ch = 0; ch < format.channels; ch++) {
                            printf("%d%s", samples[ch], ch < format.channels-1 ? ", " : "");
                        }
                        printf("]\n");
                    }

                    for (int ch = 0; ch < format.channels; ch++) {
                        // Swap bytes for big-endian to little-endian conversion
                        int16 rawValue = samples[ch];
                        int16 swappedValue = ((rawValue & 0xFF) << 8) | ((rawValue >> 8) & 0xFF);

                        if (cache.samples.size() < 5) {
                            printf("[AudioCache] Channel %d: LE=%d, BE(swapped)=%d\n", ch, rawValue, swappedValue);
                        }

                        sample += swappedValue / 32768.0f;
                    }
                } else if (format.bitDepth == 8) {
                    for (int ch = 0; ch < format.channels; ch++) {
                        sample += (frameData[ch] - 128) / 128.0f;
                    }
                } else {
                    // Unknown bit depth - assume 16-bit
                    printf("[AudioCache] WARNING: Unknown bitDepth %d, assuming 16-bit\n", format.bitDepth);
                    int16* samples = (int16*)frameData;
                    for (int ch = 0; ch < format.channels; ch++) {
                        sample += samples[ch] / 32768.0f;
                    }
                }
                sample /= format.channels;

                // Debug: log first few converted values
                if (cache.samples.size() < 5) {
                    printf("[AudioCache] Sample %zu: converted float = %.6f\n", cache.samples.size(), sample);
                }

                cache.samples.push_back(sample);
            }
        }

        delete[] chunkBuffer;
        cache.isValid = true;

        // Calculate expected duration at different sample rates for debugging
        float duration44100 = cache.samples.size() / 44100.0f;
        float duration22050 = cache.samples.size() / 22050.0f;

        printf("[AudioCache] ✓ Loaded %zu RAW samples at %.0f Hz\n",
               cache.samples.size(), cache.sampleRate);
        printf("[AudioCache] ⏱️  Duration test: %.2fs @ 44100Hz OR %.2fs @ 22050Hz\n",
               duration44100, duration22050);
        printf("[AudioCache] 📝 Use stopwatch: play file and measure real duration!\n");

        return cache;
    }

    std::map<std::string, AudioSampleCache> fSamplesCache;  // Samples cached by filePath (loaded once)
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
        glTranslatef(source.x, 0.0f, source.y);  // Map 3dmix Y to OpenGL Z (depth)

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
            gluProject(source.x, 0.8, source.y,  // 0.8 = top of pole, source.y for Z
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
        , fTrackNameWidth(150.0f)  // Match TrackLanesView
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

        // Draw track name area background (darker)
        SetHighColor(50, 50, 55);
        FillRect(BRect(0, 0, fTrackNameWidth, bounds.bottom));

        // Vertical separator between track names and timeline
        SetHighColor(30, 30, 35);
        StrokeLine(BPoint(fTrackNameWidth, 0), BPoint(fTrackNameWidth, bounds.bottom));

        // Time markers start after track name area
        const float timelineStart = fTrackNameWidth + 10.0f;  // Match TrackLanesView offset

        // Calculate actual duration from tracks (like TrackLanesView does)
        float duration = CalculateActualDuration();
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
            float x = timelineStart + (t * fPixelsPerSecond);
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
                float x = timelineStart + (beatTime * fPixelsPerSecond);
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
    float CalculateActualDuration() const {
        // Calculate max duration from all tracks (same logic as TrackLanesView)
        float maxDuration = 0.0f;

        int trackCount = fProject.CountTracks();
        for (int i = 0; i < trackCount; i++) {
            VeniceDAW::Track3DMix* track = fProject.TrackAt(i);
            if (!track) continue;

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

            if (endTime > maxDuration) {
                maxDuration = endTime;
            }
        }

        // Fallback to 30 seconds if no tracks or no valid duration
        return maxDuration > 0 ? maxDuration : 30.0f;
    }

    const VeniceDAW::Project3DMix& fProject;
    float fPixelsPerSecond;
    float fTrackNameWidth;  // Width of track name column
};

// Track Lane View - Shows individual tracks as horizontal lanes
class TrackLanesView : public BView {
public:
    TrackLanesView(BRect frame, const VeniceDAW::Project3DMix& project, float pixelsPerSecond, const char* projectFilePath)
        : BView(frame, "track_lanes", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW)
        , fProject(project)
        , fProjectPath(projectFilePath)
        , fPixelsPerSecond(pixelsPerSecond)
        , fPlayheadPosition(0.0f)
        , fLastPlayheadX(-1.0f)
    {
        SetViewColor(35, 35, 40);
        SetFlags(Flags() | B_FULL_UPDATE_ON_RESIZE);
    }

    void SetPixelsPerSecond(float pps) {
        fPixelsPerSecond = pps;
        fLastPlayheadX = -1.0f;
        Invalidate();
    }

    void SetPlayheadPosition(float seconds) {
        // OPTIMIZATION: Only invalidate playhead area, not entire view!
        const float timelineStart = 160.0f;  // trackNameWidth (150) + offset (10)
        float oldX = timelineStart + fPlayheadPosition * fPixelsPerSecond;
        float newX = timelineStart + seconds * fPixelsPerSecond;

        fPlayheadPosition = seconds;

        BRect bounds = Bounds();

        // Invalidate old playhead position (with margin for anti-aliasing)
        if (fLastPlayheadX >= 0) {
            BRect oldRect(fLastPlayheadX - 10, 0, fLastPlayheadX + 10, bounds.bottom);
            Invalidate(oldRect);
        }

        // Invalidate new playhead position
        BRect newRect(newX - 10, 0, newX + 10, bounds.bottom);
        Invalidate(newRect);

        fLastPlayheadX = newX;
    }

    virtual void Draw(BRect updateRect) override {
        BRect bounds = Bounds();

        // OPTIMIZATION: Detect if we're only updating playhead
        bool playheadOnly = (updateRect.Width() < 30);

        int trackCount = fProject.CountTracks();
        float laneHeight = 60.0f;
        float trackNameWidth = 150.0f;

        SetFont(be_plain_font);
        font_height fh;
        be_plain_font->GetHeight(&fh);

        // ULTRA-SIMPLE MODE: colored blocks only when zoomed WAY out
        // With zoom-independent rendering, we can show waveforms even at very low zoom!
        bool simpleMode = (fPixelsPerSecond < 3.0f);

        // When updating only playhead, we still need to redraw the lanes it crosses
        // but we can skip track names and other static elements
        if (!playheadOnly) {
            // Background for full redraw
            SetHighColor(35, 35, 40);
            FillRect(updateRect);

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

                // Real waveform visualization - R6-style on-the-fly rendering!
                int widthPixels = (int)(blockRect.Width());
                const AudioSampleCache* audioCache = nullptr;

                // Try to get audio file path
                // OPTIMIZATION: Skip waveform loading entirely in simple mode!
                if (!simpleMode) {
                    BString audioPath = track->AudioFilePath();

                    if (audioPath.Length() > 0 && widthPixels > 0) {
                        BString resolvedPath;

                        // Extract just the filename from the path
                        BPath pathObj(audioPath.String());
                        const char* filename = pathObj.Leaf();

                        if (filename) {
                            // Get project directory from .3dmix file path
                            BString projectDir;
                            if (fProjectPath.Length() > 0) {
                                BPath projectPath(fProjectPath.String());
                                BPath parentPath;
                                if (projectPath.GetParent(&parentPath) == B_OK) {
                                    projectDir = parentPath.Path();
                                }
                            }

                            // Try common extensions including no extension
                            const char* extensions[] = { "", ".wav", ".aiff", ".aif", ".raw", ".mp3", ".ogg", nullptr };

                            for (int ext = 0; extensions[ext] != nullptr; ext++) {
                                BString testPath = projectDir;
                                testPath << "/" << filename << extensions[ext];

                                // Check if file exists
                                entry_ref ref;
                                if (get_ref_for_path(testPath.String(), &ref) == B_OK) {
                                    resolvedPath = testPath;
                                    break;
                                }
                            }
                        }

                        if (resolvedPath.Length() > 0) {
                            // R6 approach: Get audio cache (samples loaded once, peaks calculated on-the-fly!)
                            const VeniceDAW::AudioFormat3DMix& audioFormat = track->GetAudioFormat();
                            audioCache = WaveformCache::Instance().GetAudioCache(resolvedPath.String(), &audioFormat);
                        }
                    }
                }

                // ULTRA-FAST: simple colored block when zoomed way out
                if (simpleMode) {
                    SetHighColor(trackColor.red * 0.85, trackColor.green * 0.85, trackColor.blue * 0.85, 180);
                    FillRect(blockRect);
                } else if (audioCache && audioCache->isValid) {
                    // Draw real waveform using R6-style on-the-fly GetSample() (FAST!)
                    float centerY = y + laneHeight / 2;
                    float maxHeight = (laneHeight - 12) * 0.5f;

                    // Adaptive detail
                    int pixelSkip = 1;
                    if (fPixelsPerSecond < 15.0f) pixelSkip = 2;

                    // Calculate time per pixel (like R6's "zoom" parameter)
                    float secondsPerPixel = 1.0f / fPixelsPerSecond;

                    // Count lines for BeginLineArray
                    int lineCount = 0;
                    for (int px = 0; px < widthPixels; px += pixelSkip) {
                        float x = blockRect.left + px;
                        if (x >= bounds.right) break;
                        lineCount++;
                    }

                    if (lineCount > 0) {
                        BeginLineArray(lineCount);
                        rgb_color waveColor = {
                            (uint8)(trackColor.red * 0.9),
                            (uint8)(trackColor.green * 0.9),
                            (uint8)(trackColor.blue * 0.9),
                            220
                        };

                        // R6-style rendering: GetSample() called per pixel ON-THE-FLY!
                        float time = 0.0f;  // Start at beginning of audio
                        for (int px = 0; px < widthPixels; px += pixelSkip) {
                            float x = blockRect.left + px;
                            if (x >= bounds.right) break;

                            // Calculate min/max for THIS pixel's time range (like R6!)
                            float minPeak, maxPeak;
                            audioCache->GetSample(time, secondsPerPixel, &minPeak, &maxPeak);

                            float minY = centerY - (minPeak * maxHeight);
                            float maxY = centerY - (maxPeak * maxHeight);

                            AddLine(BPoint(x, minY), BPoint(x, maxY), waveColor);

                            time += secondsPerPixel;  // Advance time (like R6: time += zoom)
                        }
                        EndLineArray();
                    }
                } else if (!simpleMode) {
                    // Fallback: pseudo-random waveform (only in detailed mode)
                    SetHighColor(trackColor.red * 0.8, trackColor.green * 0.8, trackColor.blue * 0.8, 200);
                    float centerY = y + laneHeight / 2;

                    int lineCount = 0;
                    for (float x = blockRect.left; x < blockRect.right && x < bounds.right; x += 8) {
                        lineCount++;
                    }

                    if (lineCount > 0) {
                        BeginLineArray(lineCount);
                        rgb_color waveColor = {
                            (uint8)(trackColor.red * 0.8),
                            (uint8)(trackColor.green * 0.8),
                            (uint8)(trackColor.blue * 0.8),
                            200
                        };

                        for (float x = blockRect.left; x < blockRect.right && x < bounds.right; x += 8) {
                            float waveHeight = 10 + (int)(x * 137) % 15;
                            AddLine(BPoint(x, centerY - waveHeight), BPoint(x, centerY + waveHeight), waveColor);
                        }
                        EndLineArray();
                    }
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
        } else {
            // PLAYHEAD-ONLY MODE: Redraw just the lanes intersecting updateRect
            // This prevents the playhead from erasing waveforms
            for (int i = 0; i < trackCount; i++) {
                float y = i * laneHeight;
                BRect laneRect(0, y, bounds.right, y + laneHeight - 1);

                // Skip lanes that don't intersect updateRect
                if (!updateRect.Intersects(laneRect)) continue;

                VeniceDAW::Track3DMix* track = fProject.TrackAt(i);
                if (!track) continue;

                // Redraw lane background in the update area
                BRect updateLaneRect = updateRect & laneRect;
                if (i % 2 == 0) {
                    SetHighColor(40, 40, 45);
                } else {
                    SetHighColor(35, 35, 40);
                }
                FillRect(updateLaneRect);

                // Track color from 3D position
                VeniceDAW::Coordinate3D pos = track->Position();
                rgb_color trackColor;
                trackColor.red = 100 + (int)(fabs(pos.x) * 10) % 155;
                trackColor.green = 100 + (int)(fabs(pos.y) * 10) % 155;
                trackColor.blue = 100 + (int)(fabs(pos.z) * 10) % 155;

                // Redraw waveform section if it intersects updateRect
                const VeniceDAW::AudioFormat3DMix& format = track->GetAudioFormat();
                int32 startSample = track->StartPosition();
                int32 endSample = track->EndPosition();
                float sampleRate = format.sampleRate > 0 ? format.sampleRate : 44100.0f;
                float startTime = startSample / sampleRate;
                float endTime = endSample / sampleRate;

                if (endSample == 0 && format.fileSize > 0 && format.channels > 0 && format.bitDepth > 0) {
                    int32 bytesPerSample = (format.bitDepth + 7) / 8;
                    int32 totalSamples = format.fileSize / (format.channels * bytesPerSample);
                    endTime = startTime + (totalSamples / sampleRate);
                }

                if (endTime <= startTime) {
                    endTime = fProject.CalculateTotalDuration();
                }

                float trackDuration = endTime - startTime;
                if (trackDuration > 0) {
                    float startX = trackNameWidth + 10 + (startTime * fPixelsPerSecond);
                    float endX = trackNameWidth + 10 + (endTime * fPixelsPerSecond);
                    BRect blockRect(startX, y + 5, endX, y + laneHeight - 6);

                    // Only redraw the part that intersects updateRect
                    if (updateRect.Intersects(blockRect)) {
                        BRect updateBlockRect = updateRect & blockRect;

                        // Redraw audio block background
                        SetHighColor(trackColor.red * 0.6, trackColor.green * 0.6, trackColor.blue * 0.6);
                        FillRect(updateBlockRect);

                        // Redraw waveform in the update area
                        if (simpleMode) {
                            // Just the colored block - already filled above
                        } else {
                            // Redraw waveform for the section that intersects updateRect
                            // Get audio file path and resolve it
                            BString audioPath = track->AudioFilePath();
                            if (audioPath.Length() > 0) {
                                BString resolvedPath;
                                BPath pathObj(audioPath.String());
                                const char* filename = pathObj.Leaf();
                                if (filename) {
                                    // Get project directory
                                    BPath projectPath(fProjectPath.String());
                                    BPath parentPath;
                                    if (projectPath.GetParent(&parentPath) == B_OK) {
                                        BString projectDir(parentPath.Path());
                                        const char* extensions[] = { "", ".wav", ".aiff", ".aif", ".raw", ".mp3", ".ogg", nullptr };
                                        for (int ext = 0; extensions[ext] != nullptr; ext++) {
                                            BString testPath = projectDir;
                                            testPath << "/" << filename << extensions[ext];
                                            entry_ref ref;
                                            if (get_ref_for_path(testPath.String(), &ref) == B_OK) {
                                                resolvedPath = testPath;
                                                break;
                                            }
                                        }
                                    }
                                }

                                // Get audio cache and render waveform
                                if (resolvedPath.Length() > 0) {
                                    VeniceDAW::AudioFormat3DMix audioFormat = track->GetAudioFormat();
                                    if (audioFormat.sampleRate == 0) {
                                        audioFormat.sampleRate = fProject.ProjectSampleRate();
                                    }
                                    const AudioSampleCache* audioCache = WaveformCache::Instance().GetAudioCache(resolvedPath.String(), &audioFormat);
                                    if (audioCache && audioCache->isValid && !audioCache->samples.empty()) {
                                        // Render waveform for updateBlockRect area
                                        float maxHeight = (laneHeight - 10) / 2.0f;
                                        float centerY = y + laneHeight / 2;
                                        float secondsPerPixel = 1.0f / fPixelsPerSecond;

                                        int pixelSkip = 1;
                                        if (fPixelsPerSecond > 100.0f) pixelSkip = 1;
                                        else if (fPixelsPerSecond > 50.0f) pixelSkip = 2;
                                        else if (fPixelsPerSecond > 20.0f) pixelSkip = 3;
                                        else pixelSkip = 4;

                                        int widthPixels = (int)(updateBlockRect.Width());
                                        int lineCount = widthPixels / pixelSkip;
                                        if (lineCount > 0) {
                                            BeginLineArray(lineCount);
                                            rgb_color waveColor = {
                                                (uint8)(trackColor.red * 0.9),
                                                (uint8)(trackColor.green * 0.9),
                                                (uint8)(trackColor.blue * 0.9),
                                                220
                                            };

                                            // Calculate start time for this update rect
                                            float updateStartTime = (updateBlockRect.left - startX) / fPixelsPerSecond;

                                            for (int px = 0; px < widthPixels; px += pixelSkip) {
                                                float x = updateBlockRect.left + px;
                                                if (x >= blockRect.right || x >= bounds.right) break;

                                                float time = updateStartTime + (px / fPixelsPerSecond);
                                                float minPeak, maxPeak;
                                                audioCache->GetSample(time, secondsPerPixel, &minPeak, &maxPeak);

                                                float minY = centerY - (minPeak * maxHeight);
                                                float maxY = centerY - (maxPeak * maxHeight);

                                                AddLine(BPoint(x, minY), BPoint(x, maxY), waveColor);
                                            }
                                            EndLineArray();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Redraw lane separator if needed
                if (updateRect.Intersects(BRect(0, y + laneHeight - 1, bounds.right, y + laneHeight - 1))) {
                    SetHighColor(25, 25, 30);
                    StrokeLine(BPoint(updateRect.left, y + laneHeight - 1),
                              BPoint(updateRect.right, y + laneHeight - 1));
                }
            }
        }  // End of if (!playheadOnly) else

        // Draw playhead (red vertical line) - ALWAYS draw this
        if (fPlayheadPosition >= 0) {
            const float timelineStart = 160.0f;  // trackNameWidth (150) + offset (10)
            float playheadX = timelineStart + fPlayheadPosition * fPixelsPerSecond;

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
    BString fProjectPath;  // Full path to .3dmix file for audio file resolution
    float fPixelsPerSecond;
    float fPlayheadPosition;
    float fLastPlayheadX;  // Cache last playhead X position for partial redraws
};

// Main Timeline Content View - Container for ruler and lanes
class TimelineContentView : public BView {
public:
    TimelineContentView(BRect frame, const VeniceDAW::Project3DMix& project, const char* projectFilePath,
                        BSoundPlayer* sharedSoundPlayer, int64* sharedFramePosition, bool* sharedIsPlaying)
        : BView(frame, "timeline_content", B_FOLLOW_ALL, B_WILL_DRAW)
        , fProject(project)
        , fProjectPath(projectFilePath)
        , fPixelsPerSecond(50.0f)  // Initial zoom
        , fPlayheadPosition(0.0f)
        , fSharedSoundPlayer(sharedSoundPlayer)
        , fSharedFramePosition(sharedFramePosition)
        , fSharedIsPlaying(sharedIsPlaying)
    {
        SetViewColor(30, 30, 35);

        // Create ruler view
        BRect rulerFrame = Bounds();
        rulerFrame.bottom = 40;
        fRulerView = new TimeRulerView(rulerFrame, project, fPixelsPerSecond);
        AddChild(fRulerView);

        // Create track lanes view with project file path for audio file resolution
        BRect lanesFrame = Bounds();
        lanesFrame.top = 41;
        fLanesView = new TrackLanesView(lanesFrame, project, fPixelsPerSecond, projectFilePath);
        AddChild(fLanesView);

        printf("[Timeline] Using shared audio engine from 3D Mixer window\n");
    }

    ~TimelineContentView() {
        // Don't delete fSharedSoundPlayer - it's owned by DemoWindow
    }

    void ZoomIn() {
        fPixelsPerSecond *= 1.5f;
        if (fPixelsPerSecond > 200.0f) fPixelsPerSecond = 200.0f;
        UpdateZoom();
    }

    void ZoomOut() {
        fPixelsPerSecond /= 1.5f;
        if (fPixelsPerSecond < 1.0f) fPixelsPerSecond = 1.0f;
        UpdateZoom();
    }

    void FitToWindow() {
        // Calculate duration from track data
        float maxDuration = 0.0f;
        int trackCount = fProject.CountTracks();
        for (int i = 0; i < trackCount; i++) {
            VeniceDAW::Track3DMix* track = fProject.TrackAt(i);
            if (!track) continue;

            const VeniceDAW::AudioFormat3DMix& format = track->GetAudioFormat();
            float sampleRate = format.sampleRate > 0 ? format.sampleRate : 44100.0f;

            float startTime = track->StartPosition() / sampleRate;
            float endTime = track->EndPosition() / sampleRate;

            if (endTime == 0 && format.fileSize > 0) {
                int32 bytesPerSample = (format.bitDepth + 7) / 8;
                int32 totalSamples = format.fileSize / (format.channels * bytesPerSample);
                endTime = startTime + (totalSamples / sampleRate);
            }

            if (endTime > maxDuration) maxDuration = endTime;
        }

        if (maxDuration > 0) {
            // Calculate zoom to fit entire song with some margin
            float availableWidth = Bounds().Width() - 170.0f;  // Subtract track name area
            fPixelsPerSecond = availableWidth / maxDuration * 0.95f;  // 95% to leave margin

            if (fPixelsPerSecond < 1.0f) fPixelsPerSecond = 1.0f;
            if (fPixelsPerSecond > 200.0f) fPixelsPerSecond = 200.0f;

            UpdateZoom();
            printf("[Timeline] Fit to window: %.1f seconds @ %.1f px/sec\n",
                   maxDuration, fPixelsPerSecond);
        }
    }

    void UpdateZoom() {
        fRulerView->SetPixelsPerSecond(fPixelsPerSecond);
        fLanesView->SetPixelsPerSecond(fPixelsPerSecond);
    }

    void TogglePlayback() {
        *fSharedIsPlaying = !(*fSharedIsPlaying);

        if (fSharedSoundPlayer) {
            if (*fSharedIsPlaying) {
                // Start audio playback
                fSharedSoundPlayer->Start();
                fSharedSoundPlayer->SetHasData(true);
                printf("[AudioPlayback] STARTED at position %.2fs (frame %ld)\n",
                       fPlayheadPosition, *fSharedFramePosition);
            } else {
                // Stop audio playback
                fSharedSoundPlayer->Stop();
                printf("[AudioPlayback] PAUSED at position %.2fs (frame %ld)\n",
                       fPlayheadPosition, *fSharedFramePosition);
            }
        }

        // Update window title to show play state
        BWindow* window = Window();
        if (window) {
            BString title = "Timeline View - Modern DAW Style";
            if (*fSharedIsPlaying) {
                title << " [PLAYING ♪]";
            } else {
                title << " [PAUSED]";
            }
            window->SetTitle(title.String());
        }

        // Force immediate redraw of playhead
        if (fLanesView) {
            fLanesView->Invalidate();
        }
    }

    void UpdatePlayhead(float deltaSeconds) {
        if (*fSharedIsPlaying && fSharedSoundPlayer) {
            // Sync playhead with actual audio position using BSoundPlayer's sample rate
            media_raw_audio_format format = fSharedSoundPlayer->Format();
            fPlayheadPosition = (*fSharedFramePosition) / format.frame_rate;

            float duration = fProject.CalculateTotalDuration();

            // Loop back to start only if we have a valid duration
            if (duration > 0 && fPlayheadPosition > duration) {
                fPlayheadPosition = 0.0f;
                *fSharedFramePosition = 0;
            }
        }
        // Always update playhead display, even when paused
        fLanesView->SetPlayheadPosition(fPlayheadPosition);
    }

    void ResetPlayhead() {
        fPlayheadPosition = 0.0f;
        *fSharedFramePosition = 0;
        fLanesView->SetPlayheadPosition(fPlayheadPosition);
        printf("[AudioPlayback] Playhead reset to 0.0s\n");
    }

    void JumpToEnd() {
        // Calculate max duration
        float maxDuration = 0.0f;
        int trackCount = fProject.CountTracks();
        for (int i = 0; i < trackCount; i++) {
            VeniceDAW::Track3DMix* track = fProject.TrackAt(i);
            if (!track) continue;

            const VeniceDAW::AudioFormat3DMix& format = track->GetAudioFormat();
            float sampleRate = format.sampleRate > 0 ? format.sampleRate : 44100.0f;
            float endTime = track->EndPosition() / sampleRate;

            if (endTime == 0 && format.fileSize > 0) {
                int32 bytesPerSample = (format.bitDepth + 7) / 8;
                int32 totalSamples = format.fileSize / (format.channels * bytesPerSample);
                endTime = track->StartPosition() / sampleRate + (totalSamples / sampleRate);
            }

            if (endTime > maxDuration) maxDuration = endTime;
        }

        if (maxDuration > 0) {
            fPlayheadPosition = maxDuration;
            fLanesView->SetPlayheadPosition(fPlayheadPosition);
        }
    }

    virtual void AttachedToWindow() override {
        BView::AttachedToWindow();
        MakeFocus(true);

        // Auto-fit to show entire song on first open
        FitToWindow();
    }

    virtual void KeyDown(const char* bytes, int32 numBytes) override {
        char key = bytes[0];

        switch (key) {
            case '+':
            case '=':
                ZoomIn();
                break;
            case '-':
            case '_':
                ZoomOut();
                break;
            case 'f':
            case 'F':
                FitToWindow();
                break;
            case ' ':  // Spacebar
                TogglePlayback();
                break;
            case 'r':
            case 'R':
                ResetPlayhead();
                break;
            case B_HOME:
                ResetPlayhead();
                break;
            case B_END:
                JumpToEnd();
                break;
            default:
                BView::KeyDown(bytes, numBytes);
                break;
        }
    }

private:
    const VeniceDAW::Project3DMix& fProject;
    BString fProjectPath;  // Full path to .3dmix file for audio file resolution
    TimeRulerView* fRulerView;
    TrackLanesView* fLanesView;
    float fPixelsPerSecond;
    float fPlayheadPosition;

    // Shared audio playback system (owned by DemoWindow)
    BSoundPlayer* fSharedSoundPlayer;
    int64* fSharedFramePosition;
    bool* fSharedIsPlaying;
};

// Timeline visualization window
class TimelineWindow : public BWindow {
public:
    TimelineWindow(const VeniceDAW::Project3DMix& project, const char* projectFilePath,
                   BSoundPlayer* sharedSoundPlayer, int64* sharedFramePosition, bool* sharedIsPlaying)
        : BWindow(BRect(100, 100, 1400, 850), "Timeline View - Modern DAW Style",
                  B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
        , fProject(project)
        , fProjectPath(projectFilePath)
        , fContentView(nullptr)
        , fUpdateRunner(nullptr)
    {
        fContentView = new TimelineContentView(Bounds(), project, projectFilePath,
                                                sharedSoundPlayer, sharedFramePosition, sharedIsPlaying);
        AddChild(fContentView);

        // Timer will be created in Show() to ensure window loop is active
        printf("[TimelineWindow] Created with shared audio engine\n");
    }

    virtual void Show() override {
        BWindow::Show();

        // Create animation timer now that window is shown and loop is active
        if (!fUpdateRunner) {
            printf("[TimelineWindow] Starting animation timer (30 FPS)\n");
            BMessage pulse('Tpls');
            fUpdateRunner = new BMessageRunner(this, &pulse, 33333);  // 33ms = ~30 FPS
        }
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

            case B_KEY_DOWN: {
                // Intercept keyboard events at window level to handle regardless of focus
                int32 key = 0;
                if (message->FindInt32("key", &key) == B_OK) {
                    if (key == B_SPACE) {
                        // Spacebar - toggle playback
                        if (fContentView) {
                            fContentView->TogglePlayback();
                        }
                        return;  // Event handled
                    } else if (key == 0x72 || key == 0x52) {  // 'r' or 'R'
                        // Reset playhead
                        if (fContentView) {
                            fContentView->ResetPlayhead();
                        }
                        return;
                    }
                }
                // Fall through to default for unhandled keys
                BWindow::MessageReceived(message);
                break;
            }

            default:
                BWindow::MessageReceived(message);
                break;
        }
    }

    void DebugTimerStatus() {
        printf("[TimelineWindow] Timer status check:\n");
        printf("  fUpdateRunner: %p\n", fUpdateRunner);
        printf("  fContentView: %p\n", fContentView);
    }

    virtual bool QuitRequested() override {
        // Don't quit the app, just hide this window
        Hide();
        return false;
    }

private:
    const VeniceDAW::Project3DMix& fProject;
    BString fProjectPath;  // Full path to the .3dmix file
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
        , fProjectPath(projectPath)  // Store the project file path
        , fPlayButton(nullptr)
        , fSoundPlayer(nullptr)
        , fCurrentFramePosition(0)
        , fIsPlaying(false)
    {
        BRect bounds = Bounds();

        // Reserve space at bottom for controls
        const float controlHeight = 50;
        BRect glRect = bounds;
        glRect.bottom -= controlHeight;

        // Create GL view (reduced height)
        fGLView = new DemoGL3DView(glRect, "gl_view");
        AddChild(fGLView);

        // Create Play/Pause button at bottom
        BRect buttonRect = bounds;
        buttonRect.top = buttonRect.bottom - controlHeight + 10;
        buttonRect.bottom = buttonRect.bottom - 10;
        buttonRect.left = (bounds.Width() / 2) - 60;
        buttonRect.right = buttonRect.left + 120;

        fPlayButton = new BButton(buttonRect, "play_button", "▶ Play",
                                  new BMessage('Play'));
        fPlayButton->SetTarget(this);
        AddChild(fPlayButton);

        // Load and parse 3dmix file
        LoadProject(projectPath);

        // Initialize audio playback
        InitAudioPlayback();

        // Start animation
        BMessage pulse(MSG_PULSE);
        fUpdateRunner = new BMessageRunner(this, &pulse, 33333);  // 30 FPS
    }

    ~DemoWindow() {
        // Stop audio first
        if (fSoundPlayer) {
            fSoundPlayer->Stop();
            delete fSoundPlayer;
            fSoundPlayer = nullptr;
        }

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
            // Pass project, project path, and shared audio engine references
            fTimelineWindow = new TimelineWindow(*fProject, fProjectPath.String(),
                                                  fSoundPlayer, &fCurrentFramePosition, &fIsPlaying);
            printf("[DemoWindow] Created Timeline window with shared audio engine\n");
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

            case 'Play':
                TogglePlayback();
                break;

            case B_KEY_DOWN: {
                const char* bytes;
                if (message->FindString("bytes", &bytes) == B_OK) {
                    char key = bytes[0];
                    if (key == 't' || key == 'T') {
                        OpenTimelineWindow();
                        return;
                    } else if (key == ' ') {  // Spacebar also toggles playback
                        TogglePlayback();
                        return;
                    }
                }
                break;
            }
            default:
                BWindow::MessageReceived(message);
        }
    }

    void InitAudioPlayback() {
        if (!fProject) return;

        // Detect sample rate from first valid track
        float detectedSampleRate = 44100.0f;  // Default fallback
        int trackCount = fProject->CountTracks();

        for (int i = 0; i < trackCount; i++) {
            VeniceDAW::Track3DMix* track = fProject->TrackAt(i);
            if (!track) continue;

            BString audioPath = track->AudioFilePath();
            if (audioPath.Length() == 0) continue;

            BString resolvedPath;
            BPath pathObj(audioPath.String());
            const char* filename = pathObj.Leaf();
            if (!filename) continue;

            // Try to find the file in project directory
            BPath projectPath(fProjectPath.String());
            projectPath.GetParent(&projectPath);
            BString projectDir(projectPath.Path());

            const char* extensions[] = { "", ".wav", ".aiff", ".aif", ".raw", ".mp3", ".ogg", nullptr };
            for (int ext = 0; extensions[ext] != nullptr; ext++) {
                BString testPath = projectDir;
                testPath << "/" << filename << extensions[ext];

                entry_ref ref;
                if (get_ref_for_path(testPath.String(), &ref) == B_OK) {
                    resolvedPath = testPath;
                    break;
                }
            }

            if (resolvedPath.Length() == 0) continue;

            // Get audio cache and check sample rate - use project sample rate if track doesn't have one
            VeniceDAW::AudioFormat3DMix audioFormat = track->GetAudioFormat();
            if (audioFormat.sampleRate == 0 && fProject) {
                audioFormat.sampleRate = fProject->ProjectSampleRate();
                printf("[InitAudio] Track %d: Using project sample rate: %d Hz\n", i, audioFormat.sampleRate);
            }
            const AudioSampleCache* audioCache = WaveformCache::Instance().GetAudioCache(resolvedPath.String(), &audioFormat);
            if (audioCache && audioCache->isValid && audioCache->sampleRate > 0) {
                detectedSampleRate = audioCache->sampleRate;
                printf("[3D Audio] Detected sample rate from track: %.0f Hz\n", detectedSampleRate);
                break;  // Use first valid track's rate
            }
        }

        // Initialize BSoundPlayer for audio playback
        media_raw_audio_format format;
        format.frame_rate = detectedSampleRate;
        format.channel_count = 2;      // Stereo
        format.format = media_raw_audio_format::B_AUDIO_FLOAT;
        format.byte_order = B_MEDIA_HOST_ENDIAN;
        format.buffer_size = 4096;

        fSoundPlayer = new BSoundPlayer(&format, "VeniceDAW 3D Player", PlayBufferFunc, nullptr, this);
        if (fSoundPlayer->InitCheck() == B_OK) {
            printf("[3D Audio] BSoundPlayer initialized at %.0f Hz\n", detectedSampleRate);
        } else {
            printf("[3D Audio] ERROR: Failed to initialize BSoundPlayer!\n");
            delete fSoundPlayer;
            fSoundPlayer = nullptr;
        }
    }

    void TogglePlayback() {
        if (!fSoundPlayer || !fProject) return;

        fIsPlaying = !fIsPlaying;

        if (fIsPlaying) {
            fSoundPlayer->Start();
            fSoundPlayer->SetHasData(true);
            fPlayButton->SetLabel("⏸ Pause");
            printf("[3D Audio] Playback STARTED\n");
        } else {
            fSoundPlayer->Stop();
            fPlayButton->SetLabel("▶ Play");
            printf("[3D Audio] Playback PAUSED\n");
        }
    }

    // Static callback for BSoundPlayer
    static void PlayBufferFunc(void* cookie, void* buffer, size_t size, const media_raw_audio_format& format) {
        DemoWindow* window = (DemoWindow*)cookie;
        window->MixTracks((float*)buffer, size / sizeof(float) / format.channel_count, format);
    }

    // Mix all tracks with proper sample rate conversion
    void MixTracks(float* buffer, int32 frameCount, const media_raw_audio_format& format) {
        // Clear buffer
        memset(buffer, 0, frameCount * format.channel_count * sizeof(float));

        if (!fProject) return;

        int trackCount = fProject->CountTracks();
        if (trackCount == 0) return;

        // Calculate current time position using BSoundPlayer's actual sample rate
        float currentTime = fCurrentFramePosition / format.frame_rate;

        // Mix each track
        for (int i = 0; i < trackCount; i++) {
            VeniceDAW::Track3DMix* track = fProject->TrackAt(i);
            if (!track) continue;

            // Get audio file path and resolve it
            BString audioPath = track->AudioFilePath();
            if (audioPath.Length() == 0) continue;

            BString resolvedPath;
            BPath pathObj(audioPath.String());
            const char* filename = pathObj.Leaf();
            if (!filename) continue;

            BString projectDir;
            if (fProjectPath.Length() > 0) {
                BPath projectPath(fProjectPath.String());
                BPath parentPath;
                if (projectPath.GetParent(&parentPath) == B_OK) {
                    projectDir = parentPath.Path();
                }
            }

            const char* extensions[] = { "", ".wav", ".aiff", ".aif", ".raw", ".mp3", ".ogg", nullptr };
            for (int ext = 0; extensions[ext] != nullptr; ext++) {
                BString testPath = projectDir;
                testPath << "/" << filename << extensions[ext];

                entry_ref ref;
                if (get_ref_for_path(testPath.String(), &ref) == B_OK) {
                    resolvedPath = testPath;
                    break;
                }
            }

            if (resolvedPath.Length() == 0) continue;

            // Get audio cache - use project sample rate if track doesn't have one
            VeniceDAW::AudioFormat3DMix audioFormat = track->GetAudioFormat();
            if (audioFormat.sampleRate == 0 && fProject) {
                audioFormat.sampleRate = fProject->ProjectSampleRate();
            }
            const AudioSampleCache* audioCache = WaveformCache::Instance().GetAudioCache(resolvedPath.String(), &audioFormat);
            if (!audioCache || !audioCache->isValid || audioCache->samples.empty()) continue;

            // Debug: log sample rate info once per track
            static bool logged[32] = {false};
            if (!logged[i] && i < 32) {
                printf("[MixTracks] Track %d: file rate=%.0f Hz, playback rate=%.0f Hz, ratio=%.3f\n",
                       i, audioCache->sampleRate, format.frame_rate, audioCache->sampleRate / format.frame_rate);
                logged[i] = true;
            }

            // Calculate track start time
            float trackStartTime = track->StartPosition() / audioCache->sampleRate;
            float relativeTime = currentTime - trackStartTime;

            if (relativeTime < 0) continue;  // Track hasn't started yet

            // Get loop parameters (BeOS 3DMix compatibility)
            // In BeOS format: st_skip is stored in LoopStart, loop_point in LoopEnd
            int64 loopStart = track->LoopStart();    // Trim: where to start in the audio file
            int64 loopEnd = track->LoopEnd();        // Loop point: where to loop back
            int64 loopLength = loopEnd - loopStart;

            // Debug: log loop info once per track
            static bool loopLogged[32] = {false};
            if (!loopLogged[i] && i < 32 && loopLength > 0) {
                printf("[Loop] Track %d: trim=%.3fs, loop=%.3fs (length=%.3fs)\n",
                       i, loopStart / audioCache->sampleRate, loopEnd / audioCache->sampleRate,
                       loopLength / audioCache->sampleRate);
                loopLogged[i] = true;
            }

            // Calculate sample rate conversion ratio
            float sampleRateRatio = audioCache->sampleRate / format.frame_rate;

            // Copy samples from track to output buffer with sample rate conversion and looping
            for (int32 frame = 0; frame < frameCount; frame++) {
                // Calculate source sample position (fractional)
                float srcPosition = relativeTime * audioCache->sampleRate + (frame * sampleRateRatio);
                int64 srcIdx = (int64)srcPosition;

                // Apply BeOS-style looping if loop parameters are valid
                if (loopLength > 0 && loopEnd > loopStart) {
                    // Add trim offset
                    srcIdx += loopStart;

                    // Apply modulo looping when we exceed loop end point
                    if (srcIdx >= loopEnd) {
                        int64 offsetFromLoopStart = srcIdx - loopStart;
                        srcIdx = loopStart + (offsetFromLoopStart % loopLength);
                    }
                }

                // Safety check: ensure we're within bounds
                if (srcIdx < 0 || srcIdx >= (int64)audioCache->samples.size()) break;

                // Linear interpolation for smooth sample rate conversion
                float sample;
                if (srcIdx + 1 < (int64)audioCache->samples.size()) {
                    float frac = srcPosition - (int64)srcPosition;
                    float s1 = audioCache->samples[srcIdx];
                    float s2 = audioCache->samples[srcIdx + 1];
                    sample = s1 + frac * (s2 - s1);
                } else {
                    sample = audioCache->samples[srcIdx];
                }

                float volume = 0.8f;

                // Mix into output (sum all tracks)
                for (uint32 ch = 0; ch < format.channel_count; ch++) {
                    buffer[frame * format.channel_count + ch] += sample * volume;
                }
            }
        }

        // Update frame position for next callback
        fCurrentFramePosition += frameCount;
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
    BString fProjectPath;  // Full path to the .3dmix file

    // Audio playback
    BButton* fPlayButton;
    BSoundPlayer* fSoundPlayer;
    int64 fCurrentFramePosition;
    bool fIsPlaying;
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

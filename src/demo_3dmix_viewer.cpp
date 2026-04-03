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
#include <atomic>
#include <Bitmap.h>
#include <View.h>
#include <Font.h>
#include <Button.h>
#include <StringView.h>
#include <Slider.h>
#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <FilePanel.h>
#include <ScrollView.h>
#include <Screen.h>

// Include our 3dmix parser
#include "audio/3dmix/3DMixFormat.h"
#include "audio/3dmix/3DMixParser.h"
#include "audio/3dmix/AudioPathResolver.h"
#include "audio/BiquadFilter.h"
#include <MediaFile.h>
#include <SoundPlayer.h>
#include <MediaDefs.h>
#include <MediaTrack.h>
#include <Path.h>
#include <Entry.h>
#include <File.h>
#include <Alert.h>
#include <FindDirectory.h>

// AudioLogger stub - for 3dmix import system compatibility
namespace VeniceDAW {
    enum class LogLevel { DEBUG, INFO, WARNING, ERROR };
    class AudioLogger {
    public:
        static void Log(LogLevel level, const char* component, const char* format, ...) {
            // Empty stub - suppress logging in demo viewer
        }
    };
}

// Precomputed reciprocal for int16→float conversion (multiply is faster than divide)
static const float kInt16ToFloat = 1.0f / 32768.0f;

struct AudioSource {
    BString name;
    float x, y, z;  // 3D position
    rgb_color color;
    float level;     // Current audio level (0.0 to 1.0) for visual pulsing
    float volume;    // Track volume from 3DMix file (0.0 to 1.0)

    AudioSource() : x(0), y(0), z(0), level(0), volume(1.0f) {
        color.red = color.green = color.blue = 255;
        color.alpha = 255;
    }
};

// Filter chain entry - represents one filter in a track's processing chain
// Uses separate L/R filter instances to avoid stereo crosstalk (shared coefficients, independent state)
struct FilterInChain {
    HaikuDAW::BiquadFilter filterL;  // Left channel filter instance
    HaikuDAW::BiquadFilter filterR;  // Right channel filter instance
    int mode;                        // FilterMode enum value (LOW_PASS, HIGH_PASS, etc.)
    float frequency;                 // Filter frequency in Hz
    float Q;                         // Filter Q factor
    float gainDB;                    // Gain in dB (for peaking/shelf filters)

    FilterInChain()
        : mode(HaikuDAW::BiquadFilter::LOW_PASS)
        , frequency(1000.0f)
        , Q(0.707f)
        , gainDB(0.0f)
    {
        // Initialize both L/R filters with identical parameters
        filterL.SetMode(HaikuDAW::BiquadFilter::LOW_PASS);
        filterL.SetSampleRate(44100.0f);
        filterL.SetFrequency(frequency);
        filterL.SetQ(Q);
        filterL.SetGain(gainDB);
        filterR.SetMode(HaikuDAW::BiquadFilter::LOW_PASS);
        filterR.SetSampleRate(44100.0f);
        filterR.SetFrequency(frequency);
        filterR.SetQ(Q);
        filterR.SetGain(gainDB);
    }

    // Helper to set parameters on both L/R consistently
    void SetParams(int newMode, float freq, float q, float gain) {
        mode = newMode;
        frequency = freq;
        Q = q;
        gainDB = gain;
        filterL.SetMode((HaikuDAW::BiquadFilter::FilterMode)newMode);
        filterL.SetFrequency(freq);
        filterL.SetQ(q);
        filterL.SetGain(gain);
        filterR.SetMode((HaikuDAW::BiquadFilter::FilterMode)newMode);
        filterR.SetFrequency(freq);
        filterR.SetQ(q);
        filterR.SetGain(gain);
    }
};

// Audio sample cache - stores ALL audio samples (loaded once)
// BeOS R6 original: stereo int16 interleaved format
struct AudioSampleCache {
    std::vector<int16_t> samples;    // Stereo int16 samples (L,R,L,R,...)
    float sampleRate;                // Sample rate (e.g., 44100)
    int channels;                    // Number of channels (always 2)
    bool isValid;

    AudioSampleCache() : sampleRate(0.0f), channels(2), isValid(false) {}

    // R6-style GetSample: Calculate min/max ON-THE-FLY for a time range
    // Stereo int16: average L+R channels for waveform display
    void GetSample(float time, float duration, float* outMin, float* outMax) const {
        *outMin = 0.0f;
        *outMax = 0.0f;

        if (!isValid || samples.empty() || time < 0) {
            return;
        }

        // Convert time to frame indices (1 frame = 2 samples for stereo)
        int startFrame = (int)(time * sampleRate);
        int numFrames = (int)(duration * sampleRate);

        if (numFrames == 0) numFrames = 1;

        int startIdx = startFrame * 2;  // Stereo: 2 samples per frame
        if (startIdx >= (int)samples.size()) return;

        int endIdx = (startFrame + numFrames) * 2;
        if (endIdx > (int)samples.size()) {
            endIdx = samples.size();
        }

        // Adaptive step: examine at least ~256 sample points for accurate min/max
        int step = 2;  // Default: every frame (2 samples for stereo)
        if (numFrames > 256) {
            step = (numFrames / 256) * 2;  // *2 for stereo
            if (step < 2) step = 2;
        }

        int16_t minVal = 0;
        int16_t maxVal = 0;

        // Scan frames, average L+R for mono waveform display
        for (int i = startIdx; i < endIdx && i + 1 < (int)samples.size(); i += step) {
            int16_t left = samples[i];
            int16_t right = samples[i + 1];
            int16_t avg = (left + right) / 2;
            if (avg < minVal) minVal = avg;
            if (avg > maxVal) maxVal = avg;
        }

        // Normalize to -1.0 to +1.0
        *outMin = minVal * kInt16ToFloat;
        *outMax = maxVal * kInt16ToFloat;
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
                    // Decode to raw audio - BeOS R6 format: stereo int16
                    media_format format;
                    format.type = B_MEDIA_RAW_AUDIO;
                    format.u.raw_audio = media_raw_audio_format::wildcard;
                    format.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
                    format.u.raw_audio.channel_count = 2;

                    if (track->DecodedFormat(&format) == B_OK) {
                        int64 frameCount = track->CountFrames();
                        if (frameCount > 0) {
                            cache.sampleRate = format.u.raw_audio.frame_rate;
                            cache.channels = 2;  // Always stereo

                            // Reserve space for stereo samples (2 samples per frame)
                            cache.samples.reserve(frameCount * 2);

                            // Read samples as stereo int16
                            const int bufferSize = 8192;
                            int16_t buffer[bufferSize * 2];
                            int64 framesRead = 0;

                            while (framesRead < frameCount) {
                                int64 frames = 0;
                                if (track->ReadFrames(buffer, &frames) != B_OK || frames == 0) {
                                    break;
                                }

                                // Store stereo samples as-is (L,R,L,R,...)
                                for (int64 i = 0; i < frames * 2; i++) {
                                    cache.samples.push_back(buffer[i]);
                                }

                                framesRead += frames;
                            }

                            mediaFile.ReleaseTrack(track);
                            cache.isValid = true;
                            printf("[AudioCache] ✓ Loaded %zu samples (%zu frames) at %.0f Hz\n",
                                   cache.samples.size(), cache.samples.size() / 2, cache.sampleRate);
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

            // BeOS Track Object files: header contains 44100 Hz but audio data is at 22050 Hz
            // This is because the original BeOS mixer played at half speed
            if (detectedRate > 0) {
                cache.sampleRate = detectedRate / 2.0f;  // Correct: 44100 → 22050 Hz
                printf("[AudioCache] Using sample rate from header: %.0f Hz (corrected from %.0f Hz)\n",
                       cache.sampleRate, detectedRate);
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

                // Store stereo int16 samples directly (BeOS R6 format)
                if (format.bitDepth == 16 && format.channels == 2) {
                    int16* samples = (int16*)frameData;

                    // BeOS Track Object files are stored in BIG-ENDIAN on Intel!
                    // The original BeOS code does byte swap on Intel:
                    //   swap((ushort *)samples, size);  // track_obj.cpp line 1960
                    // We must do the same!

                    // Swap bytes: convert big-endian to little-endian on x86
                    uint16_t* rawSamples = (uint16_t*)samples;
                    int16_t leftSwapped = (int16_t)((rawSamples[0] >> 8) | (rawSamples[0] << 8));
                    int16_t rightSwapped = (int16_t)((rawSamples[1] >> 8) | (rawSamples[1] << 8));

                    // Debug: log first few values BEFORE and AFTER swap
                    if (cache.samples.size() < 10) {
                        printf("[AudioCache] Sample %zu: BEFORE swap=[%d, %d], AFTER swap=[%d, %d]\n",
                               cache.samples.size() / 2, samples[0], samples[1], leftSwapped, rightSwapped);
                    }

                    // Store byte-swapped samples
                    cache.samples.push_back(leftSwapped);   // Left
                    cache.samples.push_back(rightSwapped);  // Right
                } else {
                    // Fallback for non-stereo or non-16bit
                    printf("[AudioCache] WARNING: Unsupported format (bitDepth=%d, channels=%d)\n",
                           format.bitDepth, format.channels);
                }
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
        , fProject(nullptr)
        , fRotationY(180.0f)  // Rotate 180 degrees
        , fZoom(-25.0f)  // Increased zoom to see all tracks
        , fProjectName("")
        , fTrackCount(0)
        , fAutoRotate(false)  // Disable auto-rotation
        , fMouseX(0)
        , fMouseY(0)
        , fHoveredTrackIndex(-1)
        , fIsDragging(false)
        , fDraggedTrackIndex(-1)
        , fDragStartX(0.0f)
        , fDragStartZ(0.0f)
        , fQuadric(nullptr)
    {
        SetEventMask(B_POINTER_EVENTS, 0);
    }

    ~DemoGL3DView() {
        if (fQuadric) {
            gluDeleteQuadric(fQuadric);
            fQuadric = nullptr;
        }
    }

    virtual void AttachedToWindow() override {
        BGLView::AttachedToWindow();
        LockGL();
        InitGL();
        fQuadric = gluNewQuadric();
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

    void SetProject(VeniceDAW::Project3DMix* project) {
        fProject = project;
    }

    void ClearSources() {
        fSources.clear();
        fHoveredTrackIndex = -1;
        fIsDragging = false;
        fDraggedTrackIndex = -1;
        Invalidate();
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
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluDisk(fQuadric, 0.0, 0.5, 20, 1);
        glPopMatrix();

        // Draw "ears" as small cylinders
        glColor3f(0.9f, 0.9f, 0.0f);
        glPushMatrix();
        glTranslatef(-0.5f, 0.0f, 0.0f);
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        gluCylinder(fQuadric, 0.15, 0.15, 0.3, 12, 1);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.2f, 0.0f, 0.0f);
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        gluCylinder(fQuadric, 0.15, 0.15, 0.3, 12, 1);
        glPopMatrix();
        glPopMatrix();
    }

    void DrawAudioSource(const AudioSource& source) {
        glPushMatrix();
        glTranslatef(source.x, 0.0f, source.y);  // Map 3dmix Y to OpenGL Z (depth)

        // Audio level pulsing effect
        float pulseScale = 1.0f + (source.level * 0.5f);  // Scale 1.0 to 1.5
        float brightness = 1.0f + (source.level * 0.8f);  // Brightness multiplier

        // Draw vertical cylinder/pole
        glColor3ub(source.color.red, source.color.green, source.color.blue);

        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);  // Point cylinder upward
        gluCylinder(fQuadric, 0.1, 0.1, 0.8, 12, 1);  // Thin pole
        glPopMatrix();

        // Draw sphere on top of pole with audio-reactive pulsing
        glTranslatef(0.0f, 0.8f, 0.0f);

        // Brighten color based on audio level
        float r = fmin(source.color.red * brightness, 255.0f);
        float g = fmin(source.color.green * brightness, 255.0f);
        float b = fmin(source.color.blue * brightness, 255.0f);
        glColor3ub((uint8)r, (uint8)g, (uint8)b);

        // Scale sphere based on audio level
        glPushMatrix();  // Push before scaling so we can pop back to unscaled position
        glScalef(pulseScale, pulseScale, pulseScale);
        gluSphere(fQuadric, 0.3, 16, 16);
        glPopMatrix();  // Pop scaling - back to (source.x, 0.8, source.y)

        // === VU METER ===
        // Draw VU meter above the sphere (we're at source.x, 0.8, source.y)
        glPushMatrix();  // Push so we can pop back to 0.8 after drawing VU meter
        glTranslatef(0.0f, 0.4f, 0.0f);  // Move up to 1.2 total (0.8 + 0.4)

        // VU meter height based on audio level (0.0 to 2.0)
        float meterHeight = source.level * 2.0f;
        float meterWidth = 0.15f;

        // Color based on level: Green → Yellow → Red
        if (source.level < 0.5f) {
            // Green zone (0.0 - 0.5)
            float greenAmount = source.level * 2.0f;  // 0.0 to 1.0
            glColor3f(0.0f, 0.8f * greenAmount, 0.0f);
        } else if (source.level < 0.8f) {
            // Yellow zone (0.5 - 0.8)
            float yellowMix = (source.level - 0.5f) / 0.3f;  // 0.0 to 1.0
            glColor3f(0.8f * yellowMix, 0.8f, 0.0f);
        } else {
            // Red zone (0.8 - 1.0+)
            glColor3f(0.9f, 0.0f, 0.0f);
        }

        // Draw VU meter as vertical box
        glBegin(GL_QUADS);
        // Front face
        glVertex3f(-meterWidth, 0.0f, meterWidth);
        glVertex3f( meterWidth, 0.0f, meterWidth);
        glVertex3f( meterWidth, meterHeight, meterWidth);
        glVertex3f(-meterWidth, meterHeight, meterWidth);
        // Back face
        glVertex3f(-meterWidth, 0.0f, -meterWidth);
        glVertex3f(-meterWidth, meterHeight, -meterWidth);
        glVertex3f( meterWidth, meterHeight, -meterWidth);
        glVertex3f( meterWidth, 0.0f, -meterWidth);
        // Left face
        glVertex3f(-meterWidth, 0.0f, -meterWidth);
        glVertex3f(-meterWidth, 0.0f,  meterWidth);
        glVertex3f(-meterWidth, meterHeight,  meterWidth);
        glVertex3f(-meterWidth, meterHeight, -meterWidth);
        // Right face
        glVertex3f( meterWidth, 0.0f, -meterWidth);
        glVertex3f( meterWidth, meterHeight, -meterWidth);
        glVertex3f( meterWidth, meterHeight,  meterWidth);
        glVertex3f( meterWidth, 0.0f,  meterWidth);
        // Top face
        glVertex3f(-meterWidth, meterHeight, -meterWidth);
        glVertex3f(-meterWidth, meterHeight,  meterWidth);
        glVertex3f( meterWidth, meterHeight,  meterWidth);
        glVertex3f( meterWidth, meterHeight, -meterWidth);
        glEnd();
        glPopMatrix();

        // Draw base circle on ground
        glPushMatrix();
        glTranslatef(0.0f, -0.8f, 0.0f);  // Back to ground level
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

        if (fIsDragging && fDraggedTrackIndex >= 0 && fDraggedTrackIndex < (int)fSources.size()) {
            // Drag in progress - move the track (BeOS 3D Mixer style)
            // Calculate delta in screen space
            float dx = where.x - fDragStartMouse.x;
            float dy = where.y - fDragStartMouse.y;

            // Convert screen delta to 3D world delta
            // Simple approximation: scale by zoom distance
            float scaleFactor = fabs(fZoom) / 800.0f;  // Empirical scaling

            // Account for 180° camera rotation: X axis is inverted
            float worldDX = -dx * scaleFactor;  // Negate X because camera is rotated 180°
            float worldDZ = -dy * scaleFactor;  // Invert Y (screen Y goes down, world Z goes up)

            // Update track position with constraints (±10 units like BeOS ±250)
            AudioSource& track = fSources[fDraggedTrackIndex];
            track.x = fDragStartX + worldDX;
            track.y = fDragStartZ + worldDZ;  // Note: we use Y for Z axis in our system

            // Apply boundary constraints
            if (track.x > 10.0f) track.x = 10.0f;
            if (track.x < -10.0f) track.x = -10.0f;
            if (track.y > 10.0f) track.y = 10.0f;
            if (track.y < -10.0f) track.y = -10.0f;

            // Update project track position for real-time audio spatialization
            if (fProject && fDraggedTrackIndex < fProject->CountTracks()) {
                VeniceDAW::Track3DMix* projectTrack = fProject->TrackAt(fDraggedTrackIndex);
                if (projectTrack) {
                    projectTrack->SetPosition(track.x, track.y, track.z);
                }
            }

            Invalidate();  // Redraw with new position
        } else {
            UpdateHoveredTrack();
            Invalidate();  // Redraw for hover feedback
        }
    }

    virtual void MouseDown(BPoint where) override {
        fMouseX = where.x;
        fMouseY = where.y;
        UpdateHoveredTrack();

        // Start drag if clicking on a track
        if (fHoveredTrackIndex >= 0 && fHoveredTrackIndex < (int)fSources.size()) {
            fIsDragging = true;
            fDraggedTrackIndex = fHoveredTrackIndex;
            fDragStartMouse = where;
            fDragStartX = fSources[fDraggedTrackIndex].x;
            fDragStartZ = fSources[fDraggedTrackIndex].y;  // Using Y for Z axis
            printf("[Drag] Started dragging track %d '%s' from position (%.2f, %.2f)\n",
                   fDraggedTrackIndex, fSources[fDraggedTrackIndex].name.String(),
                   fDragStartX, fDragStartZ);
        }
        Invalidate();  // Redraw for visual feedback
    }

    virtual void MouseUp(BPoint where) override {
        if (fIsDragging && fDraggedTrackIndex >= 0 && fDraggedTrackIndex < (int)fSources.size()) {
            printf("[Drag] Finished dragging track %d '%s' to position (%.2f, %.2f)\n",
                   fDraggedTrackIndex, fSources[fDraggedTrackIndex].name.String(),
                   fSources[fDraggedTrackIndex].x, fSources[fDraggedTrackIndex].y);
            fIsDragging = false;
            fDraggedTrackIndex = -1;
            Invalidate();  // Redraw after drag ends
        }
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
            Invalidate();  // Only redraw if rotation is active
        }
    }

    void ZoomIn() {
        fZoom += 1.0f;
        if (fZoom > -2.0f) fZoom = -2.0f;
        Invalidate();  // Redraw with new zoom level
    }

    void ZoomOut() {
        fZoom -= 1.0f;
        if (fZoom < -30.0f) fZoom = -30.0f;
        Invalidate();  // Redraw with new zoom level
    }

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

    void SetTrackLevels(const float* levels, int count) {
        // Update audio levels for each track
        static int debugCount = 0;
        for (int i = 0; i < count && i < (int)fSources.size(); i++) {
            fSources[i].level = levels[i];
            if (debugCount < 5 && levels[i] > 0.1f) {
                printf("[VU Debug] Track %d: level=%.3f\n", i, levels[i]);
            }
        }
        if (debugCount < 5) {
            printf("[VU Debug] Updated %d/%d tracks\n", count, (int)fSources.size());
            debugCount++;
        }
    }

    void DrawProjectInfo() {
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

        // Draw project name and track count in top-left corner
        if (fProjectName.Length() > 0) {
            char infoText[256];
            snprintf(infoText, sizeof(infoText), "%s - %d tracks",
                     fProjectName.String(), fTrackCount);

            int textLen = strlen(infoText);
            float textWidth = textLen * 8.0f + 20.0f;
            float boxHeight = 30.0f;
            float boxX = 10.0f;
            float boxY = 10.0f;

            // Draw semi-transparent background
            glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
            glBegin(GL_QUADS);
            glVertex2f(boxX, boxY);
            glVertex2f(boxX + textWidth, boxY);
            glVertex2f(boxX + textWidth, boxY + boxHeight);
            glVertex2f(boxX, boxY + boxHeight);
            glEnd();

            // Draw cyan border
            glLineWidth(2.0f);
            glColor3f(0.3f, 0.8f, 1.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(boxX, boxY);
            glVertex2f(boxX + textWidth, boxY);
            glVertex2f(boxX + textWidth, boxY + boxHeight);
            glVertex2f(boxX, boxY + boxHeight);
            glEnd();

            // Draw text
            glColor3f(1.0f, 1.0f, 1.0f);
            float charX = boxX + 10.0f;
            float charY = boxY + 10.0f;

            for (int i = 0; infoText[i] != '\0'; i++) {
                DrawChar(charX, charY, infoText[i]);
                charX += 8.0f;
            }
        }

        // Draw track name if hovering over a track
        if (fHoveredTrackIndex >= 0 && fHoveredTrackIndex < (int)fSources.size()) {
            const AudioSource& hoveredSource = fSources[fHoveredTrackIndex];
            const char* trackName = hoveredSource.name.String();
            int textLen = strlen(trackName);
            float textWidth = textLen * 8.0f + 20.0f;
            float boxHeight = 30.0f;

            // Position near mouse cursor
            float boxX = fMouseX + 15.0f;
            float boxY = fMouseY - 15.0f;

            // Keep box within screen bounds
            if (boxX + textWidth > bounds.Width() - 10)
                boxX = bounds.Width() - textWidth - 10;
            if (boxY < 10)
                boxY = 50;  // Move below the project info box
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

            // Draw text
            glColor3f(1.0f, 1.0f, 1.0f);
            float charX = boxX + 10.0f;
            float charY = boxY + 10.0f;

            for (int i = 0; trackName[i] != '\0'; i++) {
                DrawChar(charX, charY, trackName[i]);
                charX += 8.0f;
            }
        }

        glEnable(GL_DEPTH_TEST);

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }

private:
    std::vector<AudioSource> fSources;
    VeniceDAW::Project3DMix* fProject;  // Pointer to project for real-time position updates
    float fRotationY;
    float fZoom;
    BString fProjectName;
    int fTrackCount;
    bool fAutoRotate;
    float fMouseX;
    float fMouseY;
    int fHoveredTrackIndex;

    // Drag & drop state (BeOS 3D Mixer style)
    bool fIsDragging;
    int fDraggedTrackIndex;
    BPoint fDragStartMouse;
    float fDragStartX, fDragStartZ;  // Original track position

    // Cached GLU quadric (avoid alloc/free per frame)
    GLUquadric* fQuadric;
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
        , fPlayheadPosition(-1.0f)  // Playhead position in seconds
        , fLoopEnabled(false)
        , fLoopInPoint(0.0f)
        , fLoopOutPoint(0.0f)
    {
        SetViewColor(45, 45, 50);
    }

    virtual void AttachedToWindow() override {
        BView::AttachedToWindow();
        printf("[TimeRuler] AttachedToWindow() - forcing initial draw\n");
        Invalidate();
    }

    virtual void FrameResized(float width, float height) override {
        BView::FrameResized(width, height);
        printf("[TimeRuler] FrameResized(%.1f, %.1f)\n", width, height);
        Invalidate();
    }

    void SetPixelsPerSecond(float pps) {
        fPixelsPerSecond = pps;
        Invalidate();
    }

    void SetPlayheadPosition(float position) {
        fPlayheadPosition = position;
        Invalidate();
    }

    void SetLoopRegion(float inPoint, float outPoint, bool enabled) {
        fLoopInPoint = inPoint;
        fLoopOutPoint = outPoint;
        fLoopEnabled = enabled;
        Invalidate();
    }

    virtual void Draw(BRect updateRect) override {
        printf("[TimeRuler] Draw() called - bounds: (%.1f, %.1f, %.1f, %.1f)\n",
               Bounds().left, Bounds().top, Bounds().right, Bounds().bottom);

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

        // Draw loop region markers
        if (fLoopEnabled && fLoopOutPoint > fLoopInPoint) {
            const float timelineStart = fTrackNameWidth + 10.0f;
            float loopInX = timelineStart + fLoopInPoint * fPixelsPerSecond;
            float loopOutX = timelineStart + fLoopOutPoint * fPixelsPerSecond;

            // Draw semi-transparent green region
            SetHighColor(0, 255, 100, 40);
            FillRect(BRect(loopInX, 0, loopOutX, bounds.bottom));

            // Draw loop in marker (green vertical line)
            SetHighColor(0, 255, 100);
            SetPenSize(2.0);
            StrokeLine(BPoint(loopInX, 0), BPoint(loopInX, bounds.bottom));

            // Draw loop out marker (green vertical line)
            StrokeLine(BPoint(loopOutX, 0), BPoint(loopOutX, bounds.bottom));
            SetPenSize(1.0);

            // Draw "Loop" label
            SetHighColor(0, 255, 100);
            SetFont(be_bold_font);
            DrawString("LOOP", BPoint(loopInX + 5, 15));
            SetFont(be_plain_font);
        }

        // Draw playhead (red vertical line)
        if (fPlayheadPosition >= 0) {
            const float timelineStart = fTrackNameWidth + 10.0f;
            float playheadX = timelineStart + fPlayheadPosition * fPixelsPerSecond;

            SetHighColor(255, 60, 60);
            SetPenSize(2.0);
            StrokeLine(BPoint(playheadX, 0), BPoint(playheadX, bounds.bottom));
            SetPenSize(1.0);
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
            // IMPORTANT: StartPosition/EndPosition were calculated at 22050 Hz in parser
            // We must use the same rate here for correct timeline positioning
            const float kTimelineReferenceRate = 22050.0f;
            float sampleRate = kTimelineReferenceRate;  // Always use 22050 for timeline positioning
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
    float fPlayheadPosition;  // Playhead position in seconds

    // Loop region state
    bool fLoopEnabled;
    float fLoopInPoint;
    float fLoopOutPoint;
};

// PERFORMANCE: Adaptive quality levels for low-end systems
enum class RenderQuality {
    LOW,     // Colored blocks only (< 3 px/sec) - minimal CPU
    MEDIUM,  // Downsampled waveform (3-20 px/sec) - balanced
    HIGH     // Full quality waveform (> 20 px/sec) - best quality
};

// Determine rendering quality based on zoom level
inline RenderQuality GetRenderQuality(float pixelsPerSecond) {
    if (pixelsPerSecond < 3.0f) {
        return RenderQuality::LOW;   // Too zoomed out - blocks only
    } else if (pixelsPerSecond < 20.0f) {
        return RenderQuality::MEDIUM;  // Medium zoom - downsampled
    } else {
        return RenderQuality::HIGH;    // Zoomed in - full quality
    }
}

// Track Lane View - Shows individual tracks as horizontal lanes
class TrackLanesView : public BView {
public:
    TrackLanesView(BRect frame, const VeniceDAW::Project3DMix& project, float pixelsPerSecond, const char* projectFilePath,
                   bool* trackMute, bool* trackSolo, bool* filterEnabled, int* filterMode,
                   std::vector<FilterInChain>* filterChains, BWindow* parentWindow,
                   BSoundPlayer* sharedSoundPlayer = nullptr, std::atomic<int64>* sharedFramePosition = nullptr)
        : BView(frame, "track_lanes", B_FOLLOW_NONE, B_WILL_DRAW)
        , fProject(project)
        , fProjectPath(projectFilePath)
        , fPixelsPerSecond(pixelsPerSecond)
        , fPlayheadPosition(0.0f)
        , fLastPlayheadX(-1.0f)
        , fTrackMute(trackMute)
        , fTrackSolo(trackSolo)
        , fFilterEnabled(filterEnabled)
        , fFilterMode(filterMode)
        , fFilterChains(filterChains)
        , fParentWindow(parentWindow)
        , fSharedSoundPlayer(sharedSoundPlayer)
        , fSharedFramePosition(sharedFramePosition)
        , fWaveformCacheValid(false)
        , fCachedZoom(-1.0f)
        , fWaveformCacheBitmap(nullptr)
    {
        SetViewColor(35, 35, 40);
        SetFlags(Flags() | B_FULL_UPDATE_ON_RESIZE);
    }

    ~TrackLanesView() {
        // Clean up waveform cache
        if (fWaveformCacheBitmap) {
            delete fWaveformCacheBitmap;
            fWaveformCacheBitmap = nullptr;
        }
    }

    void SetPixelsPerSecond(float pps) {
        fPixelsPerSecond = pps;
        fLastPlayheadX = -1.0f;

        // OPTIMIZATION: Invalidate waveform cache when zoom changes
        fWaveformCacheValid = false;

        Invalidate();
    }

    void SetPlayheadPosition(float seconds) {
        // OPTIMIZATION: Only invalidate playhead area, not entire view!
        const float timelineStart = 160.0f;  // trackNameWidth (150) + offset (10)
        float oldX = timelineStart + fPlayheadPosition * fPixelsPerSecond;
        float newX = timelineStart + seconds * fPixelsPerSecond;

        fPlayheadPosition = seconds;

        BRect bounds = Bounds();

        // Invalidate old playhead position (wider margin to ensure waveforms redraw)
        if (fLastPlayheadX >= 0) {
            BRect oldRect(fLastPlayheadX - 50, 0, fLastPlayheadX + 50, bounds.bottom);
            Invalidate(oldRect);
        }

        // Invalidate new playhead position (wider margin to force full redraw, not playhead-only)
        BRect newRect(newX - 50, 0, newX + 50, bounds.bottom);
        Invalidate(newRect);

        fLastPlayheadX = newX;
    }

    void ShowAddFilterMenu(BPoint where, int trackIndex) {
        BPopUpMenu* menu = new BPopUpMenu("add_filter_menu", false, false);
        menu->SetFont(be_plain_font);

        // Create filter type options
        menu->AddItem(new BMenuItem("Low-Pass Filter", new BMessage(MSG_ADD_FILTER)));
        menu->AddItem(new BMenuItem("High-Pass Filter", new BMessage(MSG_ADD_FILTER + 1)));
        menu->AddItem(new BMenuItem("Band-Pass Filter", new BMessage(MSG_ADD_FILTER + 2)));
        menu->AddItem(new BMenuItem("Notch Filter", new BMessage(MSG_ADD_FILTER + 3)));
        menu->AddItem(new BMenuItem("Peaking EQ", new BMessage(MSG_ADD_FILTER + 4)));
        menu->AddItem(new BMenuItem("Low-Shelf EQ", new BMessage(MSG_ADD_FILTER + 5)));
        menu->AddItem(new BMenuItem("High-Shelf EQ", new BMessage(MSG_ADD_FILTER + 6)));

        // Store track index in each menu item's message
        for (int32 i = 0; i < menu->CountItems(); i++) {
            BMenuItem* item = menu->ItemAt(i);
            if (item && item->Message()) {
                item->Message()->AddInt32("track_index", trackIndex);
            }
        }

        // Set target to parent window (DemoWindow)
        menu->SetTargetForItems(fParentWindow);

        // Convert to screen coordinates
        ConvertToScreen(&where);
        menu->Go(where, true, true, true);
    }

    void ShowRemoveFilterMenu(BPoint where, int trackIndex, int filterIndex) {
        BPopUpMenu* menu = new BPopUpMenu("remove_filter_menu", false, false);
        menu->SetFont(be_plain_font);

        BMessage* removeMsg = new BMessage(MSG_REMOVE_FILTER);
        removeMsg->AddInt32("track_index", trackIndex);
        removeMsg->AddInt32("filter_index", filterIndex);

        menu->AddItem(new BMenuItem("Remove Filter", removeMsg));

        // Set target to parent window (DemoWindow)
        menu->SetTargetForItems(fParentWindow);

        // Convert to screen coordinates
        ConvertToScreen(&where);
        menu->Go(where, true, true, true);
    }

private:
    static const uint32 MSG_ADD_FILTER = 'addf';
    static const uint32 MSG_REMOVE_FILTER = 'rmvf';
    // PERFORMANCE: Build offscreen waveform cache to avoid re-rendering every frame
    void RebuildWaveformCache() {
        BRect bounds = Bounds();

        // Delete old cache if exists
        if (fWaveformCacheBitmap) {
            delete fWaveformCacheBitmap;
            fWaveformCacheBitmap = nullptr;
        }

        // Create new bitmap for cache (B_RGB32 for fast blitting)
        fWaveformCacheBitmap = new BBitmap(bounds, B_RGB32, true);  // true = accepts views
        if (!fWaveformCacheBitmap || !fWaveformCacheBitmap->IsValid()) {
            printf("[WaveformCache] Failed to create cache bitmap, falling back to direct rendering\n");
            if (fWaveformCacheBitmap) {
                delete fWaveformCacheBitmap;
                fWaveformCacheBitmap = nullptr;
            }
            fWaveformCacheValid = false;
            return;
        }

        // Create view attached to bitmap for rendering
        BView* cacheView = new BView(bounds, "cache_view", B_FOLLOW_NONE, B_WILL_DRAW);
        fWaveformCacheBitmap->AddChild(cacheView);

        // Lock and render to bitmap
        if (fWaveformCacheBitmap->Lock()) {
            // Clear background
            cacheView->SetHighColor(35, 35, 40);
            cacheView->FillRect(bounds);

            // Render all track lanes (excluding playhead which changes every frame)
            RenderTracksToView(cacheView, bounds);

            // Sync and unlock
            cacheView->Sync();
            fWaveformCacheBitmap->Unlock();

            // Cache is now valid
            fWaveformCacheValid = true;
            fCachedZoom = fPixelsPerSecond;

            printf("[WaveformCache] Cache rebuilt at zoom %.1f px/sec (%.1f KB)\n",
                   fPixelsPerSecond, (bounds.Width() * bounds.Height() * 4) / 1024.0f);
        } else {
            printf("[WaveformCache] Failed to lock cache bitmap\n");
            delete fWaveformCacheBitmap;
            fWaveformCacheBitmap = nullptr;
            fWaveformCacheValid = false;
        }
    }

    // Helper: Render tracks to a view (used for both cache and direct rendering)
    void RenderTracksToView(BView* targetView, BRect bounds) {
        // This will contain the actual track rendering logic
        // For now, placeholder that draws simple colored blocks
        int trackCount = fProject.CountTracks();
        float laneHeight = 60.0f;
        float trackNameWidth = 150.0f;

        for (int i = 0; i < trackCount; i++) {
            VeniceDAW::Track3DMix* track = fProject.TrackAt(i);
            if (!track) continue;

            float y = i * laneHeight;

            // Alternating lane background
            if (i % 2 == 0) {
                targetView->SetHighColor(40, 40, 45);
            } else {
                targetView->SetHighColor(35, 35, 40);
            }
            targetView->FillRect(BRect(0, y, bounds.right, y + laneHeight - 1));

            // Track name area
            targetView->SetHighColor(50, 50, 55);
            targetView->FillRect(BRect(0, y, trackNameWidth, y + laneHeight - 1));

            // Track color indicator
            VeniceDAW::Coordinate3D pos = track->Position();
            rgb_color trackColor;
            trackColor.red = 100 + (int)(fabs(pos.x) * 10) % 155;
            trackColor.green = 100 + (int)(fabs(pos.y) * 10) % 155;
            trackColor.blue = 100 + (int)(fabs(pos.z) * 10) % 155;

            targetView->SetHighColor(trackColor);
            targetView->FillRect(BRect(5, y + 5, 8, y + laneHeight - 6));

            // Track name text
            targetView->SetFont(be_plain_font);
            font_height fh;
            be_plain_font->GetHeight(&fh);

            targetView->SetHighColor(200, 200, 205);
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

            targetView->DrawString(trackName.String(), BPoint(10, y + (laneHeight / 2) + (fh.ascent / 2)));

            // Lane separator
            targetView->SetHighColor(25, 25, 30);
            targetView->StrokeLine(BPoint(0, y + laneHeight - 1), BPoint(bounds.right, y + laneHeight - 1));
        }
    }

public:

    virtual void MouseDown(BPoint where) override {
        // Check if click is on filter chain widgets or Mute/Solo widgets
        int trackCount = fProject.CountTracks();
        float laneHeight = 60.0f;
        float trackNameWidth = 150.0f;

        for (int i = 0; i < trackCount; i++) {
            float y = i * laneHeight;

            // === FILTER CHAIN HANDLING ===
            if (fFilterChains && i < 64) {
                const float widgetWidth = 20.0f;
                const float filterWidth = 22.0f;
                const float filterHeight = 24.0f;
                const float filterSpacing = 2.0f;

                float widgetTop = y + 8;
                float widgetRight = trackNameWidth - 10;
                float widgetLeft = widgetRight - widgetWidth;
                float filterLeft = widgetLeft - 26;

                const std::vector<FilterInChain>& chain = fFilterChains[i];

                // Check click on existing filters
                for (size_t filterIdx = 0; filterIdx < chain.size(); filterIdx++) {
                    float xPos = filterLeft - (filterIdx * (filterWidth + filterSpacing));
                    BRect filterRect(xPos, widgetTop, xPos + filterWidth, widgetTop + filterHeight);

                    if (filterRect.Contains(where)) {
                        uint32 buttons = 0;
                        BMessage* msg = Window()->CurrentMessage();
                        if (msg) msg->FindInt32("buttons", (int32*)&buttons);

                        if (buttons & B_SECONDARY_MOUSE_BUTTON) {
                            ShowRemoveFilterMenu(where, i, filterIdx);
                        } else {
                            printf("[FilterChain] Clicked filter %zu on track %d (parameters edit - TODO)\n",
                                   filterIdx, i);
                        }
                        return;
                    }
                }

                // Check click on "+ Add Filter" button
                if (chain.size() < 8) {
                    float addBtnX = filterLeft - (chain.size() * (filterWidth + filterSpacing));
                    BRect addBtnRect(addBtnX, widgetTop, addBtnX + filterWidth, widgetTop + filterHeight);

                    if (addBtnRect.Contains(where)) {
                        ShowAddFilterMenu(where, i);
                        return;
                    }
                }
            }

            // === MUTE/SOLO HANDLING ===
            float widgetLeft = trackNameWidth - 24;
            float widgetTop = y + (laneHeight - 24) / 2;
            BRect muteSoloRect(widgetLeft, widgetTop, widgetLeft + 12, widgetTop + 24);

            if (muteSoloRect.Contains(where)) {
                // Click on widget - determine if mute or solo
                float midpoint = widgetTop + 12;

                if (where.y < midpoint) {
                    // Click on Mute (top half)
                    if (fTrackMute && i < 64) {
                        fTrackMute[i] = !fTrackMute[i];
                        printf("[Mute/Solo] Track %d mute: %s\n", i, fTrackMute[i] ? "ON" : "OFF");
                        Invalidate();  // Redraw to show state change
                    }
                } else {
                    // Click on Solo (bottom half) - EXCLUSIVE: only one track can be solo'd at a time
                    if (fTrackSolo && i < 64) {
                        // If clicking the same track that's already solo'd, toggle it off
                        if (fTrackSolo[i]) {
                            fTrackSolo[i] = false;
                            printf("[Mute/Solo] Track %d solo: OFF\n", i);
                        } else {
                            // Clear all other solos first (radio button behavior)
                            for (int j = 0; j < 64; j++) {
                                fTrackSolo[j] = false;
                            }
                            // Then enable this track's solo
                            fTrackSolo[i] = true;
                            printf("[Mute/Solo] Track %d solo: ON (exclusive - all others disabled)\n", i);
                        }
                        Invalidate();  // Redraw to show state change
                    }
                }
                return;  // Click handled
            }
        }

        // Click on timeline area: check if clicking on an audio block (drag) or empty space (seek)
        if (where.x > trackNameWidth + 10) {
            const float kTimelineReferenceRate = 22050.0f;

            // Check if click hits an audio block
            for (int i = 0; i < trackCount; i++) {
                VeniceDAW::Track3DMix* track = fProject.TrackAt(i);
                if (!track) continue;

                float y = i * laneHeight;
                float startTime = track->StartPosition() / kTimelineReferenceRate;
                float endTime = track->EndPosition() / kTimelineReferenceRate;

                // Fallback duration if EndPosition not set
                if (endTime <= startTime) {
                    endTime = startTime + fProject.CalculateTotalDuration();
                }

                float startX = trackNameWidth + 10 + (startTime * fPixelsPerSecond);
                float endX = trackNameWidth + 10 + (endTime * fPixelsPerSecond);
                BRect blockRect(startX, y + 5, endX, y + laneHeight - 6);

                if (blockRect.Contains(where)) {
                    // Start dragging this audio block
                    fDraggingBlock = true;
                    fDragBlockTrackIndex = i;
                    fDragBlockStartMouseX = where.x;
                    fDragBlockOriginalStart = track->StartPosition();
                    printf("[BlockDrag] Started dragging track %d '%s'\n", i, track->TrackName().String());
                    return;
                }
            }

            // Click on empty space = seek
            if (fSharedFramePosition && fSharedSoundPlayer) {
                float clickedTime = (where.x - trackNameWidth - 10) / fPixelsPerSecond;
                if (clickedTime >= 0) {
                    media_raw_audio_format fmt = fSharedSoundPlayer->Format();
                    fSharedFramePosition->store((int64)(clickedTime * fmt.frame_rate));
                    SetPlayheadPosition(clickedTime);
                    printf("[Seek] Jumped to %.2fs\n", clickedTime);
                }
            }
            return;
        }

        BView::MouseDown(where);
    }

    virtual void MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage) override {
        if (fDraggingBlock && fDragBlockTrackIndex >= 0) {
            float deltaX = where.x - fDragBlockStartMouseX;
            float deltaTime = deltaX / fPixelsPerSecond;
            const float kTimelineReferenceRate = 22050.0f;
            int32 deltaSamples = (int32)(deltaTime * kTimelineReferenceRate);

            int32 newStart = fDragBlockOriginalStart + deltaSamples;
            if (newStart < 0) newStart = 0;

            VeniceDAW::Track3DMix* track = fProject.TrackAt(fDragBlockTrackIndex);
            if (track) {
                // Calculate duration to maintain block length
                int32 originalDuration = track->EndPosition() - track->StartPosition();
                track->SetStartPosition(newStart);
                track->SetEndPosition(newStart + originalDuration);
            }

            // Invalidate waveform cache and redraw
            fWaveformCacheValid = false;
            Invalidate();
        }
        BView::MouseMoved(where, code, dragMessage);
    }

    virtual void MouseUp(BPoint where) override {
        if (fDraggingBlock && fDragBlockTrackIndex >= 0) {
            VeniceDAW::Track3DMix* track = fProject.TrackAt(fDragBlockTrackIndex);
            if (track) {
                const float kTimelineReferenceRate = 22050.0f;
                printf("[BlockDrag] Finished: track %d now starts at %.2fs\n",
                       fDragBlockTrackIndex, track->StartPosition() / kTimelineReferenceRate);
            }
            fDraggingBlock = false;
            fDragBlockTrackIndex = -1;
        }
        BView::MouseUp(where);
    }

    virtual void Draw(BRect updateRect) override {
        BRect bounds = Bounds();

        // PERFORMANCE: Use cached waveform bitmap when available
        // This dramatically reduces CPU usage by avoiding re-rendering every frame
        bool playheadOnly = (updateRect.Width() < 30);

        // TEMPORARY: Disable waveform cache to force direct rendering with waveforms
        // TODO: Implement waveform rendering inside RenderTracksToView() for proper caching
        bool useCacheTemporarilyDisabled = false;  // Set to false to disable cache

        // HYBRID APPROACH: Cache renders static elements (backgrounds, track names)
        // while waveforms are drawn on top each frame for accuracy
        if (useCacheTemporarilyDisabled && !playheadOnly && !fWaveformCacheValid) {
            // Cache invalid, rebuild it (happens on zoom changes or first draw)
            RebuildWaveformCache();
        }

        // If cache is valid and we're doing full redraw, use it as base layer!
        if (useCacheTemporarilyDisabled && fWaveformCacheValid && fWaveformCacheBitmap && !playheadOnly) {
            // FAST PATH: Just blit the cached bitmap
            DrawBitmap(fWaveformCacheBitmap, BPoint(0, 0));

            // Skip to playhead drawing (cache already has everything else)
            goto draw_playhead;
        }

        // SLOW PATH: Direct rendering (fallback if cache creation failed)
        // Wrapped in block scope to allow goto jump
        {
        // OPTIMIZATION: Detect if we're only updating playhead
        //bool playheadOnly = (updateRect.Width() < 30);  // Already defined above

        int trackCount = fProject.CountTracks();
        float laneHeight = 60.0f;
        float trackNameWidth = 150.0f;

        SetFont(be_plain_font);
        font_height fh;
        be_plain_font->GetHeight(&fh);

        // PERFORMANCE: Adaptive quality based on zoom level
        // LOW: blocks only, MEDIUM: downsampled waveform, HIGH: full quality
        RenderQuality quality = GetRenderQuality(fPixelsPerSecond);

        // Debug: log quality level once
        static bool qualityLogged = false;
        if (!qualityLogged) {
            const char* qualityStr = (quality == RenderQuality::LOW) ? "LOW" :
                                     (quality == RenderQuality::MEDIUM) ? "MEDIUM" : "HIGH";
            printf("[WaveformRender] Quality=%s (pixelsPerSecond=%.1f)\n", qualityStr, fPixelsPerSecond);
            printf("[WaveformRender] Thresholds: LOW<3, MEDIUM 3-20, HIGH>20\n");
            qualityLogged = true;
        }

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

            // Mute/Solo widget (BeOS 3D Mixer style)
            // Position: left of track name area, 12px wide, 24px tall
            float widgetLeft = trackNameWidth - 24;
            float widgetTop = y + (laneHeight - 24) / 2;  // Center vertically
            BRect muteSoloRect(widgetLeft, widgetTop, widgetLeft + 12, widgetTop + 24);

            // Background for full widget (green by default)
            SetHighColor(80, 200, 80);
            FillRect(muteSoloRect);

            // Mute section (top half)
            BRect muteRect = muteSoloRect;
            muteRect.bottom = widgetTop + 12;
            if (fTrackMute && fTrackMute[i]) {
                SetHighColor(200, 80, 80);  // Red when muted
                FillRect(muteRect);
            }

            // Solo section (bottom half)
            BRect soloRect = muteSoloRect;
            soloRect.top = widgetTop + 12;
            if (fTrackSolo && fTrackSolo[i]) {
                SetHighColor(200, 80, 80);  // Red when solo'd
                FillRect(soloRect);
            }

            // Draw "M" and "S" labels
            SetFontSize(9);
            SetHighColor(20, 20, 20);  // Dark text
            SetDrawingMode(B_OP_OVER);
            DrawString("M", BPoint(widgetLeft + 3, widgetTop + 10));
            DrawString("S", BPoint(widgetLeft + 4, widgetTop + 22));
            SetDrawingMode(B_OP_COPY);

            // Border around widget
            SetHighColor(150, 0, 0);
            StrokeRect(muteSoloRect);
            // Divider line between M and S
            MovePenTo(BPoint(widgetLeft, widgetTop + 12));
            StrokeLine(BPoint(widgetLeft + 12, widgetTop + 12));

            // Reset font size
            SetFontSize(be_plain_font->Size());

            // Filter chain indicators - show ALL filters in the chain
            if (fFilterChains && i < 64) {
                const std::vector<FilterInChain>& chain = fFilterChains[i];

                // Draw each filter in the chain as a small colored box
                float filterLeft = widgetLeft - 26;  // Start left of M/S widget
                const float filterWidth = 22.0f;
                const float filterHeight = 24.0f;
                const float filterSpacing = 2.0f;  // Gap between filter boxes

                for (size_t filterIdx = 0; filterIdx < chain.size(); filterIdx++) {
                    const FilterInChain& filter = chain[filterIdx];

                    // Calculate position (stack horizontally to the left)
                    float xPos = filterLeft - (filterIdx * (filterWidth + filterSpacing));
                    BRect filterRect(xPos, widgetTop, xPos + filterWidth, widgetTop + filterHeight);

                    // Background color based on filter type
                    switch (filter.mode) {
                        case 0:  // LOW_PASS
                            SetHighColor(80, 150, 255);  // Blue
                            break;
                        case 1:  // HIGH_PASS
                            SetHighColor(255, 150, 80);  // Orange
                            break;
                        case 2:  // BAND_PASS
                            SetHighColor(255, 200, 80);  // Yellow
                            break;
                        case 3:  // NOTCH
                            SetHighColor(200, 80, 200);  // Purple
                            break;
                        case 4:  // PEAKING
                            SetHighColor(80, 255, 150);  // Green
                            break;
                        case 5:  // LOW_SHELF
                            SetHighColor(100, 180, 255);  // Light Blue
                            break;
                        case 6:  // HIGH_SHELF
                            SetHighColor(255, 180, 100);  // Light Orange
                            break;
                        default:
                            SetHighColor(150, 150, 150);  // Gray for unknown
                            break;
                    }
                    FillRect(filterRect);

                    // Draw filter type label
                    SetFontSize(7);
                    SetHighColor(20, 20, 20);  // Dark text
                    SetDrawingMode(B_OP_OVER);
                    const char* filterLabel = "";
                    switch (filter.mode) {
                        case 0: filterLabel = "LP"; break;  // Low-Pass
                        case 1: filterLabel = "HP"; break;  // High-Pass
                        case 2: filterLabel = "BP"; break;  // Band-Pass
                        case 3: filterLabel = "NT"; break;  // Notch
                        case 4: filterLabel = "PK"; break;  // Peaking
                        case 5: filterLabel = "LS"; break;  // Low-Shelf
                        case 6: filterLabel = "HS"; break;  // High-Shelf
                        default: filterLabel = "??"; break;
                    }
                    DrawString(filterLabel, BPoint(xPos + 3, widgetTop + 14));
                    SetDrawingMode(B_OP_COPY);

                    // Border
                    SetHighColor(80, 80, 80);
                    StrokeRect(filterRect);
                }

                // Draw "+ Add Filter" button if chain is not full (max 8 filters)
                if (chain.size() < 8) {
                    float addBtnX = filterLeft - (chain.size() * (filterWidth + filterSpacing));
                    BRect addBtnRect(addBtnX, widgetTop, addBtnX + filterWidth, widgetTop + filterHeight);

                    // Subtle background for add button
                    SetHighColor(60, 60, 65);
                    FillRect(addBtnRect);

                    // Draw "+" symbol
                    SetFontSize(12);
                    SetHighColor(150, 150, 150);
                    SetDrawingMode(B_OP_OVER);
                    DrawString("+", BPoint(addBtnX + 7, widgetTop + 17));
                    SetDrawingMode(B_OP_COPY);

                    // Border
                    SetHighColor(100, 100, 100);
                    StrokeRect(addBtnRect);
                }

                // Reset font size
                SetFontSize(be_plain_font->Size());
            }

            // Track region (audio block representation)
            // IMPORTANT: StartPosition/EndPosition define timeline position in samples @ 22050 Hz
            // Each track can start at different time in the mix!

            const float kTimelineReferenceRate = 22050.0f;
            float sampleRate = kTimelineReferenceRate;

            // Get track's timeline position from StartPosition (in samples)
            int32 startSample = track->StartPosition();
            int32 endSample = track->EndPosition();
            float startTime = startSample / kTimelineReferenceRate;
            float endTime = endSample / kTimelineReferenceRate;

            // Calculate duration from audio cache (most accurate!)
            // Resolve audio file path
            BString audioPath = track->AudioFilePath();
            BString resolvedPath;

            if (audioPath.Length() > 0) {
                // Extract filename from path
                BPath pathObj(audioPath.String());
                const char* filename = pathObj.Leaf();
                if (filename && fProjectPath.Length() > 0) {
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

                // Get audio cache - only use it if EndPosition is not set
                if (resolvedPath.Length() > 0) {
                    VeniceDAW::AudioFormat3DMix audioFormat = track->GetAudioFormat();
                    if (audioFormat.sampleRate == 0) {
                        audioFormat.sampleRate = fProject.ProjectSampleRate();
                    }
                    const AudioSampleCache* audioCache = WaveformCache::Instance().GetAudioCache(resolvedPath.String(), &audioFormat);
                    if (audioCache && audioCache->isValid && !audioCache->samples.empty()) {
                        sampleRate = audioCache->sampleRate;

                        // Only override endTime if it wasn't set from EndPosition
                        if (endSample == 0 || endTime <= startTime) {
                            // Calculate end time from audio file length (stereo samples / 2)
                            float audioDuration = (audioCache->samples.size() / 2.0f) / audioCache->sampleRate;
                            endTime = startTime + audioDuration;
                        }
                    }
                }
            }

            // Fallback: use project duration if still not set
            if (endTime <= startTime) {
                float projectDuration = fProject.CalculateTotalDuration();
                endTime = startTime + projectDuration;
            }

            float trackDuration = endTime - startTime;

            // Debug logging for ALL tracks to check positioning
            static bool tracksPrinted = false;
            if (!tracksPrinted) {
                printf("[TrackLanes] Track #%d '%s': StartPos=%d (%.2fs), EndPos=%d (%.2fs), duration=%.2fs, BlockX=%.1f-%.1fpx\n",
                       i, track->TrackName().String(), startSample, startTime, endSample, endTime, trackDuration,
                       trackNameWidth + 10 + (startTime * fPixelsPerSecond), trackNameWidth + 10 + (endTime * fPixelsPerSecond));
                if (i == trackCount - 1) tracksPrinted = true;  // Print once on last track
            }

            if (trackDuration > 0) {
                // Convert times to horizontal positions
                float startX = trackNameWidth + 10 + (startTime * fPixelsPerSecond);
                float endX = trackNameWidth + 10 + (endTime * fPixelsPerSecond);
                BRect blockRect(startX, y + 5, endX, y + laneHeight - 6);

                // Audio block - Bright solid colors (screenshot 50 style)
                SetHighColor(trackColor.red * 0.85, trackColor.green * 0.85, trackColor.blue * 0.85);
                FillRect(blockRect);

                // Audio block border - Bright
                SetHighColor(trackColor.red, trackColor.green, trackColor.blue);
                StrokeRect(blockRect);

                // Real waveform visualization - R6-style on-the-fly rendering!
                int widthPixels = (int)(blockRect.Width());
                const AudioSampleCache* audioCache = nullptr;

                // Try to get audio file path
                // PERFORMANCE: Skip waveform loading in LOW quality mode
                if (quality != RenderQuality::LOW) {
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

                            if (i == 0) {  // Log first track only
                                printf("[WaveformRender] Track '%s': resolved='%s', cache=%p, valid=%d\n",
                                       track->TrackName().String(), resolvedPath.String(),
                                       audioCache, audioCache ? audioCache->isValid : 0);
                            }
                        } else if (i == 0) {
                            printf("[WaveformRender] Track '%s': audioPath='%s', filename='%s' - NO RESOLVED PATH!\n",
                                   track->TrackName().String(), audioPath.String(), filename ? filename : "NULL");
                        }
                    }
                }

                // WAVEFORM RENDERING ENABLED - Classic DAW style with bright colors
                if (quality == RenderQuality::LOW) {
                    // LOW: Simple colored block only (minimal CPU)
                    SetHighColor(trackColor.red * 0.85, trackColor.green * 0.85, trackColor.blue * 0.85, 180);
                    FillRect(blockRect);
                    if (i == 0) {
                        printf("[WaveformRender] Track '%s': Using LOW quality (blocks only, no waveform)\n",
                               track->TrackName().String());
                    }
                } else if (audioCache && audioCache->isValid) {
                    // MEDIUM/HIGH: Draw waveform (detail varies by quality)
                    // Draw real waveform using R6-style on-the-fly GetSample() (FAST!)
                    float centerY = y + laneHeight / 2;
                    // Classic DAW style: moderate height for professional look
                    float maxHeight = (laneHeight - 10) * 0.75f;  // 75% of lane height

                    // Adaptive detail
                    int pixelSkip = 1;
                    if (fPixelsPerSecond < 15.0f) pixelSkip = 2;

                    // Calculate time per pixel (like R6's "zoom" parameter)
                    float secondsPerPixel = 1.0f / fPixelsPerSecond;

                    // Get loop parameters for visual loop repetition (BeOS-style)
                    int64 loopStart = track->LoopStart();
                    int64 loopEnd = track->LoopEnd();
                    float loopStartTime = loopStart / sampleRate;
                    float loopEndTime = loopEnd / sampleRate;
                    float loopLength = loopEndTime - loopStartTime;

                    // Count lines for BeginLineArray
                    int lineCount = 0;
                    for (int px = 0; px < widthPixels; px += pixelSkip) {
                        float x = blockRect.left + px;
                        if (x >= bounds.right) break;
                        lineCount++;
                    }

                    if (lineCount > 0) {
                        // IMPORTANT: Constrain drawing to blockRect to prevent waveforms from escaping!
                        BRegion clipRegion(blockRect);
                        ConstrainClippingRegion(&clipRegion);

                        BeginLineArray(lineCount);
                        // Pastel waveform colors (classic DAW style)
                        rgb_color waveColor = {
                            255,  // White waveform
                            255,
                            255,
                            200  // Slightly transparent for softer look
                        };

                        // Debug: Log waveform rendering for first track
                        if (i == 0) {
                            printf("[WaveformRender] Drawing %d waveform lines for track '%s' (widthPixels=%d, skip=%d)\n",
                                   lineCount, track->TrackName().String(), widthPixels, pixelSkip);
                        }

                        // R6-style rendering: GetSample() called per pixel ON-THE-FLY!
                        float time = 0.0f;  // Start at beginning of audio
                        for (int px = 0; px < widthPixels; px += pixelSkip) {
                            float x = blockRect.left + px;
                            if (x >= bounds.right) break;

                            // Apply loop modulo to show repeating pattern (BeOS-style)
                            float sampleTime = time;
                            if (loopLength > 0 && loopEnd > loopStart) {
                                sampleTime = loopStartTime + fmod(time, loopLength);
                            }

                            // Calculate min/max for THIS pixel's time range (like R6!)
                            float minPeak, maxPeak;
                            audioCache->GetSample(sampleTime, secondsPerPixel, &minPeak, &maxPeak);

                            // Moderate amplification for natural look (classic DAW style)
                            const float amplification = 1.3f;  // Gentle boost for visibility
                            minPeak = fmax(-1.0f, fmin(1.0f, minPeak * amplification));
                            maxPeak = fmax(-1.0f, fmin(1.0f, maxPeak * amplification));

                            float minY = centerY - (minPeak * maxHeight);
                            float maxY = centerY - (maxPeak * maxHeight);

                            AddLine(BPoint(x, minY), BPoint(x, maxY), waveColor);

                            time += secondsPerPixel;  // Advance time (like R6: time += zoom)
                        }
                        EndLineArray();

                        // Reset clipping region
                        ConstrainClippingRegion(NULL);
                    }

                    // Draw loop markers (vertical lines showing where loop repeats)
                    if (loopLength > 0 && loopEnd > loopStart) {
                        SetHighColor(255, 255, 255, 100);  // Semi-transparent white
                        SetPenSize(1.0);

                        // Draw a line at each loop repetition point
                        float currentLoopTime = loopLength;
                        while (currentLoopTime < trackDuration) {
                            float loopMarkerX = blockRect.left + (currentLoopTime * fPixelsPerSecond);
                            if (loopMarkerX >= blockRect.left && loopMarkerX <= blockRect.right) {
                                // Draw dashed line effect
                                float dashY = blockRect.top;
                                while (dashY < blockRect.bottom) {
                                    StrokeLine(BPoint(loopMarkerX, dashY),
                                              BPoint(loopMarkerX, fmin(dashY + 4, blockRect.bottom)));
                                    dashY += 8;  // 4px dash, 4px gap
                                }
                            }
                            currentLoopTime += loopLength;
                        }
                    }
                } else if (quality != RenderQuality::LOW) {
                    // Fallback: pseudo-random waveform (MEDIUM/HIGH quality)
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

                // Convert samples to seconds
                // IMPORTANT: StartPosition/EndPosition were calculated at 22050 Hz in parser
                // We must use the same rate here for correct timeline positioning
                const float kTimelineReferenceRate = 22050.0f;
                float sampleRate = kTimelineReferenceRate;  // Always use 22050 for timeline positioning
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
                        if (quality == RenderQuality::LOW) {
                            // LOW quality: Just the colored block - already filled above
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

                                            // Get loop parameters for visual loop repetition
                                            int64 loopStart = track->LoopStart();
                                            int64 loopEnd = track->LoopEnd();
                                            float loopStartTime = loopStart / audioCache->sampleRate;
                                            float loopEndTime = loopEnd / audioCache->sampleRate;
                                            float loopLength = loopEndTime - loopStartTime;

                                            // Calculate start time for this update rect
                                            float updateStartTime = (updateBlockRect.left - startX) / fPixelsPerSecond;

                                            for (int px = 0; px < widthPixels; px += pixelSkip) {
                                                float x = updateBlockRect.left + px;
                                                if (x >= blockRect.right || x >= bounds.right) break;

                                                float time = updateStartTime + (px / fPixelsPerSecond);

                                                // Apply loop modulo to show repeating pattern (BeOS-style)
                                                float sampleTime = time;
                                                if (loopLength > 0 && loopEnd > loopStart) {
                                                    sampleTime = loopStartTime + fmod(time, loopLength);
                                                }

                                                float minPeak, maxPeak;
                                                audioCache->GetSample(sampleTime, secondsPerPixel, &minPeak, &maxPeak);

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
        }  // End of slow path block scope

    draw_playhead:
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

    // Mute/Solo state (pointers to DemoWindow arrays)
    bool* fTrackMute;
    bool* fTrackSolo;

    // Filter state (pointers to DemoWindow arrays)
    bool* fFilterEnabled;  // LEGACY: Filter enabled for each track (single filter)
    int* fFilterMode;      // LEGACY: FilterMode enum value for each track (single filter)
    std::vector<FilterInChain>* fFilterChains;  // NEW: Filter chains for each track (array of 64 vectors)

    BWindow* fParentWindow;  // For posting mute/solo change messages

    // Shared audio state for seek/scrub (from DemoWindow via TimelineContentView)
    BSoundPlayer* fSharedSoundPlayer;
    std::atomic<int64>* fSharedFramePosition;

    // Block drag state (drag audio blocks in timeline)
    bool fDraggingBlock = false;
    int fDragBlockTrackIndex = -1;
    float fDragBlockStartMouseX = 0.0f;
    int32 fDragBlockOriginalStart = 0;

    // PERFORMANCE: Waveform bitmap cache to avoid redrawing every frame
    BBitmap* fWaveformCacheBitmap;  // Pre-rendered waveforms
    bool fWaveformCacheValid;       // Is cache still valid?
    float fCachedZoom;              // Zoom level when cache was created
};

// Forward declaration for TimelineWindow
class TimelineWindow;

// Main Timeline Content View - Container for ruler and lanes
class TimelineContentView : public BView {
public:
    TimelineContentView(BRect frame, const VeniceDAW::Project3DMix& project, const char* projectFilePath,
                        BSoundPlayer* sharedSoundPlayer, std::atomic<int64>* sharedFramePosition, std::atomic<bool>* sharedIsPlaying,
                        bool* trackMute, bool* trackSolo, bool* filterEnabled, int* filterMode,
                        std::vector<FilterInChain>* filterChains, BWindow* parentWindow)
        : BView(frame, "timeline_content", B_FOLLOW_ALL, 0)
        , fProject(project)
        , fProjectPath(projectFilePath)
        , fPixelsPerSecond(50.0f)  // Initial zoom
        , fMinPixelsPerSecond(1.0f)  // Will be set by FitToWindow()
        , fPlayheadPosition(0.0f)
        , fLoopEnabled(false)
        , fLoopInPoint(0.0f)
        , fLoopOutPoint(0.0f)
        , fSharedSoundPlayer(sharedSoundPlayer)
        , fSharedFramePosition(sharedFramePosition)
        , fSharedIsPlaying(sharedIsPlaying)
    {
        SetViewColor(B_TRANSPARENT_COLOR);  // Transparent so children draw themselves

        // Calculate content size for scrollable area
        // Width: Calculate based on longest track duration
        float maxDuration = 0.0f;
        int trackCount = project.CountTracks();
        for (int i = 0; i < trackCount; i++) {
            VeniceDAW::Track3DMix* track = project.TrackAt(i);
            if (!track) continue;
            float endTime = track->EndPosition() / 22050.0f;
            if (endTime > maxDuration) maxDuration = endTime;
        }
        float contentWidth = 160.0f + (maxDuration * fPixelsPerSecond) + 100.0f; // trackNameWidth + timeline + margin
        float contentHeight = trackCount * 60.0f + 20.0f; // laneHeight * tracks + margin

        // Create track lanes view (this goes INSIDE scroll view)
        BRect lanesFrame(0, 0, contentWidth, contentHeight);
        fLanesView = new TrackLanesView(lanesFrame, project, fPixelsPerSecond, projectFilePath,
                                        trackMute, trackSolo, filterEnabled, filterMode, filterChains, parentWindow,
                                        sharedSoundPlayer, sharedFramePosition);

        // Wrap lanes in scroll view (fills entire content view)
        BRect scrollFrame = Bounds();
        fScrollView = new BScrollView("timeline_scroll", fLanesView,
                                      B_FOLLOW_ALL_SIDES, 0,
                                      true,  // horizontal scrollbar
                                      true,  // vertical scrollbar
                                      B_FANCY_BORDER);
        AddChild(fScrollView);

        printf("[Timeline] Scroll view fills content area: (%.1f, %.1f, %.1f, %.1f)\n",
               scrollFrame.left, scrollFrame.top, scrollFrame.right, scrollFrame.bottom);
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
        if (fPixelsPerSecond < fMinPixelsPerSecond) fPixelsPerSecond = fMinPixelsPerSecond;
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
            // IMPORTANT: StartPosition/EndPosition were calculated at 22050 Hz in parser
            // We must use the same rate here for correct timeline positioning
            const float kTimelineReferenceRate = 22050.0f;
            float sampleRate = kTimelineReferenceRate;  // Always use 22050 for timeline positioning

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

            // Save this as the minimum zoom level (can't zoom out beyond initial view)
            fMinPixelsPerSecond = fPixelsPerSecond;

            UpdateZoom();
            printf("[Timeline] Fit to window: %.1f seconds @ %.1f px/sec (min zoom set)\n",
                   maxDuration, fPixelsPerSecond);
        }
    }

    void UpdateZoom() {
        // Update ruler at window level via message
        BMessage msg('TZom');  // Timeline Zoom
        msg.AddFloat("zoom", fPixelsPerSecond);
        Window()->PostMessage(&msg);

        // Update lanes
        fLanesView->SetPixelsPerSecond(fPixelsPerSecond);

        // Update content size for scrollable area based on new zoom level
        float maxDuration = 0.0f;
        int trackCount = fProject.CountTracks();
        for (int i = 0; i < trackCount; i++) {
            VeniceDAW::Track3DMix* track = fProject.TrackAt(i);
            if (!track) continue;
            float endTime = track->EndPosition() / 22050.0f;
            if (endTime > maxDuration) maxDuration = endTime;
        }

        float contentWidth = 160.0f + (maxDuration * fPixelsPerSecond) + 100.0f;
        float contentHeight = trackCount * 60.0f + 20.0f;

        // Resize track lanes view (the scrollable content)
        fLanesView->ResizeTo(contentWidth, contentHeight);

        // Update scrollbar ranges
        BScrollBar* hScroll = fScrollView->ScrollBar(B_HORIZONTAL);
        BScrollBar* vScroll = fScrollView->ScrollBar(B_VERTICAL);
        if (hScroll) {
            BRect scrollBounds = fScrollView->Bounds();
            float maxH = contentWidth - scrollBounds.Width();
            if (maxH < 0) maxH = 0;
            hScroll->SetRange(0, maxH);
            hScroll->SetProportion(scrollBounds.Width() / contentWidth);
        }
        if (vScroll) {
            BRect scrollBounds = fScrollView->Bounds();
            float maxV = contentHeight - scrollBounds.Height();
            if (maxV < 0) maxV = 0;
            vScroll->SetRange(0, maxV);
            vScroll->SetProportion(scrollBounds.Height() / contentHeight);
        }
    }

    void TogglePlayback() {
        *fSharedIsPlaying = !(*fSharedIsPlaying);

        if (fSharedSoundPlayer) {
            if (*fSharedIsPlaying) {
                // Start audio playback
                fSharedSoundPlayer->Start();
                fSharedSoundPlayer->SetHasData(true);
                printf("[AudioPlayback] STARTED at position %.2fs (frame %ld)\n",
                       fPlayheadPosition, fSharedFramePosition->load());
            } else {
                // Stop audio playback
                fSharedSoundPlayer->Stop();
                printf("[AudioPlayback] PAUSED at position %.2fs (frame %ld)\n",
                       fPlayheadPosition, fSharedFramePosition->load());
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

            // Handle loop region playback
            if (fLoopEnabled && fLoopOutPoint > fLoopInPoint) {
                if (fPlayheadPosition >= fLoopOutPoint) {
                    // Loop back to in point
                    fPlayheadPosition = fLoopInPoint;
                    *fSharedFramePosition = (int64)(fLoopInPoint * format.frame_rate);
                    printf("[Loop] Looping back from %.2fs to %.2fs\n", fLoopOutPoint, fLoopInPoint);
                }
            } else {
                // Normal playback: loop back to start at end of song
                float duration = fProject.CalculateTotalDuration();
                if (duration > 0 && fPlayheadPosition > duration) {
                    fPlayheadPosition = 0.0f;
                    *fSharedFramePosition = 0;
                }
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
            // IMPORTANT: StartPosition/EndPosition were calculated at 22050 Hz in parser
            // We must use the same rate here for correct timeline positioning
            const float kTimelineReferenceRate = 22050.0f;
            float sampleRate = kTimelineReferenceRate;  // Always use 22050 for timeline positioning
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

        printf("[Timeline] AttachedToWindow: Ruler is FIXED at top, always visible\n");
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
            case 'i':
            case 'I':
                // Set loop in point at current playhead position
                fLoopInPoint = fPlayheadPosition;
                if (fLoopOutPoint <= fLoopInPoint) {
                    fLoopOutPoint = fLoopInPoint + 5.0f;  // Default 5 second loop
                }
                fLanesView->Invalidate();
                SyncLoopToDemoWindow();
                printf("[Loop] In point set at %.2fs\n", fLoopInPoint);
                break;
            case 'o':
            case 'O':
                // Set loop out point at current playhead position
                fLoopOutPoint = fPlayheadPosition;
                if (fLoopInPoint >= fLoopOutPoint) {
                    fLoopInPoint = fmax(0.0f, fLoopOutPoint - 5.0f);
                }
                fLanesView->Invalidate();
                SyncLoopToDemoWindow();
                printf("[Loop] Out point set at %.2fs\n", fLoopOutPoint);
                break;
            case 'l':
            case 'L':
                // Toggle loop enabled/disabled
                fLoopEnabled = !fLoopEnabled;
                fLanesView->Invalidate();
                SyncLoopToDemoWindow();
                printf("[Loop] Loop %s (%.2fs - %.2fs)\n",
                       fLoopEnabled ? "ENABLED" : "DISABLED",
                       fLoopInPoint, fLoopOutPoint);
                break;
            default:
                BView::KeyDown(bytes, numBytes);
                break;
        }
    }

private:
    const VeniceDAW::Project3DMix& fProject;
    BString fProjectPath;  // Full path to .3dmix file for audio file resolution
    TrackLanesView* fLanesView;  // Inside scroll view
    BScrollView* fScrollView;  // Scroll container for lanes only
    float fPixelsPerSecond;
    float fMinPixelsPerSecond;  // Minimum zoom level (set at window creation)
    float fPlayheadPosition;

    // Sync loop region to DemoWindow's audio-thread atomics via BMessage
    void SyncLoopToDemoWindow() {
        BMessage msg('LpSy');  // Loop Sync
        msg.AddBool("enabled", fLoopEnabled);
        msg.AddFloat("in", fLoopInPoint);
        msg.AddFloat("out", fLoopOutPoint);
        Window()->PostMessage(&msg);  // TimelineWindow receives and forwards to DemoWindow
    }

    // Loop region state
    bool fLoopEnabled;
    float fLoopInPoint;   // Loop start time in seconds
    float fLoopOutPoint;  // Loop end time in seconds

    // Shared audio playback system (owned by DemoWindow)
    BSoundPlayer* fSharedSoundPlayer;
    std::atomic<int64>* fSharedFramePosition;
    std::atomic<bool>* fSharedIsPlaying;
};

// Calculate window size based on track count
static BRect CalculateWindowRect(const VeniceDAW::Project3DMix& project) {
    int trackCount = project.CountTracks();

    // Get screen dimensions
    BScreen screen;
    BRect screenFrame = screen.Frame();
    float maxScreenHeight = screenFrame.Height() - 100.0f;  // Leave 100px for window title/taskbar

    // Window dimensions
    const float kWindowLeft = 100.0f;
    const float kWindowTop = 100.0f;
    const float kWindowWidth = 1300.0f;  // Fixed width

    // Height calculation
    const float kRulerHeight = 41.0f;     // Ruler at top
    const float kLaneHeight = 60.0f;      // Each track lane
    const float kScrollbarHeight = 15.0f; // Horizontal scrollbar
    const float kMaxTracks = 15;          // Maximum tracks before scrollbar

    // Calculate how many tracks can fit on screen
    float maxTracksForScreen = (maxScreenHeight - kRulerHeight - kScrollbarHeight) / kLaneHeight;

    // Exact fit: show exactly as many tracks as present, up to max
    float visibleTracks = trackCount;
    if (visibleTracks > kMaxTracks) visibleTracks = kMaxTracks;
    if (visibleTracks > maxTracksForScreen) visibleTracks = maxTracksForScreen;
    if (visibleTracks < 1) visibleTracks = 1;  // At least 1 track

    float windowHeight = kRulerHeight + (visibleTracks * kLaneHeight) + kScrollbarHeight;

    printf("[Timeline] Screen height: %.0f, max tracks on screen: %.0f\n", maxScreenHeight, maxTracksForScreen);
    printf("[Timeline] Window size: %d tracks → %.0f x %.0f pixels (showing %.0f tracks)\n",
           trackCount, kWindowWidth, windowHeight, visibleTracks);

    return BRect(kWindowLeft, kWindowTop,
                 kWindowLeft + kWindowWidth,
                 kWindowTop + windowHeight);
}

// Timeline visualization window
class TimelineWindow : public BWindow {
public:
    TimelineWindow(const VeniceDAW::Project3DMix& project, const char* projectFilePath,
                   BSoundPlayer* sharedSoundPlayer, std::atomic<int64>* sharedFramePosition, std::atomic<bool>* sharedIsPlaying,
                   bool* trackMute, bool* trackSolo, bool* filterEnabled, int* filterMode,
                   std::vector<FilterInChain>* filterChains, BWindow* parentWindow)
        : BWindow(CalculateWindowRect(project), "Timeline View - Modern DAW Style",
                  B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
        , fProject(project)
        , fProjectPath(projectFilePath)
        , fContentView(nullptr)
        , fUpdateRunner(nullptr)
        , fRulerView(nullptr)
        , fParentDemoWindow(parentWindow)
    {
        BRect bounds = Bounds();
        const float kRulerHeight = 41.0f;

        // Create ruler DIRECTLY in window (not in content view!)
        BRect rulerFrame(0, 0, bounds.Width(), kRulerHeight - 1);
        fRulerView = new TimeRulerView(rulerFrame, project, 50.0f);
        fRulerView->SetViewColor(45, 45, 50);  // Dark gray background
        AddChild(fRulerView);

        // Create content view BELOW ruler
        BRect contentFrame(0, kRulerHeight, bounds.Width(), bounds.Height());
        fContentView = new TimelineContentView(contentFrame, project, projectFilePath,
                                                sharedSoundPlayer, sharedFramePosition, sharedIsPlaying,
                                                trackMute, trackSolo, filterEnabled, filterMode, filterChains, parentWindow);
        AddChild(fContentView);

        printf("[TimelineWindow] Created with ruler at window level\n");
        printf("[TimelineWindow] Ruler: (%.0f, %.0f, %.0f, %.0f)\n",
               rulerFrame.left, rulerFrame.top, rulerFrame.right, rulerFrame.bottom);
        printf("[TimelineWindow] Content: (%.0f, %.0f, %.0f, %.0f)\n",
               contentFrame.left, contentFrame.top, contentFrame.right, contentFrame.bottom);
    }

    virtual void Show() override {
        BWindow::Show();

        // Animation timer at 30 FPS
        if (!fUpdateRunner) {
            printf("[TimelineWindow] Starting animation timer (30 FPS)\n");
            BMessage pulse('Tpls');
            fUpdateRunner = new BMessageRunner(this, &pulse, 33333);  // 33ms = ~30 FPS
        }
    }

    ~TimelineWindow() {
        if (fUpdateRunner) {
            delete fUpdateRunner;
            fUpdateRunner = nullptr;
        }
    }

    virtual void MessageReceived(BMessage* message) override {
        switch (message->what) {
            case 'Tpls':  // Timeline pulse (30 FPS)
                if (fContentView) {
                    fContentView->UpdatePlayhead(0.033f);  // 33ms in seconds
                }
                break;

            case 'TZom': {  // Timeline Zoom change
                float zoom = 50.0f;
                if (message->FindFloat("zoom", &zoom) == B_OK) {
                    SetRulerZoom(zoom);
                }
                break;
            }

            case 'LpSy': {  // Loop Sync from TimelineContentView → forward to DemoWindow
                if (fParentDemoWindow) {
                    fParentDemoWindow->PostMessage(message);
                }
                break;
            }

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
        printf("[TimelineWindow] On-demand rendering mode (no timer)\n");
        // printf("  fUpdateRunner: %p\n", fUpdateRunner);  // DISABLED
        printf("  fContentView: %p\n", fContentView);
    }

    void SetPlayheadPosition(float position) {
        if (fContentView) {
            fContentView->ResetPlayhead();  // Reset to start (position 0)
        }
    }

    void SetRulerZoom(float pixelsPerSecond) {
        if (fRulerView) {
            fRulerView->SetPixelsPerSecond(pixelsPerSecond);
            fRulerView->Invalidate();
        }
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
    TimeRulerView* fRulerView;  // Ruler at window level
    BMessageRunner* fUpdateRunner;
    BWindow* fParentDemoWindow;  // DemoWindow for loop sync messages
};

// Master VU Meter View - Shows stereo L/R levels horizontally
class MasterVUMeterView : public BView {
public:
    MasterVUMeterView(BRect frame)
        : BView(frame, "master_vu", B_FOLLOW_BOTTOM | B_FOLLOW_LEFT, B_WILL_DRAW)
        , fLeftLevel(0.0f)
        , fRightLevel(0.0f)
    {
        SetViewColor(B_TRANSPARENT_COLOR);
    }

    void SetLevels(float left, float right) {
        fLeftLevel = left;
        fRightLevel = right;
        Invalidate();
    }

    virtual void Draw(BRect updateRect) override {
        BRect bounds = Bounds();

        // Background
        SetHighColor(20, 20, 25);
        FillRect(bounds);

        // Labels
        SetHighColor(180, 180, 180);
        SetFontSize(9);
        DrawString("L", BPoint(5, 12));
        DrawString("R", BPoint(5, 27));

        // Meter dimensions
        const float meterLeft = 20;
        const float meterWidth = bounds.Width() - 25;
        const float meterHeight = 8;

        // Left channel meter (top)
        BRect leftRect(meterLeft, 4, meterLeft + meterWidth, 4 + meterHeight);
        DrawMeter(leftRect, fLeftLevel);

        // Right channel meter (bottom)
        BRect rightRect(meterLeft, 19, meterLeft + meterWidth, 19 + meterHeight);
        DrawMeter(rightRect, fRightLevel);
    }

private:
    void DrawMeter(BRect rect, float level) {
        // Background
        SetHighColor(40, 40, 45);
        FillRect(rect);

        // Level bar
        float fillWidth = rect.Width() * level;
        if (fillWidth > 0) {
            BRect fillRect = rect;
            fillRect.right = fillRect.left + fillWidth;

            // Color based on level: Green → Yellow → Red
            if (level < 0.5f) {
                // Green zone (0.0 - 0.5)
                uint8 green = (uint8)(255 * (level * 2.0f));
                SetHighColor(0, green, 0);
            } else if (level < 0.8f) {
                // Yellow zone (0.5 - 0.8)
                float yellowMix = (level - 0.5f) / 0.3f;
                uint8 red = (uint8)(255 * yellowMix);
                SetHighColor(red, 200, 0);
            } else {
                // Red zone (0.8 - 1.0+)
                SetHighColor(220, 0, 0);
            }

            FillRect(fillRect);
        }

        // Border
        SetHighColor(80, 80, 85);
        StrokeRect(rect);
    }

    float fLeftLevel;
    float fRightLevel;
};

// Forward declaration
class MasterVUMeterView;

// Control Bar View - Professional transport controls (Haiku MediaPlayer style)
class ControlBarView : public BView {
public:
    ControlBarView(BRect frame)
        : BView(frame, "control_bar", B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW)
    {
        SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

        const float margin = 8;
        const float spacing = 6;
        const float buttonHeight = 32;
        const float topMargin = 6;

        // Calculate centered position for transport controls
        float centerY = (frame.Height() - buttonHeight) / 2;
        float xPos = margin;

        // === SECTION 1: VU METER (left) ===
        BRect vuRect(xPos, topMargin + 4, xPos + 180, frame.Height() - topMargin - 4);
        fVUMeter = new MasterVUMeterView(vuRect);
        AddChild(fVUMeter);
        xPos += 180 + spacing * 3;

        // Separator line
        DrawSeparatorLine(xPos);
        xPos += spacing * 2;

        // === SECTION 2: TRANSPORT CONTROLS (center-left) ===
        // Stop button
        BRect stopRect(xPos, centerY, xPos + 60, centerY + buttonHeight);
        fStopButton = new BButton(stopRect, "stop", "■", new BMessage('Stop'));
        fStopButton->SetTarget(NULL);  // Will be set by parent
        AddChild(fStopButton);
        xPos += 60 + spacing;

        // Play/Pause button (larger, primary action)
        BRect playRect(xPos, centerY, xPos + 80, centerY + buttonHeight);
        fPlayButton = new BButton(playRect, "play", "▶ Play", new BMessage('Play'));
        fPlayButton->SetTarget(NULL);  // Will be set by parent
        fPlayButton->MakeDefault(true);  // Primary button
        AddChild(fPlayButton);
        xPos += 80 + spacing * 3;

        // Separator line
        DrawSeparatorLine(xPos);
        xPos += spacing * 2;

        // === SECTION 3: TIME DISPLAY (center) ===
        BRect timeRect(xPos, centerY + 6, xPos + 140, centerY + buttonHeight - 6);
        fTimeDisplay = new BStringView(timeRect, "time", "0:00.0 / 0:00.0");
        fTimeDisplay->SetAlignment(B_ALIGN_CENTER);
        fTimeDisplay->SetFont(be_fixed_font);
        fTimeDisplay->SetHighColor(tint_color(ui_color(B_PANEL_TEXT_COLOR), B_DARKEN_1_TINT));
        AddChild(fTimeDisplay);
        xPos += 140 + spacing * 3;

        // Separator line
        DrawSeparatorLine(xPos);
        xPos += spacing * 2;

        // === SECTION 4: MASTER VOLUME (right) ===
        // Volume label
        BRect volLabelRect(xPos, topMargin, xPos + 50, topMargin + 14);
        BStringView* volLabel = new BStringView(volLabelRect, "vol_label", "Volume");
        volLabel->SetFont(be_plain_font);
        volLabel->SetFontSize(9);
        volLabel->SetAlignment(B_ALIGN_CENTER);
        AddChild(volLabel);

        // Volume slider
        BRect sliderRect(xPos, topMargin + 16, xPos + 50, frame.Height() - topMargin - 2);
        fVolumeSlider = new BSlider(sliderRect, "volume", nullptr,
                                    new BMessage('MstV'), 0, 200, B_TRIANGLE_THUMB);
        fVolumeSlider->SetModificationMessage(new BMessage('MstV'));
        fVolumeSlider->SetValue(100);
        fVolumeSlider->SetHashMarks(B_HASH_MARKS_RIGHT);
        fVolumeSlider->SetHashMarkCount(3);
        fVolumeSlider->SetLimitLabels("0", "200");
        fVolumeSlider->SetTarget(NULL);  // Will be set by parent
        AddChild(fVolumeSlider);
    }

    virtual void Draw(BRect updateRect) override {
        BView::Draw(updateRect);

        // Draw top border line
        SetHighColor(tint_color(ViewColor(), B_DARKEN_2_TINT));
        StrokeLine(BPoint(0, 0), BPoint(Bounds().right, 0));

        // Draw bottom shadow
        SetHighColor(tint_color(ViewColor(), B_LIGHTEN_MAX_TINT));
        StrokeLine(BPoint(0, 1), BPoint(Bounds().right, 1));
    }

    void DrawSeparatorLine(float x) {
        // Add visual separator (will be drawn in Draw())
        // Store positions for drawing
    }

    // Accessors for parent window
    BButton* PlayButton() const { return fPlayButton; }
    BButton* StopButton() const { return fStopButton; }
    BStringView* TimeDisplay() const { return fTimeDisplay; }
    MasterVUMeterView* VUMeter() const { return fVUMeter; }
    BSlider* VolumeSlider() const { return fVolumeSlider; }

private:
    BButton* fPlayButton;
    BButton* fStopButton;
    BStringView* fTimeDisplay;
    MasterVUMeterView* fVUMeter;
    BSlider* fVolumeSlider;
};

// Helper function to build window title with filename
static BString BuildWindowTitle(const char* projectPath) {
    BString title = "3DMix Viewer - ";

    if (projectPath) {
        // Extract filename from full path
        BPath path(projectPath);
        if (path.InitCheck() == B_OK) {
            title << path.Leaf();
        } else {
            // Fallback: find last slash manually
            const char* lastSlash = strrchr(projectPath, '/');
            if (lastSlash) {
                title << (lastSlash + 1);
            } else {
                title << projectPath;
            }
        }
    } else {
        title << "No File";
    }

    return title;
}

class DemoWindow : public BWindow {
public:
    DemoWindow(const char* projectPath)
        : BWindow(BRect(100, 100, 900, 700),
                  projectPath ? BuildWindowTitle(projectPath).String() : "3DMix Viewer - No File",
                  B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
        , fGLView(nullptr)
        // , fUpdateRunner(nullptr)  // DISABLED for on-demand rendering
        , fTimelineWindow(nullptr)
        , fProject(nullptr)
        , fProjectPath(projectPath ? projectPath : "")  // Store the project file path
        , fOpenPanel(nullptr)
        , fControlBar(nullptr)
        , fPlayButton(nullptr)
        , fStopButton(nullptr)
        , fTimeDisplay(nullptr)
        , fMasterVUMeter(nullptr)
        , fMasterVolumeSlider(nullptr)
        , fSoundPlayer(nullptr)
        , fMasterVolume(1.0f)  // Unity gain (100%)
        , fCurrentFramePosition(0)
        , fIsPlaying(false)
        , fLastTimeDisplayUpdate(0)
        , fMasterLevelLeft(0.0f)
        , fMasterLevelRight(0.0f)
        , fAnySoloActive(false)
        , fSelectedTrackIndex(-1)  // No track selected initially
        , fFrameCounter(0)
    {
        // Initialize track levels to zero
        memset(fTrackLevels, 0, sizeof(fTrackLevels));

        // Initialize mute/solo states (all off by default)
        memset(fTrackMute, 0, sizeof(fTrackMute));
        memset(fTrackSolo, 0, sizeof(fTrackSolo));

        // Initialize filters (all disabled by default)
        memset(fFilterEnabled, 0, sizeof(fFilterEnabled));
        memset(fFilterMode, 0, sizeof(fFilterMode));  // All set to LOW_PASS (0) by default

        BRect bounds = Bounds();

        // Create menu bar
        const float menuHeight = 20;
        BRect menuRect = bounds;
        menuRect.bottom = menuRect.top + menuHeight;
        BMenuBar* menuBar = new BMenuBar(menuRect, "menu_bar");

        BMenu* fileMenu = new BMenu("File");
        fileMenu->AddItem(new BMenuItem("Open...", new BMessage(MSG_OPEN_FILE), 'O'));

        // Recent Files submenu
        fRecentFilesMenu = new BMenu("Open Recent");
        fileMenu->AddItem(fRecentFilesMenu);
        LoadRecentFiles();
        UpdateRecentFilesMenu();

        fileMenu->AddSeparatorItem();
        fileMenu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
        menuBar->AddItem(fileMenu);

        // Help menu
        BMenu* helpMenu = new BMenu("Help");
        helpMenu->AddItem(new BMenuItem("About VeniceDAW...", new BMessage(MSG_ABOUT)));
        menuBar->AddItem(helpMenu);

        AddChild(menuBar);

        // Reserve space at bottom for controls and at top for menu
        const float controlHeight = 50;
        BRect glRect = bounds;
        glRect.top += menuHeight;
        glRect.bottom -= controlHeight;

        // Create GL view (reduced height)
        fGLView = new DemoGL3DView(glRect, "gl_view");
        AddChild(fGLView);

        // Create professional control bar (Haiku MediaPlayer style)
        BRect controlBarRect(0, bounds.bottom - controlHeight,
                            bounds.right, bounds.bottom);
        fControlBar = new ControlBarView(controlBarRect);
        AddChild(fControlBar);

        // Get control references and set targets
        fPlayButton = fControlBar->PlayButton();
        fStopButton = fControlBar->StopButton();
        fTimeDisplay = fControlBar->TimeDisplay();
        fMasterVUMeter = fControlBar->VUMeter();
        fMasterVolumeSlider = fControlBar->VolumeSlider();

        fPlayButton->SetTarget(this);
        fStopButton->SetTarget(this);
        fMasterVolumeSlider->SetTarget(this);

        // Initialize master volume from slider value (ensure sync at startup)
        if (fMasterVolumeSlider) {
            int32 sliderValue = fMasterVolumeSlider->Value();
            fMasterVolume = sliderValue / 100.0f;  // Sync internal state with slider
            printf("[Init] Master volume set to %d%% (gain=%.2f)\n", (int)sliderValue, fMasterVolume);
        }

        // Load and parse 3dmix file (if provided)
        if (projectPath && strlen(projectPath) > 0) {
            LoadProject(projectPath);
            InitAudioPlayback();
            // Auto-start playback
            if (fSoundPlayer && fProject) {
                TogglePlayback();
            }
        } else {
            printf("No file specified. Use File > Open... to load a 3DMix project.\n");
        }

        // Start animation timer at 30 FPS
        BMessage pulse(MSG_PULSE);
        fUpdateRunner = new BMessageRunner(this, &pulse, 33333);  // 33ms = ~30 FPS
    }

    ~DemoWindow() {
        // Stop audio first
        if (fSoundPlayer) {
            fSoundPlayer->Stop();
            delete fSoundPlayer;
            fSoundPlayer = nullptr;
        }

        delete fUpdateRunner;

        // Clean up file panel
        delete fOpenPanel;

        // Properly close TimelineWindow if it exists
        if (fTimelineWindow && fTimelineWindow->Lock()) {
            fTimelineWindow->Quit();
            // Don't delete - Quit() will handle cleanup
        }

        delete fProject;
        SaveRecentFiles();
    }

    // Load recent files from settings
    void LoadRecentFiles() {
        BPath settingsPath;
        find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath);
        settingsPath.Append("VeniceDAW_Recent");

        BFile file(settingsPath.Path(), B_READ_ONLY);
        if (file.InitCheck() == B_OK) {
            // Read entire file
            off_t fileSize;
            file.GetSize(&fileSize);
            if (fileSize > 0 && fileSize < 10240) {  // Max 10KB
                char* fileContent = new char[fileSize + 1];
                ssize_t bytesRead = file.Read(fileContent, fileSize);
                if (bytesRead > 0) {
                    fileContent[bytesRead] = '\0';

                    // Parse lines
                    int index = 0;
                    char* line = strtok(fileContent, "\n");
                    while (line != NULL && index < 5) {
                        if (strlen(line) > 0) {
                            fRecentFiles[index] = line;
                            index++;
                        }
                        line = strtok(NULL, "\n");
                    }
                }
                delete[] fileContent;
            }
        }
    }

    // Save recent files to settings
    void SaveRecentFiles() {
        BPath settingsPath;
        find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath);
        settingsPath.Append("VeniceDAW_Recent");

        BFile file(settingsPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
        if (file.InitCheck() == B_OK) {
            for (int i = 0; i < 5; i++) {
                if (fRecentFiles[i].Length() > 0) {
                    file.Write(fRecentFiles[i].String(), fRecentFiles[i].Length());
                    file.Write("\n", 1);
                }
            }
        }
    }

    // Add file to recent files list
    void AddToRecentFiles(const char* path) {
        BString newFile(path);

        // Check if already in list
        for (int i = 0; i < 5; i++) {
            if (fRecentFiles[i] == newFile) {
                // Move to top
                for (int j = i; j > 0; j--) {
                    fRecentFiles[j] = fRecentFiles[j-1];
                }
                fRecentFiles[0] = newFile;
                UpdateRecentFilesMenu();
                return;
            }
        }

        // Shift all down and add to top
        for (int i = 4; i > 0; i--) {
            fRecentFiles[i] = fRecentFiles[i-1];
        }
        fRecentFiles[0] = newFile;
        UpdateRecentFilesMenu();
    }

    // Update recent files menu
    void UpdateRecentFilesMenu() {
        // Clear menu
        while (fRecentFilesMenu->CountItems() > 0) {
            delete fRecentFilesMenu->RemoveItem((int32)0);
        }

        // Add recent files
        int count = 0;
        for (int i = 0; i < 5; i++) {
            if (fRecentFiles[i].Length() > 0) {
                BPath path(fRecentFiles[i].String());
                BMessage* msg = new BMessage(MSG_RECENT_FILE);
                msg->AddString("path", fRecentFiles[i].String());

                // Show only filename, not full path
                BString label;
                label << (i+1) << ". " << path.Leaf();

                fRecentFilesMenu->AddItem(new BMenuItem(label.String(), msg));
                count++;
            }
        }

        if (count == 0) {
            fRecentFilesMenu->AddItem(new BMenuItem("(No recent files)", NULL));
            fRecentFilesMenu->ItemAt(0)->SetEnabled(false);
        }
    }

    // Show About window
    void ShowAbout() {
        BAlert* about = new BAlert("About VeniceDAW",
            "VeniceDAW - 3DMix Viewer Alpha\n"
            "\"CD-ROM Edition\" 🎵\n\n"
            "A time machine back to 1995-2001\n"
            "when BeOS was the coolest OS on Earth!\n\n"
            "Features:\n"
            "• Plays ancient BeOS 3dmix files from the archives\n"
            "• 3D spatial audio positioning (feel the 90s!)\n"
            "• Waveform visualization like it's Y2K\n"
            "• Compatible with AMD E-Series CPUs from 2011 ⚡\n\n"
            "Built with ❤️ for Haiku OS\n"
            "November 2025\n\n"
            "🎧 Keep the BeOS spirit alive! 🎧",
            "Cool!", NULL, NULL,
            B_WIDTH_AS_USUAL, B_INFO_ALERT);
        about->Go();
    }

    void LoadProject(const char* path) {
        printf("Loading 3DMix project: %s\n", path);

        // Check if this is a "pointer file" (small text file with relative path)
        BString actualPath = path;
        BFile checkFile(path, B_READ_ONLY);
        if (checkFile.InitCheck() == B_OK) {
            off_t fileSize = 0;
            checkFile.GetSize(&fileSize);

            // If file is very small (< 256 bytes), it might be a pointer file
            if (fileSize > 0 && fileSize < 256) {
                char buffer[256];
                ssize_t bytesRead = checkFile.Read(buffer, fileSize);
                if (bytesRead > 0) {
                    buffer[bytesRead] = '\0';

                    // Remove trailing newlines/whitespace
                    BString relativePath(buffer);
                    relativePath.Trim();

                    // Check if it looks like a relative path (no leading /)
                    if (relativePath.Length() > 0 && relativePath[0] != '/') {
                        // Resolve relative to the directory of the pointer file
                        BPath originalPath(path);
                        if (originalPath.InitCheck() == B_OK) {
                            BPath parentDir;
                            originalPath.GetParent(&parentDir);

                            BPath resolvedPath(parentDir.Path(), relativePath.String());
                            if (resolvedPath.InitCheck() == B_OK) {
                                printf("→ Pointer file detected, redirecting to: %s\n", resolvedPath.Path());
                                actualPath = resolvedPath.Path();

                                // Update fProjectPath to use the actual file
                                fProjectPath = actualPath.String();
                            }
                        }
                    }
                }
            }
        }

        VeniceDAW::Legacy3DMixLoader loader;

        if (loader.LoadProject(actualPath.String()) != B_OK) {
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

        // Extract filename from project path
        BString fileName = "Unknown";
        if (fProjectPath) {
            BPath path(fProjectPath);
            if (path.InitCheck() == B_OK) {
                fileName = path.Leaf();
            } else {
                const char* lastSlash = strrchr(fProjectPath, '/');
                if (lastSlash) {
                    fileName = lastSlash + 1;
                } else {
                    fileName = fProjectPath;
                }
            }
        }

        // Clear old sources before adding new ones
        fGLView->ClearSources();

        // Set project info for display in 3D view (use filename instead of project name)
        fGLView->SetProjectInfo(fileName.String(), project.CountTracks());
        fGLView->SetProject(fProject);  // Enable real-time audio spatialization during drag

        // Update window title
        char windowTitle[512];
        snprintf(windowTitle, sizeof(windowTitle), "3DMix Viewer - %s (%d tracks)",
                 fileName.String(), project.CountTracks());
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
            // Pass project, project path, shared audio engine, and mute/solo state
            fTimelineWindow = new TimelineWindow(*fProject, fProjectPath.String(),
                                                  fSoundPlayer, &fCurrentFramePosition, &fIsPlaying,
                                                  fTrackMute, fTrackSolo, fFilterEnabled, fFilterMode,
                                                  fTrackFilterChains, this);
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
            case MSG_OPEN_FILE: {
                // Create file panel if it doesn't exist
                if (!fOpenPanel) {
                    BMessenger target(this);
                    fOpenPanel = new BFilePanel(B_OPEN_PANEL, &target);
                    fOpenPanel->SetButtonLabel(B_DEFAULT_BUTTON, "Open");
                    fOpenPanel->Window()->SetTitle("Open 3DMix File");
                }
                fOpenPanel->Show();
                break;
            }

            case MSG_RECENT_FILE: {
                // Open a recent file
                const char* path = NULL;
                if (message->FindString("path", &path) == B_OK) {
                    // Stop current playback if any
                    StopPlayback();

                    // Close timeline window BEFORE deleting project
                    if (fTimelineWindow && fTimelineWindow->Lock()) {
                        fTimelineWindow->Quit();
                        fTimelineWindow = nullptr;
                    }

                    // Clean up old project and sound player
                    if (fProject) {
                        delete fProject;
                        fProject = nullptr;
                    }
                    if (fSoundPlayer) {
                        delete fSoundPlayer;
                        fSoundPlayer = nullptr;
                    }

                    // Load new project
                    fProjectPath = path;
                    LoadProject(path);
                    InitAudioPlayback();

                    // Add to recent files (moves to top)
                    AddToRecentFiles(path);

                    // Auto-start playback
                    if (fSoundPlayer && fProject) {
                        TogglePlayback();
                    }
                }
                break;
            }

            case MSG_ABOUT: {
                ShowAbout();
                break;
            }

            case B_REFS_RECEIVED: {
                // User selected a file from the file panel
                entry_ref ref;
                if (message->FindRef("refs", &ref) == B_OK) {
                    BPath path(&ref);
                    if (path.InitCheck() == B_OK) {
                        // Stop current playback if any
                        StopPlayback();

                        // IMPORTANT: Close timeline window BEFORE deleting project
                        // Timeline holds a reference to fProject, so it must be closed first
                        if (fTimelineWindow && fTimelineWindow->Lock()) {
                            fTimelineWindow->Quit();
                            fTimelineWindow = nullptr;  // Will be recreated when user presses T
                        }

                        // Clean up old project
                        if (fProject) {
                            delete fProject;
                            fProject = nullptr;
                        }

                        // Clean up old sound player
                        if (fSoundPlayer) {
                            delete fSoundPlayer;
                            fSoundPlayer = nullptr;
                        }

                        // Load new project
                        fProjectPath = path.Path();
                        LoadProject(path.Path());
                        InitAudioPlayback();

                        // Add to recent files
                        AddToRecentFiles(path.Path());

                        // Auto-start playback
                        if (fSoundPlayer && fProject) {
                            TogglePlayback();
                        }
                    }
                }
                break;
            }

            case MSG_PULSE:
                if (fGLView) {
                    fGLView->Pulse();
                    // Throttle 3D view updates to reduce CPU usage with software rendering
                    // Update 3D VU meters at 10 FPS (every 3rd frame) instead of 30 FPS
                    fFrameCounter++;
                    if (fIsPlaying && (fFrameCounter % 3 == 0)) {
                        fGLView->Invalidate();
                    }
                }
                // Update time display and 2D VU meter at full 30 FPS (lightweight)
                UpdateTimeDisplay();
                if (fMasterVUMeter) {
                    fMasterVUMeter->SetLevels(fMasterLevelLeft, fMasterLevelRight);
                }
                break;

            case 'Play':
                TogglePlayback();
                break;

            case 'Stop':
                StopPlayback();
                break;

            case 'LpSy': {  // Loop Sync from TimelineWindow
                bool enabled = false;
                float inPt = 0.0f, outPt = 0.0f;
                message->FindBool("enabled", &enabled);
                message->FindFloat("in", &inPt);
                message->FindFloat("out", &outPt);
                fAudioLoopEnabled.store(enabled, std::memory_order_relaxed);
                fAudioLoopInTime.store(inPt, std::memory_order_relaxed);
                fAudioLoopOutTime.store(outPt, std::memory_order_relaxed);
                printf("[Loop Sync] Audio loop %s (%.2fs - %.2fs)\n",
                       enabled ? "ENABLED" : "DISABLED", inPt, outPt);
                break;
            }

            case 'MstV': {  // Master Volume slider
                if (fMasterVolumeSlider) {
                    int32 value = fMasterVolumeSlider->Value();
                    fMasterVolume = value / 100.0f;  // 0-200 -> 0.0-2.0
                    printf("[MasterVolume] Set to %.0f%% (gain=%.2f)\n", value, fMasterVolume);
                }
                break;
            }

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
                    } else if (key == 's' || key == 'S') {  // 'S' key for Stop
                        StopPlayback();
                        return;
                    }
                    // Track selection (1-9)
                    else if (key >= '1' && key <= '9') {
                        fSelectedTrackIndex = key - '1';  // 0-based index
                        if (fProject && fSelectedTrackIndex < fProject->CountTracks()) {
                            printf("[Filter] Track %d selected ('%s')\n",
                                   fSelectedTrackIndex + 1,
                                   fProject->TrackAt(fSelectedTrackIndex)->TrackName().String());
                        }
                        return;
                    }
                    // Filter controls (require selected track)
                    else if (fSelectedTrackIndex >= 0 && fSelectedTrackIndex < 64) {
                        int idx = fSelectedTrackIndex;
                        if (key == 'l' || key == 'L') {  // Low-Pass filter
                            fTrackFilters[idx].SetMode(HaikuDAW::BiquadFilter::LOW_PASS);
                            fTrackFilters[idx].SetFrequency(1000.0f);
                            fTrackFilters[idx].SetQ(0.707f);
                            fTrackFiltersR[idx].SetMode(HaikuDAW::BiquadFilter::LOW_PASS);
                            fTrackFiltersR[idx].SetFrequency(1000.0f);
                            fTrackFiltersR[idx].SetQ(0.707f);
                            fFilterEnabled[idx] = true;
                            fFilterMode[idx] = HaikuDAW::BiquadFilter::LOW_PASS;
                            printf("[Filter] LOW-PASS applied to track %d (cutoff=1kHz)\n", idx + 1);
                            return;
                        } else if (key == 'h' || key == 'H') {  // High-Pass filter
                            fTrackFilters[idx].SetMode(HaikuDAW::BiquadFilter::HIGH_PASS);
                            fTrackFilters[idx].SetFrequency(200.0f);
                            fTrackFilters[idx].SetQ(0.707f);
                            fTrackFiltersR[idx].SetMode(HaikuDAW::BiquadFilter::HIGH_PASS);
                            fTrackFiltersR[idx].SetFrequency(200.0f);
                            fTrackFiltersR[idx].SetQ(0.707f);
                            fFilterEnabled[idx] = true;
                            fFilterMode[idx] = HaikuDAW::BiquadFilter::HIGH_PASS;
                            printf("[Filter] HIGH-PASS applied to track %d (cutoff=200Hz)\n", idx + 1);
                            return;
                        } else if (key == 'n' || key == 'N') {  // Notch filter
                            fTrackFilters[idx].SetMode(HaikuDAW::BiquadFilter::NOTCH);
                            fTrackFilters[idx].SetFrequency(1000.0f);
                            fTrackFilters[idx].SetQ(2.0f);
                            fTrackFiltersR[idx].SetMode(HaikuDAW::BiquadFilter::NOTCH);
                            fTrackFiltersR[idx].SetFrequency(1000.0f);
                            fTrackFiltersR[idx].SetQ(2.0f);
                            fFilterEnabled[idx] = true;
                            fFilterMode[idx] = HaikuDAW::BiquadFilter::NOTCH;
                            printf("[Filter] NOTCH applied to track %d (freq=1kHz)\n", idx + 1);
                            return;
                        } else if (key == 'p' || key == 'P') {  // Peaking EQ
                            fTrackFilters[idx].SetMode(HaikuDAW::BiquadFilter::PEAKING);
                            fTrackFilters[idx].SetFrequency(500.0f);
                            fTrackFilters[idx].SetQ(1.0f);
                            fTrackFilters[idx].SetGain(6.0f);
                            fTrackFiltersR[idx].SetMode(HaikuDAW::BiquadFilter::PEAKING);
                            fTrackFiltersR[idx].SetFrequency(500.0f);
                            fTrackFiltersR[idx].SetQ(1.0f);
                            fTrackFiltersR[idx].SetGain(6.0f);
                            fFilterEnabled[idx] = true;
                            fFilterMode[idx] = HaikuDAW::BiquadFilter::PEAKING;
                            printf("[Filter] PEAKING EQ applied to track %d (freq=500Hz, gain=+6dB)\n", idx + 1);
                            return;
                        } else if (key == 'x' || key == 'X') {  // Disable filter
                            fFilterEnabled[idx] = false;
                            fTrackFilters[idx].Reset();
                            fTrackFiltersR[idx].Reset();
                            printf("[Filter] Filter DISABLED on track %d\n", idx + 1);
                            return;
                        }
                    }
                }
                break;
            }

            // === FILTER CHAIN MANAGEMENT (from timeline UI) ===
            case 'addf':  // MSG_ADD_FILTER + 0 (LOW_PASS)
            case 'addf' + 1:  // HIGH_PASS
            case 'addf' + 2:  // BAND_PASS
            case 'addf' + 3:  // NOTCH
            case 'addf' + 4:  // PEAKING
            case 'addf' + 5:  // LOW_SHELF
            case 'addf' + 6: {  // HIGH_SHELF
                int32 trackIndex = -1;
                if (message->FindInt32("track_index", &trackIndex) == B_OK && trackIndex >= 0 && trackIndex < 64) {
                    // Determine filter type from message 'what' code
                    int filterType = message->what - 'addf';  // 0-6

                    // Create new filter with SetParams() to initialize both L/R instances
                    FilterInChain newFilter;
                    newFilter.filterL.SetSampleRate(44100.0f);
                    newFilter.filterR.SetSampleRate(44100.0f);

                    // Set default parameters based on filter type (configures both L/R)
                    switch (filterType) {
                        case 0:  // LOW_PASS
                            newFilter.SetParams(filterType, 1000.0f, 0.707f, 0.0f);
                            printf("[FilterChain] Added LOW-PASS to track %d (cutoff=1kHz)\n", trackIndex);
                            break;
                        case 1:  // HIGH_PASS
                            newFilter.SetParams(filterType, 200.0f, 0.707f, 0.0f);
                            printf("[FilterChain] Added HIGH-PASS to track %d (cutoff=200Hz)\n", trackIndex);
                            break;
                        case 2:  // BAND_PASS
                            newFilter.SetParams(filterType, 1000.0f, 1.0f, 0.0f);
                            printf("[FilterChain] Added BAND-PASS to track %d (center=1kHz)\n", trackIndex);
                            break;
                        case 3:  // NOTCH
                            newFilter.SetParams(filterType, 1000.0f, 2.0f, 0.0f);
                            printf("[FilterChain] Added NOTCH to track %d (freq=1kHz)\n", trackIndex);
                            break;
                        case 4:  // PEAKING
                            newFilter.SetParams(filterType, 500.0f, 1.0f, 6.0f);
                            printf("[FilterChain] Added PEAKING EQ to track %d (freq=500Hz, gain=+6dB)\n", trackIndex);
                            break;
                        case 5:  // LOW_SHELF
                            newFilter.SetParams(filterType, 200.0f, 0.707f, 6.0f);
                            printf("[FilterChain] Added LOW-SHELF to track %d (freq=200Hz, gain=+6dB)\n", trackIndex);
                            break;
                        case 6:  // HIGH_SHELF
                            newFilter.SetParams(filterType, 8000.0f, 0.707f, 6.0f);
                            printf("[FilterChain] Added HIGH-SHELF to track %d (freq=8kHz, gain=+6dB)\n", trackIndex);
                            break;
                    }

                    // Add filter to chain
                    fTrackFilterChains[trackIndex].push_back(newFilter);

                    // Invalidate timeline window to show new filter indicator
                    if (fTimelineWindow) {
                        fTimelineWindow->PostMessage(B_INVALIDATE);
                    }
                }
                break;
            }

            case 'rmvf': {  // MSG_REMOVE_FILTER
                int32 trackIndex = -1;
                int32 filterIndex = -1;
                if (message->FindInt32("track_index", &trackIndex) == B_OK &&
                    message->FindInt32("filter_index", &filterIndex) == B_OK &&
                    trackIndex >= 0 && trackIndex < 64) {

                    std::vector<FilterInChain>& chain = fTrackFilterChains[trackIndex];
                    if (filterIndex >= 0 && (size_t)filterIndex < chain.size()) {
                        // Remove filter at index
                        chain.erase(chain.begin() + filterIndex);
                        printf("[FilterChain] Removed filter %d from track %d (chain now has %zu filters)\n",
                               filterIndex, trackIndex, chain.size());

                        // Invalidate timeline window to update display
                        if (fTimelineWindow) {
                            fTimelineWindow->PostMessage(B_INVALIDATE);
                        }
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

        // Pre-resolve all track paths and pre-cache audio (removes I/O from audio thread)
        float detectedSampleRate = 44100.0f;  // Default fallback
        int trackCount = fProject->CountTracks();
        for (int i = 0; i < 64; i++) fResolvedPaths[i] = "";

        for (int i = 0; i < trackCount && i < 64; i++) {
            VeniceDAW::Track3DMix* track = fProject->TrackAt(i);
            if (!track) continue;

            fResolvedPaths[i] = ResolveTrackAudioPath(track);
            if (fResolvedPaths[i].Length() == 0) continue;

            // Pre-cache audio and detect sample rate
            VeniceDAW::AudioFormat3DMix audioFormat = track->GetAudioFormat();
            if (audioFormat.sampleRate == 0 && fProject) {
                audioFormat.sampleRate = fProject->ProjectSampleRate();
                printf("[InitAudio] Track %d: Using project sample rate: %d Hz\n", i, audioFormat.sampleRate);
            }
            const AudioSampleCache* audioCache = WaveformCache::Instance().GetAudioCache(fResolvedPaths[i].String(), &audioFormat);
            if (audioCache && audioCache->isValid && audioCache->sampleRate > 0 && detectedSampleRate == 44100.0f) {
                detectedSampleRate = audioCache->sampleRate;
                printf("[3D Audio] Detected sample rate from track %d: %.0f Hz\n", i, detectedSampleRate);
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
            if (fPlayButton) {
                fPlayButton->SetLabel("⏸ Pause");
            }
            printf("[3D Audio] Playback STARTED\n");
        } else {
            fSoundPlayer->Stop();
            if (fPlayButton) {
                fPlayButton->SetLabel("▶ Play");
            }
            printf("[3D Audio] Playback PAUSED\n");
        }
    }

    void StopPlayback() {
        if (!fSoundPlayer || !fProject) return;

        // Stop playback AND reset playhead to beginning
        if (fIsPlaying) {
            fSoundPlayer->Stop();
            fIsPlaying = false;
        }

        fCurrentFramePosition = 0;
        if (fPlayButton) {
            fPlayButton->SetLabel("▶ Play");
        }

        // Update timeline windows
        if (fTimelineWindow && fTimelineWindow->Lock()) {
            fTimelineWindow->SetPlayheadPosition(0.0f);
            fTimelineWindow->Unlock();
        }

        printf("[3D Audio] Playback STOPPED (reset to start)\n");
    }

    void UpdateTimeDisplay() {
        if (!fTimeDisplay || !fProject || !fSoundPlayer) return;

        // Calculate current time from frame position
        media_raw_audio_format format = fSoundPlayer->Format();
        float currentTime = fCurrentFramePosition / format.frame_rate;

        // Calculate total duration
        float totalDuration = GetProjectDuration();

        // Format: M:SS.d / M:SS.d
        int currentMin = (int)(currentTime / 60.0f);
        float currentSec = currentTime - (currentMin * 60.0f);
        int totalMin = (int)(totalDuration / 60.0f);
        float totalSec = totalDuration - (totalMin * 60.0f);

        BString timeText;
        timeText.SetToFormat("%d:%04.1f / %d:%04.1f", currentMin, currentSec, totalMin, totalSec);
        fTimeDisplay->SetText(timeText.String());
    }

    float GetProjectDuration() {
        if (!fProject) return 0.0f;

        float maxDuration = 0.0f;
        int trackCount = fProject->CountTracks();

        for (int i = 0; i < trackCount; i++) {
            VeniceDAW::Track3DMix* track = fProject->TrackAt(i);
            if (!track) continue;

            const VeniceDAW::AudioFormat3DMix& format = track->GetAudioFormat();
            int32 startSample = track->StartPosition();
            int32 endSample = track->EndPosition();

            const float kTimelineReferenceRate = 22050.0f;
            float sampleRate = kTimelineReferenceRate;
            float startTime = startSample / sampleRate;
            float endTime = endSample / sampleRate;

            if (endSample == 0 && format.fileSize > 0 && format.channels > 0 && format.bitDepth > 0) {
                int32 bytesPerSample = (format.bitDepth + 7) / 8;
                int32 totalSamples = format.fileSize / (format.channels * bytesPerSample);
                endTime = startTime + (totalSamples / sampleRate);
            }

            if (endTime > maxDuration) {
                maxDuration = endTime;
            }
        }

        return maxDuration > 0 ? maxDuration : 30.0f;
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
        float currentTime = (float)fCurrentFramePosition.load() / format.frame_rate;

        // Enforce loop region from audio thread (sample-accurate looping)
        if (fAudioLoopEnabled.load(std::memory_order_relaxed)) {
            float loopOut = fAudioLoopOutTime.load(std::memory_order_relaxed);
            float loopIn = fAudioLoopInTime.load(std::memory_order_relaxed);
            if (loopOut > loopIn && currentTime >= loopOut) {
                fCurrentFramePosition.store((int64)(loopIn * format.frame_rate));
                currentTime = loopIn;
            }
        }

        // Clear track levels
        memset(fTrackLevels, 0, sizeof(fTrackLevels));

        // Check if any track has solo enabled
        fAnySoloActive = false;
        for (int i = 0; i < trackCount && i < 64; i++) {
            if (fTrackSolo[i]) {
                fAnySoloActive = true;
                break;
            }
        }

        // Mix each track
        for (int i = 0; i < trackCount; i++) {
            VeniceDAW::Track3DMix* track = fProject->TrackAt(i);
            if (!track) continue;

            // Apply mute/solo logic (BeOS 3D Mixer style)
            if (i < 64) {
                // Skip if muted
                if (fTrackMute[i]) continue;

                // Skip if solo is active elsewhere and this track is not solo'd
                if (fAnySoloActive && !fTrackSolo[i]) continue;
            }

            // Use pre-resolved audio path (no filesystem I/O in audio thread)
            if (i >= 64 || fResolvedPaths[i].Length() == 0) continue;
            const BString& resolvedPath = fResolvedPaths[i];

            // Get audio cache - use project sample rate if track doesn't have one
            VeniceDAW::AudioFormat3DMix audioFormat = track->GetAudioFormat();
            if (audioFormat.sampleRate == 0 && fProject) {
                audioFormat.sampleRate = fProject->ProjectSampleRate();
            }
            const AudioSampleCache* audioCache = WaveformCache::Instance().GetAudioCache(resolvedPath.String(), &audioFormat);
            if (!audioCache || !audioCache->isValid || audioCache->samples.empty()) continue;

            // Debug: log sample rate info once per track
            static bool logged[64] = {false};
            if (!logged[i] && i < 64) {
                printf("[MixTracks] Track %d: file rate=%.0f Hz, playback rate=%.0f Hz, ratio=%.3f\n",
                       i, audioCache->sampleRate, format.frame_rate, audioCache->sampleRate / format.frame_rate);
                logged[i] = true;
            }

            // Calculate track start time
            // IMPORTANT: StartPosition() is in samples at 22050 Hz reference rate
            const float kTimelineReferenceRate = 22050.0f;
            float trackStartTime = track->StartPosition() / kTimelineReferenceRate;
            float relativeTime = currentTime - trackStartTime;

            if (relativeTime < 0) continue;  // Track hasn't started yet

            // Get loop parameters (BeOS 3DMix compatibility)
            // In BeOS format: st_skip is stored in LoopStart, loop_point in LoopEnd
            int64 loopStart = track->LoopStart();    // Trim: where to start in the audio file
            int64 loopEnd = track->LoopEnd();        // Loop point: where to loop back
            int64 loopLength = loopEnd - loopStart;

            // Debug: log loop info once per track
            static bool loopLogged[64] = {false};
            if (!loopLogged[i] && i < 64 && loopLength > 0) {
                printf("[Loop] Track %d: trim=%.3fs, loop=%.3fs (length=%.3fs)\n",
                       i, loopStart / audioCache->sampleRate, loopEnd / audioCache->sampleRate,
                       loopLength / audioCache->sampleRate);
                loopLogged[i] = true;
            }

            // Calculate sample rate conversion ratio
            float sampleRateRatio = audioCache->sampleRate / format.frame_rate;

            // === 3D SPATIAL AUDIO CALCULATION ===
            // Get track 3D position
            VeniceDAW::Coordinate3D pos = track->Position();

            // NOTE: The 3D view camera is rotated 180°, so visual X is inverted.
            // The audio pan uses pos.x directly (positive = right in audio)
            // which matches the 3D view AFTER the 180° rotation is accounted for.
            // The drag code already negates dx for the camera rotation (line 776).

            // Calculate distance from listener (at origin 0,0,0)
            float distance = sqrt(pos.x * pos.x + pos.y * pos.y);

            // === DISTANCE ATTENUATION ===
            // Linear attenuation with generous minimum floor
            // At distance 0 = full volume, at distance 15 = 30% volume
            float maxDistance = 15.0f;
            float distanceGain = 1.0f - (distance / maxDistance) * 0.7f;
            if (distanceGain < 0.3f) distanceGain = 0.3f;
            if (distanceGain > 1.0f) distanceGain = 1.0f;

            // Pan calculation based on X position
            // X: negative = left, positive = right
            const float panRange = 10.0f;
            float pan = fmax(-1.0f, fmin(1.0f, pos.x / panRange));

            // === FRONT/BACK SPATIAL DEPTH ===
            // Y axis: positive = in front of listener, negative = behind
            // Behind the listener: head shadow effect (attenuate high frequencies)
            // This creates the perception of depth on the 2D plane
            // depthFactor: 1.0 = fully in front (bright), 0.0 = fully behind (muffled)
            float depthNorm = fmax(-1.0f, fmin(1.0f, pos.y / panRange));  // -1 to +1
            float depthFactor = (depthNorm + 1.0f) * 0.5f;  // 0.0 (behind) to 1.0 (front)
            // Low-pass filter coefficient for head shadow:
            // alpha=1.0 means no filtering (pass-through), alpha close to 0 = heavy filtering
            // Front (depthFactor=1.0): alpha=1.0 (no filter)
            // Behind (depthFactor=0.0): alpha=0.15 (strong low-pass ~1kHz at 22050Hz)
            float spatialAlpha = 0.15f + depthFactor * 0.85f;  // Range: 0.15 to 1.0

            // Slight volume boost for front sources, reduction for behind (±3dB)
            float depthGain = 0.8f + depthFactor * 0.4f;  // 0.8 (behind) to 1.2 (front)

            // Calculate stereo gains using constant power panning
            float leftGain = cos((pan + 1.0f) * M_PI / 4.0f) * distanceGain * depthGain;
            float rightGain = sin((pan + 1.0f) * M_PI / 4.0f) * distanceGain * depthGain;

            // === ITD (Interaural Time Difference) CALCULATION ===
            // BeOS 3D Mixer algorithm from sound_view.cpp lines 4314-4322
            // Creates realistic spatial audio by delaying one ear based on X position
            int32 delayLeft = 0;
            int32 delayRight = 0;

            // Calculate delay in samples based on X position
            // Formula: delay = pos_x / 3.5 (from BeOS original)
            int32 baseDelay = (int32)(pos.x / 3.5f);

            if (baseDelay < 0) {
                // Source on the left: delay RIGHT ear
                delayRight = -baseDelay;
                delayLeft = 0;
            } else {
                // Source on the right: delay LEFT ear
                delayLeft = baseDelay;
                delayRight = 0;
            }

            // Master volume scaling (BeOS original: global_gain/n)
            const float baseVolume = 0.8f;
            const float masterVolume = baseVolume / (float)trackCount;

            // Apply per-track volume from 3DMix project (BeOS heritage compatibility)
            float trackVolume = track->Volume();
            if (trackVolume < 0.0f) trackVolume = 0.0f;
            if (trackVolume > 2.0f) trackVolume = 2.0f;  // Allow up to 2x boost

            leftGain *= masterVolume * trackVolume * fMasterVolume;
            rightGain *= masterVolume * trackVolume * fMasterVolume;

            // Debug: log spatial parameters periodically
            static int32 spatialLogTimer = 0;
            if (i == 0) spatialLogTimer++;
            if (i == 0 && (spatialLogTimer % 100) == 0) {
                printf("[3D Spatial] T0: pos(%.1f,%.1f) pan=%.2f dist=%.2f depth=%.2f alpha=%.2f L=%.3f R=%.3f\n",
                       pos.x, pos.y, pan, distanceGain, depthFactor, spatialAlpha, leftGain, rightGain);
            }

            // Track level calculation (RMS)
            float rmsSum = 0.0f;
            int32 sampleCount = 0;

            // Copy samples from track to output buffer with sample rate conversion and looping
            // BeOS R6 format: stereo int16 interleaved (L,R,L,R,...)
            for (int32 frame = 0; frame < frameCount; frame++) {
                // Calculate source FRAME position (fractional)
                // Each frame = 2 samples (L+R) in stereo int16 format
                float srcFramePosition = relativeTime * audioCache->sampleRate + (frame * sampleRateRatio);
                int64 srcFrame = (int64)srcFramePosition;

                // Apply BeOS-style looping if loop parameters are valid
                if (loopLength > 0 && loopEnd > loopStart) {
                    // Add trim offset
                    srcFrame += loopStart;

                    // Apply modulo looping when we exceed loop end point
                    if (srcFrame >= loopEnd) {
                        int64 offsetFromLoopStart = srcFrame - loopStart;
                        srcFrame = loopStart + (offsetFromLoopStart % loopLength);
                    }
                }

                // Convert frame index to sample index (stereo: 2 samples per frame)
                int64 srcIdx = srcFrame * 2;

                // Safety check: ensure stereo pair is within bounds
                if (srcIdx < 0 || srcIdx + 1 >= (int64)audioCache->samples.size()) break;

                // Read stereo int16 samples and convert to float (-1.0 to +1.0)
                float leftSampleFloat = audioCache->samples[srcIdx] * kInt16ToFloat;      // Left channel
                float rightSampleFloat = audioCache->samples[srcIdx + 1] * kInt16ToFloat;  // Right channel

                // Linear interpolation for smooth sample rate conversion
                if (srcIdx + 3 < (int64)audioCache->samples.size()) {
                    float frac = srcFramePosition - (int64)srcFramePosition;
                    float nextLeftFloat = audioCache->samples[srcIdx + 2] * kInt16ToFloat;
                    float nextRightFloat = audioCache->samples[srcIdx + 3] * kInt16ToFloat;
                    leftSampleFloat = leftSampleFloat + frac * (nextLeftFloat - leftSampleFloat);
                    rightSampleFloat = rightSampleFloat + frac * (nextRightFloat - rightSampleFloat);
                }

                // Average L+R for RMS calculation
                float monoSample = (leftSampleFloat + rightSampleFloat) * 0.5f;
                rmsSum += monoSample * monoSample;
                sampleCount++;

                // Mix into output with 3D spatial positioning AND ITD delays
                if (format.channel_count >= 2) {
                    // === Apply ITD (Interaural Time Difference) ===
                    // BeOS algorithm from sound_view.cpp lines 4333-4350
                    // Delayed samples create realistic head-shadow effect

                    // Calculate delayed FRAME indices
                    // Source left (delayRight>0): right ear is farther, delay right
                    // Source right (delayLeft>0): left ear is farther, delay left
                    int64 leftDelayedFrame = srcFrame - delayLeft;    // Left ear delayed by left delay
                    int64 rightDelayedFrame = srcFrame - delayRight;  // Right ear delayed by right delay

                    // Get left channel sample (with right delay applied)
                    float finalLeftSample = leftSampleFloat;  // Default to current sample
                    if (delayRight > 0 && leftDelayedFrame >= 0) {
                        int64 leftDelayedIdx = leftDelayedFrame * 2;
                        if (leftDelayedIdx + 1 < (int64)audioCache->samples.size()) {
                            finalLeftSample = audioCache->samples[leftDelayedIdx] * kInt16ToFloat;
                        }
                    }

                    // Get right channel sample (with left delay applied)
                    float finalRightSample = rightSampleFloat;  // Default to current sample
                    if (delayLeft > 0 && rightDelayedFrame >= 0) {
                        int64 rightDelayedIdx = rightDelayedFrame * 2;
                        if (rightDelayedIdx + 1 < (int64)audioCache->samples.size()) {
                            finalRightSample = audioCache->samples[rightDelayedIdx + 1] * kInt16ToFloat;
                        }
                    }

                    // Apply filter chain (multiple filters in cascade) if any filters exist for this track
                    if (i < 64 && !fTrackFilterChains[i].empty()) {
                        // Process through all filters in chain sequentially (cascade)
                        for (size_t filterIdx = 0; filterIdx < fTrackFilterChains[i].size(); filterIdx++) {
                            FilterInChain& filterInChain = fTrackFilterChains[i][filterIdx];

                            // Initialize filter sample rate on first use
                            if (filterInChain.filterL.GetSampleRate() != format.frame_rate) {
                                filterInChain.filterL.SetSampleRate(format.frame_rate);
                                filterInChain.filterR.SetSampleRate(format.frame_rate);
                            }

                            // Process L/R through independent filter instances (no crosstalk)
                            finalLeftSample = filterInChain.filterL.Process(finalLeftSample);
                            finalRightSample = filterInChain.filterR.Process(finalRightSample);
                        }
                    }

                    // LEGACY: Single filter support (kept for backward compatibility during transition)
                    // This can be removed once UI fully migrates to filter chains
                    else if (i < 64 && fFilterEnabled[i]) {
                        // Initialize filter sample rate on first use
                        if (fTrackFilters[i].GetSampleRate() != format.frame_rate) {
                            fTrackFilters[i].SetSampleRate(format.frame_rate);
                            fTrackFiltersR[i].SetSampleRate(format.frame_rate);
                        }
                        // Process L/R through independent filter instances (no crosstalk)
                        finalLeftSample = fTrackFilters[i].Process(finalLeftSample);
                        finalRightSample = fTrackFiltersR[i].Process(finalRightSample);
                    }

                    // 3D Spatial mixing: downmix stereo source to mono, then apply spatial panning
                    float monoSource = (finalLeftSample + finalRightSample) * 0.5f;

                    // Apply head shadow low-pass filter for behind-listener sources
                    // 1-pole IIR: y[n] = y[n-1] + alpha * (x[n] - y[n-1])
                    // alpha=1.0 = pass-through, alpha→0 = heavy low-pass (muffled)
                    if (spatialAlpha < 0.99f && i < 64) {
                        monoSource = fSpatialFilterState[i] + spatialAlpha * (monoSource - fSpatialFilterState[i]);
                        fSpatialFilterState[i] = monoSource;
                    }

                    buffer[frame * format.channel_count + 0] += monoSource * leftGain;
                    buffer[frame * format.channel_count + 1] += monoSource * rightGain;
                } else {
                    // Mono output: use average gain (no ITD)
                    float monoGain = (leftGain + rightGain) * 0.5f;
                    buffer[frame * format.channel_count + 0] += monoSample * monoGain;
                }
            }

            // Calculate RMS level for this track (0.0 to 1.0)
            if (sampleCount > 0 && i < 64) {
                float rms = sqrt(rmsSum / sampleCount);
                fTrackLevels[i] = fmin(rms * 2.0f, 1.0f);  // Scale and clamp to 0-1
            }
        }

        // Master volume is already applied per-track in the mixing loop above

        // Calculate master output levels for VU meters (after master volume)
        // Use peak detection for more responsive VU meters
        float leftPeak = 0.0f;
        float rightPeak = 0.0f;
        if (format.channel_count >= 2) {
            for (int32 frame = 0; frame < frameCount; frame++) {
                float leftSample = fabs(buffer[frame * format.channel_count + 0]);
                float rightSample = fabs(buffer[frame * format.channel_count + 1]);

                // Track peak values
                if (leftSample > leftPeak) leftPeak = leftSample;
                if (rightSample > rightPeak) rightPeak = rightSample;
            }

            // Scale peaks for VU meter display (compensate for low normalization)
            // Adaptive scaling based on actual track count: more tracks = more gain needed
            // But cap at 6x to avoid constant red zone (was 12x - too aggressive)
            float vuScale = trackCount > 0 ? fmin((float)trackCount, 6.0f) : 3.0f;
            fMasterLevelLeft = fmin(leftPeak * vuScale, 1.0f);
            fMasterLevelRight = fmin(rightPeak * vuScale, 1.0f);

            // Debug: log VU meter levels periodically (every ~1 second at 44100 Hz)
            static int32 vuLogCounter = 0;
            vuLogCounter++;
            if (vuLogCounter % (int32)(format.frame_rate) == 0) {
                printf("[VU Meter] L=%.2f R=%.2f (raw peaks: L=%.3f R=%.3f, scale=%.1fx)\n",
                       fMasterLevelLeft, fMasterLevelRight, leftPeak, rightPeak, vuScale);
            }
        }

        // Update frame position for next callback
        fCurrentFramePosition += frameCount;

        // Auto-loop to start at end of project (prevents infinite silence)
        float endTime = (float)fCurrentFramePosition.load() / format.frame_rate;
        float projectDuration = GetProjectDuration();
        if (projectDuration > 0 && endTime > projectDuration + 0.5f) {
            fCurrentFramePosition.store(0);
        }

        // Pass track levels to 3D view for visual feedback
        if (fGLView) {
            fGLView->SetTrackLevels(fTrackLevels, trackCount);
        }

        // NOTE: VU meter and time display updates REMOVED from audio callback
        // Calling LockLooper() from real-time audio thread causes distortion/glitches
        // These updates should be done from a separate timer thread, not here
    }

    virtual bool QuitRequested() override {
        be_app->PostMessage(B_QUIT_REQUESTED);
        return true;
    }

private:
    static const uint32 MSG_PULSE = 'puls';
    static const uint32 MSG_OPEN_FILE = 'open';
    static const uint32 MSG_RECENT_FILE = 'recf';
    static const uint32 MSG_ABOUT = 'abou';

    DemoGL3DView* fGLView;
    BMessageRunner* fUpdateRunner;
    TimelineWindow* fTimelineWindow;
    VeniceDAW::Project3DMix* fProject;
    BString fProjectPath;  // Full path to the .3dmix file
    BFilePanel* fOpenPanel;

    // Pre-resolved audio file paths (populated during InitAudioPlayback, used by MixTracks)
    BString fResolvedPaths[64];

    // Resolve audio file path for a track (searches project directory with multiple extensions)
    BString ResolveTrackAudioPath(VeniceDAW::Track3DMix* track) {
        if (!track) return BString();
        BString audioPath = track->AudioFilePath();
        if (audioPath.Length() == 0) return BString();

        BPath pathObj(audioPath.String());
        const char* filename = pathObj.Leaf();
        if (!filename) return BString();

        BString projectDir;
        if (fProjectPath.Length() > 0) {
            BPath projPath(fProjectPath.String());
            BPath parentPath;
            if (projPath.GetParent(&parentPath) == B_OK) {
                projectDir = parentPath.Path();
            }
        }

        const char* extensions[] = { "", ".wav", ".aiff", ".aif", ".raw", ".mp3", ".ogg", nullptr };
        for (int ext = 0; extensions[ext] != nullptr; ext++) {
            BString testPath = projectDir;
            testPath << "/" << filename << extensions[ext];
            entry_ref ref;
            if (get_ref_for_path(testPath.String(), &ref) == B_OK) {
                return testPath;
            }
        }
        return BString();
    }

    // Recent files support
    BMenu* fRecentFilesMenu;
    BString fRecentFiles[5];  // Store last 5 opened files

    // Control bar (contains all transport controls)
    ControlBarView* fControlBar;

    // Control references (owned by fControlBar)
    BButton* fPlayButton;
    BButton* fStopButton;
    BStringView* fTimeDisplay;
    MasterVUMeterView* fMasterVUMeter;
    BSlider* fMasterVolumeSlider;
    BSoundPlayer* fSoundPlayer;
    float fMasterVolume;  // 0.0 to 2.0 (100% = 1.0, 200% = 2.0)
    std::atomic<int64> fCurrentFramePosition;
    std::atomic<bool> fIsPlaying;
    int64 fLastTimeDisplayUpdate;  // For throttling time display updates

    // Track levels for visual feedback (RMS level 0.0-1.0)
    float fTrackLevels[64];  // Max 64 tracks

    // Master output levels (stereo L/R)
    float fMasterLevelLeft;
    float fMasterLevelRight;

    // Mute/Solo state per track (BeOS 3D Mixer style)
    bool fTrackMute[64];     // true = track silenced
    bool fTrackSolo[64];     // true = track solo'd
    bool fAnySoloActive;     // Cache: true if any track has solo

    // Audio filters per track (NEW: Filter chains for multiple filters per track)
    std::vector<FilterInChain> fTrackFilterChains[64];  // Filter chain for each track (0-N filters)

    // LEGACY: Single filter per track (kept for backward compatibility during transition)
    HaikuDAW::BiquadFilter fTrackFilters[64];   // Left channel filter per track
    HaikuDAW::BiquadFilter fTrackFiltersR[64];  // Right channel filter per track
    bool fFilterEnabled[64];                    // true = filter active for this track
    int fFilterMode[64];                        // FilterMode enum value for each track
    int fSelectedTrackIndex;                    // Currently selected track for filter control (-1 = none)

    // Spatial depth filter state per track (1-pole IIR for head shadow effect)
    float fSpatialFilterState[64] = {0};

    // Audio-thread loop region (synced from TimelineWindow)
    std::atomic<bool> fAudioLoopEnabled{false};
    std::atomic<float> fAudioLoopInTime{0.0f};
    std::atomic<float> fAudioLoopOutTime{0.0f};

    // Frame counter for throttling 3D view updates
    int32 fFrameCounter;     // Incremented each MSG_PULSE
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
    // Allow starting without arguments - user can open file from GUI
    const char* initialFile = (argc >= 2) ? argv[1] : nullptr;

    DemoApp app(initialFile);
    app.Run();
    return 0;
}

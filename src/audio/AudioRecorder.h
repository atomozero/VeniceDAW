/*
 * AudioRecorder.h - Native Haiku audio recording using BSoundRecorder
 * Integrates with VeniceDAW for real-time audio input from sound card
 */

#ifndef AUDIO_RECORDER_H
#define AUDIO_RECORDER_H

// Temporarily disable BSoundRecorder - use alternative API
// #include <media/SoundRecorder.h>
#include <media/MediaFile.h>
#include <media/MediaTrack.h>
#include <media/MediaRoster.h>
#include <media/SoundPlayer.h>  // Use available API instead
#include <storage/Entry.h>
#include <storage/Path.h>
#include <support/String.h>
#include <OS.h>
#include <vector>
#include "AudioBufferPool.h"
#include "AsyncAudioWriter.h"

// Forward declarations
namespace HaikuDAW {
    class SimpleHaikuEngine;
}

namespace VeniceDAW {

/*
 * Audio recording callback interface
 */
class AudioRecordingListener {
public:
    virtual ~AudioRecordingListener() {}
    virtual void OnRecordingStarted() = 0;
    virtual void OnRecordingStopped() = 0;
    virtual void OnRecordingError(status_t error) = 0;
    virtual void OnAudioData(const void* data, size_t size, const media_format& format) = 0;
};

/*
 * Native Haiku audio recorder using BSoundRecorder
 */
class AudioRecorder {
public:
    AudioRecorder();
    ~AudioRecorder();

    // Recording control
    status_t StartRecording(const char* filename = nullptr);
    status_t StopRecording();
    bool IsRecording() const { return fRecording; }

    // Input device selection
    status_t EnumerateInputDevices();
    int32 GetInputDeviceCount() const { return fInputDevices.size(); }
    const char* GetInputDeviceName(int32 index) const;
    status_t SelectInputDevice(int32 index);

    // Recording format configuration
    status_t SetRecordingFormat(const media_format& format);
    const media_format& GetRecordingFormat() const { return fRecordingFormat; }

    // Real-time monitoring
    void SetListener(AudioRecordingListener* listener) { fListener = listener; }
    float GetInputLevel() const { return fInputLevel; }

    // File-based recording
    status_t SetRecordingFile(const char* path);
    const char* GetRecordingFile() const { return fRecordingPath.String(); }

    // Integration with VeniceDAW engine
    status_t ConnectToEngine(HaikuDAW::SimpleHaikuEngine* engine);
    void DisconnectFromEngine();

private:
    // BSoundRecorder hook function
    static bool RecordingHook(void* userData, const void* data, size_t size, const media_format& format);

    // Internal methods
    bool HandleRecordedData(const void* data, size_t size, const media_format& format);
    status_t InitializeRecorder();
    void CleanupRecorder();
    status_t CreateRecordingFile();
    void CloseRecordingFile();

    // BSoundRecorder instance (temporarily disabled)
    // BSoundRecorder* fSoundRecorder;
    void* fSoundRecorder;  // Placeholder for now

    // Recording state
    bool fRecording;
    bool fInitialized;

    // Input devices
    struct InputDevice {
        BString name;
        media_node node;
        media_input input;
    };
    std::vector<InputDevice> fInputDevices;
    int32 fSelectedDevice;

    // Recording format
    media_format fRecordingFormat;

    // Async file recording
    AsyncAudioWriter* fAsyncWriter;
    BString fRecordingPath;

    // Real-time monitoring
    AudioRecordingListener* fListener;
    float fInputLevel;
    bigtime_t fLastLevelUpdate;

    // VeniceDAW integration
    HaikuDAW::SimpleHaikuEngine* fEngine;

    // Buffer pool for zero-allocation real-time processing
    AudioBuffer fRecordBuffer;
};

/*
 * Recording session manager - handles multiple recording tracks
 */
class RecordingSession {
public:
    RecordingSession(HaikuDAW::SimpleHaikuEngine* engine);
    ~RecordingSession();

    // Session control
    status_t StartSession();
    status_t StopSession();
    bool IsActive() const { return fActive; }

    // Track recording
    status_t StartTrackRecording(int32 trackIndex, const char* filename = nullptr);
    status_t StopTrackRecording(int32 trackIndex);
    bool IsTrackRecording(int32 trackIndex) const;

    // Input routing
    status_t RouteInputToTrack(int32 inputDevice, int32 trackIndex);
    status_t SetTrackInputGain(int32 trackIndex, float gain);

    // Monitoring
    float GetTrackInputLevel(int32 trackIndex) const;

private:
    HaikuDAW::SimpleHaikuEngine* fEngine;
    bool fActive;

    struct TrackRecorder {
        AudioRecorder* recorder;
        int32 trackIndex;
        float inputGain;
        bool recording;
    };

    std::vector<TrackRecorder> fTrackRecorders;

    // Session management
    void CleanupSession();
};

} // namespace VeniceDAW

#endif
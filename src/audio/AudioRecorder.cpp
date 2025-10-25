/*
 * AudioRecorder.cpp - Native Haiku audio recording implementation
 */

#include "AudioRecorder.h"
#include "SimpleHaikuEngine.h"
#include "AudioLogging.h"
#include "AudioLevelCalculator.h"
#include "AsyncAudioWriter.h"
#include <media/MediaRoster.h>
#include <media/MediaFormats.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include <storage/Path.h>

namespace VeniceDAW {

// =====================================
// AudioRecorder Implementation
// =====================================

AudioRecorder::AudioRecorder()
    : fSoundRecorder(nullptr)  // Temporarily disabled
    , fRecording(false)
    , fInitialized(false)
    , fSelectedDevice(-1)
    , fAsyncWriter(nullptr)
    , fListener(nullptr)
    , fInputLevel(0.0f)
    , fLastLevelUpdate(0)
    , fEngine(nullptr)
{
    RECORDER_LOG_INFO("Constructor - native Haiku audio input");

    // Initialize default recording format (CD quality)
    memset(&fRecordingFormat, 0, sizeof(media_format));
    fRecordingFormat.type = B_MEDIA_RAW_AUDIO;
    fRecordingFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
    fRecordingFormat.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
    fRecordingFormat.u.raw_audio.frame_rate = 44100.0f;
    fRecordingFormat.u.raw_audio.channel_count = 2;
    fRecordingFormat.u.raw_audio.buffer_size = 2048 * sizeof(float) * 2;

    // Enumerate available input devices
    EnumerateInputDevices();
}

AudioRecorder::~AudioRecorder()
{
    RECORDER_LOG_INFO("Destructor");

    StopRecording();
    CleanupRecorder();

    // Cleanup async writer
    if (fAsyncWriter) {
        fAsyncWriter->StopWriting();
        delete fAsyncWriter;
        fAsyncWriter = nullptr;
    }

    // fRecordBuffer is automatically cleaned up by AudioBuffer destructor
}

status_t AudioRecorder::StartRecording(const char* filename)
{
    RECORDER_LOG_INFO("StartRecording('%s')", filename ? filename : "live monitoring");

    if (fRecording) {
        RECORDER_LOG_WARNING("Already recording");
        return B_ERROR;
    }

    // Initialize recorder if needed
    status_t status = InitializeRecorder();
    if (status != B_OK) {
        RECORDER_LOG_ERROR("Failed to initialize recorder: %s", strerror(status));
        return status;
    }

    // Set up async file recording if filename provided
    if (filename) {
        fRecordingPath.SetTo(filename);

        // Create async writer
        fAsyncWriter = new AsyncAudioWriter();
        status = fAsyncWriter->StartWriting(filename, fRecordingFormat);
        if (status != B_OK) {
            RECORDER_LOG_ERROR("Failed to start async recording: %s", strerror(status));
            delete fAsyncWriter;
            fAsyncWriter = nullptr;
            return status;
        }

        RECORDER_LOG_INFO("Async file recording initialized for '%s'", filename);
    }

    // Start the recorder
    status = fSoundRecorder->Start();
    if (status != B_OK) {
        RECORDER_LOG_ERROR("Failed to start BSoundRecorder: %s", strerror(status));

        // Cleanup async writer on error
        if (fAsyncWriter) {
            fAsyncWriter->StopWriting();
            delete fAsyncWriter;
            fAsyncWriter = nullptr;
        }

        return status;
    }

    fRecording = true;
    RECORDER_LOG_INFO("Recording started successfully");

    // Notify listener
    if (fListener) {
        fListener->OnRecordingStarted();
    }

    return B_OK;
}

status_t AudioRecorder::StopRecording()
{
    if (!fRecording) {
        return B_OK;
    }

    RECORDER_LOG_INFO("StopRecording()");

    // Stop the recorder
    if (fSoundRecorder) {
        fSoundRecorder->Stop();
    }

    fRecording = false;

    // Stop async file writing
    if (fAsyncWriter) {
        fAsyncWriter->StopWriting();
        delete fAsyncWriter;
        fAsyncWriter = nullptr;
        RECORDER_LOG_INFO("Async file writing stopped");
    }

    RECORDER_LOG_INFO("Recording stopped");

    // Notify listener
    if (fListener) {
        fListener->OnRecordingStopped();
    }

    return B_OK;
}

status_t AudioRecorder::EnumerateInputDevices()
{
    RECORDER_LOG_INFO("Enumerating input devices");

    fInputDevices.clear();

    BMediaRoster* roster = BMediaRoster::Roster();
    if (!roster) {
        RECORDER_LOG_ERROR("Failed to get media roster");
        return B_ERROR;
    }

    // Find audio input nodes
    media_node node;
    int32 cookie = 0;

    while (roster->GetNextNode(&cookie, &node) == B_OK) {
        if (node.kind & B_PHYSICAL_INPUT) {
            // Get node information
            media_node_id nodeID = node.node;

            // Get inputs for this node
            media_input* inputs = nullptr;
            int32 inputCount = 0;

            status_t status = roster->GetAllInputsFor(node, &inputs, &inputCount);
            if (status == B_OK && inputCount > 0) {
                for (int32 i = 0; i < inputCount; i++) {
                    if (inputs[i].format.type == B_MEDIA_RAW_AUDIO) {
                        InputDevice device;
                        device.name.SetToFormat("Input %d (%s)", (int)fInputDevices.size() + 1, inputs[i].name);
                        device.node = node;
                        device.input = inputs[i];

                        fInputDevices.push_back(device);
                        RECORDER_LOG_DEBUG("Found input device: %s", device.name.String());
                    }
                }

                delete[] inputs;
            }
        }

        roster->ReleaseNode(node);
    }

    RECORDER_LOG_INFO("Found %d input devices", (int)fInputDevices.size());

    // Select first device by default
    if (!fInputDevices.empty()) {
        fSelectedDevice = 0;
    }

    return B_OK;
}

const char* AudioRecorder::GetInputDeviceName(int32 index) const
{
    if (index >= 0 && (size_t)index < fInputDevices.size()) {
        return fInputDevices[index].name.String();
    }
    return nullptr;
}

status_t AudioRecorder::SelectInputDevice(int32 index)
{
    if (index < 0 || (size_t)index >= fInputDevices.size()) {
        RECORDER_LOG_ERROR("Invalid device index %d", (int)index);
        return B_BAD_INDEX;
    }

    fSelectedDevice = index;
    RECORDER_LOG_INFO("Selected input device: %s", fInputDevices[index].name.String());

    // If recorder is initialized, need to reinitialize with new device
    if (fInitialized) {
        CleanupRecorder();
        return InitializeRecorder();
    }

    return B_OK;
}

status_t AudioRecorder::SetRecordingFormat(const media_format& format)
{
    if (fRecording) {
        RECORDER_LOG_WARNING("Cannot change format while recording");
        return B_ERROR;
    }

    fRecordingFormat = format;
    RECORDER_LOG_INFO("Set recording format: %.1f Hz, %d channels",
           format.u.raw_audio.frame_rate, format.u.raw_audio.channel_count);

    // Reinitialize if needed
    if (fInitialized) {
        CleanupRecorder();
        return InitializeRecorder();
    }

    return B_OK;
}

status_t AudioRecorder::ConnectToEngine(HaikuDAW::SimpleHaikuEngine* engine)
{
    if (!engine) {
        return B_BAD_VALUE;
    }

    fEngine = engine;
    RECORDER_LOG_INFO("Connected to VeniceDAW engine");
    return B_OK;
}

void AudioRecorder::DisconnectFromEngine()
{
    fEngine = nullptr;
    RECORDER_LOG_INFO("Disconnected from engine");
}

status_t AudioRecorder::SetRecordingFile(const char* path)
{
    if (!path) {
        return B_BAD_VALUE;
    }

    if (fRecording) {
        RECORDER_LOG_WARNING("Cannot change recording file while recording");
        return B_ERROR;
    }

    fRecordingPath.SetTo(path);
    RECORDER_LOG_INFO("Set recording file to: %s", path);
    return B_OK;
}

// Static callback for BSoundRecorder
bool AudioRecorder::RecordingHook(void* userData, const void* data, size_t size, const media_format& format)
{
    AudioRecorder* recorder = static_cast<AudioRecorder*>(userData);
    if (recorder) {
        return recorder->HandleRecordedData(data, size, format);
    }
    return false;
}

bool AudioRecorder::HandleRecordedData(const void* data, size_t size, const media_format& format)
{
    if (!data || size == 0) {
        return true;
    }

    // Update input level for monitoring using SIMD-optimized calculation
    if (format.u.raw_audio.format == media_raw_audio_format::B_AUDIO_FLOAT) {
        const float* samples = static_cast<const float*>(data);
        size_t sampleCount = size / sizeof(float);
        uint32 channels = format.u.raw_audio.channel_count;

        // Use fast peak detection for real-time performance
        if (channels == 2 && sampleCount >= 2) {
            // Stereo optimized path
            size_t frameCount = sampleCount / 2;
            float peakLeft, peakRight;
            FastAudio::FastStereoPeak(samples, frameCount, peakLeft, peakRight);
            fInputLevel = std::max(peakLeft, peakRight);
        } else {
            // Mono or fallback path
            fInputLevel = FastAudio::FastPeak(samples, sampleCount);
        }
    }

    // Queue audio data for async file writing (non-blocking)
    if (fAsyncWriter && fRecording) {
        status_t status = fAsyncWriter->QueueAudioData(data, size, format);
        if (status != B_OK) {
            // Only log errors occasionally to avoid spamming in real-time context
            static bigtime_t lastErrorLog = 0;
            bigtime_t now = system_time();
            if (now - lastErrorLog > 1000000) {  // 1 second
                RECORDER_RT_LOG_ERROR("Failed to queue audio data: %s", strerror(status));
                lastErrorLog = now;
            }
            // Don't return false - continue processing even if file write fails
        }
    }

    // Send to listener for real-time processing
    if (fListener) {
        fListener->OnAudioData(data, size, format);
    }

    // Send to VeniceDAW engine for live monitoring
    if (fEngine) {
        // Feed audio to monitoring track for real-time visualization
        fEngine->FeedMonitoringAudio(data, size, format);
    }

    return true;
}

status_t AudioRecorder::InitializeRecorder()
{
    if (fInitialized) {
        return B_OK;
    }

    RECORDER_LOG_DEBUG("Initializing BSoundRecorder");

    // Create BSoundRecorder
    fSoundRecorder = new BSoundRecorder(&fRecordingFormat, RecordingHook, this);

    status_t status = fSoundRecorder->InitCheck();
    if (status != B_OK) {
        RECORDER_LOG_ERROR("BSoundRecorder init failed: %s", strerror(status));
        delete fSoundRecorder;
        fSoundRecorder = nullptr;
        return status;
    }

    // Get buffer from pool for zero-allocation audio processing
    size_t bufferFrames = fRecordingFormat.u.raw_audio.buffer_size /
                         (fRecordingFormat.u.raw_audio.channel_count * sizeof(float));
    uint32 channels = fRecordingFormat.u.raw_audio.channel_count;

    fRecordBuffer = AudioBufferPool::GetGlobalPool().GetBuffer(bufferFrames, channels);
    if (!fRecordBuffer.IsValid()) {
        RECORDER_LOG_ERROR("Failed to get buffer from pool (%zu frames, %u channels)",
               bufferFrames, channels);
        delete fSoundRecorder;
        fSoundRecorder = nullptr;
        return B_NO_MEMORY;
    }

    RECORDER_LOG_DEBUG("Got buffer from pool (%zu frames, %u channels, %zu bytes)",
           bufferFrames, channels, fRecordBuffer.SizeInBytes());

    fInitialized = true;
    RECORDER_LOG_INFO("BSoundRecorder initialized successfully");
    return B_OK;
}

void AudioRecorder::CleanupRecorder()
{
    if (fSoundRecorder) {
        if (fRecording) {
            fSoundRecorder->Stop();
        }
        delete fSoundRecorder;
        fSoundRecorder = nullptr;
    }

    fInitialized = false;
    RECORDER_LOG_DEBUG("Cleaned up BSoundRecorder");
}

status_t AudioRecorder::CreateRecordingFile()
{
    // DEPRECATED: This method has been replaced by AsyncAudioWriter
    // File recording is now handled asynchronously for better real-time performance
    RECORDER_LOG_WARNING("CreateRecordingFile() is deprecated, use AsyncAudioWriter instead");
    return B_OK;
}

void AudioRecorder::CloseRecordingFile()
{
    // DEPRECATED: This method has been replaced by AsyncAudioWriter
    // File closing is now handled asynchronously for better real-time performance
    RECORDER_LOG_WARNING("CloseRecordingFile() is deprecated, use AsyncAudioWriter instead");
}

// =====================================
// RecordingSession Implementation
// =====================================

RecordingSession::RecordingSession(HaikuDAW::SimpleHaikuEngine* engine)
    : fEngine(engine)
    , fActive(false)
{
    printf("RecordingSession: Created for VeniceDAW engine\n");
}

RecordingSession::~RecordingSession()
{
    StopSession();
    CleanupSession();
    printf("RecordingSession: Destroyed\n");
}

status_t RecordingSession::StartSession()
{
    if (fActive) {
        return B_OK;
    }

    printf("RecordingSession: Starting recording session\n");

    if (!fEngine) {
        printf("RecordingSession: No engine available\n");
        return B_ERROR;
    }

    fActive = true;
    printf("RecordingSession: Session started\n");
    return B_OK;
}

status_t RecordingSession::StopSession()
{
    if (!fActive) {
        return B_OK;
    }

    printf("RecordingSession: Stopping recording session\n");

    // Stop all track recordings
    for (size_t i = 0; i < fTrackRecorders.size(); i++) {
        if (fTrackRecorders[i].recording) {
            StopTrackRecording(i);
        }
    }

    fActive = false;
    printf("RecordingSession: Session stopped\n");
    return B_OK;
}

status_t RecordingSession::StartTrackRecording(int32 trackIndex, const char* filename)
{
    if (!fActive || !fEngine) {
        return B_ERROR;
    }

    printf("RecordingSession: Starting recording on track %d\n", (int)trackIndex);

    // Find or create track recorder
    TrackRecorder* trackRecorder = nullptr;

    for (size_t i = 0; i < fTrackRecorders.size(); i++) {
        if (fTrackRecorders[i].trackIndex == trackIndex) {
            trackRecorder = &fTrackRecorders[i];
            break;
        }
    }

    if (!trackRecorder) {
        // Create new track recorder
        TrackRecorder newRecorder;
        newRecorder.recorder = new AudioRecorder();
        newRecorder.trackIndex = trackIndex;
        newRecorder.inputGain = 1.0f;
        newRecorder.recording = false;

        // Connect to engine
        newRecorder.recorder->ConnectToEngine(fEngine);

        fTrackRecorders.push_back(newRecorder);
        trackRecorder = &fTrackRecorders.back();
    }

    if (trackRecorder->recording) {
        printf("RecordingSession: Track %d already recording\n", (int)trackIndex);
        return B_ERROR;
    }

    // Start recording
    status_t status = trackRecorder->recorder->StartRecording(filename);
    if (status == B_OK) {
        trackRecorder->recording = true;
        printf("RecordingSession: Track %d recording started\n", (int)trackIndex);
    }

    return status;
}

status_t RecordingSession::StopTrackRecording(int32 trackIndex)
{
    for (size_t i = 0; i < fTrackRecorders.size(); i++) {
        if (fTrackRecorders[i].trackIndex == trackIndex && fTrackRecorders[i].recording) {
            status_t status = fTrackRecorders[i].recorder->StopRecording();
            fTrackRecorders[i].recording = false;
            printf("RecordingSession: Track %d recording stopped\n", (int)trackIndex);
            return status;
        }
    }

    return B_ERROR;
}

bool RecordingSession::IsTrackRecording(int32 trackIndex) const
{
    for (size_t i = 0; i < fTrackRecorders.size(); i++) {
        if (fTrackRecorders[i].trackIndex == trackIndex) {
            return fTrackRecorders[i].recording;
        }
    }
    return false;
}

status_t RecordingSession::RouteInputToTrack(int32 inputDevice, int32 trackIndex)
{
    for (size_t i = 0; i < fTrackRecorders.size(); i++) {
        if (fTrackRecorders[i].trackIndex == trackIndex) {
            return fTrackRecorders[i].recorder->SelectInputDevice(inputDevice);
        }
    }
    return B_ERROR;
}

status_t RecordingSession::SetTrackInputGain(int32 trackIndex, float gain)
{
    for (size_t i = 0; i < fTrackRecorders.size(); i++) {
        if (fTrackRecorders[i].trackIndex == trackIndex) {
            fTrackRecorders[i].inputGain = gain;
            printf("RecordingSession: Set track %d input gain to %.2f\n", (int)trackIndex, gain);
            return B_OK;
        }
    }
    return B_ERROR;
}

float RecordingSession::GetTrackInputLevel(int32 trackIndex) const
{
    for (size_t i = 0; i < fTrackRecorders.size(); i++) {
        if (fTrackRecorders[i].trackIndex == trackIndex) {
            return fTrackRecorders[i].recorder->GetInputLevel() * fTrackRecorders[i].inputGain;
        }
    }
    return 0.0f;
}

void RecordingSession::CleanupSession()
{
    for (size_t i = 0; i < fTrackRecorders.size(); i++) {
        delete fTrackRecorders[i].recorder;
    }
    fTrackRecorders.clear();
    printf("RecordingSession: Cleaned up all track recorders\n");
}

} // namespace VeniceDAW
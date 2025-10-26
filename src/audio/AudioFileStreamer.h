/*
 * AudioFileStreamer.h - Lock-free audio file streaming with ring buffer
 * Eliminates synchronous I/O in RT audio thread
 */

#ifndef AUDIO_FILE_STREAMER_H
#define AUDIO_FILE_STREAMER_H

#include <media/MediaFile.h>
#include <media/MediaTrack.h>
#include <storage/Entry.h>
#include <kernel/OS.h>
#include <atomic>

// Forward declarations to avoid circular includes
namespace VeniceDAW {
    class AudioBufferPool;
}

namespace HaikuDAW {

/*
 * AudioFileStreamer - Asynchronous audio file streaming
 *
 * Architecture:
 * - Ring buffer holds 4 seconds of pre-loaded audio data
 * - Background I/O thread continuously reads ahead from BMediaTrack
 * - RT audio thread reads from ring buffer (lock-free, <100μs latency)
 * - Atomic read/write pointers for thread synchronization
 *
 * Memory usage: ~350KB per track (4 sec @ 44.1kHz stereo float)
 */
class AudioFileStreamer {
public:
    AudioFileStreamer();
    ~AudioFileStreamer();

    // File management
    status_t OpenFile(const entry_ref& ref);
    void CloseFile();
    bool IsFileOpen() const { return fFileOpen.load(); }

    // File information
    int64 GetDuration() const { return fFileDuration; }
    float GetSampleRate() const { return fFileSampleRate; }
    const char* GetFilePath() const { return fFilePath.String(); }

    // Playback control
    void SetPlaybackPosition(int64 frame);
    int64 GetPlaybackPosition() const { return fPlaybackFrame.load(); }

    // RT-safe audio data access
    status_t GetAudioData(float* buffer, int32 frameCount);

    // Ring buffer status (for monitoring/debugging)
    int32 GetBufferFillPercent() const;
    bool IsUnderrun() const { return fUnderrunOccurred.load(); }

private:
    // Ring buffer configuration
    static constexpr size_t RING_BUFFER_SECONDS = 4;
    static constexpr size_t RING_BUFFER_SAMPLE_RATE = 44100;
    static constexpr size_t RING_BUFFER_CHANNELS = 2;
    static constexpr size_t RING_BUFFER_FRAMES = RING_BUFFER_SAMPLE_RATE * RING_BUFFER_SECONDS;
    static constexpr size_t RING_BUFFER_SAMPLES = RING_BUFFER_FRAMES * RING_BUFFER_CHANNELS;

    // I/O thread configuration
    static constexpr int32 IO_THREAD_PRIORITY = B_LOW_PRIORITY;  // Lower than RT audio
    static constexpr size_t READ_CHUNK_FRAMES = 2048;  // Read 2048 frames at a time

    // File state
    BMediaFile* fMediaFile;
    BMediaTrack* fMediaTrack;
    media_format fFileFormat;
    int64 fFileDuration;
    float fFileSampleRate;
    BString fFilePath;
    std::atomic<bool> fFileOpen;

    // Ring buffer
    float* fRingBuffer;
    std::atomic<int64> fReadPos;   // RT thread read position
    std::atomic<int64> fWritePos;  // IO thread write position
    std::atomic<int64> fPlaybackFrame;  // Current playback position in file

    // I/O thread
    thread_id fIOThread;
    std::atomic<bool> fIOThreadRunning;
    sem_id fWakeupSemaphore;  // Wake up IO thread when buffer low

    // Status tracking
    std::atomic<bool> fUnderrunOccurred;
    std::atomic<bool> fLoopEnabled;

    // Shared buffer pool (eliminates per-thread allocations)
    ::VeniceDAW::AudioBufferPool* fBufferPool;

    // Private methods
    static int32 _IOThreadEntry(void* data);
    void _IOThreadFunc();
    int64 _GetAvailableFrames() const;
    int64 _GetFreeFrames() const;
    void _FillRingBuffer();

    // Non-copyable
    AudioFileStreamer(const AudioFileStreamer&) = delete;
    AudioFileStreamer& operator=(const AudioFileStreamer&) = delete;
};

} // namespace HaikuDAW

#endif // AUDIO_FILE_STREAMER_H

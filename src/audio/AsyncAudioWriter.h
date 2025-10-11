/*
 * AsyncAudioWriter.h - Non-blocking file writing for real-time audio
 * Separates audio I/O from file I/O to prevent dropouts
 */

#ifndef ASYNC_AUDIO_WRITER_H
#define ASYNC_AUDIO_WRITER_H

#include <OS.h>
#include <support/String.h>
#include <media/MediaDefs.h>
#include <vector>
#include <atomic>
#include "AudioBufferPool.h"
#include "AudioLogging.h"

// Forward declarations
class BMediaFile;
class BMediaTrack;

namespace VeniceDAW {

/*
 * Audio data queue entry for async writing
 */
struct AudioWriteRequest {
    AudioBuffer buffer;      // Audio data to write (RAII managed)
    media_format format;     // Audio format information
    bigtime_t timestamp;     // When this data was recorded
    bool isLast;             // True if this is the final chunk
    int64 frameCount;        // Number of frames to write

    AudioWriteRequest() : timestamp(0), isLast(false), frameCount(0) {}

    AudioWriteRequest(AudioBuffer&& buf, const media_format& fmt, int64 frames)
        : buffer(std::move(buf)), format(fmt), isLast(false), frameCount(frames)
    {
        timestamp = system_time();
    }
};

/*
 * High-performance async audio file writer
 * Queues audio data from real-time thread and writes in background
 */
class AsyncAudioWriter {
public:
    AsyncAudioWriter();
    ~AsyncAudioWriter();

    // File management
    status_t StartWriting(const char* filename, const media_format& format);
    status_t StopWriting();
    bool IsWriting() const { return fWriting.load(); }

    // Non-blocking audio data submission (called from audio thread)
    status_t QueueAudioData(const void* data, size_t size, const media_format& format);
    status_t QueueAudioBuffer(AudioBuffer&& buffer, const media_format& format);

    // Statistics and monitoring
    struct WriterStats {
        uint32 queuedRequests;
        uint32 processedRequests;
        uint32 droppedRequests;    // Due to queue overflow
        uint32 writeErrors;
        uint64 totalBytesWritten;
        float averageWriteTimeMs;
        bool queueOverflow;
    };
    WriterStats GetStats() const;

    // Configuration
    void SetMaxQueueSize(uint32 maxSize) { fMaxQueueSize = maxSize; }
    void SetWriteThreadPriority(int32 priority);

private:
    // Writer thread management
    static int32 WriterThreadEntry(void* data);
    int32 WriterThreadLoop();
    status_t StartWriterThread();
    void StopWriterThread();

    // Queue management
    status_t EnqueueRequest(AudioWriteRequest&& request);
    bool DequeueRequest(AudioWriteRequest& request);
    void DrainQueue();

    // File operations (called from writer thread only)
    status_t InitializeFile(const char* filename, const media_format& format);
    status_t WriteBufferToFile(const AudioWriteRequest& request);
    void CloseFile();

    // Thread synchronization
    thread_id fWriterThread;
    sem_id fQueueSemaphore;     // Signals data available
    sem_id fQueueMutex;         // Protects queue access
    std::atomic<bool> fShouldStop;

    // File writing state
    std::atomic<bool> fWriting;
    BMediaFile* fMediaFile;
    BMediaTrack* fMediaTrack;
    BString fOutputPath;
    media_format fFileFormat;

    // Audio data queue
    std::vector<AudioWriteRequest> fWriteQueue;
    uint32 fMaxQueueSize;
    uint32 fQueueHead;          // Next position to read
    uint32 fQueueTail;          // Next position to write
    uint32 fQueueCount;         // Current number of items

    // Performance statistics (atomic for lock-free access)
    mutable std::atomic<uint32> fQueuedRequests;
    mutable std::atomic<uint32> fProcessedRequests;
    mutable std::atomic<uint32> fDroppedRequests;
    mutable std::atomic<uint32> fWriteErrors;
    mutable std::atomic<uint64> fTotalBytesWritten;
    mutable std::atomic<uint32> fAverageWriteTimeUs;

    // Configuration
    int32 fWriterThreadPriority;
    static const uint32 kDefaultMaxQueueSize = 256;
    static const int32 kDefaultWriterPriority = B_LOW_PRIORITY;
    static const bigtime_t kQueueTimeoutUs = 10000;  // 10ms timeout
};

/*
 * RAII wrapper for async audio writing
 */
class ScopedAsyncWriter {
public:
    ScopedAsyncWriter(const char* filename, const media_format& format);
    ~ScopedAsyncWriter();

    // Check if writer is ready
    bool IsValid() const { return fWriter && fWriter->IsWriting(); }

    // Forward to writer
    status_t QueueAudioData(const void* data, size_t size, const media_format& format) {
        return fWriter ? fWriter->QueueAudioData(data, size, format) : B_NO_INIT;
    }

    status_t QueueAudioBuffer(AudioBuffer&& buffer, const media_format& format) {
        return fWriter ? fWriter->QueueAudioBuffer(std::move(buffer), format) : B_NO_INIT;
    }

    AsyncAudioWriter::WriterStats GetStats() const {
        return fWriter ? fWriter->GetStats() : AsyncAudioWriter::WriterStats();
    }

private:
    AsyncAudioWriter* fWriter;
};

/*
 * Global async writer pool for shared usage
 */
class AsyncWriterPool {
public:
    static AsyncWriterPool& Instance();

    // Get a writer from the pool
    AsyncAudioWriter* GetWriter();
    void ReturnWriter(AsyncAudioWriter* writer);

    // Pool statistics
    uint32 GetAvailableWriters() const;
    uint32 GetActiveWriters() const;

private:
    AsyncWriterPool();
    ~AsyncWriterPool();

    std::vector<AsyncAudioWriter*> fAvailableWriters;
    std::vector<AsyncAudioWriter*> fActiveWriters;
    sem_id fPoolMutex;

    static const uint32 kMaxPoolSize = 8;
};

// Convenience macros
#define ASYNC_WRITE_AUDIO(writer, data, size, format) \
    do { \
        status_t _status = (writer)->QueueAudioData(data, size, format); \
        if (_status != B_OK) { \
            AUDIO_RT_LOG_WARNING("AsyncWriter", "Failed to queue audio: %s", strerror(_status)); \
        } \
    } while(0)

#define ASYNC_WRITE_BUFFER(writer, buffer, format) \
    do { \
        status_t _status = (writer)->QueueAudioBuffer(std::move(buffer), format); \
        if (_status != B_OK) { \
            AUDIO_RT_LOG_WARNING("AsyncWriter", "Failed to queue buffer: %s", strerror(_status)); \
        } \
    } while(0)

} // namespace VeniceDAW

#endif
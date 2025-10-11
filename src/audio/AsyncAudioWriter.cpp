/*
 * AsyncAudioWriter.cpp - Non-blocking file writing implementation
 */

#include "AsyncAudioWriter.h"
#include <media/MediaFile.h>
#include <media/MediaTrack.h>
#include <media/MediaFormats.h>
#include <storage/Path.h>
#include <string.h>
#include <algorithm>

namespace VeniceDAW {

// =====================================
// AsyncAudioWriter Implementation
// =====================================

AsyncAudioWriter::AsyncAudioWriter()
    : fWriterThread(-1)
    , fQueueSemaphore(-1)
    , fQueueMutex(-1)
    , fShouldStop(false)
    , fWriting(false)
    , fMediaFile(nullptr)
    , fMediaTrack(nullptr)
    , fMaxQueueSize(kDefaultMaxQueueSize)
    , fQueueHead(0)
    , fQueueTail(0)
    , fQueueCount(0)
    , fQueuedRequests(0)
    , fProcessedRequests(0)
    , fDroppedRequests(0)
    , fWriteErrors(0)
    , fTotalBytesWritten(0)
    , fAverageWriteTimeUs(0)
    , fWriterThreadPriority(kDefaultWriterPriority)
{
    // Create synchronization primitives
    fQueueSemaphore = create_sem(0, "AsyncAudioWriter_Queue");
    fQueueMutex = create_sem(1, "AsyncAudioWriter_Mutex");

    // Pre-allocate queue
    fWriteQueue.resize(fMaxQueueSize);

    AUDIO_LOG_DEBUG("AsyncAudioWriter", "Created with queue size %u", fMaxQueueSize);
}

AsyncAudioWriter::~AsyncAudioWriter()
{
    StopWriting();

    if (fQueueSemaphore >= 0) delete_sem(fQueueSemaphore);
    if (fQueueMutex >= 0) delete_sem(fQueueMutex);

    AUDIO_LOG_DEBUG("AsyncAudioWriter", "Destroyed");
}

status_t AsyncAudioWriter::StartWriting(const char* filename, const media_format& format)
{
    if (fWriting.load()) {
        AUDIO_LOG_WARNING("AsyncAudioWriter", "Already writing to a file");
        return B_ERROR;
    }

    if (!filename) {
        AUDIO_LOG_ERROR("AsyncAudioWriter", "Invalid filename");
        return B_BAD_VALUE;
    }

    AUDIO_LOG_INFO("AsyncAudioWriter", "Starting async writing to '%s'", filename);

    // Store file parameters
    fOutputPath.SetTo(filename);
    fFileFormat = format;

    // Start writer thread
    status_t status = StartWriterThread();
    if (status != B_OK) {
        AUDIO_LOG_ERROR("AsyncAudioWriter", "Failed to start writer thread: %s", strerror(status));
        return status;
    }

    fWriting = true;
    AUDIO_LOG_INFO("AsyncAudioWriter", "Async writing started successfully");
    return B_OK;
}

status_t AsyncAudioWriter::StopWriting()
{
    if (!fWriting.load()) {
        return B_OK;
    }

    AUDIO_LOG_INFO("AsyncAudioWriter", "Stopping async writing");

    fWriting = false;
    StopWriterThread();

    // Drain any remaining queue items
    DrainQueue();

    AUDIO_LOG_INFO("AsyncAudioWriter", "Async writing stopped");
    return B_OK;
}

status_t AsyncAudioWriter::QueueAudioData(const void* data, size_t size, const media_format& format)
{
    if (!fWriting.load() || !data || size == 0) {
        return B_BAD_VALUE;
    }

    AUDIO_PERF_TIMER("AsyncAudioWriter", "QueueAudioData");

    // Calculate frame count
    uint32 channels = format.u.raw_audio.channel_count;
    int64 frameCount = size / (channels * sizeof(float));

    // Get buffer from pool and copy data
    AudioBuffer buffer = AudioBufferPool::GetGlobalPool().GetBuffer(frameCount, channels);
    if (!buffer.IsValid()) {
        fDroppedRequests++;
        AUDIO_RT_LOG_WARNING("AsyncAudioWriter", "Failed to get buffer from pool");
        return B_NO_MEMORY;
    }

    // Copy audio data to buffer
    memcpy(buffer.Data(), data, size);

    // Queue the buffer
    return QueueAudioBuffer(std::move(buffer), format);
}

status_t AsyncAudioWriter::QueueAudioBuffer(AudioBuffer&& buffer, const media_format& format)
{
    if (!fWriting.load() || !buffer.IsValid()) {
        return B_BAD_VALUE;
    }

    // Create write request
    AudioWriteRequest request(std::move(buffer), format, buffer.Frames());

    // Enqueue (non-blocking from audio thread perspective)
    status_t status = EnqueueRequest(std::move(request));
    if (status == B_OK) {
        fQueuedRequests++;
    } else {
        fDroppedRequests++;
    }

    return status;
}

AsyncAudioWriter::WriterStats AsyncAudioWriter::GetStats() const
{
    WriterStats stats;
    stats.queuedRequests = fQueuedRequests.load();
    stats.processedRequests = fProcessedRequests.load();
    stats.droppedRequests = fDroppedRequests.load();
    stats.writeErrors = fWriteErrors.load();
    stats.totalBytesWritten = fTotalBytesWritten.load();
    stats.averageWriteTimeMs = fAverageWriteTimeUs.load() / 1000.0f;

    // Check queue status
    if (acquire_sem_etc(fQueueMutex, 1, B_TIMEOUT, 1000) == B_OK) {
        stats.queueOverflow = (fQueueCount >= fMaxQueueSize * 0.9f);  // 90% full
        release_sem(fQueueMutex);
    } else {
        stats.queueOverflow = true;  // Assume overflow if can't get mutex
    }

    return stats;
}

void AsyncAudioWriter::SetWriteThreadPriority(int32 priority)
{
    fWriterThreadPriority = priority;
    if (fWriterThread >= 0) {
        set_thread_priority(fWriterThread, priority);
    }
}

// =====================================
// Thread Management
// =====================================

int32 AsyncAudioWriter::WriterThreadEntry(void* data)
{
    AsyncAudioWriter* writer = static_cast<AsyncAudioWriter*>(data);
    return writer->WriterThreadLoop();
}

int32 AsyncAudioWriter::WriterThreadLoop()
{
    AUDIO_LOG_DEBUG("AsyncAudioWriter", "Writer thread started");

    // Initialize file
    status_t status = InitializeFile(fOutputPath.String(), fFileFormat);
    if (status != B_OK) {
        AUDIO_LOG_ERROR("AsyncAudioWriter", "Failed to initialize file: %s", strerror(status));
        fWriting = false;
        return status;
    }

    // Main processing loop
    while (!fShouldStop) {
        AudioWriteRequest request;

        // Wait for data or timeout
        if (acquire_sem_etc(fQueueSemaphore, 1, B_TIMEOUT, kQueueTimeoutUs) != B_OK) {
            continue;  // Timeout or error, check stop condition
        }

        // Get request from queue
        if (!DequeueRequest(request)) {
            continue;  // Queue empty
        }

        // Write to file
        bigtime_t writeStart = system_time();
        status = WriteBufferToFile(request);
        bigtime_t writeTime = system_time() - writeStart;

        if (status == B_OK) {
            fProcessedRequests++;
            fTotalBytesWritten += request.buffer.SizeInBytes();

            // Update average write time (simple moving average)
            uint32 oldAvg = fAverageWriteTimeUs.load();
            uint32 newAvg = (oldAvg * 7 + writeTime) / 8;  // 7/8 weight to old value
            fAverageWriteTimeUs = newAvg;
        } else {
            fWriteErrors++;
            AUDIO_LOG_ERROR("AsyncAudioWriter", "Write error: %s", strerror(status));
        }

        // Check if this was the last request
        if (request.isLast) {
            break;
        }
    }

    // Cleanup
    CloseFile();

    AUDIO_LOG_DEBUG("AsyncAudioWriter", "Writer thread finished");
    return B_OK;
}

status_t AsyncAudioWriter::StartWriterThread()
{
    if (fWriterThread >= 0) {
        return B_ERROR;  // Already running
    }

    fShouldStop = false;

    fWriterThread = spawn_thread(WriterThreadEntry, "AsyncAudioWriter",
                                fWriterThreadPriority, this);

    if (fWriterThread < 0) {
        AUDIO_LOG_ERROR("AsyncAudioWriter", "Failed to spawn writer thread");
        return fWriterThread;
    }

    status_t status = resume_thread(fWriterThread);
    if (status != B_OK) {
        kill_thread(fWriterThread);
        fWriterThread = -1;
        AUDIO_LOG_ERROR("AsyncAudioWriter", "Failed to resume writer thread: %s", strerror(status));
        return status;
    }

    return B_OK;
}

void AsyncAudioWriter::StopWriterThread()
{
    if (fWriterThread < 0) {
        return;
    }

    // Signal thread to stop
    fShouldStop = true;

    // Wake up thread if it's waiting
    release_sem(fQueueSemaphore);

    // Wait for thread to finish (with timeout)
    status_t exitValue;
    wait_for_thread(fWriterThread, &exitValue);

    fWriterThread = -1;
}

// =====================================
// Queue Management
// =====================================

status_t AsyncAudioWriter::EnqueueRequest(AudioWriteRequest&& request)
{
    if (acquire_sem_etc(fQueueMutex, 1, B_TIMEOUT, 10000) != B_OK) {
        return B_TIMED_OUT;  // Don't block audio thread
    }

    status_t result = B_OK;

    if (fQueueCount >= fMaxQueueSize) {
        // Queue full - drop oldest request to make room
        // First, properly destroy the old request to free its AudioBuffer
        fWriteQueue[fQueueHead] = AudioWriteRequest(); // Clear old request (calls AudioBuffer destructor)
        fDroppedRequests++;
        fQueueHead = (fQueueHead + 1) % fMaxQueueSize;
        fQueueCount--;
        AUDIO_RT_LOG_WARNING("AsyncAudioWriter", "Queue overflow, dropping old request");
    }

    // Add new request
    fWriteQueue[fQueueTail] = std::move(request);
    fQueueTail = (fQueueTail + 1) % fMaxQueueSize;
    fQueueCount++;

    release_sem(fQueueMutex);

    // Signal writer thread
    release_sem(fQueueSemaphore);

    return result;
}

bool AsyncAudioWriter::DequeueRequest(AudioWriteRequest& request)
{
    if (acquire_sem_etc(fQueueMutex, 1, B_TIMEOUT, 10000) != B_OK) {
        return false;
    }

    bool hasData = false;

    if (fQueueCount > 0) {
        request = std::move(fWriteQueue[fQueueHead]);
        fQueueHead = (fQueueHead + 1) % fMaxQueueSize;
        fQueueCount--;
        hasData = true;
    }

    release_sem(fQueueMutex);
    return hasData;
}

void AsyncAudioWriter::DrainQueue()
{
    AudioWriteRequest request;
    while (DequeueRequest(request)) {
        // Just discard remaining requests
        fDroppedRequests++;
    }
}

// =====================================
// File Operations
// =====================================

status_t AsyncAudioWriter::InitializeFile(const char* filename, const media_format& format)
{
    AUDIO_LOG_DEBUG("AsyncAudioWriter", "Initializing file: %s", filename);

    // Create entry ref
    entry_ref ref;
    status_t status = get_ref_for_path(filename, &ref);
    if (status != B_OK) {
        AUDIO_LOG_ERROR("AsyncAudioWriter", "Failed to get ref for path: %s", strerror(status));
        return status;
    }

    // Get WAV file format - use simpler approach
    media_file_format fileFormat;
    memset(&fileFormat, 0, sizeof(fileFormat));

    // Set up basic WAV format manually
    strcpy(fileFormat.mime_type, "audio/wav");
    strcpy(fileFormat.pretty_name, "WAV Audio File");
    strcpy(fileFormat.short_name, "wav");
    strcpy(fileFormat.file_extension, "wav");
    fileFormat.capabilities = media_file_format::B_WRITABLE;

    // Create media file
    fMediaFile = new BMediaFile(&ref, &fileFormat, B_MEDIA_FILE_REPLACE_MODE);
    status = fMediaFile->InitCheck();
    if (status != B_OK) {
        AUDIO_LOG_ERROR("AsyncAudioWriter", "Failed to initialize media file: %s", strerror(status));
        delete fMediaFile;
        fMediaFile = nullptr;
        return status;
    }

    // Create audio track
    media_codec_info codecInfo;
    memset(&codecInfo, 0, sizeof(codecInfo));

    // CreateTrack requires non-const format pointer
    media_format formatCopy = format;
    fMediaTrack = fMediaFile->CreateTrack(&formatCopy, &codecInfo);
    if (!fMediaTrack) {
        AUDIO_LOG_ERROR("AsyncAudioWriter", "Failed to create media track");
        delete fMediaFile;
        fMediaFile = nullptr;
        return B_ERROR;
    }

    // Commit header
    status = fMediaFile->CommitHeader();
    if (status != B_OK) {
        AUDIO_LOG_ERROR("AsyncAudioWriter", "Failed to commit file header: %s", strerror(status));
        CloseFile();
        return status;
    }

    AUDIO_LOG_INFO("AsyncAudioWriter", "File initialized successfully");
    return B_OK;
}

status_t AsyncAudioWriter::WriteBufferToFile(const AudioWriteRequest& request)
{
    if (!fMediaTrack || !request.buffer.IsValid()) {
        return B_BAD_VALUE;
    }

    // Write frames to file
    status_t status = fMediaTrack->WriteFrames(request.buffer.Data(), request.frameCount);
    if (status != B_OK) {
        AUDIO_LOG_ERROR("AsyncAudioWriter", "WriteFrames failed: %s", strerror(status));
        return status;
    }

    return B_OK;
}

void AsyncAudioWriter::CloseFile()
{
    if (fMediaTrack && fMediaFile) {
        fMediaFile->ReleaseTrack(fMediaTrack);
        fMediaTrack = nullptr;
    }

    if (fMediaFile) {
        fMediaFile->CloseFile();
        delete fMediaFile;
        fMediaFile = nullptr;
    }

    AUDIO_LOG_DEBUG("AsyncAudioWriter", "File closed");
}

// =====================================
// RAII Wrapper Implementation
// =====================================

ScopedAsyncWriter::ScopedAsyncWriter(const char* filename, const media_format& format)
    : fWriter(nullptr)
{
    fWriter = new AsyncAudioWriter();
    if (fWriter->StartWriting(filename, format) != B_OK) {
        delete fWriter;
        fWriter = nullptr;
    }
}

ScopedAsyncWriter::~ScopedAsyncWriter()
{
    if (fWriter) {
        fWriter->StopWriting();
        delete fWriter;
    }
}

// =====================================
// Writer Pool Implementation
// =====================================

AsyncWriterPool& AsyncWriterPool::Instance()
{
    static AsyncWriterPool instance;
    return instance;
}

AsyncWriterPool::AsyncWriterPool()
    : fPoolMutex(-1)
{
    fPoolMutex = create_sem(1, "AsyncWriterPool");

    // Pre-create some writers
    fAvailableWriters.reserve(kMaxPoolSize);
    for (uint32 i = 0; i < kMaxPoolSize / 2; i++) {
        fAvailableWriters.push_back(new AsyncAudioWriter());
    }
}

AsyncWriterPool::~AsyncWriterPool()
{
    for (auto* writer : fAvailableWriters) {
        delete writer;
    }
    for (auto* writer : fActiveWriters) {
        delete writer;
    }

    if (fPoolMutex >= 0) {
        delete_sem(fPoolMutex);
    }
}

AsyncAudioWriter* AsyncWriterPool::GetWriter()
{
    if (acquire_sem(fPoolMutex) != B_OK) {
        return nullptr;
    }

    AsyncAudioWriter* writer = nullptr;

    if (!fAvailableWriters.empty()) {
        writer = fAvailableWriters.back();
        fAvailableWriters.pop_back();
        fActiveWriters.push_back(writer);
    } else if (fActiveWriters.size() < kMaxPoolSize) {
        writer = new AsyncAudioWriter();
        fActiveWriters.push_back(writer);
    }

    release_sem(fPoolMutex);
    return writer;
}

void AsyncWriterPool::ReturnWriter(AsyncAudioWriter* writer)
{
    if (!writer || acquire_sem(fPoolMutex) != B_OK) {
        return;
    }

    // Remove from active list
    auto it = std::find(fActiveWriters.begin(), fActiveWriters.end(), writer);
    if (it != fActiveWriters.end()) {
        fActiveWriters.erase(it);

        // Stop any ongoing writing
        writer->StopWriting();

        // Return to available pool
        fAvailableWriters.push_back(writer);
    }

    release_sem(fPoolMutex);
}

uint32 AsyncWriterPool::GetAvailableWriters() const
{
    return fAvailableWriters.size();
}

uint32 AsyncWriterPool::GetActiveWriters() const
{
    return fActiveWriters.size();
}

} // namespace VeniceDAW
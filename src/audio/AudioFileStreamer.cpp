/*
 * AudioFileStreamer.cpp - Lock-free audio file streaming implementation
 */

#include "AudioFileStreamer.h"
#include "AudioBufferPool.h"
#include <stdio.h>
#include <string.h>
#include <Path.h>

namespace HaikuDAW {

AudioFileStreamer::AudioFileStreamer()
    : fMediaFile(nullptr)
    , fMediaTrack(nullptr)
    , fFileDuration(0)
    , fFileSampleRate(44100.0f)
    , fFileOpen(false)
    , fRingBuffer(nullptr)
    , fReadPos(0)
    , fWritePos(0)
    , fPlaybackFrame(0)
    , fIOThread(-1)
    , fIOThreadRunning(false)
    , fWakeupSemaphore(-1)
    , fUnderrunOccurred(false)
    , fLoopEnabled(true)
    , fBufferPool(nullptr)
{
    // Allocate ring buffer (4 seconds @ 44.1kHz stereo = ~353KB)
    fRingBuffer = new float[RING_BUFFER_SAMPLES];
    memset(fRingBuffer, 0, RING_BUFFER_SAMPLES * sizeof(float));

    // Create semaphore for I/O thread wakeup
    fWakeupSemaphore = create_sem(0, "AudioFileStreamer wakeup");

    // Get shared buffer pool instance
    fBufferPool = &::VeniceDAW::AudioBufferPool::GetGlobalPool();

    printf("AudioFileStreamer: Created with %d second ring buffer (~%zu KB)\n",
           (int)RING_BUFFER_SECONDS,
           (RING_BUFFER_SAMPLES * sizeof(float)) / 1024);
    printf("AudioFileStreamer: Using shared AudioBufferPool for I/O operations\n");
}

AudioFileStreamer::~AudioFileStreamer()
{
    CloseFile();

    // Cleanup semaphore
    if (fWakeupSemaphore >= 0) {
        delete_sem(fWakeupSemaphore);
        fWakeupSemaphore = -1;
    }

    // Free ring buffer
    delete[] fRingBuffer;
    fRingBuffer = nullptr;

    printf("AudioFileStreamer: Destroyed\n");
}

status_t AudioFileStreamer::OpenFile(const entry_ref& ref)
{
    // Close any existing file first
    CloseFile();

    printf("AudioFileStreamer: Opening file '%s'\n", ref.name);

    // Create BMediaFile
    fMediaFile = new BMediaFile(&ref);
    status_t status = fMediaFile->InitCheck();
    if (status != B_OK) {
        printf("AudioFileStreamer: BMediaFile init failed: %s\n", strerror(status));
        delete fMediaFile;
        fMediaFile = nullptr;
        return status;
    }

    // Find first audio track
    int32 numTracks = fMediaFile->CountTracks();
    fMediaTrack = nullptr;

    for (int32 i = 0; i < numTracks; i++) {
        BMediaTrack* track = fMediaFile->TrackAt(i);
        if (!track) continue;

        media_format format;
        status = track->DecodedFormat(&format);
        if (status == B_OK && format.type == B_MEDIA_RAW_AUDIO) {
            fMediaTrack = track;
            fFileFormat = format;
            break;
        } else {
            fMediaFile->ReleaseTrack(track);
        }
    }

    if (!fMediaTrack) {
        printf("AudioFileStreamer: No audio track found\n");
        delete fMediaFile;
        fMediaFile = nullptr;
        return B_ERROR;
    }

    // Configure for stereo float output
    media_raw_audio_format& rawFormat = fFileFormat.u.raw_audio;
    rawFormat.format = media_raw_audio_format::B_AUDIO_FLOAT;
    rawFormat.channel_count = 2;  // Force stereo
    rawFormat.byte_order = B_MEDIA_HOST_ENDIAN;

    status = fMediaTrack->DecodedFormat(&fFileFormat);
    if (status != B_OK) {
        printf("AudioFileStreamer: Failed to set decoded format\n");
        fMediaFile->ReleaseTrack(fMediaTrack);
        fMediaTrack = nullptr;
        delete fMediaFile;
        fMediaFile = nullptr;
        return status;
    }

    // Store file info
    fFileSampleRate = rawFormat.frame_rate;
    fFileDuration = fMediaTrack->CountFrames();

    BPath filePath(&ref);
    fFilePath.SetTo(filePath.Path());

    // Reset positions
    fReadPos = 0;
    fWritePos = 0;
    fPlaybackFrame = 0;
    fUnderrunOccurred = false;

    // Start I/O thread
    fIOThreadRunning = true;
    fIOThread = spawn_thread(_IOThreadEntry, "AudioFileStreamer I/O",
                             IO_THREAD_PRIORITY, this);
    if (fIOThread < 0) {
        printf("AudioFileStreamer: Failed to spawn I/O thread\n");
        fIOThreadRunning = false;
        fMediaFile->ReleaseTrack(fMediaTrack);
        fMediaTrack = nullptr;
        delete fMediaFile;
        fMediaFile = nullptr;
        return B_ERROR;
    }

    resume_thread(fIOThread);
    fFileOpen = true;

    printf("AudioFileStreamer: File opened successfully\n");
    printf("  Duration: %lld frames (%.2f sec)\n", fFileDuration,
           (double)fFileDuration / fFileSampleRate);
    printf("  Sample rate: %.0f Hz\n", fFileSampleRate);

    return B_OK;
}

void AudioFileStreamer::CloseFile()
{
    if (!fFileOpen) return;

    printf("AudioFileStreamer: Closing file\n");

    // Stop I/O thread
    fIOThreadRunning = false;
    if (fIOThread >= 0) {
        // Wake up thread if sleeping
        release_sem(fWakeupSemaphore);

        // Wait for thread to finish
        status_t exitValue;
        wait_for_thread(fIOThread, &exitValue);
        fIOThread = -1;
    }

    // Release media resources
    if (fMediaFile && fMediaTrack) {
        fMediaFile->ReleaseTrack(fMediaTrack);
        fMediaTrack = nullptr;
    }

    delete fMediaFile;
    fMediaFile = nullptr;

    // Clear ring buffer
    memset(fRingBuffer, 0, RING_BUFFER_SAMPLES * sizeof(float));

    fFileOpen = false;
    fReadPos = 0;
    fWritePos = 0;
    fPlaybackFrame = 0;
}

void AudioFileStreamer::SetPlaybackPosition(int64 frame)
{
    if (!fFileOpen) return;

    // Clamp to file duration
    if (frame < 0) frame = 0;
    if (frame >= fFileDuration) frame = fFileDuration - 1;

    fPlaybackFrame = frame;

    // Invalidate ring buffer (force I/O thread to re-fill from new position)
    fReadPos = 0;
    fWritePos = 0;

    // Wake up I/O thread to start filling from new position
    release_sem(fWakeupSemaphore);
}

int32 AudioFileStreamer::GetBufferFillPercent() const
{
    int64 available = _GetAvailableFrames();
    return (int32)((available * 100) / RING_BUFFER_FRAMES);
}

status_t AudioFileStreamer::GetAudioData(float* buffer, int32 frameCount)
{
    if (!fFileOpen || !buffer) {
        // Fill with silence if file not open
        memset(buffer, 0, frameCount * RING_BUFFER_CHANNELS * sizeof(float));
        return B_OK;
    }

    int64 availableFrames = _GetAvailableFrames();

    // Check for underrun
    if (availableFrames < frameCount) {
        if (!fUnderrunOccurred) {
            printf("AudioFileStreamer: WARNING - Buffer underrun! Available: %lld, Requested: %d\n",
                   availableFrames, (int)frameCount);
            fUnderrunOccurred = true;
        }

        // Fill with silence for underrun
        memset(buffer, 0, frameCount * RING_BUFFER_CHANNELS * sizeof(float));

        // Wake up I/O thread to refill buffer urgently
        release_sem(fWakeupSemaphore);
        return B_OK;
    }

    // RT-safe read from ring buffer (lock-free)
    int64 readPos = fReadPos.load();

    for (int32 i = 0; i < frameCount; i++) {
        int64 ringPos = (readPos + i) % RING_BUFFER_FRAMES;
        int64 sampleIndex = ringPos * RING_BUFFER_CHANNELS;

        buffer[i * RING_BUFFER_CHANNELS] = fRingBuffer[sampleIndex];
        buffer[i * RING_BUFFER_CHANNELS + 1] = fRingBuffer[sampleIndex + 1];
    }

    // Update read position atomically
    fReadPos = (readPos + frameCount) % RING_BUFFER_FRAMES;

    // Wake up I/O thread if buffer is getting low (<25% full)
    int64 newAvailable = _GetAvailableFrames();
    if (newAvailable < (RING_BUFFER_FRAMES / 4)) {
        release_sem(fWakeupSemaphore);
    }

    return B_OK;
}

// Private methods

int32 AudioFileStreamer::_IOThreadEntry(void* data)
{
    AudioFileStreamer* streamer = static_cast<AudioFileStreamer*>(data);
    streamer->_IOThreadFunc();
    return 0;
}

void AudioFileStreamer::_IOThreadFunc()
{
    printf("AudioFileStreamer: I/O thread started\n");

    while (fIOThreadRunning) {
        // Check if buffer needs filling
        int64 freeFrames = _GetFreeFrames();

        if (freeFrames >= READ_CHUNK_FRAMES) {
            // Buffer has space - fill it
            _FillRingBuffer();
        } else {
            // Buffer is full - sleep until woken up or timeout
            acquire_sem_etc(fWakeupSemaphore, 1, B_RELATIVE_TIMEOUT, 100000);  // 100ms timeout
        }
    }

    printf("AudioFileStreamer: I/O thread stopped\n");
}

int64 AudioFileStreamer::_GetAvailableFrames() const
{
    int64 writePos = fWritePos.load();
    int64 readPos = fReadPos.load();

    if (writePos >= readPos) {
        return writePos - readPos;
    } else {
        // Wrapped around
        return (RING_BUFFER_FRAMES - readPos) + writePos;
    }
}

int64 AudioFileStreamer::_GetFreeFrames() const
{
    return RING_BUFFER_FRAMES - _GetAvailableFrames() - 1;  // -1 to avoid read==write ambiguity
}

void AudioFileStreamer::_FillRingBuffer()
{
    if (!fMediaTrack || !fBufferPool) return;

    // Calculate how many frames to read
    int64 freeFrames = _GetFreeFrames();
    if (freeFrames < READ_CHUNK_FRAMES) return;

    int32 framesToRead = READ_CHUNK_FRAMES;
    int64 writePos = fWritePos.load();

    // Get buffer from shared pool (eliminates allocation)
    ::VeniceDAW::AudioBuffer buffer = fBufferPool->GetBuffer(framesToRead, RING_BUFFER_CHANNELS);
    if (!buffer.IsValid()) {
        printf("AudioFileStreamer: WARNING - Failed to acquire buffer from pool\n");
        return;
    }

    float* tempBuffer = buffer.Data();

    // Read from BMediaTrack
    int64 framesRead = 0;
    media_header mh;
    status_t status = fMediaTrack->ReadFrames(tempBuffer, &framesRead, &mh);

    if (status != B_OK || framesRead == 0) {
        // End of file - loop back if enabled
        if (fLoopEnabled) {
            fMediaTrack->SeekToFrame(&fPlaybackFrame);
            int64 newFrame = 0;
            fPlaybackFrame = newFrame;

            // Try reading again from beginning
            status = fMediaTrack->ReadFrames(tempBuffer, &framesRead, &mh);
        }

        if (status != B_OK || framesRead == 0) {
            // Buffer automatically returned to pool via RAII
            return;
        }
    }

    // Copy data to ring buffer
    for (int64 i = 0; i < framesRead; i++) {
        int64 ringPos = (writePos + i) % RING_BUFFER_FRAMES;
        int64 sampleIndex = ringPos * RING_BUFFER_CHANNELS;

        fRingBuffer[sampleIndex] = tempBuffer[i * RING_BUFFER_CHANNELS];
        fRingBuffer[sampleIndex + 1] = tempBuffer[i * RING_BUFFER_CHANNELS + 1];
    }

    // Update write position atomically
    fWritePos = (writePos + framesRead) % RING_BUFFER_FRAMES;
    fPlaybackFrame = fPlaybackFrame + framesRead;

    // Buffer automatically returned to pool when 'buffer' goes out of scope (RAII)
}

} // namespace HaikuDAW

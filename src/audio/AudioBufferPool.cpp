/*
 * AudioBufferPool.cpp - Zero-allocation buffer management implementation
 */

#include "AudioBufferPool.h"
#include "AudioLogging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

namespace VeniceDAW {

// =====================================
// AudioBuffer Implementation
// =====================================

AudioBuffer::AudioBuffer(float* data, size_t frames, uint32 channels, AudioBufferPool* pool)
    : fData(data)
    , fSize(frames * channels * sizeof(float))
    , fFrames(frames)
    , fChannels(channels)
    , fPool(pool)
{
}

AudioBuffer::~AudioBuffer()
{
    if (fPool && fData) {
        fPool->ReturnBuffer(fData, fFrames, fChannels);
    }
}

AudioBuffer::AudioBuffer(AudioBuffer&& other) noexcept
    : fData(other.fData)
    , fSize(other.fSize)
    , fFrames(other.fFrames)
    , fChannels(other.fChannels)
    , fPool(other.fPool)
{
    // Transfer ownership
    other.fData = nullptr;
    other.fSize = 0;
    other.fFrames = 0;
    other.fChannels = 0;
    other.fPool = nullptr;
}

AudioBuffer& AudioBuffer::operator=(AudioBuffer&& other) noexcept
{
    if (this != &other) {
        // Return current buffer to pool if we have one
        if (fPool && fData) {
            fPool->ReturnBuffer(fData, fFrames, fChannels);
        }

        // Transfer ownership from other
        fData = other.fData;
        fSize = other.fSize;
        fFrames = other.fFrames;
        fChannels = other.fChannels;
        fPool = other.fPool;

        // Clear other
        other.fData = nullptr;
        other.fSize = 0;
        other.fFrames = 0;
        other.fChannels = 0;
        other.fPool = nullptr;
    }
    return *this;
}

// =====================================
// AudioBufferPool Implementation
// =====================================

// Common audio buffer sizes (in frames)
const size_t AudioBufferPool::kCommonSizes[] = {
    256, 512, 1024, 2048, 4096, 8192
};
const size_t AudioBufferPool::kCommonSizeCount = sizeof(kCommonSizes) / sizeof(kCommonSizes[0]);

// Global pool singleton
AudioBufferPool* AudioBufferPool::sGlobalPool = nullptr;
sem_id AudioBufferPool::sGlobalPoolSemaphore = -1;

AudioBufferPool::AudioBufferPool()
    : fPoolSemaphore(-1)
    , fTotalBuffers(0)
    , fAllocatedBuffers(0)
    , fAllocationCount(0)
    , fHitCount(0)
    , fMissCount(0)
{
    // Create semaphore for thread safety
    fPoolSemaphore = create_sem(1, "AudioBufferPool");
    if (fPoolSemaphore < B_OK) {
        POOL_LOG_ERROR("Failed to create semaphore");
    }

    POOL_LOG_INFO("Created with %zu common buffer sizes", kCommonSizeCount);
}

AudioBufferPool::~AudioBufferPool()
{
    Cleanup();

    if (fPoolSemaphore >= 0) {
        delete_sem(fPoolSemaphore);
    }

    POOL_LOG_INFO("Destroyed");
}

AudioBuffer AudioBufferPool::GetBuffer(size_t frames, uint32 channels)
{
    if (fPoolSemaphore < 0) {
        // Emergency fallback - direct allocation
        float* data = (float*)malloc(frames * channels * sizeof(float));
        if (data) {
            fMissCount++;
            memset(data, 0, frames * channels * sizeof(float));
            return AudioBuffer(data, frames, channels, nullptr); // No pool cleanup
        }
        return AudioBuffer(); // Invalid buffer
    }

    // Thread-safe buffer search
    if (acquire_sem(fPoolSemaphore) != B_OK) {
        fMissCount++;
        return AudioBuffer(); // Invalid buffer
    }

    BufferEntry* entry = FindAvailableBuffer(frames, channels);

    if (!entry) {
        // No suitable buffer found, create new one
        entry = CreateNewBuffer(frames, channels);
        if (entry) {
            fMissCount++;
        }
    } else {
        fHitCount++;
    }

    AudioBuffer result;
    if (entry) {
        entry->inUse = true;
        entry->lastUsed = system_time();
        fAllocatedBuffers++;

        // Clear buffer before returning
        memset(entry->data, 0, frames * channels * sizeof(float));

        result = AudioBuffer(entry->data, frames, channels, this);
    }

    release_sem(fPoolSemaphore);

    if (!result.IsValid()) {
        fMissCount++;
    }

    return result;
}

void AudioBufferPool::ReturnBuffer(float* data, size_t frames, uint32 channels)
{
    if (!data || fPoolSemaphore < 0) {
        // Direct free for emergency allocations
        if (data) {
            free(data);
        }
        return;
    }

    if (acquire_sem(fPoolSemaphore) != B_OK) {
        return;
    }

    // Find buffer entry and mark as available
    for (auto& entry : fBuffers) {
        if (entry.data == data && entry.frames == frames && entry.channels == channels) {
            if (entry.inUse) {
                entry.inUse = false;
                entry.lastUsed = system_time();
                fAllocatedBuffers--;
            }
            break;
        }
    }

    // Periodically clean up old buffers
    static bigtime_t lastCleanup = 0;
    bigtime_t now = system_time();
    if (now - lastCleanup > 5000000) { // Every 5 seconds
        CleanupOldBuffers();
        lastCleanup = now;
    }

    release_sem(fPoolSemaphore);
}

void AudioBufferPool::Warmup()
{
    POOL_LOG_INFO("Warming up with common buffer sizes...");

    if (acquire_sem(fPoolSemaphore) != B_OK) {
        return;
    }

    // Pre-allocate common buffer sizes
    for (size_t i = 0; i < kCommonSizeCount; i++) {
        size_t frames = kCommonSizes[i];

        // Create buffers for mono and stereo
        for (uint32 channels = 1; channels <= 2; channels++) {
            for (uint32 j = 0; j < kMaxBuffersPerSize; j++) {
                CreateNewBuffer(frames, channels);
            }
        }
    }

    release_sem(fPoolSemaphore);

    PoolStats stats = GetStats();
    POOL_LOG_INFO("Warmed up with %u buffers", stats.totalBuffers);
}

void AudioBufferPool::Cleanup()
{
    if (fPoolSemaphore < 0) {
        return;
    }

    if (acquire_sem(fPoolSemaphore) != B_OK) {
        return;
    }

    POOL_LOG_INFO("Cleaning up %zu buffers", fBuffers.size());

    for (auto& entry : fBuffers) {
        if (entry.data) {
            free(entry.data);
        }
    }
    fBuffers.clear();

    fTotalBuffers = 0;
    fAllocatedBuffers = 0;

    release_sem(fPoolSemaphore);
}

AudioBufferPool::PoolStats AudioBufferPool::GetStats() const
{
    PoolStats stats;
    stats.totalBuffers = fTotalBuffers.load();
    stats.allocatedBuffers = fAllocatedBuffers.load();
    stats.availableBuffers = stats.totalBuffers - stats.allocatedBuffers;
    stats.allocationCount = fAllocationCount.load();
    stats.hitCount = fHitCount.load();
    stats.missCount = fMissCount.load();
    return stats;
}

AudioBufferPool& AudioBufferPool::GetGlobalPool()
{
    // Thread-safe singleton initialization
    if (!sGlobalPool) {
        if (sGlobalPoolSemaphore < 0) {
            sGlobalPoolSemaphore = create_sem(1, "GlobalAudioBufferPool");
        }

        if (sGlobalPoolSemaphore >= 0 && acquire_sem(sGlobalPoolSemaphore) == B_OK) {
            if (!sGlobalPool) {
                sGlobalPool = new AudioBufferPool();
                sGlobalPool->Warmup();
            }
            release_sem(sGlobalPoolSemaphore);
        }
    }

    return *sGlobalPool;
}

AudioBufferPool::BufferEntry* AudioBufferPool::FindAvailableBuffer(size_t frames, uint32 channels)
{
    // Look for exact match first
    for (auto& entry : fBuffers) {
        if (!entry.inUse && entry.frames == frames && entry.channels == channels) {
            return &entry;
        }
    }

    // Look for larger buffer that can be used
    for (auto& entry : fBuffers) {
        if (!entry.inUse && entry.frames >= frames && entry.channels == channels) {
            return &entry;
        }
    }

    return nullptr;
}

AudioBufferPool::BufferEntry* AudioBufferPool::CreateNewBuffer(size_t frames, uint32 channels)
{
    // Check if we have too many buffers
    if (fBuffers.size() >= kMaxBuffersPerSize * kCommonSizeCount * 2) {
        return nullptr;
    }

    size_t sizeInBytes = frames * channels * sizeof(float);
    float* data = (float*)malloc(sizeInBytes);

    if (!data) {
        POOL_LOG_ERROR("Failed to allocate %zu bytes for %zu frames, %u channels",
               sizeInBytes, frames, channels);
        return nullptr;
    }

    BufferEntry entry;
    entry.data = data;
    entry.frames = frames;
    entry.channels = channels;
    entry.inUse = false;
    entry.lastUsed = system_time();

    fBuffers.push_back(entry);
    fTotalBuffers++;
    fAllocationCount++;

    return &fBuffers.back();
}

void AudioBufferPool::CleanupOldBuffers()
{
    bigtime_t now = system_time();
    const bigtime_t maxAge = 30000000; // 30 seconds

    auto it = fBuffers.begin();
    while (it != fBuffers.end()) {
        if (!it->inUse && (now - it->lastUsed) > maxAge) {
            free(it->data);
            it = fBuffers.erase(it);
            fTotalBuffers--;
        } else {
            ++it;
        }
    }
}

} // namespace VeniceDAW
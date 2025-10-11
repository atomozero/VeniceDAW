/*
 * AudioBufferPool.h - Zero-allocation buffer management for real-time audio
 * Provides pre-allocated buffers to avoid malloc/free in audio callbacks
 */

#ifndef AUDIO_BUFFER_POOL_H
#define AUDIO_BUFFER_POOL_H

#include <OS.h>
#include <vector>
#include <atomic>
#include <string.h>

namespace VeniceDAW {

/*
 * RAII buffer wrapper - automatically returns buffer to pool when destroyed
 */
class AudioBuffer {
public:
    AudioBuffer() : fData(nullptr), fSize(0), fFrames(0), fChannels(0), fPool(nullptr) {}
    AudioBuffer(float* data, size_t frames, uint32 channels, class AudioBufferPool* pool);
    ~AudioBuffer();

    // Move semantics for efficient transfer
    AudioBuffer(AudioBuffer&& other) noexcept;
    AudioBuffer& operator=(AudioBuffer&& other) noexcept;

    // No copy semantics (prevent accidental duplication)
    AudioBuffer(const AudioBuffer&) = delete;
    AudioBuffer& operator=(const AudioBuffer&) = delete;

    // Access methods
    float* Data() const { return fData; }
    size_t Frames() const { return fFrames; }
    uint32 Channels() const { return fChannels; }
    size_t SizeInBytes() const { return fSize; }

    // Utility methods
    void Clear() { if (fData) memset(fData, 0, fSize); }
    bool IsValid() const { return fData != nullptr; }

    // Channel access (for interleaved audio: L,R,L,R...)
    // Note: This returns pointer to first sample of specified channel
    // For full channel processing, you need to step by fChannels
    float* GetChannel(uint32 channel) const {
        if (channel >= fChannels) return nullptr;
        return fData + channel;
    }

private:
    friend class AudioBufferPool;

    float* fData;
    size_t fSize;        // Size in bytes
    size_t fFrames;      // Frames per channel
    uint32 fChannels;
    class AudioBufferPool* fPool;
};

/*
 * Thread-safe buffer pool for real-time audio processing
 */
class AudioBufferPool {
public:
    AudioBufferPool();
    ~AudioBufferPool();

    // Get buffer from pool (returns immediately, never blocks)
    AudioBuffer GetBuffer(size_t frames, uint32 channels = 2);

    // Return buffer to pool (called automatically by AudioBuffer destructor)
    void ReturnBuffer(float* data, size_t frames, uint32 channels);

    // Pool management
    void Warmup(); // Pre-allocate common buffer sizes
    void Cleanup(); // Free all buffers

    // Statistics
    struct PoolStats {
        uint32 totalBuffers;
        uint32 availableBuffers;
        uint32 allocatedBuffers;
        uint32 allocationCount;
        uint32 hitCount;
        uint32 missCount;
    };
    PoolStats GetStats() const;

    // Singleton access for global pool
    static AudioBufferPool& GetGlobalPool();

private:
    struct BufferEntry {
        float* data;
        size_t frames;
        uint32 channels;
        bool inUse;
        bigtime_t lastUsed;
    };

    // Thread-safe buffer management
    mutable sem_id fPoolSemaphore;
    std::vector<BufferEntry> fBuffers;

    // Statistics (atomic for lock-free access)
    mutable std::atomic<uint32> fTotalBuffers;
    mutable std::atomic<uint32> fAllocatedBuffers;
    mutable std::atomic<uint32> fAllocationCount;
    mutable std::atomic<uint32> fHitCount;
    mutable std::atomic<uint32> fMissCount;

    // Common buffer sizes for pre-allocation
    static const size_t kCommonSizes[];
    static const size_t kCommonSizeCount;
    static const uint32 kMaxBuffersPerSize = 8;

    // Internal methods
    BufferEntry* FindAvailableBuffer(size_t frames, uint32 channels);
    BufferEntry* CreateNewBuffer(size_t frames, uint32 channels);
    void CleanupOldBuffers();

    // Static instance for singleton
    static AudioBufferPool* sGlobalPool;
    static sem_id sGlobalPoolSemaphore;
};

/*
 * Scoped buffer allocator - RAII wrapper for temporary buffers
 */
class ScopedAudioBuffer {
public:
    ScopedAudioBuffer(size_t frames, uint32 channels = 2)
        : fBuffer(AudioBufferPool::GetGlobalPool().GetBuffer(frames, channels)) {}

    ~ScopedAudioBuffer() = default; // AudioBuffer destructor handles cleanup

    // Access operators
    AudioBuffer& operator*() { return fBuffer; }
    const AudioBuffer& operator*() const { return fBuffer; }
    AudioBuffer* operator->() { return &fBuffer; }
    const AudioBuffer* operator->() const { return &fBuffer; }

    // Direct access
    AudioBuffer& Get() { return fBuffer; }
    const AudioBuffer& Get() const { return fBuffer; }

private:
    AudioBuffer fBuffer;
};

// Convenience macros for common buffer operations
#define SCOPED_AUDIO_BUFFER(name, frames, channels) \
    ScopedAudioBuffer name(frames, channels); \
    if (!name->IsValid()) return B_NO_MEMORY;

#define GET_AUDIO_BUFFER(frames, channels) \
    AudioBufferPool::GetGlobalPool().GetBuffer(frames, channels)

} // namespace VeniceDAW

#endif
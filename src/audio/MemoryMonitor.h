/*
 * MemoryMonitor.h - Memory usage tracking and pressure detection
 * Prevents OOM conditions with large audio file sessions
 */

#ifndef MEMORY_MONITOR_H
#define MEMORY_MONITOR_H

#include <kernel/OS.h>
#include <atomic>
#include <vector>

namespace HaikuDAW {

/*
 * MemoryMonitor - Track memory usage and detect pressure conditions
 *
 * Purpose:
 * - Monitor system and process memory usage
 * - Detect low memory conditions before OOM
 * - Provide warnings to allow graceful degradation
 * - Track per-component memory allocations
 *
 * Thresholds:
 * - Normal: >30% free system memory
 * - Warning: 10-30% free (start reducing buffer sizes)
 * - Critical: <10% free (disable non-essential features)
 * - Emergency: <5% free (unload cold tracks)
 */
class MemoryMonitor {
public:
    enum MemoryPressure {
        PRESSURE_NORMAL = 0,
        PRESSURE_WARNING,
        PRESSURE_CRITICAL,
        PRESSURE_EMERGENCY
    };

    MemoryMonitor();
    ~MemoryMonitor();

    // Memory status queries
    size_t GetTotalSystemMemory() const;
    size_t GetAvailableSystemMemory() const;
    size_t GetProcessMemoryUsage() const;
    int32 GetMemoryPressurePercent() const;  // 0-100% (100 = full)

    // Pressure detection
    MemoryPressure GetCurrentPressure() const;
    bool IsLowMemory() const;
    bool IsCriticalMemory() const;

    // Component tracking
    void RegisterComponent(const char* name, size_t bytes);
    void UnregisterComponent(const char* name, size_t bytes);
    void UpdateComponent(const char* name, size_t oldBytes, size_t newBytes);

    // Statistics
    struct MemoryStats {
        size_t totalSystem;
        size_t availableSystem;
        size_t processUsage;
        size_t ringBuffers;
        size_t bufferPool;
        size_t other;
        int32 pressurePercent;
        MemoryPressure pressure;
    };
    MemoryStats GetStats() const;

    // Singleton access
    static MemoryMonitor& GetInstance();

private:
    // System memory info
    system_info fSystemInfo;
    mutable bigtime_t fLastUpdate;
    static constexpr bigtime_t UPDATE_INTERVAL = 1000000;  // 1 second

    // Component tracking
    struct ComponentMemory {
        char name[64];
        size_t bytes;
    };
    std::vector<ComponentMemory> fComponents;
    mutable sem_id fComponentLock;

    // Cached calculations
    mutable std::atomic<size_t> fTotalMemory;
    mutable std::atomic<size_t> fAvailableMemory;
    mutable std::atomic<int32> fPressurePercent;
    mutable std::atomic<MemoryPressure> fCurrentPressure;

    // Internal methods
    void _UpdateMemoryInfo() const;
    MemoryPressure _CalculatePressure(int32 percentUsed) const;

    // Singleton instance
    static MemoryMonitor* sInstance;
    static sem_id sInstanceLock;

    // Non-copyable
    MemoryMonitor(const MemoryMonitor&) = delete;
    MemoryMonitor& operator=(const MemoryMonitor&) = delete;
};

} // namespace HaikuDAW

#endif // MEMORY_MONITOR_H

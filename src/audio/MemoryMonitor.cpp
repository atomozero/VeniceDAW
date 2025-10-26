/*
 * MemoryMonitor.cpp - Memory tracking implementation
 */

#include "MemoryMonitor.h"
#include <stdio.h>
#include <string.h>

namespace HaikuDAW {

// Static members
MemoryMonitor* MemoryMonitor::sInstance = nullptr;
sem_id MemoryMonitor::sInstanceLock = -1;

MemoryMonitor::MemoryMonitor()
    : fLastUpdate(0)
    , fComponentLock(-1)
    , fTotalMemory(0)
    , fAvailableMemory(0)
    , fPressurePercent(0)
    , fCurrentPressure(PRESSURE_NORMAL)
{
    // Create component lock
    fComponentLock = create_sem(1, "MemoryMonitor components");

    // Initial update
    _UpdateMemoryInfo();

    printf("MemoryMonitor: Initialized\n");
    printf("  Total system memory: %zu MB\n", fTotalMemory.load() / (1024 * 1024));
    printf("  Available memory: %zu MB\n", fAvailableMemory.load() / (1024 * 1024));
}

MemoryMonitor::~MemoryMonitor()
{
    if (fComponentLock >= 0) {
        delete_sem(fComponentLock);
    }
}

size_t MemoryMonitor::GetTotalSystemMemory() const
{
    _UpdateMemoryInfo();
    return fTotalMemory.load();
}

size_t MemoryMonitor::GetAvailableSystemMemory() const
{
    _UpdateMemoryInfo();
    return fAvailableMemory.load();
}

size_t MemoryMonitor::GetProcessMemoryUsage() const
{
    _UpdateMemoryInfo();

    // Sum up component memory
    size_t total = 0;
    if (acquire_sem(fComponentLock) == B_OK) {
        for (const auto& comp : fComponents) {
            total += comp.bytes;
        }
        release_sem(fComponentLock);
    }

    return total;
}

int32 MemoryMonitor::GetMemoryPressurePercent() const
{
    _UpdateMemoryInfo();
    return fPressurePercent.load();
}

MemoryMonitor::MemoryPressure MemoryMonitor::GetCurrentPressure() const
{
    _UpdateMemoryInfo();
    return fCurrentPressure.load();
}

bool MemoryMonitor::IsLowMemory() const
{
    return GetCurrentPressure() >= PRESSURE_WARNING;
}

bool MemoryMonitor::IsCriticalMemory() const
{
    return GetCurrentPressure() >= PRESSURE_CRITICAL;
}

void MemoryMonitor::RegisterComponent(const char* name, size_t bytes)
{
    if (!name || bytes == 0) return;

    if (acquire_sem(fComponentLock) == B_OK) {
        ComponentMemory comp;
        strncpy(comp.name, name, sizeof(comp.name) - 1);
        comp.name[sizeof(comp.name) - 1] = '\0';
        comp.bytes = bytes;
        fComponents.push_back(comp);
        release_sem(fComponentLock);

        printf("MemoryMonitor: Registered '%s' using %zu KB\n",
               name, bytes / 1024);
    }
}

void MemoryMonitor::UnregisterComponent(const char* name, size_t bytes)
{
    if (!name) return;

    if (acquire_sem(fComponentLock) == B_OK) {
        for (auto it = fComponents.begin(); it != fComponents.end(); ++it) {
            if (strcmp(it->name, name) == 0 && it->bytes == bytes) {
                printf("MemoryMonitor: Unregistered '%s' (freed %zu KB)\n",
                       name, bytes / 1024);
                fComponents.erase(it);
                break;
            }
        }
        release_sem(fComponentLock);
    }
}

void MemoryMonitor::UpdateComponent(const char* name, size_t oldBytes, size_t newBytes)
{
    if (!name) return;

    if (acquire_sem(fComponentLock) == B_OK) {
        for (auto& comp : fComponents) {
            if (strcmp(comp.name, name) == 0 && comp.bytes == oldBytes) {
                comp.bytes = newBytes;
                printf("MemoryMonitor: Updated '%s': %zu KB -> %zu KB\n",
                       name, oldBytes / 1024, newBytes / 1024);
                break;
            }
        }
        release_sem(fComponentLock);
    }
}

MemoryMonitor::MemoryStats MemoryMonitor::GetStats() const
{
    _UpdateMemoryInfo();

    MemoryStats stats;
    stats.totalSystem = fTotalMemory.load();
    stats.availableSystem = fAvailableMemory.load();
    stats.processUsage = GetProcessMemoryUsage();
    stats.pressurePercent = fPressurePercent.load();
    stats.pressure = fCurrentPressure.load();

    // Categorize component memory
    stats.ringBuffers = 0;
    stats.bufferPool = 0;
    stats.other = 0;

    if (acquire_sem(fComponentLock) == B_OK) {
        for (const auto& comp : fComponents) {
            if (strstr(comp.name, "RingBuffer") != nullptr) {
                stats.ringBuffers += comp.bytes;
            } else if (strstr(comp.name, "BufferPool") != nullptr) {
                stats.bufferPool += comp.bytes;
            } else {
                stats.other += comp.bytes;
            }
        }
        release_sem(fComponentLock);
    }

    return stats;
}

MemoryMonitor& MemoryMonitor::GetInstance()
{
    if (!sInstance) {
        if (sInstanceLock < 0) {
            sInstanceLock = create_sem(1, "MemoryMonitor singleton");
        }

        if (acquire_sem(sInstanceLock) == B_OK) {
            if (!sInstance) {
                sInstance = new MemoryMonitor();
            }
            release_sem(sInstanceLock);
        }
    }

    return *sInstance;
}

// Private methods

void MemoryMonitor::_UpdateMemoryInfo() const
{
    // Rate-limit updates to once per second
    bigtime_t now = system_time();
    if (now - fLastUpdate < UPDATE_INTERVAL) {
        return;
    }

    fLastUpdate = now;

    // Get system memory info
    system_info info;
    if (get_system_info(&info) != B_OK) {
        return;
    }

    // Calculate totals
    size_t totalMem = (size_t)info.max_pages * B_PAGE_SIZE;
    size_t usedMem = (size_t)info.used_pages * B_PAGE_SIZE;
    size_t availableMem = totalMem - usedMem;

    // Update atomic values
    fTotalMemory = totalMem;
    fAvailableMemory = availableMem;

    // Calculate pressure percentage (0-100, where 100 = no free memory)
    int32 pressurePercent = 0;
    if (totalMem > 0) {
        pressurePercent = (int32)((usedMem * 100) / totalMem);
    }
    fPressurePercent = pressurePercent;

    // Determine pressure level
    MemoryPressure newPressure = _CalculatePressure(pressurePercent);
    MemoryPressure oldPressure = fCurrentPressure.exchange(newPressure);

    // Log pressure changes
    if (newPressure != oldPressure) {
        const char* pressureNames[] = {"NORMAL", "WARNING", "CRITICAL", "EMERGENCY"};
        printf("MemoryMonitor: Pressure changed: %s -> %s (%d%% used)\n",
               pressureNames[oldPressure], pressureNames[newPressure], pressurePercent);
    }
}

MemoryMonitor::MemoryPressure MemoryMonitor::_CalculatePressure(int32 percentUsed) const
{
    if (percentUsed >= 95) {
        return PRESSURE_EMERGENCY;  // <5% free
    } else if (percentUsed >= 90) {
        return PRESSURE_CRITICAL;   // <10% free
    } else if (percentUsed >= 70) {
        return PRESSURE_WARNING;    // <30% free
    } else {
        return PRESSURE_NORMAL;     // >=30% free
    }
}

} // namespace HaikuDAW

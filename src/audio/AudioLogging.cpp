/*
 * AudioLogging.cpp - High-performance logging implementation
 */

#include "AudioLogging.h"
#include <stdarg.h>
#include <string.h>
#include <time.h>

namespace VeniceDAW {

// Static members
sem_id AudioLogger::sLoggingSemaphore = -1;
bool AudioLogger::sInitialized = false;

void AudioLogger::InitializeIfNeeded()
{
    if (!sInitialized) {
        sLoggingSemaphore = create_sem(1, "AudioLogger");
        sInitialized = true;
    }
}

void AudioLogger::Log(LogLevel level, const char* component, const char* format, ...)
{
    if (level > AUDIO_LOG_LEVEL) {
        return;
    }

    InitializeIfNeeded();

    // Thread-safe logging
    if (sLoggingSemaphore >= 0 && acquire_sem_etc(sLoggingSemaphore, 1, B_TIMEOUT, 10000) == B_OK) {

        // High-resolution timestamp
        bigtime_t timestamp = GetHighResTimestamp();
        time_t seconds = timestamp / 1000000;
        int32 microseconds = timestamp % 1000000;

        // Format timestamp
        struct tm* timeinfo = localtime(&seconds);
        char timeStr[32];
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);

        // Thread ID for debugging
        thread_id currentThread = find_thread(NULL);

        // Print log entry with structured format
        printf("[%s.%06d] [%s] [T:%d] [%s] ",
               timeStr, (int)microseconds,
               LevelToString(level),
               (int)currentThread,
               component);

        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);

        printf("\n");
        fflush(stdout);

        release_sem(sLoggingSemaphore);
    }
}

void AudioLogger::LogRealTime(LogLevel /* level */, const char* /* component */, const char* /* format */, ...)
{
    // For real-time contexts, use minimal overhead logging
    // Only if explicitly enabled and in debug builds
    #if AUDIO_RT_LOGGING
        if (level <= LOG_LEVEL_ERROR) {
            // Only log errors in real-time context, and do it quickly
            printf("[RT] [%s] ", component);
            va_list args;
            va_start(args, format);
            vprintf(format, args);
            va_end(args);
            printf("\n");
        }
    #endif
}

void AudioLogger::LogPerformance(const char* component, const char* operation, bigtime_t duration)
{
    #if AUDIO_LOG_LEVEL >= LOG_LEVEL_DEBUG
        double durationMs = duration / 1000.0;
        Log(LOG_LEVEL_DEBUG, component, "PERF: %s took %.3f ms", operation, durationMs);
    #endif
}

void AudioLogger::LogLatency(const char* component, bigtime_t latency)
{
    #if AUDIO_LOG_LEVEL >= LOG_LEVEL_INFO
        double latencyMs = latency / 1000.0;
        Log(LOG_LEVEL_INFO, component, "LATENCY: %.3f ms", latencyMs);
    #endif
}

void AudioLogger::LogBufferStats(const char* component, size_t frames, uint32 channels, float cpu_usage)
{
    #if AUDIO_LOG_LEVEL >= LOG_LEVEL_DEBUG
        Log(LOG_LEVEL_DEBUG, component, "BUFFER: %zu frames, %u ch, CPU: %.1f%%",
            frames, channels, cpu_usage * 100.0f);
    #endif
}

const char* AudioLogger::LevelToString(LogLevel level)
{
    switch (level) {
        case LOG_LEVEL_ERROR:   return "ERROR";
        case LOG_LEVEL_WARNING: return "WARN ";
        case LOG_LEVEL_INFO:    return "INFO ";
        case LOG_LEVEL_DEBUG:   return "DEBUG";
        case LOG_LEVEL_VERBOSE: return "VERB ";
        default:                return "UNKN ";
    }
}

} // namespace VeniceDAW
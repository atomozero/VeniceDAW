/*
 * AudioLogging.h - Conditional logging system for VeniceDAW
 * High-performance logging with zero overhead in release builds
 */

#ifndef AUDIO_LOGGING_H
#define AUDIO_LOGGING_H

#include <stdio.h>
#ifdef __HAIKU__
    #include <OS.h>
#else
    // Mock definitions for cross-platform compilation
    // These types are defined in HaikuMockHeaders.h when not on Haiku
#endif

namespace VeniceDAW {

// Logging levels
enum LogLevel {
    LOG_LEVEL_OFF     = 0,
    LOG_LEVEL_ERROR   = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_INFO    = 3,
    LOG_LEVEL_DEBUG   = 4,
    LOG_LEVEL_VERBOSE = 5
};

// Compile-time logging configuration
#ifndef AUDIO_LOG_LEVEL
    #ifdef DEBUG
        #define AUDIO_LOG_LEVEL LOG_LEVEL_DEBUG
    #else
        #define AUDIO_LOG_LEVEL LOG_LEVEL_WARNING  // Only warnings/errors in release
    #endif
#endif

// Real-time audio logging flag - disable in audio callbacks for performance
#ifndef AUDIO_RT_LOGGING
    #ifdef DEBUG
        #define AUDIO_RT_LOGGING 1
    #else
        #define AUDIO_RT_LOGGING 0  // No logging in audio callbacks (release)
    #endif
#endif

// Performance-critical path detection
#define AUDIO_RT_CONTEXT 1  // In audio callback
#define AUDIO_UI_CONTEXT 0  // In UI thread

/*
 * High-performance timestamp for audio logging
 */
inline bigtime_t GetHighResTimestamp() {
    return system_time();  // Microsecond precision on Haiku
}

/*
 * Thread-safe logging with component identification
 */
class AudioLogger {
public:
    static void Log(LogLevel level, const char* component, const char* format, ...);
    static void LogRealTime(LogLevel level, const char* component, const char* format, ...);

    // Structured logging for performance analysis
    static void LogPerformance(const char* component, const char* operation, bigtime_t duration);
    static void LogLatency(const char* component, bigtime_t latency);
    static void LogBufferStats(const char* component, size_t frames, uint32_t channels, float cpu_usage);

private:
    static sem_id sLoggingSemaphore;
    static bool sInitialized;
    static void InitializeIfNeeded();
    static const char* LevelToString(LogLevel level);
};

} // namespace VeniceDAW

// =====================================
// Logging Macros - Zero overhead when disabled
// =====================================

// Basic logging macros
#if AUDIO_LOG_LEVEL >= LOG_LEVEL_ERROR
    #define AUDIO_LOG_ERROR(component, fmt, ...) \
        VeniceDAW::AudioLogger::Log(VeniceDAW::LOG_LEVEL_ERROR, component, fmt, ##__VA_ARGS__)
#else
    #define AUDIO_LOG_ERROR(component, fmt, ...) do {} while(0)
#endif

#if AUDIO_LOG_LEVEL >= LOG_LEVEL_WARNING
    #define AUDIO_LOG_WARNING(component, fmt, ...) \
        VeniceDAW::AudioLogger::Log(VeniceDAW::LOG_LEVEL_WARNING, component, fmt, ##__VA_ARGS__)
#else
    #define AUDIO_LOG_WARNING(component, fmt, ...) do {} while(0)
#endif

#if AUDIO_LOG_LEVEL >= LOG_LEVEL_INFO
    #define AUDIO_LOG_INFO(component, fmt, ...) \
        VeniceDAW::AudioLogger::Log(VeniceDAW::LOG_LEVEL_INFO, component, fmt, ##__VA_ARGS__)
#else
    #define AUDIO_LOG_INFO(component, fmt, ...) do {} while(0)
#endif

#if AUDIO_LOG_LEVEL >= LOG_LEVEL_DEBUG
    #define AUDIO_LOG_DEBUG(component, fmt, ...) \
        VeniceDAW::AudioLogger::Log(VeniceDAW::LOG_LEVEL_DEBUG, component, fmt, ##__VA_ARGS__)
#else
    #define AUDIO_LOG_DEBUG(component, fmt, ...) do {} while(0)
#endif

#if AUDIO_LOG_LEVEL >= LOG_LEVEL_VERBOSE
    #define AUDIO_LOG_VERBOSE(component, fmt, ...) \
        VeniceDAW::AudioLogger::Log(VeniceDAW::LOG_LEVEL_VERBOSE, component, fmt, ##__VA_ARGS__)
#else
    #define AUDIO_LOG_VERBOSE(component, fmt, ...) do {} while(0)
#endif

// Real-time audio logging (disabled in release for performance)
#if AUDIO_RT_LOGGING
    #define AUDIO_RT_LOG_ERROR(component, fmt, ...) \
        VeniceDAW::AudioLogger::LogRealTime(VeniceDAW::LOG_LEVEL_ERROR, component, fmt, ##__VA_ARGS__)
    #define AUDIO_RT_LOG_WARNING(component, fmt, ...) \
        VeniceDAW::AudioLogger::LogRealTime(VeniceDAW::LOG_LEVEL_WARNING, component, fmt, ##__VA_ARGS__)
    #define AUDIO_RT_LOG_DEBUG(component, fmt, ...) \
        VeniceDAW::AudioLogger::LogRealTime(VeniceDAW::LOG_LEVEL_DEBUG, component, fmt, ##__VA_ARGS__)
#else
    #define AUDIO_RT_LOG_ERROR(component, fmt, ...) do {} while(0)
    #define AUDIO_RT_LOG_WARNING(component, fmt, ...) do {} while(0)
    #define AUDIO_RT_LOG_DEBUG(component, fmt, ...) do {} while(0)
#endif

// Performance logging macros
#define AUDIO_LOG_PERF(component, operation, duration) \
    VeniceDAW::AudioLogger::LogPerformance(component, operation, duration)

#define AUDIO_LOG_LATENCY(component, latency) \
    VeniceDAW::AudioLogger::LogLatency(component, latency)

#define AUDIO_LOG_BUFFER_STATS(component, frames, channels, cpu) \
    VeniceDAW::AudioLogger::LogBufferStats(component, frames, channels, cpu)

// Scoped performance timer
#define AUDIO_PERF_TIMER(component, operation) \
    bigtime_t _perf_start = VeniceDAW::GetHighResTimestamp(); \
    struct _PerfTimerCleanup { \
        const char* comp; const char* op; bigtime_t start; \
        ~_PerfTimerCleanup() { \
            AUDIO_LOG_PERF(comp, op, VeniceDAW::GetHighResTimestamp() - start); \
        } \
    } _perf_cleanup = { component, operation, _perf_start };

// Component-specific logging shortcuts
#define RECORDER_LOG_ERROR(fmt, ...) AUDIO_LOG_ERROR("AudioRecorder", fmt, ##__VA_ARGS__)
#define RECORDER_LOG_WARNING(fmt, ...) AUDIO_LOG_WARNING("AudioRecorder", fmt, ##__VA_ARGS__)
#define RECORDER_LOG_INFO(fmt, ...) AUDIO_LOG_INFO("AudioRecorder", fmt, ##__VA_ARGS__)
#define RECORDER_LOG_DEBUG(fmt, ...) AUDIO_LOG_DEBUG("AudioRecorder", fmt, ##__VA_ARGS__)

#define PLAYER_LOG_ERROR(fmt, ...) AUDIO_LOG_ERROR("AudioPlayer", fmt, ##__VA_ARGS__)
#define PLAYER_LOG_WARNING(fmt, ...) AUDIO_LOG_WARNING("AudioPlayer", fmt, ##__VA_ARGS__)
#define PLAYER_LOG_INFO(fmt, ...) AUDIO_LOG_INFO("AudioPlayer", fmt, ##__VA_ARGS__)
#define PLAYER_LOG_DEBUG(fmt, ...) AUDIO_LOG_DEBUG("AudioPlayer", fmt, ##__VA_ARGS__)

#define ENGINE_LOG_ERROR(fmt, ...) AUDIO_LOG_ERROR("SimpleHaikuEngine", fmt, ##__VA_ARGS__)
#define ENGINE_LOG_WARNING(fmt, ...) AUDIO_LOG_WARNING("SimpleHaikuEngine", fmt, ##__VA_ARGS__)
#define ENGINE_LOG_INFO(fmt, ...) AUDIO_LOG_INFO("SimpleHaikuEngine", fmt, ##__VA_ARGS__)
#define ENGINE_LOG_DEBUG(fmt, ...) AUDIO_LOG_DEBUG("SimpleHaikuEngine", fmt, ##__VA_ARGS__)

#define POOL_LOG_ERROR(fmt, ...) AUDIO_LOG_ERROR("AudioBufferPool", fmt, ##__VA_ARGS__)
#define POOL_LOG_WARNING(fmt, ...) AUDIO_LOG_WARNING("AudioBufferPool", fmt, ##__VA_ARGS__)
#define POOL_LOG_INFO(fmt, ...) AUDIO_LOG_INFO("AudioBufferPool", fmt, ##__VA_ARGS__)
#define POOL_LOG_DEBUG(fmt, ...) AUDIO_LOG_DEBUG("AudioBufferPool", fmt, ##__VA_ARGS__)

// Real-time audio callback logging (performance-critical)
#define RECORDER_RT_LOG_ERROR(fmt, ...) AUDIO_RT_LOG_ERROR("AudioRecorder", fmt, ##__VA_ARGS__)
#define RECORDER_RT_LOG_WARNING(fmt, ...) AUDIO_RT_LOG_WARNING("AudioRecorder", fmt, ##__VA_ARGS__)
#define RECORDER_RT_LOG_DEBUG(fmt, ...) AUDIO_RT_LOG_DEBUG("AudioRecorder", fmt, ##__VA_ARGS__)

#define ENGINE_RT_LOG_ERROR(fmt, ...) AUDIO_RT_LOG_ERROR("SimpleHaikuEngine", fmt, ##__VA_ARGS__)
#define ENGINE_RT_LOG_WARNING(fmt, ...) AUDIO_RT_LOG_WARNING("SimpleHaikuEngine", fmt, ##__VA_ARGS__)
#define ENGINE_RT_LOG_DEBUG(fmt, ...) AUDIO_RT_LOG_DEBUG("SimpleHaikuEngine", fmt, ##__VA_ARGS__)

#endif
/*
 * AudioConfig.h - Audio configuration settings for HaikuDAW
 */

#ifndef AUDIO_CONFIG_H
#define AUDIO_CONFIG_H

namespace HaikuDAW {

// Audio buffer sizes (in samples/frames)
enum BufferSize {
    BUFFER_SIZE_64   = 64,    // ~1.5ms latency - Ultra low (may cause dropouts)
    BUFFER_SIZE_128  = 128,   // ~2.9ms latency - Very low
    BUFFER_SIZE_256  = 256,   // ~5.8ms latency - Low (recommended for performance)
    BUFFER_SIZE_512  = 512,   // ~11.6ms latency - Normal
    BUFFER_SIZE_1024 = 1024,  // ~23.2ms latency - Safe
    BUFFER_SIZE_2048 = 2048   // ~46.4ms latency - Very safe (default)
};

// Sample rates
enum SampleRate {
    SAMPLE_RATE_44100  = 44100,   // CD quality
    SAMPLE_RATE_48000  = 48000,   // Professional
    SAMPLE_RATE_88200  = 88200,   // High quality
    SAMPLE_RATE_96000  = 96000,   // Studio quality
    SAMPLE_RATE_192000 = 192000   // Ultra high quality
};

// Default settings for low latency
const int DEFAULT_BUFFER_SIZE = BUFFER_SIZE_256;
const int DEFAULT_SAMPLE_RATE = SAMPLE_RATE_44100;

// Calculate latency in milliseconds
inline float CalculateLatencyMs(int bufferSize, int sampleRate) {
    return (bufferSize * 1000.0f) / sampleRate;
}

// Get recommended buffer size based on system capabilities
inline int GetRecommendedBufferSize(int cpuCores) {
    if (cpuCores >= 8) {
        return BUFFER_SIZE_128;  // Powerful system
    } else if (cpuCores >= 4) {
        return BUFFER_SIZE_256;  // Good system
    } else if (cpuCores >= 2) {
        return BUFFER_SIZE_512;  // Modest system
    } else {
        return BUFFER_SIZE_1024; // Low-end system
    }
}

} // namespace HaikuDAW

#endif // AUDIO_CONFIG_H
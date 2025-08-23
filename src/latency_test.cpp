/*
 * latency_test.cpp - Real latency measurement for HaikuDAW
 */

#include <Application.h>
#include <SoundPlayer.h>
#include <OS.h>
#include <MediaDefs.h>
#include <stdio.h>
#include <math.h>

class LatencyTestApp : public BApplication {
public:
    LatencyTestApp() : BApplication("application/x-vnd.HaikuDAW-LatencyTest") {}
    
    virtual void ReadyToRun() {
        printf("\n========================================\n");
        printf("  HaikuDAW Real Latency Test\n");
        printf("========================================\n\n");
        
        TestRealLatency();
        Quit();
    }
    
private:
    static bigtime_t sCallbackTime;
    static int sCallbackCount;
    static bigtime_t sTotalCallbackTime;
    static bigtime_t sMinCallback;
    static bigtime_t sMaxCallback;
    
    static void AudioCallback(void* cookie, void* buffer, size_t size, const media_raw_audio_format& format) {
        bigtime_t now = system_time();
        
        if (sCallbackCount > 0) {  // Skip first callback
            bigtime_t delta = now - sCallbackTime;
            sTotalCallbackTime += delta;
            
            if (delta < sMinCallback) sMinCallback = delta;
            if (delta > sMaxCallback) sMaxCallback = delta;
        }
        
        sCallbackTime = now;
        sCallbackCount++;
        
        // Fill buffer with silence
        memset(buffer, 0, size);
    }
    
    void TestRealLatency() {
        // Reset stats
        sCallbackCount = 0;
        sTotalCallbackTime = 0;
        sMinCallback = 999999999;
        sMaxCallback = 0;
        
        printf("Testing different buffer sizes...\n");
        printf("=====================================\n\n");
        
        // Test different buffer sizes
        int bufferSizes[] = {128, 256, 512, 1024, 2048};
        
        for (int i = 0; i < 5; i++) {
            TestBufferSize(bufferSizes[i]);
        }
        
        printf("\n=====================================\n");
        printf("Recommendations:\n");
        printf("- For live performance: 128-256 samples\n");
        printf("- For production: 256-512 samples\n");
        printf("- For mixing: 512-1024 samples\n");
        printf("=====================================\n");
    }
    
    void TestBufferSize(int bufferFrames) {
        // Reset counters
        sCallbackCount = 0;
        sTotalCallbackTime = 0;
        sMinCallback = 999999999;
        sMaxCallback = 0;
        
        // Configure format
        media_raw_audio_format format;
        format.frame_rate = 44100.0;
        format.channel_count = 2;
        format.format = media_raw_audio_format::B_AUDIO_FLOAT;
        format.byte_order = B_MEDIA_LITTLE_ENDIAN;
        format.buffer_size = bufferFrames * sizeof(float) * 2;
        
        // Create player
        BSoundPlayer* player = new BSoundPlayer(&format, "LatencyTest", AudioCallback, nullptr, this);
        
        if (player->InitCheck() != B_OK) {
            printf("Failed to create player with %d samples\n", bufferFrames);
            delete player;
            return;
        }
        
        // Start playback
        player->Start();
        player->SetHasData(true);
        
        // Run for 2 seconds
        snooze(2000000);
        
        // Stop and cleanup
        player->Stop();
        delete player;
        
        // Calculate results
        if (sCallbackCount > 1) {
            float avgCallbackInterval = sTotalCallbackTime / (float)(sCallbackCount - 1) / 1000.0f; // ms
            float theoreticalLatency = (bufferFrames * 1000.0f) / 44100.0f;
            float measuredLatency = avgCallbackInterval;
            
            printf("Buffer: %4d samples\n", bufferFrames);
            printf("  Theoretical latency:  %6.2f ms\n", theoreticalLatency);
            printf("  Measured interval:    %6.2f ms (avg)\n", avgCallbackInterval);
            printf("  Min/Max interval:     %6.2f / %.2f ms\n", 
                   sMinCallback / 1000.0f, sMaxCallback / 1000.0f);
            printf("  Callbacks received:   %d\n", sCallbackCount);
            printf("  Jitter:              %6.2f ms\n", (sMaxCallback - sMinCallback) / 1000.0f);
            
            if (measuredLatency < 10.0f) {
                printf("  Status:              ✅ Real-time capable\n");
            } else if (measuredLatency < 20.0f) {
                printf("  Status:              ⚠️  Borderline\n");
            } else {
                printf("  Status:              ❌ Too high for real-time\n");
            }
            
            // Check for dropouts
            float expectedCallbacks = 2000.0f / theoreticalLatency;
            float callbackRatio = sCallbackCount / expectedCallbacks;
            if (callbackRatio < 0.95f) {
                printf("  ⚠️  WARNING: Possible audio dropouts detected!\n");
            }
        }
        printf("\n");
    }
};

// Static member initialization
bigtime_t LatencyTestApp::sCallbackTime = 0;
int LatencyTestApp::sCallbackCount = 0;
bigtime_t LatencyTestApp::sTotalCallbackTime = 0;
bigtime_t LatencyTestApp::sMinCallback = 999999999;
bigtime_t LatencyTestApp::sMaxCallback = 0;

int main() {
    LatencyTestApp app;
    app.Run();
    return 0;
}
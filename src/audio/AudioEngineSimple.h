/*
 * AudioEngineSimple.h - Simplified audio engine for cross-platform testing
 * 
 * This version compiles on any system for logic testing
 * Will be easily portable to real Haiku BMediaKit
 */

#ifndef HAIKU_DAW_AUDIO_ENGINE_SIMPLE_H
#define HAIKU_DAW_AUDIO_ENGINE_SIMPLE_H

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>

namespace HaikuDAW {

/*
 * Simplified audio engine for testing
 * Core logic that will translate directly to BMediaKit
 */
class AudioEngineSimple {
public:
    AudioEngineSimple(const char* name);
    ~AudioEngineSimple();
    
    // Engine control
    bool            Start();
    bool            Stop();
    bool            IsRunning() const;
    
    // Track management
    int32_t         AddTrack(const std::string& name);
    bool            RemoveTrack(int32_t trackId);
    int32_t         GetTrackCount() const;
    
    // Audio parameters
    void            SetMasterVolume(float volume);
    float           GetMasterVolume() const;
    void            SetSampleRate(float rate);
    float           GetSampleRate() const;
    void            SetBufferSize(int32_t frames);
    int32_t         GetBufferSize() const;
    
    // Performance monitoring
    float           GetCPUUsage() const;
    int32_t         GetLatency() const;
    int64_t         GetProcessingTime() const;
    
    // Track control
    bool            SetTrackVolume(int32_t trackId, float volume);
    bool            SetTrackMute(int32_t trackId, bool muted);
    bool            SetTrackSolo(int32_t trackId, bool solo);
    bool            SetTrackPosition(int32_t trackId, float x, float y, float z);
    
    // Status info
    std::string     GetStatusString() const;
    
private:
    // Processing
    void            ProcessingThread();
    void            ProcessAudioBuffer();
    
    // Track data
    struct Track {
        int32_t     id;
        std::string name;
        float       volume;
        bool        muted;
        bool        solo;
        float       posX, posY, posZ;
        
        Track(int32_t _id, const std::string& _name) 
            : id(_id), name(_name), volume(1.0f), muted(false), solo(false)
            , posX(0.0f), posY(0.0f), posZ(0.0f) {}
    };
    
    // State
    std::string                 fName;
    std::atomic<bool>          fRunning;
    std::atomic<float>         fMasterVolume;
    float                      fSampleRate;
    int32_t                    fBufferSize;
    
    // Tracks
    std::vector<std::unique_ptr<Track>> fTracks;
    mutable std::mutex         fTrackMutex;
    int32_t                    fNextTrackId;
    
    // Performance
    std::atomic<float>         fCPUUsage;
    std::atomic<int32_t>       fLatency;
    std::atomic<int64_t>       fLastProcessTime;
    
    // Threading
    std::unique_ptr<std::thread> fProcessingThread;
    
    // Constants
    static constexpr int32_t   kMaxTracks = 64;
    static constexpr int32_t   kDefaultBufferSize = 512;
    static constexpr float     kDefaultSampleRate = 44100.0f;
};

/*
 * Simple audio buffer for testing
 */
class AudioBufferSimple {
public:
    AudioBufferSimple(int32_t channels, int32_t frames);
    ~AudioBufferSimple();
    
    float*          GetChannel(int32_t channel);
    const float*    GetChannel(int32_t channel) const;
    int32_t         GetChannelCount() const { return fChannels; }
    int32_t         GetFrameCount() const { return fFrames; }
    
    void            Clear();
    void            GenerateTestTone(float frequency, float phase);
    
private:
    int32_t         fChannels;
    int32_t         fFrames;
    std::vector<std::vector<float>> fData;
};

} // namespace HaikuDAW

#endif // HAIKU_DAW_AUDIO_ENGINE_SIMPLE_H
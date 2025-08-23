/*
 * AudioEngine.h - Modern audio engine for HaikuDAW
 * 
 * Core audio processing using Haiku's BMediaKit
 * Clean architecture, no legacy code
 */

#ifndef HAIKU_DAW_AUDIO_ENGINE_H
#define HAIKU_DAW_AUDIO_ENGINE_H

#include <media/MediaNode.h>
#include <media/MediaRoster.h>
#include <media/TimeSource.h>
#include <media/BufferGroup.h>
#include <media/MediaEventLooper.h>
#include <support/SupportDefs.h>
#include <app/Looper.h>
#include <app/MessageRunner.h>
#include <support/Locker.h>
#include <atomic>
#include <memory>
#include <vector>

namespace HaikuDAW {

// Forward declarations
class AudioTrack;
class AudioBuffer;
class PluginHost;

/*
 * Modern audio engine using BMediaKit
 * 
 * Features:
 * - Real-time, low-latency processing
 * - Lock-free ring buffers
 * - Multi-track mixing
 * - Plugin support
 * - Hardware acceleration where available
 */
class AudioEngine : public BMediaNode, public BMediaEventLooper {
public:
    AudioEngine(const char* name);
    virtual ~AudioEngine();
    
    // BMediaNode interface
    virtual BMediaAddOn* AddOn(int32* internalID) const;
    virtual status_t HandleMessage(int32 message, const void* data, size_t size);
    
    // Audio control
    status_t        Start();
    status_t        Stop();
    bool            IsRunning() const;
    
    // Track management
    status_t        AddTrack(std::shared_ptr<AudioTrack> track);
    status_t        RemoveTrack(int32 trackId);
    AudioTrack*     GetTrack(int32 trackId);
    int32           GetTrackCount() const;
    
    // Mixing parameters
    void            SetMasterVolume(float volume);
    float           GetMasterVolume() const;
    void            SetSampleRate(float rate);
    float           GetSampleRate() const;
    void            SetBufferSize(int32 frames);
    int32           GetBufferSize() const;
    
    // Plugin management
    status_t        LoadPlugin(const char* path);
    status_t        UnloadPlugin(int32 pluginId);
    
    // Performance monitoring
    float           GetCPUUsage() const;
    int32           GetLatency() const;
    bigtime_t       GetProcessingTime() const;
    
protected:
    // BMediaEventLooper interface
    virtual void    HandleEvent(const media_timed_event* event,
                               bigtime_t lateness,
                               bool realTimeEvent = false);
    
private:
    // Core processing
    void            ProcessAudioBuffer();
    void            MixTracks(AudioBuffer* output);
    status_t        InitializeMediaKit();
    
    // Thread management
    static int32    ProcessingThread(void* data);
    void            ProcessingLoop();
    
    // Member variables
    BMediaRoster*               fMediaRoster;
    BTimeSource*                fTimeSource;
    BBufferGroup*               fBufferGroup;
    
    // Audio state
    std::atomic<bool>           fIsRunning;
    std::atomic<float>          fMasterVolume;
    float                       fSampleRate;
    int32                       fBufferSize;
    
    // Tracks
    std::vector<std::shared_ptr<AudioTrack>> fTracks;
    mutable BLocker             fTrackLock;
    
    // Plugin host
    std::unique_ptr<PluginHost> fPluginHost;
    
    // Performance monitoring
    std::atomic<float>          fCPUUsage;
    std::atomic<int32>          fLatency;
    bigtime_t                   fLastProcessTime;
    
    // Message runner for meter updates
    BMessageRunner*             fMeterRunner;
    
    // Constants
    static const int32          kMaxTracks = 64;
    static const int32          kDefaultBufferSize = 512;
    static const float          kDefaultSampleRate = 48000.0f;
};

/*
 * AudioTrack - Individual audio track
 */
class AudioTrack {
public:
    AudioTrack(int32 id, const char* name);
    ~AudioTrack();
    
    // Track properties
    int32           GetId() const { return fId; }
    const char*     GetName() const { return fName.String(); }
    void            SetName(const char* name);
    
    // Audio control
    void            SetVolume(float volume);
    float           GetVolume() const;
    void            SetPan(float pan);
    float           GetPan() const;
    void            SetMute(bool mute);
    bool            IsMuted() const;
    void            SetSolo(bool solo);
    bool            IsSolo() const;
    
    // 3D position (for spatial mixing)
    void            SetPosition(float x, float y, float z);
    void            GetPosition(float& x, float& y, float& z) const;
    
    // Processing
    status_t        ProcessBuffer(AudioBuffer* input, AudioBuffer* output);
    
private:
    int32           fId;
    BString         fName;
    
    std::atomic<float> fVolume;
    std::atomic<float> fPan;
    std::atomic<bool>  fMuted;
    std::atomic<bool>  fSolo;
    
    // 3D position
    std::atomic<float> fPosX;
    std::atomic<float> fPosY;
    std::atomic<float> fPosZ;
    
    // Effects chain
    std::vector<int32> fEffectIds;
};

/*
 * AudioBuffer - Lock-free audio buffer
 */
class AudioBuffer {
public:
    AudioBuffer(int32 channels, int32 frames);
    ~AudioBuffer();
    
    // Buffer access
    float*          GetChannel(int32 channel);
    const float*    GetChannel(int32 channel) const;
    int32           GetChannelCount() const { return fChannels; }
    int32           GetFrameCount() const { return fFrames; }
    
    // Operations
    void            Clear();
    void            CopyFrom(const AudioBuffer* source);
    void            MixFrom(const AudioBuffer* source, float gain);
    
private:
    int32           fChannels;
    int32           fFrames;
    float**         fData;
};

} // namespace HaikuDAW

#endif // HAIKU_DAW_AUDIO_ENGINE_H
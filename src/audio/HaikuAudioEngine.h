/*
 * HaikuAudioEngine.h - 100% Native Haiku audio engine using BMediaKit
 * 
 * This is the real implementation for Haiku OS
 * Clean, modern, and fully native to Haiku
 */

#ifndef HAIKU_DAW_HAIKU_AUDIO_ENGINE_H
#define HAIKU_DAW_HAIKU_AUDIO_ENGINE_H

// 100% Native Haiku headers - compile ONLY on Haiku OS
#include <media/MediaNode.h>
#include <media/MediaEventLooper.h>
#include <media/BufferProducer.h>
#include <media/BufferConsumer.h>
#include <media/MediaRoster.h>
#include <media/TimeSource.h>
#include <media/BufferGroup.h>
#include <media/SoundPlayer.h>
#include <media/MediaDefs.h>
#include <support/Locker.h>
#include <support/String.h>
#include <app/MessageRunner.h>
#include <kernel/OS.h>
#include <vector>
#include <memory>

// Forward declarations for Haiku classes
class BMediaFile;
class BMediaTrack;

namespace HaikuDAW {

// Forward declarations
class HaikuAudioTrack;

/*
 * Native Haiku audio engine using BMediaKit
 * 
 * This replaces the cross-platform version with 100% native Haiku APIs:
 * - BMediaNode for media graph integration
 * - BMediaEventLooper for real-time processing  
 * - BSoundPlayer for audio output
 * - BBufferGroup for efficient buffer management
 * - Native Haiku threading with BLooper
 */
class HaikuAudioEngine : public BMediaEventLooper, public BBufferProducer {
public:
    HaikuAudioEngine(const char* name = "HaikuDAW Engine");
    virtual ~HaikuAudioEngine();
    
    // === BMediaNode Interface ===
    virtual BMediaAddOn* AddOn(int32* internalID) const;
    virtual status_t     HandleMessage(int32 message, const void* data, size_t size);
    
    // === BMediaEventLooper Interface ===
    virtual void         HandleEvent(const media_timed_event* event,
                                   bigtime_t lateness,
                                   bool realTimeEvent = false);
    virtual void         NodeRegistered();
    virtual status_t     RequestCompleted(const media_request_info& info);
    
    // === BBufferProducer Interface ===
    virtual status_t     FormatSuggestionRequested(media_type type, int32 quality,
                                                  media_format* format);
    virtual status_t     FormatProposal(const media_source& output, media_format* format);
    virtual status_t     FormatChangeRequested(const media_source& source,
                                             const media_destination& destination,
                                             media_format* io_format,
                                             int32* _deprecated_);
    virtual status_t     GetNextOutput(int32* cookie, media_output* out_output);
    virtual status_t     DisposeOutputCookie(int32 cookie);
    virtual status_t     SetBufferGroup(const media_source& for_source,
                                      BBufferGroup* group);
    virtual status_t     PrepareToConnect(const media_source& what,
                                        const media_destination& where,
                                        media_format* format,
                                        media_source* out_source,
                                        char* out_name);
    virtual void         Connect(status_t error, const media_source& source,
                               const media_destination& destination,
                               const media_format& format, char* io_name);
    virtual void         Disconnect(const media_source& what,
                                  const media_destination& where);
    virtual void         LateNoticeReceived(const media_source& what,
                                          bigtime_t how_much, bigtime_t performance_time);
    virtual void         EnableOutput(const media_source& what, bool enabled,
                                    int32* _deprecated_);
    virtual status_t     GetLatency(bigtime_t* out_latency);
    
    // === Audio Engine Control ===
    status_t            StartEngine();
    status_t            StopEngine();
    bool                IsRunning() const;
    
    // === Track Management ===
    status_t            AddTrack(std::shared_ptr<HaikuAudioTrack> track);
    status_t            RemoveTrack(int32 trackId);
    HaikuAudioTrack*    GetTrack(int32 trackId);
    int32               GetTrackCount() const;
    
    // === Audio Parameters ===
    void                SetMasterVolume(float volume);
    float               GetMasterVolume() const;
    void                SetSampleRate(float rate);
    float               GetSampleRate() const;
    void                SetBufferSize(int32 frames);
    int32               GetBufferSize() const;
    
    // === Performance Monitoring ===
    float               GetCPUUsage() const;
    bigtime_t           GetLatency() const;
    bigtime_t           GetProcessingTime() const;
    int32               GetDroppedFrames() const;
    
    // === Status Info ===
    BString             GetStatusString() const;
    media_format        GetFormat() const { return fFormat; }
    
protected:
    // === Audio Processing ===
    void                ProcessBuffer(BBuffer* buffer);
    void                ProcessAudioCallback(void* buffer, size_t size, const media_raw_audio_format& format);
    void                MixTracks(float* outputLeft, float* outputRight, int32 frameCount);
    
    // === Buffer Management ===
    status_t            CreateBufferGroup();
    void                DeleteBufferGroup();
    
private:
    // === Media Kit Objects ===
    BMediaRoster*       fMediaRoster;
    BTimeSource*        fTimeSource;
    BBufferGroup*       fBufferGroup;
    BSoundPlayer*       fSoundPlayer;
    media_output        fOutput;
    media_format        fFormat;
    bool                fOutputEnabled;
    
    // === Engine State ===
    BLocker             fEngineLock;
    bool                fEngineRunning;
    float               fMasterVolume;
    float               fSampleRate;
    int32               fBufferSize;
    int32               fChannelCount;
    
    // === Track Management ===
    std::vector<std::shared_ptr<HaikuAudioTrack>> fTracks;
    mutable BLocker     fTrackLock;
    int32               fNextTrackId;
    
    // === Performance Monitoring ===
    float               fCPUUsage;
    bigtime_t           fLatency;
    bigtime_t           fLastProcessTime;
    int32               fDroppedFrames;
    
    // === Meter Updates ===
    BMessageRunner*     fMeterRunner;
    
    // === Constants ===
    static constexpr int32  kMaxTracks = 64;
    static constexpr int32  kDefaultBufferSize = 512;
    static constexpr float  kDefaultSampleRate = 44100.0f;
    static constexpr int32  kDefaultChannels = 2;
    
    // === Message Constants ===
    enum {
        MSG_UPDATE_METERS = 'metr'
    };
};

/*
 * Native Haiku audio track
 * 
 * Each track represents a single audio source with:
 * - Volume, pan, mute, solo controls
 * - 3D spatial positioning
 * - Effects chain integration
 * - Real-time level monitoring
 */
class HaikuAudioTrack {
public:
    HaikuAudioTrack(int32 id, const char* name);
    ~HaikuAudioTrack();
    
    // === Track Properties ===
    int32               GetId() const { return fId; }
    const char*         GetName() const { return fName.String(); }
    void                SetName(const char* name);
    
    // === Audio Control ===
    void                SetVolume(float volume);
    float               GetVolume() const;
    void                SetPan(float pan);
    float               GetPan() const;
    void                SetMute(bool mute);
    bool                IsMuted() const;
    void                SetSolo(bool solo);
    bool                IsSolo() const;
    
    // === 3D Spatial Audio ===
    void                SetPosition(float x, float y, float z);
    void                GetPosition(float& x, float& y, float& z) const;
    void                SetDistance(float distance);
    float               GetDistance() const;
    
    // === Level Monitoring ===
    float               GetPeakLevel() const;
    float               GetRMSLevel() const;
    void                UpdateLevels(const float* samples, int32 frameCount);
    
    // === Audio Processing ===
    status_t            ProcessAudio(float* inputLeft, float* inputRight,
                                   float* outputLeft, float* outputRight,
                                   int32 frameCount);
    
    // === File Playback (for demo) ===
    status_t            LoadAudioFile(const char* path);
    void                SetPlaybackPosition(bigtime_t position);
    bigtime_t           GetPlaybackPosition() const;
    bool                IsPlaying() const;
    void                StartPlayback();
    void                StopPlayback();
    
private:
    // === Track Data ===
    int32               fId;
    BString             fName;
    
    // === Audio Controls ===
    BLocker             fControlLock;
    float               fVolume;
    float               fPan;
    bool                fMuted;
    bool                fSolo;
    
    // === 3D Position ===
    float               fPosX, fPosY, fPosZ;
    float               fDistance;
    
    // === Level Monitoring ===
    float               fPeakLevel;
    float               fRMSLevel;
    bigtime_t           fLastLevelUpdate;
    
    // === File Playback ===
    BMediaFile*         fMediaFile;     // Native Haiku media file
    BMediaTrack*        fMediaTrack;    // Native Haiku media track
    media_format        fFileFormat;
    bigtime_t           fPlaybackPosition;
    bool                fIsPlaying;
    
    // === Processing Buffers ===
    float*              fProcessingBuffer;
    int32               fBufferSize;
    
    // === 3D Audio Processing ===
    void                Apply3DPositioning(float* left, float* right, int32 frameCount);
    void                ApplyVolumeAndPan(float* left, float* right, int32 frameCount);
};

/*
 * Native Haiku audio buffer wrapper
 * 
 * Provides a clean interface to BBuffer for audio processing
 */
class HaikuAudioBuffer {
public:
    HaikuAudioBuffer(BBuffer* buffer);
    ~HaikuAudioBuffer();
    
    // === Buffer Access ===
    float*              GetChannelData(int32 channel);
    const float*        GetChannelData(int32 channel) const;
    int32               GetChannelCount() const;
    int32               GetFrameCount() const;
    
    // === Buffer Operations ===
    void                Clear();
    void                CopyFrom(const HaikuAudioBuffer* source);
    void                MixFrom(const HaikuAudioBuffer* source, float gain);
    
    // === BBuffer Integration ===
    BBuffer*            GetBBuffer() const { return fBuffer; }
    bigtime_t           StartTime() const;
    void                SetStartTime(bigtime_t time);
    
private:
    BBuffer*            fBuffer;
    float*              fData;
    int32               fChannelCount;
    int32               fFrameCount;
    size_t              fBufferSize;
};

} // namespace HaikuDAW

#endif // HAIKU_DAW_HAIKU_AUDIO_ENGINE_H
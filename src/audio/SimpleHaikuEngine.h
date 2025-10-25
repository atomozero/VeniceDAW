/*
 * SimpleHaikuEngine.h - Veramente semplice per Haiku nativo
 */

#ifndef SIMPLE_HAIKU_ENGINE_H
#define SIMPLE_HAIKU_ENGINE_H

#include <media/SoundPlayer.h>
#include <media/MediaFile.h>
#include <media/MediaTrack.h>
#include <storage/Entry.h>
#include <storage/Path.h>
#include <support/String.h>
#include <vector>
#include <atomic>

// Forward declaration to avoid circular includes
namespace VeniceDAW {
    class RecordingSession;
}

namespace HaikuDAW {

class SimpleTrack {
public:
    SimpleTrack(int id, const char* name);
    ~SimpleTrack();
    
    int GetId() const { return fId; }
    const char* GetName() const { return fName.String(); }
    void SetName(const char* name) { fName.SetTo(name); }
    
    void SetVolume(float volume) { fVolume = volume; }
    float GetVolume() const { return fVolume; }
    
    void SetPan(float pan) { fPan = pan; }
    float GetPan() const { return fPan; }
    
    void SetPosition(float x, float y, float z) { fX = x; fY = y; fZ = z; }
    void GetPosition(float& x, float& y, float& z) const { x = fX; y = fY; z = fZ; }

    void SetMute(bool mute) { fMuted = mute; }
    bool IsMuted() const { return fMuted; }

    void SetSolo(bool solo) { fSolo = solo; }
    bool IsSolo() const { return fSolo; }

    // Audio levels (updated in real-time)
    float GetPeakLevel() const { return fPeakLevel; }
    float GetRMSLevel() const { return fRMSLevel; }
    void UpdateLevels(float peak, float rms) { fPeakLevel = peak; fRMSLevel = rms; }

    // Phase access (for audio engine)
    float& GetPhase() { return fPhase; }

    // Test signal type
    enum SignalType {
        kSignalSine,
        kSignalSquare,
        kSignalSaw,
        kSignalWhiteNoise,
        kSignalPinkNoise
    };

    void SetSignalType(SignalType type) { fSignalType = type; }
    SignalType GetSignalType() const { return fSignalType; }

    void SetFrequency(float freq) { fFrequency = freq; }
    float GetFrequency() const { return fFrequency; }

    // For pink noise generator
    float& GetPinkNoiseState(int index) { return fPinkNoiseState[index]; }
    float GetPinkNoiseMax() { return fPinkNoiseMax; }
    void SetPinkNoiseMax(float max) { fPinkNoiseMax = max; }
    
    // Audio file loading and playback
    status_t LoadAudioFile(const char* path);
    status_t LoadAudioFile(const entry_ref& ref);
    void UnloadFile();
    bool HasFile() const { return fFileLoaded; }
    bool HasAudioFile() const { return fFileLoaded; }  // Alias for clarity
    const char* GetFilePath() const { return fFilePath.String(); }
    
    // Debug and fallback methods
    status_t LoadAudioFileAlternative(const entry_ref& ref);
    
    // File playback state
    void SetPlaybackPosition(int64 frame) { fPlaybackFrame = frame; }
    int64 GetPlaybackPosition() const { return fPlaybackFrame; }
    int64 GetFileDuration() const { return fFileDuration; }
    float GetFileSampleRate() const { return fFileSampleRate; }
    
    // File data access (for audio engine)
    status_t ReadFileData(float* buffer, int32 frameCount, float sampleRate);

private:
    int fId;
    BString fName;
    float fVolume;
    float fPan;
    float fX, fY, fZ;
    bool fMuted;
    bool fSolo;
    float fPeakLevel, fRMSLevel;  // Real-time audio levels
    float fPhase;  // Individual phase for each track
    SignalType fSignalType;  // Type of test signal
    float fFrequency;  // Frequency for test signal
    float fPinkNoiseState[7];  // State for pink noise generator
    float fPinkNoiseMax;  // Maximum value for pink noise normalization
    
    // Audio file playback members
    BMediaFile* fMediaFile;
    BMediaTrack* fMediaTrack;
    media_format fFileFormat;
    float* fFileBuffer;
    int32 fFileBufferSize;
    int64 fPlaybackFrame;
    int64 fFileDuration;
    float fFileSampleRate;
    bool fFileLoaded;
    BString fFilePath;
};

class SimpleHaikuEngine {
public:
    SimpleHaikuEngine();
    ~SimpleHaikuEngine();
    
    status_t Start();
    status_t Stop();
    bool IsRunning() const { return fRunning; }
    
    // Playback controls  
    void ResetAllTracks();     // Reset playback position of all loaded files
    void SetMasterVolume(float volume) { fMasterVolume = volume; }
    float GetMasterVolume() const { return fMasterVolume; }
    
    status_t AddTrack(SimpleTrack* track);
    status_t RemoveTrack(int index);  // Remove track at index
    int GetTrackCount() const { return fTracks.size(); }
    
    BString GetStatus() const;
    
    // Access tracks for level meter updates
    SimpleTrack* GetTrack(int index) const { 
        return (index >= 0 && (size_t)index < fTracks.size()) ? fTracks[index] : nullptr;
    }
    
    // Solo management - only one track can be solo at a time
    void SetTrackSolo(int trackIndex, bool solo);
    int GetSoloTrack() const { return fSoloTrack; }  // -1 if no solo
    bool HasSoloTrack() const { return fSoloTrack >= 0; }
    
    // Master level monitoring
    float GetMasterPeakLeft() const { return fMasterPeakLeft; }
    float GetMasterPeakRight() const { return fMasterPeakRight; }
    float GetMasterRMSLeft() const { return fMasterRMSLeft; }
    float GetMasterRMSRight() const { return fMasterRMSRight; }
    
    // Demo scene creation
    void CreateDemoScene();

    // Empty track creation for manual loading
    status_t CreateEmptyTrack(const char* name);

    // Audio recording support
    ::VeniceDAW::RecordingSession* GetRecordingSession() const { return fRecordingSession; }
    status_t StartRecording(int32 trackIndex, const char* filename = nullptr);
    status_t StopRecording(int32 trackIndex);
    bool IsRecording(int32 trackIndex = -1) const;  // -1 checks if any track is recording
    
    // Audio file loading
    status_t LoadAudioFileAsTrack(const entry_ref& ref);
    status_t LoadAudioFileAsTrack(const char* path);

    // Live monitoring support
    status_t CreateMonitoringTrack(const char* name = "Input Monitor");
    status_t FeedMonitoringAudio(const void* data, size_t size, const media_raw_audio_format& format);
    int32 GetMonitoringTrackIndex() const { return fMonitoringTrackIndex; }

private:
    static void _AudioCallback(void* cookie, void* buffer, size_t size, const media_raw_audio_format& format);
    void _ProcessAudio(float* buffer, size_t frameCount);
    float _GenerateTestSignal(SimpleTrack* track, float sampleRate);
    void _SyncAudioTracks();  // Sync UI track list to RT-safe audio track list
    
    BSoundPlayer* fSoundPlayer;
    std::vector<SimpleTrack*> fTracks;  // UI thread track list (for modifications)
    std::atomic<std::vector<SimpleTrack*>*> fAudioTracks;  // RT thread track list (atomic pointer swap)
    std::vector<SimpleTrack*> fTrackBuffer1;  // Double-buffer for lock-free updates
    std::vector<SimpleTrack*> fTrackBuffer2;
    std::atomic<bool> fRunning;
    float fMasterVolume;
    std::atomic<int> fSoloTrack;  // Index of solo track, -1 if none (atomic for thread safety)
    
    // Master level monitoring
    float fMasterPeakLeft;
    float fMasterPeakRight;
    float fMasterRMSLeft;
    float fMasterRMSRight;

    // Recording support
    ::VeniceDAW::RecordingSession* fRecordingSession;

    // Live monitoring support
    int32 fMonitoringTrackIndex;  // Index of monitoring track, -1 if none
    std::vector<float> fMonitoringBuffer;  // Temporary buffer for incoming audio

    // RT-safe buffer pool (pre-allocated to avoid allocations in audio callback)
    static constexpr size_t MAX_BUFFER_FRAMES = 4096;
    std::vector<float> fMixBuffer;  // Pre-allocated mix buffer for audio processing
};

} // namespace HaikuDAW

#endif
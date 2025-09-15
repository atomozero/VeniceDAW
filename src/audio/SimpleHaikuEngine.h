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

namespace HaikuDAW {

class SimpleTrack {
public:
    SimpleTrack(int id, const char* name);
    ~SimpleTrack();
    
    int GetId() const { return fId; }
    const char* GetName() const { return fName.String(); }
    
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
        SIGNAL_SINE,
        SIGNAL_SQUARE,
        SIGNAL_SAW,
        SIGNAL_WHITE_NOISE,
        SIGNAL_PINK_NOISE
    };
    
    void SetSignalType(SignalType type) { fSignalType = type; }
    SignalType GetSignalType() const { return fSignalType; }
    
    void SetFrequency(float freq) { fFrequency = freq; }
    float GetFrequency() const { return fFrequency; }
    
    // For pink noise generator
    float& GetPinkNoiseState(int index) { return fPinkNoiseState[index]; }
    
    // Audio file loading and playback
    status_t LoadAudioFile(const char* path);
    status_t LoadAudioFile(const entry_ref& ref);
    void UnloadFile();
    bool HasFile() const { return fFileLoaded; }
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
    
    status_t AddTrack(SimpleTrack* track);
    status_t RemoveTrack(int index);  // Remove track at index
    int GetTrackCount() const { return fTracks.size(); }
    
    void SetMasterVolume(float volume) { fMasterVolume = volume; }
    float GetMasterVolume() const { return fMasterVolume; }
    
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
    
    // Audio file loading
    status_t LoadAudioFileAsTrack(const entry_ref& ref);
    status_t LoadAudioFileAsTrack(const char* path);

private:
    static void AudioCallback(void* cookie, void* buffer, size_t size, const media_raw_audio_format& format);
    void ProcessAudio(float* buffer, size_t frameCount);
    float GenerateTestSignal(SimpleTrack* track, float sampleRate);
    
    BSoundPlayer* fSoundPlayer;
    std::vector<SimpleTrack*> fTracks;
    bool fRunning;
    float fMasterVolume;
    int fSoloTrack;  // Index of solo track, -1 if none
    
    // Master level monitoring
    float fMasterPeakLeft;
    float fMasterPeakRight; 
    float fMasterRMSLeft;
    float fMasterRMSRight;
};

} // namespace HaikuDAW

#endif
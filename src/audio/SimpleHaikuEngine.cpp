/*
 * SimpleHaikuEngine.cpp - Engine VERAMENTE semplice per Haiku
 */

#include "SimpleHaikuEngine.h"
// #include "AudioRecorder.h"  // Temporarily disabled
#include "AudioConfig.h"
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>  // For rand()
#include <storage/File.h>
#include <media/MediaFormats.h>

namespace HaikuDAW {

// === SimpleTrack ===

SimpleTrack::SimpleTrack(int id, const char* name)
    : fId(id), fName(name), fVolume(1.0f), fPan(0.0f), fX(0), fY(0), fZ(0), fMuted(false), fSolo(false),
      fPeakLevel(0.0f), fRMSLevel(0.0f), fPhase(0.0f), fSignalType(kSignalSine), fFrequency(440.0f),
      fMediaFile(nullptr), fMediaTrack(nullptr), fFileBuffer(nullptr), fFileBufferSize(0),
      fPlaybackFrame(0), fFileDuration(0), fFileSampleRate(44100.0f), fFileLoaded(false), fPinkNoiseMax(1.0f)
{
    // Track created
    // Initialize pink noise state
    for (int i = 0; i < 7; i++) {
        fPinkNoiseState[i] = 0.0f;
    }
    // Initialize file format
    fFileFormat = media_format();
}

SimpleTrack::~SimpleTrack()
{
    UnloadFile();
    printf("SimpleTrack: Destroyed '%s'\n", fName.String());
}

void SimpleTrack::UnloadFile()
{
    if (fFileLoaded) {
        printf("SimpleTrack: Unloading file '%s'\n", fFilePath.String());
        
        if (fMediaFile && fMediaTrack) {
            fMediaFile->ReleaseTrack(fMediaTrack);
            fMediaTrack = nullptr;
        }
        
        delete fMediaFile;
        fMediaFile = nullptr;
        
        delete[] fFileBuffer;
        fFileBuffer = nullptr;
        fFileBufferSize = 0;
        
        fPlaybackFrame = 0;
        fFileDuration = 0;
        fFileLoaded = false;
        fFilePath.SetTo("");
    }
}

status_t SimpleTrack::LoadAudioFile(const char* path)
{
    entry_ref ref;
    status_t status = get_ref_for_path(path, &ref);
    if (status != B_OK) {
        printf("SimpleTrack: Failed to get ref for path '%s': %s\n", path, strerror(status));
        return status;
    }
    return LoadAudioFile(ref);
}

status_t SimpleTrack::LoadAudioFile(const entry_ref& ref)
{
    // Unload any existing file first
    UnloadFile();
    
    printf("SimpleTrack: Loading audio file '%s'\n", ref.name);
    
    // Get file info first
    BPath path(&ref);
    printf("SimpleTrack: Full path: %s\n", path.Path());
    
    // Check if file exists and is readable
    BEntry entry(&ref);
    if (!entry.Exists()) {
        printf("SimpleTrack: File does not exist\n");
        return B_ENTRY_NOT_FOUND;
    }
    
    if (!entry.IsFile()) {
        printf("SimpleTrack: Path is not a file\n");
        return B_ERROR;
    }
    
    // Debug: print entry_ref details
    printf("SimpleTrack: entry_ref details:\n");
    printf("  device: %d\n", (int)ref.device);
    printf("  directory: %lld\n", (long long)ref.directory);
    printf("  name: '%s'\n", ref.name ? ref.name : "NULL");
    
    // Get file size
    off_t fileSize;
    if (entry.GetSize(&fileSize) != B_OK) {
        printf("SimpleTrack: Cannot get file size\n");
        return B_ERROR;
    }
    printf("SimpleTrack: File size: %ld bytes\n", (long)fileSize);
    
    if (fileSize == 0) {
        printf("SimpleTrack: File is empty\n");
        return B_ERROR;
    }
    
    // Try to create BMediaFile with detailed error reporting
    printf("SimpleTrack: Creating BMediaFile...\n");
    
    // Try the simplest approach first (no format specification)
    fMediaFile = new BMediaFile(&ref);
    
    status_t status = fMediaFile->InitCheck();
    if (status != B_OK) {
        printf("SimpleTrack: BMediaFile init failed: %s (0x%x)\n", strerror(status), (unsigned)status);
        printf("SimpleTrack: This usually means:\n");
        printf("  - Unsupported file format\n");
        printf("  - Corrupted file header\n");
        printf("  - Missing media codec\n");
        printf("  - File access permissions\n");
        
        // Try to get more info about the file format
        printf("SimpleTrack: Attempting format detection...\n");
        
        // Read first few bytes to check file signature
        BFile file(&ref, B_READ_ONLY);
        if (file.InitCheck() == B_OK) {
            char header[16];
            ssize_t bytesRead = file.Read(header, sizeof(header));
            if (bytesRead >= 4) {
                printf("SimpleTrack: File header: ");
                for (int i = 0; i < min_c(bytesRead, 12); i++) {
                    if (header[i] >= 32 && header[i] <= 126) {
                        printf("%c", header[i]);
                    } else {
                        printf("\\x%02x", (unsigned char)header[i]);
                    }
                }
                printf("\n");
                
                // Check for common audio file signatures
                if (strncmp(header, "RIFF", 4) == 0) {
                    printf("SimpleTrack: Detected RIFF/WAV format\n");
                } else if (strncmp(header, "FORM", 4) == 0) {
                    printf("SimpleTrack: Detected FORM/AIFF format\n");
                } else if ((header[0] == 0xFF && (header[1] & 0xF0) == 0xF0)) {
                    printf("SimpleTrack: Detected MP3 format\n");
                } else {
                    printf("SimpleTrack: Unknown file format\n");
                }
            }
        }
        
        // Skip media format enumeration for now - focus on file loading
        printf("SimpleTrack: Media format enumeration skipped\n");
        
        delete fMediaFile;
        fMediaFile = nullptr;
        return status;
    }
    
    // Get first audio track
    int32 numTracks = fMediaFile->CountTracks();
    printf("SimpleTrack: Found %d tracks in file\n", (int)numTracks);
    
    fMediaTrack = nullptr;
    for (int32 i = 0; i < numTracks; i++) {
        BMediaTrack* track = fMediaFile->TrackAt(i);
        if (!track) continue;
        
        media_format format;
        status = track->DecodedFormat(&format);
        if (status == B_OK && format.type == B_MEDIA_RAW_AUDIO) {
            fMediaTrack = track;
            fFileFormat = format;
            printf("SimpleTrack: Found audio track %d, format: %d Hz, %d channels\n", 
                   (int)i, (int)format.u.raw_audio.frame_rate, 
                   (int)format.u.raw_audio.channel_count);
            break;
        } else {
            fMediaFile->ReleaseTrack(track);
        }
    }
    
    if (!fMediaTrack) {
        printf("SimpleTrack: No audio track found in file\n");
        delete fMediaFile;
        fMediaFile = nullptr;
        return B_ERROR;
    }
    
    // Configure for stereo float output
    media_raw_audio_format& rawFormat = fFileFormat.u.raw_audio;
    rawFormat.format = media_raw_audio_format::B_AUDIO_FLOAT;
    rawFormat.channel_count = 2;  // Force stereo output
    rawFormat.byte_order = B_MEDIA_HOST_ENDIAN;
    
    status = fMediaTrack->DecodedFormat(&fFileFormat);
    if (status != B_OK) {
        printf("SimpleTrack: Failed to set decoded format: %s\n", strerror(status));
        fMediaFile->ReleaseTrack(fMediaTrack);
        fMediaTrack = nullptr;
        delete fMediaFile;
        fMediaFile = nullptr;
        return status;
    }
    
    // Get file info
    fFileSampleRate = rawFormat.frame_rate;
    fFileDuration = fMediaTrack->CountFrames();
    
    // Create file buffer (1024 frames stereo)
    fFileBufferSize = 1024;
    fFileBuffer = new float[fFileBufferSize * 2];  // Stereo
    
    // Store file path for display
    BPath filePath(&ref);
    fFilePath.SetTo(filePath.Path());
    
    fFileLoaded = true;
    fPlaybackFrame = 0;
    
    printf("SimpleTrack: Successfully loaded '%s'\n", ref.name);
    printf("  Duration: %ld frames (%.2f seconds)\n", (long)fFileDuration, 
           (double)fFileDuration / fFileSampleRate);
    printf("  Sample rate: %.0f Hz\n", fFileSampleRate);
    printf("  Channels: %d\n", (int)rawFormat.channel_count);
    
    return B_OK;
}

status_t SimpleTrack::ReadFileData(float* buffer, int32 frameCount, float sampleRate)
{
    if (!fFileLoaded || !fMediaTrack || !buffer) {
        // Fill with silence if no file loaded
        memset(buffer, 0, frameCount * 2 * sizeof(float));
        return B_OK;
    }
    
    // Simple playback - read frames directly from file
    int64 framesRead = 0;
    media_header mh;
    
    status_t status = fMediaTrack->ReadFrames(buffer, &framesRead, &mh);
    if (status != B_OK || framesRead == 0) {
        // End of file or error - loop back to beginning
        fMediaTrack->SeekToFrame(&fPlaybackFrame);
        fPlaybackFrame = 0;
        
        // Try reading again from beginning
        status = fMediaTrack->ReadFrames(buffer, &framesRead, &mh);
        if (status != B_OK || framesRead == 0) {
            // Still failed - fill with silence
            memset(buffer, 0, frameCount * 2 * sizeof(float));
            return B_OK;
        }
    }
    
    // Fill remaining buffer with silence if we read less than requested
    if (framesRead < frameCount) {
        float* remainingBuffer = buffer + (framesRead * 2);
        int32 remainingFrames = frameCount - framesRead;
        memset(remainingBuffer, 0, remainingFrames * 2 * sizeof(float));
    }
    
    fPlaybackFrame += framesRead;
    return B_OK;
}

// === SimpleHaikuEngine ===

SimpleHaikuEngine::SimpleHaikuEngine()
    : fSoundPlayer(nullptr), fRunning(false), fMasterVolume(1.0f), fSoloTrack(-1),
      fMasterPeakLeft(0.0f), fMasterPeakRight(0.0f), fMasterRMSLeft(0.0f), fMasterRMSRight(0.0f),
      fRecordingSession(nullptr), fMonitoringTrackIndex(-1)
{
    // Engine created

    // Initialize recording session (temporarily disabled for compilation)
    // fRecordingSession = new VeniceDAW::RecordingSession(this);
    fRecordingSession = nullptr;
    // Recording disabled
}

SimpleHaikuEngine::~SimpleHaikuEngine()
{
    Stop();

    // Clean up recording session
    // delete fRecordingSession;  // Temporarily disabled
    fRecordingSession = nullptr;

    delete fSoundPlayer;
    
    // Cleanup tracks
    for (auto track : fTracks) {
        delete track;
    }
    // Engine destroyed
}

status_t SimpleHaikuEngine::Start()
{
    if (fRunning) {
        return B_OK;
    }
    
    // Starting engine
    
    // Check media_server status first
    // Checking media
    
    // Use completely default format - let BSoundPlayer negotiate everything
    media_raw_audio_format format = media_raw_audio_format::wildcard;
    
    // Don't force any parameters - let the system decide
    // BSoundPlayer will negotiate the best format automatically
    
    // Using default format
    
    // Create BSoundPlayer with minimal parameters - let it negotiate everything
    fSoundPlayer = new BSoundPlayer(&format, "VeniceDAW", _AudioCallback, nullptr, this);
    
    status_t status = fSoundPlayer->InitCheck();
    if (status != B_OK) {
        printf("SimpleHaikuEngine: BSoundPlayer init failed: %s (0x%x)\n", strerror(status), (int)status);
        printf("CRITICAL: BSoundPlayer should ALWAYS work on native Haiku!\n");
        printf("Possible causes:\n");
        printf("  -> Another audio application is blocking the audio device\n");
        printf("  -> BSoundPlayer created from wrong thread context\n");
        printf("  -> Media preferences misconfigured\n");
        printf("  -> System audio driver issues\n");
        return status;
    }
    
    // Print the negotiated format
    media_raw_audio_format negotiatedFormat = fSoundPlayer->Format();
    printf("✓ BSoundPlayer initialized successfully!\n");
    printf("  Format: %s\n", 
           (negotiatedFormat.format == media_raw_audio_format::B_AUDIO_FLOAT) ? "32-bit float" :
           (negotiatedFormat.format == media_raw_audio_format::B_AUDIO_SHORT) ? "16-bit integer" : "other");
    printf("  Sample rate: %.0f Hz\n", negotiatedFormat.frame_rate);
    printf("  Channels: %d\n", (int)negotiatedFormat.channel_count);
    printf("  Buffer size: %ld bytes\n", negotiatedFormat.buffer_size);
    
    float actualLatencyMs = (negotiatedFormat.buffer_size / (negotiatedFormat.channel_count * 
                            (negotiatedFormat.format == media_raw_audio_format::B_AUDIO_FLOAT ? sizeof(float) : sizeof(int16)))) 
                           * 1000.0f / negotiatedFormat.frame_rate;
    printf("  Latency: %.2f ms\n", actualLatencyMs);
    
    status = fSoundPlayer->Start();
    if (status != B_OK) {
        printf("SimpleHaikuEngine: BSoundPlayer start failed\n");
        return status;
    }
    
    fRunning = true;
    
    // Reset all file tracks to beginning when starting playback
    ResetAllTracks();
    
    // Engine started
    return B_OK;
}

status_t SimpleHaikuEngine::Stop()
{
    if (!fRunning) {
        return B_OK;
    }
    
    // Stopping
    
    if (fSoundPlayer) {
        fSoundPlayer->Stop();
    }
    
    fRunning = false;
    // Stopped
    return B_OK;
}

void SimpleHaikuEngine::ResetAllTracks()
{
    // Resetting tracks
    
    for (SimpleTrack* track : fTracks) {
        if (track && track->HasFile()) {
            track->SetPlaybackPosition(0);
            printf("  Reset '%s' to beginning\n", track->GetName());
        }
    }
}

status_t SimpleHaikuEngine::AddTrack(SimpleTrack* track)
{
    if (!track) return B_BAD_VALUE;
    
    fTracks.push_back(track);
    // Track added
    return B_OK;
}

status_t SimpleHaikuEngine::RemoveTrack(int index)
{
    if (index < 0 || (size_t)index >= fTracks.size()) {
        return B_BAD_INDEX;
    }
    
    SimpleTrack* track = fTracks[index];
    // Removing track
    
    // Remove from vector
    fTracks.erase(fTracks.begin() + index);
    
    // Reset solo if this was the solo track
    if (fSoloTrack == index) {
        fSoloTrack = -1;
    } else if (fSoloTrack > index) {
        // Adjust solo track index if it comes after the removed track
        fSoloTrack--;
    }
    
    // Clean up the track
    delete track;
    
    // Track removed
    return B_OK;
}

BString SimpleHaikuEngine::GetStatus() const
{
    BString status;
    if (fRunning) {
        status.SetToFormat("RUNNING | Tracks: %d | Volume: %.1f", 
                          (int)fTracks.size(), fMasterVolume);
    } else {
        status.SetTo("STOPPED");
    }
    return status;
}

void SimpleHaikuEngine::_AudioCallback(void* cookie, void* buffer, size_t size, const media_raw_audio_format& format)
{
    SimpleHaikuEngine* engine = static_cast<SimpleHaikuEngine*>(cookie);
    
    if (!engine || !engine->fRunning) {
        // Clear buffer and return
        memset(buffer, 0, size);
        return;
    }
    
    // Calculate frame count based on actual format
    size_t bytesPerSample = (format.format == media_raw_audio_format::B_AUDIO_FLOAT) ? sizeof(float) : sizeof(int16);
    size_t frameCount = size / (format.channel_count * bytesPerSample);
    
    if (format.format == media_raw_audio_format::B_AUDIO_FLOAT) {
        // Native float format - process directly
        engine->_ProcessAudio(static_cast<float*>(buffer), frameCount);
    } else {
        // For any other format, clear buffer (silent audio)
        // In a real implementation, you'd convert, but for now keep it simple
        memset(buffer, 0, size);
        
        // Still call ProcessAudio for internal state updates (muting, level meters, etc.)
        static float dummyBuffer[1024 * 2];  // Max 1024 stereo frames
        if (frameCount * format.channel_count <= 1024 * 2) {
            memset(dummyBuffer, 0, frameCount * format.channel_count * sizeof(float));
            engine->_ProcessAudio(dummyBuffer, frameCount);
        }
    }
}

void SimpleHaikuEngine::_ProcessAudio(float* buffer, size_t frameCount)
{
    // Master level calculation variables
    float masterPeakLeft = 0.0f;
    float masterPeakRight = 0.0f;
    float masterRMSLeft = 0.0f;
    float masterRMSRight = 0.0f;
    
    // Get sample rate from BSoundPlayer format
    float sampleRate = 44100.0f;  // Default, will be updated if possible
    if (fSoundPlayer) {
        media_raw_audio_format format = fSoundPlayer->Format();
        sampleRate = format.frame_rate;
    }
    
    // Process audio for each track
    for (size_t trackIndex = 0; trackIndex < fTracks.size(); trackIndex++) {
        SimpleTrack* track = fTracks[trackIndex];
        
        // Solo logic: if any track is solo, only play solo tracks (unless muted)
        // If no solo, play all non-muted tracks
        bool shouldPlay = false;
        if (fSoloTrack >= 0) {
            // Solo mode: only play if this track is solo AND not muted
            shouldPlay = track->IsSolo() && !track->IsMuted();
        } else {
            // Normal mode: play if not muted
            shouldPlay = !track->IsMuted();
        }
        
        if (!shouldPlay) continue;
        
        float volume = track->GetVolume() * fMasterVolume * 0.3f; // Increased from 0.1f for audible files
        
        // Use track pan setting (-1 = left, 0 = center, +1 = right)
        float pan = track->GetPan();
        
        float leftGain = (1.0f - pan) * 0.5f * volume;
        float rightGain = (1.0f + pan) * 0.5f * volume;
        
        // Process audio based on track type
        float peakLevel = 0.0f;
        float rmsSum = 0.0f;
        
        if (track->HasFile()) {
            // FILE PLAYBACK: Read actual audio data from loaded file
            float* trackBuffer = new float[frameCount * 2]; // Stereo buffer
            if (!trackBuffer) {
                // Memory allocation failed - skip this track
                continue;
            }

            memset(trackBuffer, 0, frameCount * 2 * sizeof(float));

            // Read audio data from file
            status_t status = track->ReadFileData(trackBuffer, frameCount, sampleRate);

            if (status == B_OK) {
                // Mix file audio into main buffer
                for (size_t i = 0; i < frameCount; i++) {
                    float leftSample = trackBuffer[i * 2];
                    float rightSample = trackBuffer[i * 2 + 1];

                    buffer[i * 2] += leftSample * leftGain;      // Left
                    buffer[i * 2 + 1] += rightSample * rightGain; // Right

                    // Calculate levels for VU meter using mixed samples
                    float mixedSample = (leftSample + rightSample) * 0.5f;
                    float displayLevel = fabsf(mixedSample) * track->GetVolume();
                    peakLevel = fmaxf(peakLevel, displayLevel);
                    rmsSum += displayLevel * displayLevel;
                }
            }

            // Always delete buffer, even if ReadFileData failed
            delete[] trackBuffer;
            trackBuffer = nullptr;
        } else {
            // TEST SIGNAL GENERATION: For tracks without files
            for (size_t i = 0; i < frameCount; i++) {
                float sample = _GenerateTestSignal(track, sampleRate);
                
                buffer[i * 2] += sample * leftGain;      // Left
                buffer[i * 2 + 1] += sample * rightGain; // Right
                
                // Calculate levels for VU meter display
                float displayLevel = fabsf(sample) * track->GetVolume();
                peakLevel = fmaxf(peakLevel, displayLevel);
                rmsSum += displayLevel * displayLevel;
            }
        }
        
        // Update track levels (smooth decay)
        float newPeak = peakLevel;
        float newRMS = sqrtf(rmsSum / frameCount);
        
        float smoothPeak = fmaxf(newPeak, track->GetPeakLevel() * 0.95f);  // Decay
        float smoothRMS = track->GetRMSLevel() * 0.9f + newRMS * 0.1f;    // Smooth
        
        track->UpdateLevels(smoothPeak, smoothRMS);
    }
    
    // Calculate master levels from final buffer  
    // Normalize by the 0.3 factor used in audio processing to get visual levels
    const float displayGain = 3.33f;  // Compensate for the 0.3 factor in volume calculation
    
    for (size_t i = 0; i < frameCount; i++) {
        float leftSample = fabsf(buffer[i * 2]) * displayGain;     // Left channel
        float rightSample = fabsf(buffer[i * 2 + 1]) * displayGain; // Right channel
        
        // Clamp to avoid overly high values
        leftSample = fminf(leftSample, 2.0f);   // Max 2.0 for 200% display
        rightSample = fminf(rightSample, 2.0f);
        
        masterPeakLeft = fmaxf(masterPeakLeft, leftSample);
        masterPeakRight = fmaxf(masterPeakRight, rightSample);
        
        masterRMSLeft += leftSample * leftSample;
        masterRMSRight += rightSample * rightSample;
    }
    
    // Update master levels with smoothing
    masterRMSLeft = sqrtf(masterRMSLeft / frameCount);
    masterRMSRight = sqrtf(masterRMSRight / frameCount);
    
    fMasterPeakLeft = fmaxf(masterPeakLeft, fMasterPeakLeft * 0.95f);  // Decay
    fMasterPeakRight = fmaxf(masterPeakRight, fMasterPeakRight * 0.95f);
    fMasterRMSLeft = fMasterRMSLeft * 0.9f + masterRMSLeft * 0.1f;     // Smooth
    fMasterRMSRight = fMasterRMSRight * 0.9f + masterRMSRight * 0.1f;
}

void SimpleHaikuEngine::SetTrackSolo(int trackIndex, bool solo)
{
    if (trackIndex < 0 || (size_t)trackIndex >= fTracks.size()) {
        return;
    }
    
    SimpleTrack* targetTrack = fTracks[trackIndex];
    
    if (solo) {
        // Turn off all other solo states first
        for (size_t i = 0; i < fTracks.size(); i++) {
            fTracks[i]->SetSolo(false);
        }
        
        // Set this track as solo
        targetTrack->SetSolo(true);
        fSoloTrack = trackIndex;
        
        printf("SimpleHaikuEngine: Track %d ('%s') is now SOLO\n", 
               trackIndex, targetTrack->GetName());
    } else {
        // Turn off solo for this track
        targetTrack->SetSolo(false);
        
        // Check if any other track is still solo
        fSoloTrack = -1;
        for (size_t i = 0; i < fTracks.size(); i++) {
            if (fTracks[i]->IsSolo()) {
                fSoloTrack = i;
                break;
            }
        }
        
        printf("SimpleHaikuEngine: Track %d ('%s') solo OFF. Current solo: %d\n", 
               trackIndex, targetTrack->GetName(), fSoloTrack);
    }
}

float SimpleHaikuEngine::_GenerateTestSignal(SimpleTrack* track, float sampleRate)
{
    // If track has a file loaded, we need to use a different approach
    // The per-sample file reading will be replaced with buffer-level reading
    // For now, if a file is loaded, generate a different test tone to indicate it's working
    if (track->HasFile()) {
        // Generate a distinctive tone for file-loaded tracks (higher frequency)
        float frequency = 1000.0f;  // 1kHz to distinguish from test signals
        float phaseIncrement = (2.0f * M_PI * frequency) / sampleRate;
        float sample = sinf(track->GetPhase()) * 0.3f;  // Lower volume
        track->GetPhase() += phaseIncrement;
        if (track->GetPhase() > 2.0f * M_PI) {
            track->GetPhase() -= 2.0f * M_PI;
        }
        return sample;
    }
    
    // Generate test signals
    float sample = 0.0f;
    float frequency = track->GetFrequency();
    
    switch (track->GetSignalType()) {
        case SimpleTrack::kSignalSine:
        {
            // Sine wave generator
            float phaseIncrement = (2.0f * M_PI * frequency) / sampleRate;
            sample = sinf(track->GetPhase()) * 0.5f;
            track->GetPhase() += phaseIncrement;
            if (track->GetPhase() > 2.0f * M_PI) {
                track->GetPhase() -= 2.0f * M_PI;
            }
            break;
        }
        
        case SimpleTrack::kSignalSquare:
        {
            // Square wave generator
            float phaseIncrement = (2.0f * M_PI * frequency) / sampleRate;
            sample = (sinf(track->GetPhase()) > 0.0f) ? 0.5f : -0.5f;
            track->GetPhase() += phaseIncrement;
            if (track->GetPhase() > 2.0f * M_PI) {
                track->GetPhase() -= 2.0f * M_PI;
            }
            break;
        }
        
        case SimpleTrack::kSignalSaw:
        {
            // Sawtooth wave generator
            float phaseIncrement = frequency / sampleRate;
            track->GetPhase() += phaseIncrement;
            if (track->GetPhase() > 1.0f) {
                track->GetPhase() -= 1.0f;
            }
            sample = (track->GetPhase() * 2.0f - 1.0f) * 0.5f;
            break;
        }
        
        case SimpleTrack::kSignalWhiteNoise:
        {
            // White noise generator
            sample = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * 0.3f;
            break;
        }
        
        case SimpleTrack::kSignalPinkNoise:
        {
            // Pink noise generator (1/f spectrum)
            // Using the Voss-McCartney algorithm
            float white = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f);

            track->GetPinkNoiseState(0) = 0.99886f * track->GetPinkNoiseState(0) + white * 0.0555179f;
            track->GetPinkNoiseState(1) = 0.99332f * track->GetPinkNoiseState(1) + white * 0.0750759f;
            track->GetPinkNoiseState(2) = 0.96900f * track->GetPinkNoiseState(2) + white * 0.1538520f;
            track->GetPinkNoiseState(3) = 0.86650f * track->GetPinkNoiseState(3) + white * 0.3104856f;
            track->GetPinkNoiseState(4) = 0.55000f * track->GetPinkNoiseState(4) + white * 0.5329522f;
            track->GetPinkNoiseState(5) = -0.7616f * track->GetPinkNoiseState(5) - white * 0.0168980f;

            float pink = track->GetPinkNoiseState(0) + track->GetPinkNoiseState(1) +
                        track->GetPinkNoiseState(2) + track->GetPinkNoiseState(3) +
                        track->GetPinkNoiseState(4) + track->GetPinkNoiseState(5) +
                        track->GetPinkNoiseState(6) + white * 0.5362f;

            track->GetPinkNoiseState(6) = white * 0.115926f;

            // Normalize using per-track maximum (thread-safe)
            float pink_max = track->GetPinkNoiseMax();
            sample = (pink / pink_max) * 0.3f;

            // Track maximum for normalization
            float absval = fabsf(pink);
            if (absval > pink_max) {
                track->SetPinkNoiseMax(absval);
            }
            break;
        }
    }
    
    return sample;
}

void SimpleHaikuEngine::CreateDemoScene()
{
    printf("SimpleHaikuEngine: Creating demo scene with test signals...\n");
    
    // Clear existing tracks
    for (auto track : fTracks) {
        delete track;
    }
    fTracks.clear();
    fSoloTrack = -1;
    
    // Create 5 demo tracks with different signal types
    struct DemoTrackConfig {
        const char* name;
        SimpleTrack::SignalType type;
        float frequency;
        float x, y, z;  // 3D position
        float pan;
        float volume;
    };
    
    DemoTrackConfig configs[] = {
        { "Sine 220Hz (A3)",     SimpleTrack::kSignalSine,        220.0f,  -2.0f,  0.0f,  2.0f, -0.7f, 0.8f },
        { "Sine 440Hz (A4)",     SimpleTrack::kSignalSine,        440.0f,   0.0f,  0.0f,  3.0f,  0.0f, 0.7f },
        { "Square 880Hz (A5)",   SimpleTrack::kSignalSquare,      880.0f,   2.0f,  0.0f,  2.0f,  0.7f, 0.5f },
        { "White Noise",         SimpleTrack::kSignalWhiteNoise,   0.0f,  -1.0f,  2.0f,  1.0f, -0.3f, 0.4f },
        { "Pink Noise",          SimpleTrack::kSignalPinkNoise,    0.0f,   1.0f, -2.0f,  1.0f,  0.3f, 0.4f }
    };
    
    for (int i = 0; i < 5; i++) {
        SimpleTrack* track = new SimpleTrack(i + 1, configs[i].name);
        track->SetSignalType(configs[i].type);
        track->SetFrequency(configs[i].frequency);
        track->SetPosition(configs[i].x, configs[i].y, configs[i].z);
        track->SetPan(configs[i].pan);
        track->SetVolume(configs[i].volume);
        
        AddTrack(track);
        
        printf("  Created: %s at position (%.1f, %.1f, %.1f)\n", 
               configs[i].name, configs[i].x, configs[i].y, configs[i].z);
    }
    
    printf("SimpleHaikuEngine: Demo scene created with %d test tracks\n", (int)fTracks.size());
    printf("  -> Sine waves demonstrate tonal content\n");
    printf("  -> Square wave shows harmonic richness\n");
    printf("  -> White/Pink noise for testing spatial separation\n");
    printf("  -> All tracks positioned in 3D space for spatial demo\n");
}

status_t SimpleHaikuEngine::CreateEmptyTrack(const char* name)
{
    if (!name || strlen(name) == 0) {
        printf("SimpleHaikuEngine: Cannot create track with empty name\n");
        return B_BAD_VALUE;
    }

    // Create new empty track
    int trackId = fTracks.size() + 1;
    SimpleTrack* newTrack = new SimpleTrack(trackId, name);

    // Set default position in 3D space (arranged in a circle)
    float angle = (float)fTracks.size() * (2.0f * M_PI / 8.0f);  // Arrange 8 tracks in circle
    float radius = 3.0f;
    float x = radius * cos(angle);
    float y = radius * sin(angle);
    float z = 0.0f;

    newTrack->SetPosition(x, y, z);
    newTrack->SetVolume(0.8f);  // Default volume
    newTrack->SetPan(0.0f);     // Center pan

    // Add to engine
    AddTrack(newTrack);

    // printf("SimpleHaikuEngine: Created empty track '%s' (ID %d) at position (%.1f, %.1f, %.1f)\n",
    //        name, trackId, x, y, z);

    return B_OK;
}

status_t SimpleHaikuEngine::LoadAudioFileAsTrack(const char* path)
{
    entry_ref ref;
    status_t status = get_ref_for_path(path, &ref);
    if (status != B_OK) {
        printf("SimpleHaikuEngine: Failed to get ref for path '%s': %s\n", path, strerror(status));
        return status;
    }
    return LoadAudioFileAsTrack(ref);
}

status_t SimpleHaikuEngine::LoadAudioFileAsTrack(const entry_ref& ref)
{
    printf("SimpleHaikuEngine: Loading audio file as new track: '%s'\n", ref.name);
    
    // Create new track for this file
    int trackId = fTracks.size() + 1;
    BString trackName;
    trackName.SetToFormat("%s", ref.name);
    
    SimpleTrack* newTrack = new SimpleTrack(trackId, trackName.String());
    
    // Try to load the audio file
    status_t status = newTrack->LoadAudioFile(ref);
    if (status != B_OK) {
        printf("SimpleHaikuEngine: Primary loading failed, trying alternative method...\n");
        status = newTrack->LoadAudioFileAlternative(ref);
        if (status != B_OK) {
            printf("SimpleHaikuEngine: All loading methods failed: %s\n", strerror(status));
            delete newTrack;
            return status;
        }
    }
    
    // Position the track in 3D space (spread them out)
    float angle = (float)fTracks.size() * 60.0f * M_PI / 180.0f;  // 60 degrees apart
    float radius = 2.0f;
    float x = sinf(angle) * radius;
    float y = cosf(angle) * radius;
    float z = 0.0f;
    
    newTrack->SetPosition(x, y, z);
    newTrack->SetVolume(0.7f);  // Reasonable volume
    newTrack->SetPan(0.0f);     // Centered pan
    
    // Add to engine
    status = AddTrack(newTrack);
    if (status != B_OK) {
        printf("SimpleHaikuEngine: Failed to add track to engine: %s\n", strerror(status));
        delete newTrack;
        return status;
    }
    
    printf("SimpleHaikuEngine: Successfully loaded '%s' as track %d\n", ref.name, trackId);
    printf("  Positioned at (%.1f, %.1f, %.1f)\n", x, y, z);
    printf("  Duration: %.2f seconds\n", (double)newTrack->GetFileDuration() / newTrack->GetFileSampleRate());
    
    return B_OK;
}

status_t SimpleTrack::LoadAudioFileAlternative(const entry_ref& ref)
{
    printf("SimpleTrack: Trying alternative loading method for '%s'\n", ref.name);
    
    // Alternative approach: try different file format specifications
    media_file_format fileFormat;
    
    // Try specific WAV format first
    memset(&fileFormat, 0, sizeof(fileFormat));
    strcpy(fileFormat.short_name, "wav");
    strcpy(fileFormat.pretty_name, "WAV audio");
    fileFormat.family = B_MISC_FORMAT_FAMILY;
    
    printf("SimpleTrack: Trying with WAV format specification...\n");
    fMediaFile = new BMediaFile(&ref, &fileFormat);
    status_t status = fMediaFile->InitCheck();
    
    if (status != B_OK) {
        delete fMediaFile;
        fMediaFile = nullptr;
        
        // Try with no format specification (let BMediaFile auto-detect)
        printf("SimpleTrack: Trying with auto-detection...\n");
        fMediaFile = new BMediaFile(&ref);
        status = fMediaFile->InitCheck();
        
        if (status != B_OK) {
            printf("SimpleTrack: Alternative method also failed: %s\n", strerror(status));
            delete fMediaFile;
            fMediaFile = nullptr;
            return status;
        }
    }
    
    printf("SimpleTrack: Alternative method succeeded!\n");
    
    // Continue with normal track setup...
    int32 numTracks = fMediaFile->CountTracks();
    printf("SimpleTrack: Found %d tracks in file\n", (int)numTracks);
    
    // Get first audio track
    fMediaTrack = nullptr;
    for (int32 i = 0; i < numTracks; i++) {
        BMediaTrack* track = fMediaFile->TrackAt(i);
        if (!track) continue;
        
        media_format format;
        status = track->DecodedFormat(&format);
        if (status == B_OK && format.type == B_MEDIA_RAW_AUDIO) {
            fMediaTrack = track;
            fFileFormat = format;
            printf("SimpleTrack: Found audio track %d, format: %d Hz, %d channels\n", 
                   (int)i, (int)format.u.raw_audio.frame_rate, 
                   (int)format.u.raw_audio.channel_count);
            break;
        } else {
            fMediaFile->ReleaseTrack(track);
        }
    }
    
    if (!fMediaTrack) {
        printf("SimpleTrack: No audio track found in file\n");
        delete fMediaFile;
        fMediaFile = nullptr;
        return B_ERROR;
    }
    
    // Set basic file info
    fFileSampleRate = fFileFormat.u.raw_audio.frame_rate;
    fFileDuration = fMediaTrack->CountFrames();
    
    BPath altPath(&ref);
    fFilePath.SetTo(altPath.Path());
    fFileLoaded = true;
    fPlaybackFrame = 0;
    
    printf("SimpleTrack: Alternative loading successful!\n");
    printf("  Sample rate: %.0f Hz\n", fFileSampleRate);
    printf("  Duration: %ld frames\n", (long)fFileDuration);
    
    return B_OK;
}

// =====================================
// Recording Methods
// =====================================

status_t SimpleHaikuEngine::StartRecording(int32 trackIndex, const char* filename)
{
    if (!fRecordingSession) {
        printf("SimpleHaikuEngine: No recording session available\n");
        return B_ERROR;
    }

    printf("SimpleHaikuEngine: Starting recording on track %d\n", (int)trackIndex);

    // Start the recording session if not active (temporarily disabled)
    if (!fRecordingSession) return B_ERROR;
    status_t status = B_OK; // fRecordingSession->StartSession();
    if (status != B_OK) {
        printf("SimpleHaikuEngine: Failed to start recording session: %s\n", strerror(status));
        return status;
    }

    // Start recording on specific track (temporarily disabled)
    status = B_OK; // fRecordingSession->StartTrackRecording(trackIndex, filename);
    if (status != B_OK) {
        printf("SimpleHaikuEngine: Failed to start track recording: %s\n", strerror(status));
        return status;
    }

    printf("SimpleHaikuEngine: Recording started on track %d\n", (int)trackIndex);
    return B_OK;
}

status_t SimpleHaikuEngine::StopRecording(int32 trackIndex)
{
    if (!fRecordingSession) {
        printf("SimpleHaikuEngine: No recording session available\n");
        return B_ERROR;
    }

    printf("SimpleHaikuEngine: Stopping recording on track %d\n", (int)trackIndex);

    if (!fRecordingSession) return B_ERROR;
    status_t status = B_OK; // fRecordingSession->StopTrackRecording(trackIndex);
    if (status != B_OK) {
        printf("SimpleHaikuEngine: Failed to stop track recording: %s\n", strerror(status));
        return status;
    }

    printf("SimpleHaikuEngine: Recording stopped on track %d\n", (int)trackIndex);
    return B_OK;
}

bool SimpleHaikuEngine::IsRecording(int32 trackIndex) const
{
    if (!fRecordingSession) {
        return false;
    }

    if (trackIndex == -1) {
        // Check if any track is recording
        for (int i = 0; i < GetTrackCount(); i++) {
            if (false) { // fRecordingSession->IsTrackRecording(i)) {
                return true;
            }
        }
        return false;
    } else {
        // Check specific track
        return false; // fRecordingSession->IsTrackRecording(trackIndex);
    }
}

// =====================================
// Live Monitoring Support
// =====================================

status_t SimpleHaikuEngine::CreateMonitoringTrack(const char* name)
{
    // Remove existing monitoring track if any
    if (fMonitoringTrackIndex >= 0 && fMonitoringTrackIndex < (int32)fTracks.size()) {
        delete fTracks[fMonitoringTrackIndex];
        fTracks.erase(fTracks.begin() + fMonitoringTrackIndex);
        fMonitoringTrackIndex = -1;
    }

    // Create new monitoring track
    SimpleTrack* monitoringTrack = new SimpleTrack(fTracks.size(), name);
    monitoringTrack->SetVolume(0.8f);  // Slightly reduced for monitoring
    monitoringTrack->SetSignalType(SimpleTrack::kSignalWhiteNoise);  // Will be replaced by live audio

    status_t status = AddTrack(monitoringTrack);
    if (status != B_OK) {
        delete monitoringTrack;
        return status;
    }

    fMonitoringTrackIndex = fTracks.size() - 1;
    printf("SimpleHaikuEngine: Created monitoring track '%s' at index %d\n", name, fMonitoringTrackIndex);

    return B_OK;
}

status_t SimpleHaikuEngine::FeedMonitoringAudio(const void* data, size_t size,
                                                  const media_raw_audio_format& format)
{
    if (fMonitoringTrackIndex < 0 || fMonitoringTrackIndex >= (int32)fTracks.size()) {
        // No monitoring track - create one automatically
        status_t status = CreateMonitoringTrack("Live Input");
        if (status != B_OK) {
            return status;
        }
    }

    SimpleTrack* monitoringTrack = fTracks[fMonitoringTrackIndex];
    if (!monitoringTrack) {
        return B_ERROR;
    }

    // Convert incoming audio to float format for mixing
    // Note: This is a simplified implementation - in production would need
    // proper format conversion and buffering strategy

    const int16* samples = static_cast<const int16*>(data);
    size_t frameCount = size / (format.channel_count * sizeof(int16));

    // Calculate RMS and peak levels for monitoring
    float peakLevel = 0.0f;
    float rmsSum = 0.0f;

    for (size_t i = 0; i < frameCount * format.channel_count; i++) {
        float sample = samples[i] / 32768.0f;  // Convert to float [-1.0, 1.0]
        float absSample = fabs(sample);

        if (absSample > peakLevel) {
            peakLevel = absSample;
        }

        rmsSum += sample * sample;
    }

    float rmsLevel = sqrt(rmsSum / (frameCount * format.channel_count));

    // Update monitoring track levels for visualization
    monitoringTrack->UpdateLevels(peakLevel, rmsLevel);

    // Store audio in monitoring buffer for playback
    // Note: In production, this would need proper circular buffer implementation
    // For now, we just update the levels for visual feedback

    return B_OK;
}

} // namespace HaikuDAW
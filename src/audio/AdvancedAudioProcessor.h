/*
 * AdvancedAudioProcessor.h - Phase 3 Professional Audio Processing Engine
 * 
 * Advanced audio processing foundation for VeniceDAW Phase 3 development.
 * Supports surround sound, professional effects, and spatial audio processing.
 */

#ifndef ADVANCED_AUDIO_PROCESSOR_H
#define ADVANCED_AUDIO_PROCESSOR_H

#ifdef __HAIKU__
    #include <MediaKit.h>
    #include <SoundPlayer.h>
    #include <OS.h>
#else
    #include "../testing/HaikuMockHeaders.h"
#endif

#include <vector>
#include <memory>
#include <atomic>
#include <array>
#include <string>
#include "DSPAlgorithms.h"

namespace VeniceDAW {

// Audio channel configuration for surround sound
enum class ChannelConfiguration {
    MONO = 1,
    STEREO = 2,
    SURROUND_5_1 = 6,
    SURROUND_7_1 = 8,
    DOLBY_ATMOS = 16  // Base channels for object-based audio
};

// Professional audio sample formats
enum class SampleFormat {
    PCM_16,     // CD quality
    PCM_24,     // Professional standard
    PCM_32,     // High precision
    FLOAT_32    // Internal processing
};

// Audio processing quality levels
enum class ProcessingQuality {
    REALTIME,      // Optimized for low latency
    BALANCED,      // Good quality with reasonable CPU usage
    HIGHEST        // Maximum quality for offline processing
};

// Advanced audio buffer for multi-channel processing
struct AdvancedAudioBuffer {
    std::vector<std::vector<float>> channels;  // Multi-channel audio data
    size_t frameCount;
    float sampleRate;
    ChannelConfiguration channelConfig;
    
    AdvancedAudioBuffer(ChannelConfiguration config, size_t frames, float sr);
    void Clear();
    void Resize(size_t frames);
    float* GetChannelData(size_t channel);
    const float* GetChannelData(size_t channel) const;
    size_t GetChannelCount() const;
};

// Base class for all audio effects and processors
class AudioEffect {
public:
    AudioEffect(const std::string& name);
    virtual ~AudioEffect() = default;
    
    // Core processing methods
    virtual void Process(AdvancedAudioBuffer& buffer) = 0;
    virtual void ProcessRealtime(AdvancedAudioBuffer& buffer) = 0;
    
    // Parameter management
    virtual void SetParameter(const std::string& param, float value) = 0;
    virtual float GetParameter(const std::string& param) const = 0;
    virtual std::vector<std::string> GetParameterList() const = 0;
    
    // Effect management
    virtual void Reset() = 0;
    virtual void Bypass(bool enabled) { fBypassed = enabled; }
    virtual bool IsBypassed() const { return fBypassed; }
    
    // Performance monitoring
    virtual float GetCPUUsage() const { return fCPUUsage; }
    virtual size_t GetLatencySamples() const { return fLatencySamples; }
    
    const std::string& GetName() const { return fName; }

protected:
    std::string fName;
    std::atomic<bool> fBypassed{false};
    std::atomic<float> fCPUUsage{0.0f};
    size_t fLatencySamples{0};
};

// Professional EQ effect
class ProfessionalEQ : public AudioEffect {
public:
    enum class FilterType {
        LowPass,
        HighPass,
        LowShelf,
        HighShelf,
        Peak,
        Notch,
        BandPass,
        AllPass
    };

    struct EQBand {
        float frequency;    // Center frequency in Hz
        float gain;         // Gain in dB
        float Q;           // Quality factor
        FilterType type;   // Filter type
        bool enabled;
    };
    
    ProfessionalEQ();
    
    void Initialize(float sampleRate);
    void Process(AdvancedAudioBuffer& buffer) override;
    void ProcessRealtime(AdvancedAudioBuffer& buffer) override;
    float ProcessSample(float input, size_t channel);
    
    void SetParameter(const std::string& param, float value) override;
    float GetParameter(const std::string& param) const override;
    std::vector<std::string> GetParameterList() const override;
    void Reset() override;
    
    // EQ-specific methods
    void SetBand(size_t band, float freq, float gain, float Q);
    void SetBandFrequency(size_t band, float freq);
    void SetBandGain(size_t band, float gain);
    void SetBandQ(size_t band, float Q);
    void SetBandType(size_t band, FilterType type);
    void SetBandEnabled(size_t band, bool enabled);
    void SetBypassed(bool bypassed);
    
    EQBand GetBand(size_t band) const;
    void EnableBand(size_t band, bool enabled);
    float GetFrequencyResponse(float frequency) const;

private:
    static const size_t MAX_BANDS = 8;
    std::array<EQBand, MAX_BANDS> fBands;
    std::vector<std::array<DSP::BiquadFilter, MAX_BANDS>> fFilters;  // Per-channel filters
    std::vector<DSP::DCBlocker> fDCBlockers;  // Per-channel DC blockers
    
    float fSampleRate{44100.0f};
    bool fInitialized{false};
    std::atomic<bool> fNeedsUpdate{true};
    
    void InitializeChannels(size_t channelCount);
    void UpdateFilters();
    void UpdateBandFilter(size_t band);
    float ProcessBandFilter(size_t band, size_t channel, float input);
    DSP::BiquadFilter::FilterType ConvertFilterType(FilterType type) const;
};

// Dynamic range processor (compressor/limiter)
class DynamicsProcessor : public AudioEffect {
public:
    enum class Mode {
        COMPRESSOR,
        LIMITER,
        GATE,
        EXPANDER
    };
    
    enum class DetectionMode {
        PEAK,
        RMS,
        PEAK_RMS_HYBRID
    };
    
    DynamicsProcessor();
    
    void Initialize(float sampleRate);
    void Process(AdvancedAudioBuffer& buffer) override;
    void ProcessRealtime(AdvancedAudioBuffer& buffer) override;
    
    void SetParameter(const std::string& param, float value) override;
    float GetParameter(const std::string& param) const override;
    std::vector<std::string> GetParameterList() const override;
    void Reset() override;
    
    // Dynamics-specific methods
    void SetMode(Mode mode) { fMode = mode; }
    void SetDetectionMode(DetectionMode mode);
    void SetBypassed(bool bypassed);
    float GetGainReduction() const { return fGainReduction; }
    float GetInputLevel() const { return fInputLevel; }
    float GetOutputLevel() const { return fOutputLevel; }
    
    // Lookahead functionality for zero-latency limiting
    void SetLookaheadTime(float milliseconds);
    float GetLookaheadTime() const { return fLookaheadTime; }
    void EnableLookahead(bool enabled) { fLookaheadEnabled = enabled; }
    bool IsLookaheadEnabled() const { return fLookaheadEnabled; }

private:
    Mode fMode{Mode::COMPRESSOR};
    DetectionMode fDetectionMode{DetectionMode::RMS};
    float fThreshold{-12.0f};     // dB
    float fRatio{4.0f};           // Compression ratio
    float fAttack{10.0f};         // ms
    float fRelease{100.0f};       // ms
    float fKnee{2.0f};           // dB
    float fMakeupGain{0.0f};     // dB
    bool fAutoMakeup{false};
    
    std::vector<DSP::EnvelopeFollower> fEnvelopeFollowers; // Per-channel envelope followers
    float fGainReduction{0.0f};
    float fInputLevel{0.0f};
    float fOutputLevel{0.0f};
    float fSampleRate{44100.0f};
    bool fInitialized{false};
    std::atomic<bool> fNeedsUpdate{true};
    
    // Lookahead buffer system for zero-latency limiting
    bool fLookaheadEnabled{false};
    float fLookaheadTime{5.0f};  // Default 5ms lookahead
    size_t fLookaheadSamples{0}; // Lookahead time in samples
    std::vector<std::vector<float>> fLookaheadBuffers;  // Per-channel circular buffers
    std::vector<size_t> fBufferWritePos;  // Write position for each channel
    std::vector<size_t> fBufferReadPos;   // Read position for each channel
    std::vector<float> fPeakBuffer;       // Peak analysis buffer for lookahead
    
    void InitializeChannels(size_t channelCount);
    void UpdateEnvelopeFollowers();
    float ProcessEnvelope(size_t channel, float input);
    float CalculateGainReduction(float envelope);
    float ApplyKnee(float input, float threshold, float knee);
    float CalculateCompressorGain(float input);
    float CalculateLimiterGain(float input);
    float CalculateGateGain(float input);
    float CalculateExpanderGain(float input);
    
    // Lookahead processing methods
    void InitializeLookaheadBuffers(size_t channelCount);
    void UpdateLookaheadParameters();
    float ProcessLookaheadSample(size_t channel, float input);
    float AnalyzeLookaheadPeak(size_t channel);
    void WriteLookaheadSample(size_t channel, float sample);
    float ReadLookaheadSample(size_t channel);
};

// Professional spatial audio processor for multi-channel and 3D audio
class SurroundProcessor {
public:
    enum class SpatialMode {
        BASIC_SURROUND,     // Basic surround upmixing/downmixing
        SPATIAL_3D,         // Full 3D spatial positioning
        BINAURAL_HRTF,      // HRTF-based binaural processing
        AMBISONICS          // Ambisonic encoding/decoding
    };
    
    SurroundProcessor(ChannelConfiguration config);
    ~SurroundProcessor();
    
    // Initialization and configuration
    void Initialize(float sampleRate);
    void SetChannelConfiguration(ChannelConfiguration config);
    ChannelConfiguration GetChannelConfiguration() const { return fChannelConfig; }
    void SetSpatialMode(SpatialMode mode);
    SpatialMode GetSpatialMode() const { return fSpatialMode; }
    
    // Core surround processing
    void ProcessStereoToSurround(const AdvancedAudioBuffer& stereo, AdvancedAudioBuffer& surround);
    void ProcessSurroundToStereo(const AdvancedAudioBuffer& surround, AdvancedAudioBuffer& stereo);
    void ProcessSurroundDownmix(const AdvancedAudioBuffer& input, AdvancedAudioBuffer& output);
    
    // 3D Spatial processing
    void ProcessSpatial3D(AdvancedAudioBuffer& buffer);
    void ProcessBinauralHRTF(const AdvancedAudioBuffer& mono, AdvancedAudioBuffer& stereo);
    
    // Channel routing and mixing
    void SetChannelGain(size_t channel, float gain);
    float GetChannelGain(size_t channel) const;
    void MuteChannel(size_t channel, bool muted);
    bool IsChannelMuted(size_t channel) const;
    
    // 3D Spatial positioning
    void SetSourcePosition(const DSP::Vector3D& position);
    void SetListenerPosition(const DSP::Vector3D& position);
    void SetListenerOrientation(const DSP::Vector3D& forward, const DSP::Vector3D& up);
    void SetSourceVelocity(const DSP::Vector3D& velocity);
    void SetListenerVelocity(const DSP::Vector3D& velocity);
    
    // Distance and environmental modeling
    void SetRoomSize(float width, float height, float depth);
    void SetReverberation(float amount, float decay);
    void SetAirAbsorption(bool enabled, float humidity = 50.0f);
    void SetDopplerEffect(bool enabled, float speedOfSound = 343.0f);
    
    // HRTF processing
    void SetHRTFDatabase(const float* leftHRTF, const float* rightHRTF, size_t impulseLength, 
                        float azimuth, float elevation);
    void EnableCrossfeed(bool enabled, float amount = 0.3f);
    void EnableHRTF(bool enabled);
    bool IsHRTFEnabled() const { return fHRTFEnabled; }
    void LoadDefaultHRTF();  // Load built-in generic HRTF
    
    // Spatial parameters getters
    DSP::Vector3D GetSourcePosition() const { return fSourcePosition; }
    DSP::Vector3D GetListenerPosition() const { return fListenerPosition; }
    DSP::SphericalCoordinate GetRelativePosition() const;
    float GetDistance() const;
    float GetAzimuth() const;
    float GetElevation() const;
    
    // Performance monitoring
    float GetProcessingLoad() const { return fProcessingLoad; }
    size_t GetLatencySamples() const { return fLatencySamples; }
    
private:
    ChannelConfiguration fChannelConfig;
    SpatialMode fSpatialMode{SpatialMode::BASIC_SURROUND};
    float fSampleRate{44100.0f};
    bool fInitialized{false};
    
    std::vector<float> fChannelGains;
    std::vector<bool> fChannelMuted;
    
    // 3D Spatial audio parameters
    DSP::Vector3D fSourcePosition{0.0f, 0.0f, 0.0f};
    DSP::Vector3D fListenerPosition{0.0f, 0.0f, 0.0f};
    DSP::Vector3D fListenerForward{0.0f, 1.0f, 0.0f};  // Forward direction
    DSP::Vector3D fListenerUp{0.0f, 0.0f, 1.0f};       // Up direction
    DSP::Vector3D fSourceVelocity{0.0f, 0.0f, 0.0f};
    DSP::Vector3D fListenerVelocity{0.0f, 0.0f, 0.0f};
    
    // Environmental parameters
    DSP::Vector3D fRoomSize{10.0f, 8.0f, 3.0f};  // Default room dimensions in meters
    float fReverbAmount{0.2f};
    float fReverbDecay{1.5f};
    bool fAirAbsorptionEnabled{true};
    float fHumidity{50.0f};
    bool fDopplerEnabled{true};
    float fSpeedOfSound{343.0f};
    
    // Delay lines for spatial processing
    std::vector<std::unique_ptr<DSP::DelayLine>> fSpatialDelays;
    
    // HRTF convolution engines
    std::unique_ptr<DSP::ConvolutionEngine> fLeftHRTF;
    std::unique_ptr<DSP::ConvolutionEngine> fRightHRTF;
    bool fHRTFEnabled{false};
    
    // Crossfeed processing
    bool fCrossfeedEnabled{false};
    float fCrossfeedAmount{0.3f};
    std::vector<DSP::BiquadFilter> fCrossfeedFilters;
    
    // Performance monitoring
    mutable std::atomic<float> fProcessingLoad{0.0f};
    size_t fLatencySamples{0};
    
    // Internal processing methods
    void InitializeChannelMixing();
    void InitializeSpatialProcessing();
    void InitializeHRTFProcessing();
    void UpdateSpatialParameters();
    
    // Spatial calculations
    float CalculateChannelDelay(size_t channel);
    float CalculateChannelGain(size_t channel);
    float CalculateDistanceAttenuation();
    float CalculateAirAbsorptionFactor(float frequency);
    float CalculateDopplerFactor();
    
    // Channel-specific spatial processing
    void ProcessChannelSpatial(size_t channel, float* buffer, size_t numSamples);
    void ApplySpatialDelay(size_t channel, float* buffer, size_t numSamples);
    void ApplySpatialGain(size_t channel, float* buffer, size_t numSamples);
    
    // HRTF processing methods
    void ProcessHRTFConvolution(const float* monoInput, float* leftOutput, float* rightOutput, size_t numSamples);
    void UpdateHRTFParameters(float azimuth, float elevation);
    
    // Advanced surround processing  
    void ProcessIntelligentUpmix(const AdvancedAudioBuffer& stereo, AdvancedAudioBuffer& surround);
    void ProcessBassManagement(AdvancedAudioBuffer& buffer);
    
public:
    void ProcessCrossfeed(AdvancedAudioBuffer& stereo);
    
private:
};

// Advanced audio processor coordinator
class AdvancedAudioProcessor {
public:
    AdvancedAudioProcessor();
    ~AdvancedAudioProcessor() = default;
    
    // Processor management
    void Initialize(float sampleRate, size_t bufferSize, ChannelConfiguration config);
    void Shutdown();
    bool IsInitialized() const { return fInitialized; }
    
    // Effect chain management
    void AddEffect(std::unique_ptr<AudioEffect> effect);
    void RemoveEffect(const std::string& name);
    AudioEffect* GetEffect(const std::string& name);
    void ClearEffects();
    
    // Audio processing
    void ProcessBuffer(AdvancedAudioBuffer& buffer);
    void ProcessRealtimeBuffer(AdvancedAudioBuffer& buffer);
    
    // Performance monitoring
    float GetTotalCPUUsage() const;
    size_t GetTotalLatency() const;
    void UpdatePerformanceMetrics();
    
    // Configuration
    void SetProcessingQuality(ProcessingQuality quality) { fQuality = quality; }
    ProcessingQuality GetProcessingQuality() const { return fQuality; }
    
    void SetChannelConfiguration(ChannelConfiguration config);
    ChannelConfiguration GetChannelConfiguration() const { return fChannelConfig; }
    
    // Surround sound processing
    SurroundProcessor& GetSurroundProcessor() { return fSurroundProcessor; }

private:
    bool fInitialized{false};
    float fSampleRate{44100.0f};
    size_t fBufferSize{1024};
    ChannelConfiguration fChannelConfig{ChannelConfiguration::STEREO};
    ProcessingQuality fQuality{ProcessingQuality::BALANCED};
    
    std::vector<std::unique_ptr<AudioEffect>> fEffects;
    SurroundProcessor fSurroundProcessor;
    
    // Performance monitoring
    std::atomic<float> fTotalCPUUsage{0.0f};
    std::atomic<size_t> fTotalLatency{0};
    
    void ValidateConfiguration();
};

} // namespace VeniceDAW

#endif // ADVANCED_AUDIO_PROCESSOR_H
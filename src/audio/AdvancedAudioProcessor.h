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
    
    DynamicsProcessor();
    
    void Process(AdvancedAudioBuffer& buffer) override;
    void ProcessRealtime(AdvancedAudioBuffer& buffer) override;
    
    void SetParameter(const std::string& param, float value) override;
    float GetParameter(const std::string& param) const override;
    std::vector<std::string> GetParameterList() const override;
    void Reset() override;
    
    // Dynamics-specific methods
    void SetMode(Mode mode) { fMode = mode; }
    float GetGainReduction() const { return fGainReduction; }

private:
    Mode fMode{Mode::COMPRESSOR};
    float fThreshold{-12.0f};     // dB
    float fRatio{4.0f};           // Compression ratio
    float fAttack{10.0f};         // ms
    float fRelease{100.0f};       // ms
    float fKnee{2.0f};           // dB
    
    std::vector<float> fEnvelope; // Per-channel envelope followers
    float fGainReduction{0.0f};
    float fSampleRate{44100.0f};
    
    float ProcessEnvelope(size_t channel, float input);
    float CalculateGainReduction(float envelope);
};

// Surround sound processor for multi-channel audio
class SurroundProcessor {
public:
    SurroundProcessor(ChannelConfiguration config);
    ~SurroundProcessor() = default;
    
    // Channel management
    void SetChannelConfiguration(ChannelConfiguration config);
    ChannelConfiguration GetChannelConfiguration() const { return fChannelConfig; }
    
    // Surround processing
    void ProcessStereoToSurround(const AdvancedAudioBuffer& stereo, AdvancedAudioBuffer& surround);
    void ProcessSurroundToStereo(const AdvancedAudioBuffer& surround, AdvancedAudioBuffer& stereo);
    void ProcessSurroundDownmix(const AdvancedAudioBuffer& input, AdvancedAudioBuffer& output);
    
    // Channel routing and mixing
    void SetChannelGain(size_t channel, float gain);
    float GetChannelGain(size_t channel) const;
    void MuteChannel(size_t channel, bool muted);
    bool IsChannelMuted(size_t channel) const;
    
    // Distance and spatialization
    void SetSpatialPosition(float x, float y, float z);
    void SetListenerPosition(float x, float y, float z);
    
private:
    ChannelConfiguration fChannelConfig;
    std::vector<float> fChannelGains;
    std::vector<bool> fChannelMuted;
    
    // Spatial audio parameters
    float fSourceX{0.0f}, fSourceY{0.0f}, fSourceZ{0.0f};
    float fListenerX{0.0f}, fListenerY{0.0f}, fListenerZ{0.0f};
    
    void InitializeChannelMixing();
    float CalculateChannelDelay(size_t channel);
    float CalculateChannelGain(size_t channel);
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
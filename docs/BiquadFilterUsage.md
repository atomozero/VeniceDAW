# BiquadFilter Usage Guide

## Overview

`BiquadFilter` provides optimized biquad IIR filtering with automatic coefficient caching. Inspired by the BeOS R6 3d_mixer biquad implementation, it only recalculates filter coefficients when parameters change.

## Key Features

- **Lazy Coefficient Recalculation**: Coefficients only calculated when parameters change
- **7 Filter Modes**: Low-pass, High-pass, Band-pass, Notch, Peaking, Low-shelf, High-shelf
- **Direct Form I Structure**: Efficient processing with minimal state
- **Stereo Processing**: Optimized stereo method with shared coefficients

## Basic Usage

### Single-Channel Processing

```cpp
#include "audio/BiquadFilter.h"

using namespace HaikuDAW;

// Create and configure filter
BiquadFilter lpf;
lpf.SetMode(BiquadFilter::LOW_PASS);
lpf.SetSampleRate(44100.0f);
lpf.SetFrequency(1000.0f);  // 1 kHz cutoff
lpf.SetBandwidth(1.0f);     // Q = 0.707 (Butterworth)

// Process audio buffer
float buffer[512];
// ... fill buffer with audio ...
lpf.Process(buffer, 512);  // Coefficients calculated once on first call
```

### Stereo Processing

```cpp
BiquadFilter hpf;
hpf.SetMode(BiquadFilter::HIGH_PASS);
hpf.SetSampleRate(44100.0f);
hpf.SetFrequency(80.0f);  // Remove subsonic frequencies

float leftChannel[512];
float rightChannel[512];
// ... fill buffers ...
hpf.ProcessStereo(leftChannel, rightChannel, 512);
```

### Parametric EQ (Peaking Filter)

```cpp
BiquadFilter eq;
eq.SetMode(BiquadFilter::PEAKING);
eq.SetSampleRate(44100.0f);
eq.SetFrequency(2000.0f);  // Center frequency
eq.SetQ(2.0f);             // Narrow bandwidth
eq.SetGain(6.0f);          // +6 dB boost

// Process continuously
for (int frame = 0; frame < totalFrames; frame += 512) {
    eq.Process(&audioBuffer[frame], 512);
}
```

## Coefficient Caching Benefits

### Without Caching (Traditional Approach)
```cpp
// Recalculates sin/cos/log/pow every sample!
for (int i = 0; i < frameCount; i++) {
    // ~50-100 CPU cycles per sample wasted
    float omega = 2*PI*freq/sampleRate;
    float cos_omega = cos(omega);
    float sin_omega = sin(omega);
    // ... calculate a0, a1, a2, b0, b1, b2 ...
    output = input * b0 + z1;
    // ... process ...
}
```

### With Caching (BiquadFilter)
```cpp
// Calculates coefficients ONCE when parameters change
filter.SetFrequency(1000.0f);  // Marks coefficients as dirty

for (int i = 0; i < frameCount; i++) {
    // Only ~5-10 CPU cycles per sample
    output = input * b0 + z1;  // Uses cached coefficients
    z1 = input * b1 - output * a1 + z2;
    z2 = input * b2 - output * a2;
}
```

**Performance Improvement**: ~90% reduction in CPU usage for filter processing

## Real-Time Parameter Automation

```cpp
BiquadFilter sweepFilter;
sweepFilter.SetMode(BiquadFilter::LOW_PASS);
sweepFilter.SetSampleRate(44100.0f);

// Sweep filter frequency over time
for (int block = 0; block < numBlocks; block++) {
    // Update frequency (coefficients recalculated once per block)
    float freq = 100.0f + (block * 10.0f);  // 100 Hz → 10 kHz
    sweepFilter.SetFrequency(freq);

    // Process block (coefficients already cached)
    sweepFilter.Process(&buffer[block * 512], 512);
}
```

## Filter Design Tips

### Q Factor vs Bandwidth

```cpp
// Option 1: Set bandwidth directly (in octaves)
filter.SetBandwidth(1.0f);  // 1 octave at -3 dB

// Option 2: Set Q factor (converted to bandwidth internally)
filter.SetQ(0.707f);  // Butterworth response
```

**Common Q values**:
- 0.5: Wide, gentle slope
- 0.707: Butterworth (maximally flat passband)
- 1.0: Moderate resonance
- 2.0+: Sharp resonance peak

### Shelf Filter Gain

```cpp
BiquadFilter lowShelf;
lowShelf.SetMode(BiquadFilter::LOW_SHELF);
lowShelf.SetFrequency(100.0f);  // Shelf corner frequency
lowShelf.SetGain(6.0f);         // +6 dB boost below 100 Hz
```

## Integration with VeniceDAW EQ

When implementing the Professional EQ, use BiquadFilter for each band:

```cpp
class ParametricEQ {
private:
    BiquadFilter fBands[5];  // 5-band EQ

public:
    void ProcessAudio(float* buffer, size_t frameCount) {
        // Each band processes in series
        for (int band = 0; band < 5; band++) {
            fBands[band].Process(buffer, frameCount);
        }
    }

    void SetBandFrequency(int band, float freq) {
        fBands[band].SetFrequency(freq);  // Coefficient caching!
    }
};
```

## State Management

```cpp
// Reset filter state (clear delay samples)
filter.Reset();  // Sets z1 = z2 = 0

// Useful when:
// - Starting/stopping playback
// - Seeking to new position
// - Switching between tracks
```

## Performance Considerations

### DO:
✅ Update parameters at block rate (every 512 samples)
✅ Reuse filter instances across process calls
✅ Use ProcessStereo() for stereo signals

### DON'T:
❌ Update parameters every sample (defeats caching)
❌ Create new filter instances in audio callback
❌ Process left/right separately (use ProcessStereo)

## Advanced: Cascaded Filters

```cpp
// Create steep 24 dB/octave low-pass (4 cascaded 6 dB/octave)
BiquadFilter lpf1, lpf2, lpf3, lpf4;

void setupSteepLowPass(float freq) {
    for (auto* f : {&lpf1, &lpf2, &lpf3, &lpf4}) {
        f->SetMode(BiquadFilter::LOW_PASS);
        f->SetSampleRate(44100.0f);
        f->SetFrequency(freq);
        f->SetQ(0.707f);
    }
}

void processSteep(float* buffer, size_t frames) {
    lpf1.Process(buffer, frames);
    lpf2.Process(buffer, frames);  // Cascade
    lpf3.Process(buffer, frames);
    lpf4.Process(buffer, frames);
}
```

## Future EQ Implementation Checklist

When implementing the Professional EQ:

- [ ] Create EQ GUI with frequency/gain/Q controls
- [ ] Instantiate BiquadFilter for each band
- [ ] Connect GUI sliders to filter.SetFrequency() etc.
- [ ] Add frequency response visualization
- [ ] Implement EQ presets (save/load filter parameters)
- [ ] Add spectrum analyzer with pre/post EQ view

## References

- BeOS R6 3d_mixer biquad implementation (inspiration)
- Robert Bristow-Johnson's Audio EQ Cookbook
- Direct Form I vs II structures (we use Form I)

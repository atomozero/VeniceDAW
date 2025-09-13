# CLAUDE.md – VeniceDAW Guidelines

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Vision
VeniceDAW is a Digital Audio Workstation native to **Haiku OS**, built around three key principles:
- **Strudel Graph per track** (intrinsic modularity)  
- **Cortex Integration** (system-level routing)  
- **3D Audio as core feature** (native spatialization)

## Project Overview

VeniceDAW is a professional audio workstation and 3D mixing application built natively for Haiku OS. It's a modern C++ application using native Haiku APIs for audio processing and GUI.

## MVP (Minimum Viable Product)
- **Strudel Graph**  
  - Linear FX chain per track (architecture ready to become full graph)
  - Basic effect slots: gain, EQ, pan

- **Cortex Integration**  
  - VeniceDAW registered as `BMediaNode`
  - Input device → VeniceDAW Track → Master → Output device

- **3D Audio (base)**  
  - 2D panning with distance attenuation
  - Basic parameters per track: L/R position, distance

## Future Roadmap
- **Advanced Strudel**  
  - Graphical editor Max/MSP style
  - Plugin support as nodes (VST, LV2)
  - Free modular routing

- **Advanced Cortex**  
  - Inter-app routing (VeniceDAW ↔ other apps)
  - Track parameters exposed as `BParameter` for external control

- **Advanced 3D Audio**  
  - Open-source HRTF support (SOFA, CIPIC)
  - Multichannel output (5.1, 7.1)
  - Spatial audio for VR/AR

- **AI Integration (optional)**  
  - AI nodes for assisted mixing, mastering and sound design

## Architecture Overview

```ascii
                ┌────────────────────┐
                │   Input Device      │
                └─────────┬───────────┘
                          │
                          ▼
                  ┌─────────────────┐
                  │    Cortex        │
                  │ (System Routing) │
                  └───────┬─────────┘
                          │
                          ▼
                  ┌─────────────────┐
                  │ VeniceDAW Track │
                  └───────┬─────────┘
                          │
                ┌─────────┴───────────┐
                │ Strudel FX Chain     │
                │ (MVP: linear slots)  │
                └─────────┬───────────┘
                          │
                          ▼
                ┌─────────────────────┐
                │  3D Spatial Engine  │
                │ (MVP: 2D panning)   │
                └─────────┬───────────┘
                          │
                          ▼
                  ┌─────────────────┐
                  │  VeniceDAW Master│
                  └───────┬─────────┘
                          │
                          ▼
                  ┌─────────────────┐
                  │    Cortex        │
                  │   (Output Node)  │
                  └───────┬─────────┘
                          │
                          ▼
                ┌────────────────────┐
                │   Output Device     │
                │ (Stereo/Binaural)   │
                └────────────────────┘
```

## Essential Commands

### Build Commands
```bash
make                    # Release build
make DEBUG=1           # Debug build with symbols
make install           # Install to Desktop
make clean             # Clean build artifacts
```

### VeniceDAW Performance Station
```bash
make performance        # Build Performance Station (recommended)
make station           # Same as above (shortcut)
make test-weather      # Test syntax only
```

## Architecture & Key Components

### Audio Engine Architecture
The audio system uses modern Haiku Media Kit APIs:

- **Core Audio**: `play.cpp` - BSoundPlayer-based real-time audio with semaphore synchronization
- **Buffer Management**: Fixed 2048-byte buffers at 44.1kHz stereo
- **Channel System**: `channel.cpp` - Audio channel abstraction layer
- **Track Objects**: `track_obj.cpp` (2,125 lines) - Main audio track management

### DSP Components
Located in root directory, implementing modular signal processing:
- `fft.c`, `fht.c`, `fht2d.c` - Frequency domain transforms
- `biquad.cpp` - Digital filter implementations
- `fftshift.cpp` - FFT utility functions

### GUI Architecture
Multi-window system using Haiku Interface Kit:
- **Main Window**: `wave_window.cpp` - Application entry point
- **Sound Visualization**: `sound_view.cpp` (5,812 lines) - Core UI and waveform display
- **Track View**: `track_view.cpp` (2,947 lines) - Multi-track interface
- **Edit Window**: `edit_window.cpp` - Track editing functionality

## Critical Implementation Details

### Audio API Migration Pattern
When working with audio, use modern Haiku APIs:
```cpp
// Modern pattern (DO use):
BSoundPlayer *player = new BSoundPlayer(&format, "VeniceDAW", PlayBufferFunc);
BMediaFile *file = new BMediaFile(&ref, &file_format);
BMediaTrack *track = file->CreateTrack(&format, &codec_info);

// Legacy pattern (DON'T use):
// BAudioSubscriber, BSubscriber, BDACStream - all deprecated
```

### Configuration Flags
Key settings in `config.h`:
- `MIDI_CONTROLLER`: Set to 0 (MIDI disabled due to deprecated APIs)
- `DEFAULT_SAMPLE_RATE`: 44100
- `DEFAULT_CHANNELS`: 2
- `DEFAULT_BUFFER_SIZE`: 2048

### Build System Dependencies
```makefile
LIBS = -lbe -lmedia -ltracker -lroot
```

### Performance Analysis System
The VeniceDAW Performance Station (formerly UnifiedBenchmark) provides comprehensive audio workstation performance analysis:
- **Class**: `PerformanceStation` (in `src/benchmark/PerformanceStation.h/cpp`)
- **Professional UI**: Ableton Live-inspired interface with weather metaphors
- **Real-time metrics**: CPU, memory, audio latency, 3D rendering performance
- **Build commands**: `make performance` or `make station`

## Development Workflow

### Making Audio Engine Changes
1. Check `play.cpp` for BSoundPlayer implementation
2. Review `channel.cpp` for buffer management
3. Build with `make DEBUG=1` for debugging

### Adding DSP Features
1. DSP modules are standalone C/C++ files in root
2. Follow existing patterns in `fft.c` or `biquad.cpp`
3. Integrate via `sound_view.cpp` or `track_obj.cpp`

### GUI Modifications
1. UI components inherit from BView/BWindow
2. Main coordination happens in `sound_view.cpp`
3. Track-specific UI in `track_view.cpp`
4. Window management in `wave_window.cpp`

## Known Issues & Constraints

- **MIDI Support**: Disabled due to deprecated legacy MIDI APIs
- **Buffer Timing**: Real-time audio requires careful semaphore management in `play.cpp`

## TSoundView 3D GUI Critical Notes

### Required Callbacks for B_FOLLOW_ALL
When using `B_FOLLOW_ALL` in TSoundView constructor, these callbacks are ESSENTIAL:

```cpp
virtual void AttachedToWindow() override {
    BView::AttachedToWindow();
    if (the_bits && the_bits->IsValid()) {
        Animate(Bounds()); // Force initial 3D render
    }
    Invalidate();
}

virtual void FrameResized(float w, float h) override {
    BView::FrameResized(w, h);
    Invalidate(); // Recalculate 3D viewport
}
```

### Critical Draw() Implementation
The Draw() method MUST always draw the_bits bitmap:

```cpp
void TSoundView::Draw(BRect rr) {
    // NEVER use conditional drawing based on rr coordinates!
    // Modern Haiku uses updated rectangle logic
    if (the_bits && the_bits->IsValid()) {
        DrawBitmap(the_bits, BPoint(bm_hp, 0));
    }
}
```

### Color Space Migration
- **OLD**: `B_COLOR_8_BIT` (obsolete in modern Haiku)
- **NEW**: `B_RGB32` (required for proper bitmap rendering)

### Mouse Event Requirements
For mouse events to work properly:
1. Set `B_FOLLOW_ALL` in constructor (not 0)
2. Implement AttachedToWindow() and FrameResized()
3. Ensure TSoundView is on TOP in Z-order (added AFTER track_view)

## Cortex Integration Strategy

### Architectural Synergy

**Cortex** = System audio router (manages connections between media nodes)
**VeniceDAW** = Creative mixer with 3D visualization

The natural integration:
- Cortex handles **routing** (hardware input → VeniceDAW, VeniceDAW → output)
- VeniceDAW handles **creativity** (spatial mixing, effects, automation)

### Optimal Integration Pattern

#### 1. VeniceDAW as specialized media node
- Each VeniceDAW track appears as separate node in Cortex
- Flexible routing: microphones → specific tracks, tracks → headphone monitoring
- 3D parameters (position, distance) exposed as BParameter controllable from Cortex

#### 2. Professional Workflow
```
Hardware Input → Cortex → VeniceDAW Tracks → VeniceDAW Master → Cortex → Output Device
                    ↑                              ↓
              Send/Return effects           Bounce to disk
```

### Integration Benefits

#### For Musicians:
- **Complex setups**: Multi-channel routing with spatial control
- **Live performance**: Cortex for quick routing, VeniceDAW for expressive mixing
- **Recording sessions**: Direct input from multi-channel interfaces

#### For the System:
- **Zero-latency monitoring**: Direct hardware → Cortex → headphones routing
- **Resource sharing**: Single Media Server for everything
- **Natural synchronization**: Shared TimeSource automatically

### Interesting Technical Challenges

#### Compound Latency:
Hardware → Cortex → VeniceDAW → Cortex → Output
Each hop adds ~2-5ms. Solution: bypass routing for direct monitoring.

#### Distributed Control:
- Who manages master volume?
- How to synchronize automation between the two apps?
- Conflicts if both modify the same parameter?

### 3D Mixer as Killer Feature

The most interesting aspect: **spatialization controlled by Cortex**

Possibilities:
- Surround microphones automatically mapped to 3D positions
- Spatial automation via OSC/MIDI controllable from Cortex
- **Virtual soundstage**: each musician in specific 3D position during live recording

### Impact on Haiku Ecosystem

This integration would demonstrate that:
1. **BMediaKit scales** for professional applications
2. **Haiku is credible** as serious audio platform
3. **Modular architecture** enables innovation above solid foundation

The integration could establish **standard patterns** for future native audio applications, making Haiku attractive to professional music software developers.

The key is **not duplicating functionality** but creating **complementary synergy** between routing (Cortex) and creativity (VeniceDAW).

## File Organization

- **Audio Engine**: `play.cpp`, `channel.cpp`, `track_obj.cpp`
- **DSP Modules**: `*.c` files (fft, fht, etc.) in root directory
- **GUI Components**: `*_view.cpp`, `*_window.cpp` files
- **Headers**: Corresponding `.h` files for each component
- **Build**: `Makefile` for Haiku native environment
- **Testing**: Native Haiku testing framework

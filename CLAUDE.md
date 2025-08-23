# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

HaikuMix is a 3D audio mixing application ported from BeOS R6 to modern Haiku OS. It's a C++ application using native Haiku APIs for audio processing and GUI.

## Essential Commands

### Build Commands
```bash
make                    # Release build
make DEBUG=1           # Debug build with symbols
make install           # Install to Desktop
make clean             # Clean build artifacts
```

### Cross-Platform Development
```bash
make -f Makefile.cross syntax-check    # Validate syntax without Haiku
./run_cross_tests.sh                   # Run automated test suite
```

### VeniceDAW Performance Station
```bash
make performance        # Build Performance Station (recommended)
make station           # Same as above (shortcut)
make test-weather      # Test syntax only
```

### Testing
```bash
./run_cross_tests.sh    # Runs 8 comprehensive tests for cross-compilation
```

## Architecture & Key Components

### Audio Engine Architecture
The audio system has been modernized from deprecated BeOS APIs to current Haiku Media Kit:

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
BSoundPlayer *player = new BSoundPlayer(&format, "HaikuMix", PlayBufferFunc);
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
3. Test with `./run_cross_tests.sh` for syntax validation
4. Build with `make DEBUG=1` for debugging

### Adding DSP Features
1. DSP modules are standalone C/C++ files in root
2. Follow existing patterns in `fft.c` or `biquad.cpp`
3. Integrate via `sound_view.cpp` or `track_obj.cpp`

### GUI Modifications
1. UI components inherit from BView/BWindow
2. Main coordination happens in `sound_view.cpp`
3. Track-specific UI in `track_view.cpp`
4. Window management in `wave_window.cpp`

## Testing Strategy

The project uses cross-compilation testing to validate code without a full Haiku environment:

1. **Syntax Check**: `make -f Makefile.cross syntax-check`
2. **Full Test Suite**: `./run_cross_tests.sh` runs:
   - Syntax validation
   - Header dependency checks
   - API structure verification
   - Mock header generation

## Known Issues & Constraints

- **MIDI Support**: Disabled due to deprecated BeOS MIDI APIs
- **Cross-Compilation**: Some features only testable on actual Haiku system
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
    // Modern Haiku uses different rectangle logic than BeOS
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

**Cortex** = Router audio di sistema (gestisce connessioni tra nodi media)
**HaikuMix** = Mixer creativo con visualizzazione 3D

L'integrazione naturale:
- Cortex gestisce **routing** (input hardware → HaikuMix, HaikuMix → output)
- HaikuMix gestisce **creatività** (mixaggio spaziale, effetti, automazione)

### Pattern di integrazione ottimale

#### 1. HaikuMix come nodo media specializzato
- Ogni track di HaikuMix appare come nodo separato in Cortex
- Routing flessibile: microfoni → track specifici, track → monitor cuffie
- Parametri 3D (posizione, distanza) esposti come BParameter controllabili da Cortex

#### 2. Workflow professionale
```
Hardware Input → Cortex → HaikuMix Tracks → HaikuMix Master → Cortex → Output Device
                    ↑                              ↓
              Send/Return effects           Bounce to disk
```

### Vantaggi dell'integrazione

#### Per i musicisti:
- **Setup complessi**: Routing multicanale con controllo spaziale
- **Live performance**: Cortex per routing veloce, HaikuMix per mixaggio espressivo
- **Recording session**: Input diretto da interfacce multi-canale

#### Per il sistema:
- **Zero-latency monitoring**: Routing diretto hardware → Cortex → cuffie
- **Condivisione risorse**: Un solo Media Server per tutto
- **Sincronizzazione naturale**: TimeSource condivisa automaticamente

### Sfide tecniche interessanti

#### Latenza composta:
Hardware → Cortex → HaikuMix → Cortex → Output
Ogni hop aggiunge ~2-5ms. Soluzione: routing bypass per monitoring diretto.

#### Controllo distribuito:
- Chi gestisce il master volume?
- Come sincronizzare automazione tra le due app?
- Conflitti se entrambe modificano lo stesso parametro?

### Il mixer 3D come killer feature

L'aspetto più interessante: **spazializzazione controllata da Cortex**

Possibilità:
- Microfoni surround mappati automaticamente su posizioni 3D
- Automazione spaziale via OSC/MIDI controllabile da Cortex
- **Virtual soundstage**: ogni musicista in posizione 3D specifica durante live recording

### Impatto sull'ecosistema Haiku

Questa integrazione dimostrerebbe che:
1. **BMediaKit scala** per applicazioni professionali
2. **Haiku è credibile** come piattaforma audio seria
3. **Architettura modulare** permette innovazione sopra base solida

L'integrazione potrebbe stabilire **pattern standard** per future applicazioni audio native, rendendo Haiku attraente per sviluppatori di software musicale professionale.

La chiave è **non duplicare funzionalità** ma creare **sinergia complementare** tra routing (Cortex) e creatività (HaikuMix).

## File Organization

- **Audio Engine**: `play.cpp`, `channel.cpp`, `track_obj.cpp`
- **DSP Modules**: `*.c` files (fft, fht, etc.) in root directory
- **GUI Components**: `*_view.cpp`, `*_window.cpp` files
- **Headers**: Corresponding `.h` files for each component
- **Build**: `Makefile`, `Makefile.cross` for different environments
- **Testing**: `run_cross_tests.sh`, `tests/` directory
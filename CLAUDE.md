# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

VeniceDAW is a professional audio workstation and 3D mixing application built natively for Haiku OS. It's a modern C++ application using native Haiku APIs for audio processing and GUI.

## Essential Commands

### Build Commands (Haiku OS Native Only)
```bash
# Primary build system (use this!)
make -f Makefile.haiku           # Complete VeniceDAW with all features
make -f Makefile.haiku clean     # Clean build artifacts
make -f Makefile.haiku install   # Install to Desktop

# Legacy Makefile (older, less maintained)
make                             # Also builds complete VeniceDAW
make DEBUG=1                     # Debug build with symbols
```

### BeOS 3dmix Import System (Phase 6.3)
```bash
# Build 3dmix import support
make -f Makefile.3dmix all       # Build complete 3dmix import system
make -f Makefile.3dmix test      # Run 3dmix import tests

# Test 3dmix project import
./test_3dmix_complete /path/to/project.3dmix

# Individual component tests
make -f Makefile.3dmix test-parser      # Test BMessage parser
make -f Makefile.3dmix test-coordinates # Test coordinate conversion
make -f Makefile.3dmix test-paths       # Test audio file path resolution
```

### Syntax Validation (WSL/Linux - NO compilation)
```bash
./scripts/validate_syntax.sh    # Complete syntax validation (analysis only)
# IMPORTANT: This does NOT compile - only validates code structure
```

### Testing (Haiku OS Native Only)
```bash
# Complete test suite
./scripts/run_tests.sh          # Run all automated tests

# Audio processing tests
make -f Makefile.haiku test-spatial   # Spatial audio processing (57 tests)
make -f Makefile.haiku test-eq        # Professional EQ (parametric)
make -f Makefile.haiku test-dynamics  # Dynamics processing
make -f Makefile.haiku test-binaural  # HRTF binaural rendering

# Integration tests
./test_audio_playback    # Audio file playback and mixing
./test_3d_mixer          # 3D visualization and camera controls
./test_phase4_2_hrtf     # HRTF & binaural interface (8 tests)

# 3dmix import tests
./test_3dmix_complete    # Complete 3dmix import validation
```

### Performance Analysis
```bash
make performance         # Build Performance Station
make station            # Same as above (shortcut)
./VeniceDAWBenchmark    # Run performance analysis
```

## Current Development Status

**Phase 6.3 Complete**: BeOS 3dmix Heritage Import System
- Historic BeOS R5 (1995-2001) project file support
- Complete BMessage format parser for .3dmix files
- 5 coordinate conversion algorithms (Direct, Spherical, Cylindrical, Normalized, Ambisonics)
- Intelligent audio path resolution with 4 search strategies (90%+ success rate)
- RAW audio format detection and conversion
- Professional import UI with 3D coordinate preview
- 100+ comprehensive validation tests

**Phase 7 Planned**: Professional workflow features (session management, automation, effects rack, plugin architecture)

## Architecture & Key Components

### Audio Engine Architecture
The audio system uses modern Haiku Media Kit APIs:

- **Core Audio**: `SimpleHaikuEngine.cpp` - BSoundPlayer-based real-time audio with file playback
- **File Loading**: BMediaFile/BMediaTrack integration for WAV, AIFF, MP3, OGG formats
- **Multi-Track Mixing**: Up to 54 simultaneous tracks with proper gain staging
- **Buffer Management**: Professional-grade audio pipeline with <10ms latency
- **Advanced Processing**: `AdvancedAudioProcessor.cpp` - 6-band EQ, dynamics, spatial effects, HRTF

### 3dmix Import System (Phase 6.3)
BeOS heritage audio project import located in `src/audio/3dmix/`:
- **3DMixParser**: BMessage format decoder for BeOS .3dmix files
- **CoordinateSystemMapper**: 5 conversion algorithms for spatial positioning
- **AudioPathResolver**: Intelligent file location with multi-strategy search
- **3DMixProjectImporter**: Complete import pipeline with validation
- **3DMixImportDialog**: Professional UI with 3D coordinate preview

### GUI Architecture
Multi-window system using Haiku Interface Kit with OpenGL:
- **3D Mixer**: `Mixer3DWindow.cpp` - Revolutionary 3D visualization with OpenGL
- **Spatial Controls**: `SpatialMixer3DWindow.cpp` - Professional positioning interface
- **Transport Window**: Play/stop/record controls with timeline
- **Performance Station**: `PerformanceStationWindow.cpp` - Real-time metrics
- **Camera System**: Zoom (+/-), rotation (mouse), reset (R key)

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
LIBS = -lbe -lmedia -ltracker -lroot -lGL -lGLU -ltranslation
# -lGL -lGLU: Required for 3D visualization
# -ltranslation: Required for 3dmix import (audio format conversion)
```

### Key Source Directories
```
src/audio/           # Audio engine and processing
  SimpleHaikuEngine.cpp      # Core audio playback
  AdvancedAudioProcessor.cpp # DSP effects and HRTF
  3dmix/                     # BeOS 3dmix import system
src/gui/             # GUI windows and controls
  Mixer3DWindow.cpp          # 3D visualization
  SpatialMixer3DWindow.cpp   # Spatial controls
  3DMixImportDialog.cpp      # 3dmix import UI
src/benchmark/       # Performance analysis tools
src/testing/         # Test suites and validation
scripts/             # Build and test automation
```

### Performance Analysis System
The VeniceDAW Performance Station (formerly UnifiedBenchmark) provides comprehensive audio workstation performance analysis:
- **Class**: `PerformanceStation` (in `src/benchmark/PerformanceStation.h/cpp`)
- **Professional UI**: Ableton Live-inspired interface with weather metaphors
- **Real-time metrics**: CPU, memory, audio latency, 3D rendering performance
- **Build commands**: `make performance` or `make station`

## Development Workflow

### Making Audio Engine Changes
1. Core audio: Modify `src/audio/SimpleHaikuEngine.cpp` (BSoundPlayer implementation)
2. DSP effects: Update `src/audio/AdvancedAudioProcessor.cpp` (EQ, dynamics, HRTF)
3. Syntax check: Run `./scripts/validate_syntax.sh` (WSL/Linux)
4. Build/test: Transfer to Haiku, run `make -f Makefile.haiku`

### Adding DSP Features
1. Implement algorithms in `src/audio/DSPAlgorithms.cpp`
2. Integrate into `AdvancedAudioProcessor` class
3. Add test cases in `src/testing/` directory
4. Validate with dedicated test targets (e.g., `make test-eq`, `make test-spatial`)

### GUI Modifications
1. 3D visualization: `src/gui/Mixer3DWindow.cpp` (OpenGL)
2. Spatial controls: `src/gui/SpatialMixer3DWindow.cpp`
3. Dialogs/UI: Follow BWindow/BView patterns
4. All GUI requires native Haiku - no cross-platform build

### Working with 3dmix Import
1. Parser logic: `src/audio/3dmix/3DMixParser.cpp`
2. Coordinate conversion: `src/audio/3dmix/CoordinateSystemMapper.cpp`
3. Path resolution: `src/audio/3dmix/AudioPathResolver.cpp`
4. Test with: `make -f Makefile.3dmix test`

## Testing Strategy

**CRITICAL: NEVER TRY TO COMPILE OR EXECUTE VeniceDAW IN WSL/Linux ENVIRONMENTS**

VeniceDAW is a NATIVE Haiku OS application and MUST be compiled on real Haiku systems only.

**NOTA FONDAMENTALE PER CLAUDE CODE:**
- NON eseguire MAI comandi di build come `make`, `g++`, o eseguibili come `./ProfessionalEQTest` in ambiente WSL
- NON tentare MAI di compilare o eseguire il codice VeniceDAW al di fuori di Haiku nativo
- I test e l'esecuzione del codice devono avvenire ESCLUSIVAMENTE su sistemi Haiku reali
- WSL può essere usato SOLO per editing di codice e analisi statica, mai per compilazione o esecuzione

### Syntax Validation (WSL/Linux)
For syntax validation only on non-Haiku systems:
```bash
./scripts/validate_syntax.sh    # Complete syntax validation
# Checks file structure, bracket matching, basic syntax
# Does NOT compile or execute any code
```

### Real Compilation (Haiku Only)
All actual compilation MUST be done on native Haiku:
```bash
# ON HAIKU SYSTEM ONLY:
make -f Makefile.haiku          # Complete build (recommended)
make -f Makefile.haiku test-all # Run all tests
make performance                # Build Performance Station
make -f Makefile.3dmix all      # Build 3dmix import system
```

**Why this restriction exists:**
1. VeniceDAW uses native Haiku BeAPI calls that don't exist on Linux
2. Audio system requires real BSoundPlayer and BMediaKit APIs
3. GUI components need genuine BView/BWindow classes
4. Thread management uses Haiku-specific scheduler APIs

**Mock headers in `src/testing/HaikuMockHeaders.h` are ONLY for syntax checking - they don't provide real functionality.**

## Known Issues & Constraints

- **MIDI Support**: Disabled due to deprecated legacy MIDI APIs
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

## Future Integration: Cortex Audio Router

VeniceDAW is designed to eventually integrate with Haiku's Cortex audio router:
- **Cortex role**: System-level audio routing (hardware I/O connections)
- **VeniceDAW role**: Creative mixing with 3D spatial positioning
- **Integration pattern**: Each VeniceDAW track as separate BMediaNode
- **Benefit**: Professional routing flexibility with innovative spatial control

Implementation planned for Phase 8 (Advanced Integration).
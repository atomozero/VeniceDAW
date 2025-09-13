# VeniceDAW

A digital audio workstation designed specifically for Haiku OS, featuring native 3D spatial mixing and professional performance monitoring.

## 🚧 Project Status: Early Prototype

**Important:** VeniceDAW is currently in an early prototype phase. This is experimental software with many features still in development. Much work remains before it becomes a fully usable DAW. Consider this a technical preview and proof of concept rather than production-ready software.

## Vision

VeniceDAW brings professional audio production capabilities to Haiku OS, built around three core principles:

- **🔗 Strudel Graph per track**: Modular FX chains ready to evolve into full node-based processing
- **🎛️ Cortex Integration**: Deep system-level routing through Haiku's BMediaKit architecture  
- **🌐 3D Audio as core feature**: Native spatial mixing with binaural processing and HRTF support

## Overview

Built from the ground up using Haiku's native BMediaKit, VeniceDAW provides powerful audio processing with intelligent system monitoring. The application showcases what's possible when software is designed specifically for Haiku's unique architecture rather than being ported from other platforms.

## Current Status: Performance Station

The **Performance Station** is the first component ready for use - a professional benchmark system that evaluates your Haiku system's audio processing capabilities using clean, intuitive interfaces.

### Features

- **Real-time Performance Monitoring**: CPU, memory, audio, and I/O metrics
- **Professional Interface**: Ableton Live-inspired design with clean meters and controls
- **Audio-Specific Testing**: Latency measurement, buffer processing, sine generation
- **System Analysis**: Multi-threaded CPU scaling, memory bandwidth testing
- **Export Reports**: Professional benchmark reports for system optimization

## Installation

### From HaikuDepot (Recommended)
VeniceDAW will be available through HaikuDepot for easy installation:

1. Open HaikuDepot
2. Search for "VeniceDAW"  
3. Click "Install"

*Note: Package submission to HaikuDepot is in progress*

### Manual Build

### Requirements
- Haiku OS (R1/Beta4 or later recommended)
- GCC development tools
- BMediaKit headers (included with Haiku)
- GLU development library

#### Installing Dependencies
```bash
# Install GLU development package (required for 3D visualization)
pkgman install glu_devel
```

### Compilation

```bash
# Build the full VeniceDAW application (with mixer UI)
make gui
./VeniceDAWGUI

# Build the Performance Station benchmark tool  
make performance  # or 'make station' for shortcut
./VeniceDAWBenchmark

# Test native Haiku audio engine
make native
./VeniceDAWNative
```

## Architecture

VeniceDAW is built on several core principles:

- **Native Haiku APIs**: Full integration with BMediaKit, BWindow, BView, Interface Kit
- **Real-time Audio**: Low-latency processing designed for professional use with BSoundPlayer
- **Modular Design**: Components designed around Strudel Graph architecture
- **Cortex Integration**: Built to work seamlessly with Haiku's system audio routing

### Core Components

- **Audio Engines**: 
  - `SimpleHaikuEngine`: Lightweight BSoundPlayer implementation
  - `HaikuAudioEngine`: Full BMediaKit integration with Cortex compatibility
- **GUI Applications**:
  - **VeniceDAW GUI**: Full mixer with 3D spatial visualization
  - **Performance Station**: Professional benchmark and monitoring
- **3D Spatial Engine**: Revolutionary spatial audio interface with OpenGL and binaural processing

### MVP Architecture Flow

```
Input Device → Cortex → VeniceDAW Tracks → 3D Spatial Engine → Master → Cortex → Output
```

Each track features:
- Linear Strudel FX chain (gain, EQ, pan)
- 3D spatial parameters (position, distance)  
- Integration with Cortex as BMediaNode

## Development

### Project Structure
```
src/
├── audio/              # Audio engines (SimpleHaikuEngine, HaikuAudioEngine)
├── benchmark/          # Performance Station testing suite  
├── gui/                # All UI components (windows, views)
├── main_*.cpp          # Various entry points
└── benchmark/tests/    # Modular performance tests

DSP modules/            # Root-level DSP components (fft.c, biquad.cpp)
```

### Build Targets

VeniceDAW provides multiple entry points for different use cases:

```bash
make gui            # Full VeniceDAW with 3D mixer (main_gui.cpp)
make performance    # Performance Station benchmark (main_performance_station.cpp)  
make native         # Native Haiku audio test (main_simple_native.cpp)
make clean          # Clean build files
make help           # Show all targets
```

#### Entry Points Explained

- **`main_gui.cpp`**: Full DAW with multiple mixer windows and 3D spatial visualization
- **`main_performance_station.cpp`**: Professional performance analysis tool
- **`main_simple_native.cpp`**: Native Haiku audio engine test application

## Roadmap

### Phase 1: Foundation ✓
- [x] Performance benchmark system
- [x] Native Haiku audio engine
- [x] Professional UI framework

### Phase 2: Core DAW (In Progress)
- [x] Multi-track mixer interface
- [x] Real-time mixing controls with 3D visualization
- [x] Basic mixer architecture (8 tracks per window)
- [x] Strudel Graph foundation (linear FX chains)
- [ ] Audio file I/O with BMediaFile
- [ ] Basic Cortex integration as BMediaNode

### Phase 3: Professional Features
- [ ] Advanced Strudel Graph (node-based editor)
- [ ] Full Cortex integration (inter-app routing)
- [ ] Advanced 3D Audio (HRTF, multichannel)
- [ ] Plugin architecture (native Haiku plugins)
- [ ] Session management
- [ ] Hardware controller support

### Phase 4: Advanced Features
- [ ] AI-assisted mixing nodes
- [ ] VR/AR spatial audio support
- [ ] Advanced Cortex parameter exposure
- [ ] Professional spatial recording workflows

## Contributing

Contributions are welcome! VeniceDAW aims to demonstrate the potential of native Haiku development for professional audio applications.

### Getting Started
1. Check the [issues](../../issues) for current tasks
2. Read `CLAUDE.md` for development guidelines and vision
3. Build with `make DEBUG=1` for development
4. Submit PR with clear description

### Areas Needing Help
- Native Haiku BMediaKit optimizations
- Cortex integration implementation
- 3D spatial audio algorithms  
- UI/UX improvements for the 3D mixer
- Documentation and examples
- Testing on various Haiku hardware configurations

## Technical Highlights

### Native Haiku Audio APIs
```cpp
// Modern BSoundPlayer implementation
BSoundPlayer *player = new BSoundPlayer(&format, "VeniceDAW", PlayBufferFunc);
BMediaFile *file = new BMediaFile(&ref, &file_format);
BMediaTrack *track = file->CreateTrack(&format, &codec_info);
```

### 3D Spatial Processing
- Real-time binaural processing
- Distance-based attenuation
- HRTF-ready architecture
- OpenGL visualization

### Performance Optimized
- Fixed 2048-byte buffers at 44.1kHz
- Semaphore-synchronized real-time audio
- Multi-threaded DSP processing
- Professional-grade latency management

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Acknowledgments

- **Haiku OS Team**: For creating an amazing platform for native development
- **Haiku Media Kit**: The foundation that makes elegant audio programming possible
- **Cortex**: The inspiration for system-level audio routing integration
- **Venice**: The floating city that inspired the name and the engineering mindset

---

*Built with ❤️ exclusively for Haiku OS*

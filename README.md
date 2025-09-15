# VeniceDAW

A digital audio workstation designed specifically for Haiku OS, featuring native 3D spatial mixing and professional performance monitoring.

## 🚧 Project Status: Early Prototype

**Important:** VeniceDAW is currently in an early prototype phase. This is experimental software with many features still in development. Much work remains before it becomes a fully usable DAW. Consider this a technical preview and proof of concept rather than production-ready software.

## Vision

VeniceDAW brings professional audio production capabilities to Haiku OS, built around three core principles:

- **🔗 Modular Architecture**: Clean, extensible audio processing chains
- **🎛️ Native Integration**: Deep integration with Haiku's BMediaKit audio system
- **🌐 3D Audio Processing**: Spatial mixing with OpenGL visualization

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

### Quick Start

The easiest way to get started is building the demo application:

### Manual Build

### Requirements
- Haiku OS (R1/Beta4 or later recommended)
- GCC development tools
- BMediaKit headers (included with Haiku)
- GLU development library

#### Development Environment Setup

**Cross-platform Development:**
VeniceDAW is designed specifically for Haiku OS. If you're developing from another platform, you'll need to transfer your source code to your Haiku system for compilation.

#### Installing Dependencies
```bash
# Update package system
pkgman update

# Install development tools
pkgman install gcc make cmake

# Install Haiku development headers
pkgman install haiku_devel

# Install media development packages
pkgman install media_kit_devel

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

### Audio Processing Flow

```
Audio Input → VeniceDAW Mixer → 3D Spatial Processing → Audio Output
```

Each track provides:
- Basic audio effects (gain, EQ, pan)
- 3D spatial positioning
- Real-time visualization

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

Main applications you can build:

```bash
make demo           # Simple audio demo (default)
make native         # Native Haiku audio engine test
make gui            # Full mixer with 3D visualization
make performance    # Performance benchmark suite
make clean          # Clean build files
make help           # Show all targets
```

#### Getting Started

1. **Start with the demo**: `make demo && ./VeniceDAWDemo`
2. **Test native audio**: `make native && ./VeniceDAWNative`
3. **Try the full GUI**: `make gui && ./VeniceDAWGUI`
4. **Run benchmarks**: `make performance`

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

## Development Tips

### Debugging Audio Issues
If you encounter audio problems on Haiku:
```bash
# Check audio system status
/system/servers/media_addon_server --debug

# Verify audio devices
listdev | grep audio

# Test basic audio functionality
/bin/SoundPlayer /system/data/sounds/Startup.wav
```

### Performance Optimization
For best development experience:
- Use Haiku R1/Beta4 or later for optimal BMediaKit support
- Ensure sufficient RAM (4GB+ recommended for complex projects)
- Close unnecessary applications when testing real-time audio

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

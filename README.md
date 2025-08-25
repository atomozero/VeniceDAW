# VeniceDAW

A digital audio workstation designed specifically for Haiku OS, featuring intuitive performance monitoring.

## 🚧 Project Status: Early Prototype

**Important:** VeniceDAW is currently in an early prototype phase. This is experimental software with many features still in development. Much work remains before it becomes a fully usable DAW. Consider this a technical preview and proof of concept rather than production-ready software.

## Overview

VeniceDAW aims to bring professional audio production capabilities to Haiku OS with a focus on real-time performance and system efficiency. Built from the ground up using Haiku's native BMediaKit, it will provide both powerful audio processing and intelligent system monitoring.

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

# Or build the Performance Station benchmark tool
make performance
./VeniceDAWBenchmark

# For cross-platform testing
make demo
./VeniceDAWDemo
```

## Architecture

VeniceDAW is built on several core principles:

- **Native Haiku APIs**: Full integration with BMediaKit, BWindow, BView
- **Real-time Audio**: Low-latency processing designed for professional use
- **Modular Design**: Components can be developed and tested independently
- **Cross-compilation**: Syntax validation possible on other platforms

### Core Components

- **Audio Engines**: 
  - `SimpleHaikuEngine`: Lightweight BSoundPlayer implementation
  - `HaikuAudioEngine`: Full BMediaKit integration
  - `AudioEngineSimple`: Cross-platform testing engine
- **GUI Applications**:
  - **VeniceDAW GUI**: Full mixer with 3D visualization
  - **Performance Station**: Professional benchmark and monitoring
- **3D Mixer**: Revolutionary spatial audio interface with OpenGL

## Development

### Project Structure
```
src/
├── audio/              # Audio engines (SimpleHaikuEngine, HaikuAudioEngine)
├── benchmark/          # Performance Station testing suite  
├── gui/                # All UI components (windows, views)
├── main_*.cpp          # Various entry points
└── benchmark/tests/    # Modular performance tests

tests/                  # Test files and utilities
resources/              # Icons, presets, themes (planned)
plugins/                # Plugin architecture (planned)
```

### Build Targets

```bash
make gui            # Full VeniceDAW application with 3D mixer
make performance    # Performance Station benchmark tool
make demo           # Cross-platform demo (for testing)
make native         # Native Haiku audio engine test
make clean          # Clean build files
make help           # Show all targets
```

## Roadmap

### Phase 1: Foundation ✓
- [x] Performance benchmark system
- [x] Native Haiku audio engine
- [x] Professional UI framework

### Phase 2: Core DAW (In Progress)
- [x] Multi-track mixer interface
- [x] Real-time mixing controls with 3D visualization
- [x] Basic mixer architecture (8 tracks per window)
- [ ] Audio file I/O
- [ ] Effects processing

### Phase 3: Professional Features
- [ ] Plugin architecture (VST support)
- [ ] MIDI integration
- [ ] Advanced 3D mixing
- [ ] Session management
- [ ] Hardware controller support

## Contributing

Contributions are welcome! VeniceDAW aims to demonstrate the potential of native Haiku development for professional audio applications.

### Getting Started
1. Check the [issues](../../issues) for current tasks
2. Read `CLAUDE.md` for development guidelines
3. Test your changes with `make test-compile`
4. Submit PR with clear description

### Areas Needing Help
- Haiku-specific optimizations
- Audio driver compatibility
- UI/UX improvements
- Documentation and examples
- Testing on various hardware configurations

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Acknowledgments

- **Haiku OS Team**: For creating an amazing platform for native development
- **Haiku Media Kit**: The foundation that makes elegant audio programming possible
- **Venice**: The floating city that inspired the name and the engineering mindset

---

*Built with ❤️ for the Haiku community*
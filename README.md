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
# Build the performance station (recommended)
make performance

# Run the performance station
./VeniceDAWBenchmark
```

## Architecture

VeniceDAW is built on several core principles:

- **Native Haiku APIs**: Full integration with BMediaKit, BWindow, BView
- **Real-time Audio**: Low-latency processing designed for professional use
- **Modular Design**: Components can be developed and tested independently
- **Cross-compilation**: Syntax validation possible on other platforms

### Core Components

- **Audio Engine**: `SimpleHaikuEngine` using `BSoundPlayer` for real-time audio
- **Performance Monitor**: Real-time system metrics with professional visualization
- **Benchmark Suite**: Comprehensive audio workstation performance testing
- **3D Mixer**: Spatial audio positioning (in development)

## Development

### Project Structure
```
src/
├── audio/              # Audio engine implementation
├── benchmark/          # Performance testing suite  
├── gui/               # User interface components
└── utils/             # Shared utilities

resources/             # Icons, presets, themes
docs/                  # Architecture and development guides
```

### Build Targets

```bash
make performance      # Performance station (recommended)
make station         # Same as above (shortcut)
make test-weather    # Syntax validation
make clean          # Clean build files
make help           # Show all targets
```

## Roadmap

### Phase 1: Foundation ✓
- [x] Performance benchmark system
- [x] Native Haiku audio engine
- [x] Professional UI framework

### Phase 2: Core DAW (In Progress)
- [ ] Multi-track audio interface
- [ ] Real-time mixing controls
- [ ] Audio file I/O
- [ ] Basic effects processing

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
3. Test your changes with `make test-weather`
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
- **BeOS Legacy**: The foundation that makes elegant audio programming possible
- **Venice**: The floating city that inspired the name and the engineering mindset

---

*Built with ❤️ for the Haiku community*
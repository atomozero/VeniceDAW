# VeniceDAW - HaikuDepot Package Information

## Package Details

- **Package Name**: `veniceDAW`
- **Category**: AudioVideo
- **Architecture Support**: All architectures except x86_gcc2
- **License**: MIT
- **Homepage**: https://github.com/atomozero/VeniceDAW

## Installation

### From HaikuDepot
Once published, VeniceDAW will be available through HaikuDepot:

1. Open HaikuDepot
2. Search for "VeniceDAW" 
3. Click "Install"

### Manual Installation from Recipe
For development builds or early access:

```bash
# Clone HaikuPorts repository
git clone https://github.com/haikuports/haikuports.git
cd haikuports

# Copy recipe to appropriate location
cp /path/to/veniceDAW.recipe media-sound/veniceDAW/
cd media-sound/veniceDAW

# Build and install
haikuporter veniceDAW
pkgman install veniceDAW
```

## Package Contents

After installation, VeniceDAW provides:

### Applications
- **VeniceDAW**: Main DAW application (`/boot/system/apps/VeniceDAW/VeniceDAW`)
- **Performance Station**: Professional benchmark tool (`/boot/system/apps/VeniceDAW/PerformanceStation`)

### Desktop Integration  
- **Deskbar Menu**: Both apps appear in Applications > AudioVideo
- **Application Signatures**: Proper MIME types for file associations
- **System Icons**: Native Haiku vector icons (when available)

### Documentation
- **README.md**: Project overview and build instructions
- **Performance Guide**: Weather benchmark system documentation
- **Architecture Guide**: Technical implementation details

## Dependencies

VeniceDAW requires:
- **Haiku**: R1/Beta4 or later
- **OpenGL**: For 3D mixer visualization
- **BMediaKit**: Native audio processing (included with Haiku)

## Build Information

The package is built using:
- Native Haiku BMediaKit APIs
- Modern C++17 standards
- OpenGL for 3D rendering
- Professional-grade real-time audio processing

## Version History

### Version 1.0 (Initial Release)
- Professional Performance Station with weather metaphor UI
- Real-time audio performance analysis
- Ableton Live-inspired interface design
- Native Haiku integration with BMediaKit
- Export functionality for performance reports

## Support

- **GitHub Issues**: https://github.com/atomozero/VeniceDAW/issues
- **Documentation**: https://github.com/atomozero/VeniceDAW
- **Community**: Haiku community forums and channels

## Developer Notes

### Package Validation
The recipe includes comprehensive testing:
```bash
# Syntax validation without full Haiku environment  
make test-weather

# Full compilation on Haiku systems
make performance
```

### Future Enhancements
- VST plugin support
- MIDI integration
- Additional performance metrics
- Enhanced 3D spatial audio features
- Session management capabilities

---

*VeniceDAW: Professional audio production for the Haiku community* 🎵
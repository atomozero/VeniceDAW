# Changelog - VeniceDAW 3DMix Viewer

## Alpha 0.1 "CD-ROM Edition" - November 2025

### 🎉 First Release!

This is the first public alpha of the 3DMix viewer, born from a random CD-ROM find.

### ✨ Features

- **3DMix Project Loading**: Successfully reverse engineered the BeOS BMessage format
  - BIG ENDIAN byte order (because PowerPC was king in 1998)
  - Custom float representation for 3D coordinates
  - Relative audio file paths (with intelligent resolver)

- **Audio Engine**:
  - BSoundPlayer integration with proper sample rate detection
  - Automatic byte swapping for big-endian → little-endian conversion
  - 22050 Hz "secret" sample rate (despite files claiming 44100 Hz)
  - Multi-track spatial mixing with ITD (Interaural Time Difference)

- **3D Visualization**:
  - OpenGL-based spatial audio positioning
  - Software rendering fallback (llvmpipe)
  - Camera controls (rotate, zoom, reset)
  - Color-coded tracks based on 3D position

- **Timeline Window**:
  - On-the-fly waveform rendering (BeOS R6 style!)
  - Per-track Mute/Solo controls
  - Time ruler with proper scale
  - Track positioning using StartPosition/EndPosition

- **Professional UI**:
  - Haiku MediaPlayer-style control bar
  - Master VU meter (L/R with adaptive peak scaling)
  - Master volume control (0-200%)
  - Play/Pause/Stop transport controls

### 🐛 Known Issues

- **Performance**: Software rendering is SLOW (10 FPS throttling applied)
- **VU Meter**: Scale was adjusted 3 times to avoid "stuck in red zone"
- **Audio Paths**: Sometimes audio files are not found (path resolution is tricky)
- **Loop Points**: Visual loop markers work, but may differ from original
- **Timeline**: Some tracks may appear misaligned (still investigating positioning algorithm)

### 🔧 Technical Fixes Applied

- Fixed byte order swapping for int16 samples (big-endian → little-endian)
- Corrected sample rate detection (44100 Hz header → 22050 Hz actual)
- Implemented adaptive VU meter scaling (was 12x, now dynamic up to 6x)
- Fixed TimelineWindow crash due to uninitialized pointer
- Corrected track positioning in timeline using StartPosition samples

### 📝 Reverse Engineering Notes

All discoveries made through:
- Hex dump analysis of .3dmix files
- Comparison with BeOS R5 BMessage documentation fragments
- Audio waveform analysis with Audacity
- Trial and error with sample rate conversions
- Pattern recognition in binary data structures

No original source code was available. Everything was reconstructed from:
- Binary file analysis
- Internet Archive documentation scraps
- Haiku BeAPI documentation
- Pure determination and Spritz Cinar

### 🙏 Thanks To

- The unknown person who sold us that CD-ROM
- Internet Archive for preserving BeOS documentation fragments
- Haiku OS community for keeping BeAPI alive
- Italian Haiku Community for tolerating this crazy project
- Genio developers for creating a powerful IDE for Haiku
- Spritz Cinar and Tiramisu (vital for late-night debugging)

---

## Future Plans (Maybe)

- Beta 0.2: Better audio path resolution
- Beta 0.3: Hardware-accelerated rendering (when we find a GPU)
- Beta 0.4: Export to modern formats (WAV, FLAC, etc.)
- 1.0: Full BeOS 3D Mixer compatibility (if we ever figure it out completely)

---

*"If it compiles, ship it!" - Ancient Developer Proverb*

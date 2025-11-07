# VeniceDAW 3DMix Viewer - Alpha 0.1
## "When Digital Archaeology Meets Spatial Audio"

### 🎭 The Story (Mostly True)

It all started with a CD-ROM found at a flea market in Padua, Italy, in 2024.
Yes, a CD. Hand-labeled: "BeOS R5 - Audio Demo 2001". Yes, it miraculously still worked.

Inside: mysterious `.3dmix` files from BeOS R5 (1995-2001), an operating system that
most people consider "that thing Apple DIDN'T buy before Mac OS X".

### 🔬 The Reverse Engineering (AKA: Why We're Masochists)

After 23 Spritz at Cinar, 12 slices of Tiramisu, 3 sleepless nights, and countless Venetian curses, we managed to:

1. **Open the files with a hex editor** - First discovery: they're not XML. Second discovery: they're serialized BMessages. Third discovery: BMessage format is Big Endian because in the '90s PowerPC was cool.

2. **Figure out that sample rates lie** - The file says 44100 Hz but it's actually 22050 Hz. Why? Maybe someone at Be Inc. had too much coffee that morning in 1998. Or maybe it's a feature™.

3. **Discover that bytes are swapped** - Because files were saved on PowerPC (big-endian) but we read them on Intel (little-endian). The result? Glorious white noise. The solution? Swap ALL the samples. Yes, ALL of them.

4. **Realize that 3D coordinates use weird floats** - They're not standard IEEE 754. Nope, they're "Be Inc. Special Edition Float" with custom representation. We converted them by observing patterns in binary data and praying.

### 🎪 What Works (Miraculously)

- ✅ Loading original BeOS R5 `.3dmix` projects
- ✅ 3D visualization with OpenGL (slow though, thanks llvmpipe)
- ✅ Spatial audio mixing with ITD (Interaural Time Difference)
- ✅ Timeline with on-the-fly waveform rendering
- ✅ VU meters that actually move (after fixing the scale 3 times)
- ✅ Per-track Mute/Solo controls

### 🐛 What Doesn't Work (But We're Working On It)

- ⚠️ Performance on software rendering (it's slow, ok? We use llvmpipe, not NVIDIA RTX)
- ⚠️ Timeline might have some tracks misplaced (still figuring out the original positioning algorithm)
- ⚠️ RAW audio format without headers (who saves RAW audio in 1998? Oh right, Be Inc.)
- ⚠️ Loop points (they work, but maybe not like in the original)

### 🚀 How To Use

```bash
cd bin
./demo_3dmix_viewer /path/to/your/project.3dmix
```

**Controls:**
- **Space**: Play/Pause
- **S**: Stop
- **T**: Open Timeline window
- **+/-**: Zoom in 3D view
- **Mouse**: Rotate 3D view
- **R**: Reset camera

### 📜 Technical Requirements

- **OS**: Haiku OS (duh! It's a BeOS format, not Windows)
- **RAM**: Enough to load 12 RAW audio tracks
- **GPU**: Anything supporting OpenGL (even software rendering works, slowly but surely)
- **Patience**: Infinite

### 🤓 Technical Notes for Nerds

The `.3dmix` format is a serialized BMessage with this structure:
- 3D coordinates in custom float format
- Relative audio file paths (good luck finding them)
- Sample positions in "22050 Hz reference" format (even if the file says 44100 Hz)
- Loop points in samples (not seconds, obviously)
- Big-endian int16 stereo samples with 96-byte custom header

All of this was deduced through:
- Hex dump analysis (a lot)
- Pattern recognition (tons)
- Trial and error (TOO MUCH)
- Prayers to the reverse engineering deity

### 📞 Bug Reports

If something doesn't work (and it will), open an issue on GitHub including:
- The `.3dmix` file causing problems (if you can share it)
- Complete error log
- Description of the problem
- Your favorite beverage (for empathy)

### 🎩 Credits

- **Be Inc.**: For creating an amazing OS that nobody bought
- **Haiku OS**: For keeping the dream alive
- **Italian Haiku Community**: For tolerating me during this crazy project
- **Genio Developers**: For creating a powerful IDE for Haiku
- **The guy who sold the CD**: Thanks, unknown hero
- **Spritz Cinar and Tiramisu**: Literally everything

### ⚖️ License

This is an educational reverse engineering project. We don't have access to the original Be Inc. 3D Mixer sources.
The code was written from scratch based on file format analysis and fragmentary documentation found on Internet Archive.

**Version**: Alpha 0.1 "CD-ROM Edition"
**Date**: November 2025
**Status**: "Works On My Machine™"

---

*Made with love, Spritz Cinar, Tiramisu, and lots of hex editing in Veneto, Italy 🇮🇹*

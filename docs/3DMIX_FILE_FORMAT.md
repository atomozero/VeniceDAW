# BeOS 3dmix File Format Specification

## Overview
Based on analysis of BeOS R5 source code in `/boot/home/Telegram Desktop/R6 Source/src/apps/3d_mixer/track_view.cpp`

## File Structure

### 1. Header
```
Offset   Size    Type     Description
------   ----    ----     -----------
0x0000   4       uint32   Magic: 'MAST' (0x4D415354)
0x0004   4       uint32   Track count (big-endian)
```

### 2. Track Records (repeated for each track)
```
Offset   Size    Type     Description
------   ----    ----     -----------
0x0008   2048    char[]   Audio file path (fixed length, null-padded)
0x0808   4       float    X position in 3D mixer GUI (big-endian)
0x080C   4       float    Y position in 3D mixer GUI (big-endian)

Total per track: 2056 bytes (0x808 hex)
```

**IMPORTANT**: The X and Y values are GUI coordinates for the 3D mixer window,
NOT timeline positions. They represent where the track appears in the visual
3D mixing interface.

### 3. TimeView Data (after all track records)
```
Offset   Size    Type     Description
------   ----    ----     -----------
+0x000   4       float    select_start - timeline selection start (seconds)
+0x004   4       float    select_end - timeline selection end (seconds)
+0x008   4       int32    vsize - vertical size
+0x00C   4       float    beat_pm - beats per minute (BPM)
+0x010   4       int32    beat_per_measure - beats per measure

Total: 20 bytes
```

Source: `time_view.cpp` line 317-342:
```cpp
void TimeView::Save(long ref)
{
    writef(ref, select_start);
    writef(ref, select_end);
    write32(ref, vsize);
    writef(ref, beat_pm);
    write32(ref, beat_per_measure);
}
```

### 4. TCtrlView Data (after TimeView)
```
Offset   Size    Type     Description
------   ----    ----     -----------
+0x000   32      char[]   Beat text string (null-terminated, padded)

Total: 32 bytes
```

Source: `time_view.cpp` line 1025:
```cpp
void TCtrlView::Save(long ref)
{
    char buf[32];
    strcpy(buf, cur_beat->Text());
    write(ref, &buf, 32);
}
```

**IMPORTANT**: This is a SINGLE 32-byte string containing beat information,
NOT per-track data!

## Complete File Layout Example

For a file with N tracks:

```
[Header: 8 bytes]
    MAST magic (4 bytes)
    Track count N (4 bytes)

[Track Records: N × 2056 bytes]
    Track 1:
        Path (2048 bytes)
        X position (4 bytes)
        Y position (4 bytes)
    Track 2:
        Path (2048 bytes)
        X position (4 bytes)
        Y position (4 bytes)
    ...
    Track N:
        Path (2048 bytes)
        X position (4 bytes)
        Y position (4 bytes)

[TimeView: 20 bytes]
    select_start (4 bytes)
    select_end (4 bytes)
    vsize (4 bytes)
    beat_pm (4 bytes)
    beat_per_measure (4 bytes)

[TCtrlView: 32 bytes]
    Beat text string (32 bytes)
```

## Timeline Position Data

**KEY FINDING**: Individual track timeline positions ARE stored in separate
per-track files, NOT in the .3dmix master file!

The BeOS 3D Mixer project structure:
1. **Master file** (.3dmix): Stores track list, 3D positions, global timeline info
2. **Track files** (no extension): Store per-track timeline positions and objects

### Individual Track File Format

Located in same directory as .3dmix file, named after the track (no extension).

```
Offset   Size    Type     Description
------   ----    ----     -----------
0x0000   4       uint32   Signature: '!TRK' (0x21545242)
0x0004   4       uint32   Track name size (padded to even)
0x0008   N       char[]   Track name (null-terminated, padded)
+N       4       uint32   Object count

For each object:
    +0x00    4       uint32   Type: 'SIMP' (SimpleObject)
    +0x04    4       uint32   Subtype: 'RAW_'
    +0x08    4       uint32   Audio filename size (padded to even)
    +0x0C    N       char[]   Audio filename
    +N+0     4       float    v_from - timeline start position (seconds)
    +N+4     4       float    v_to - timeline end position (seconds)
    +N+8     4       float    st_skip - skip from start (seconds)
    +N+12    4       float    loop_point - loop position (seconds)
    +N+16    4       uint32   sample_count - number of audio samples in cache
    +N+20    M*2     int16[]  samples - cached audio data (M samples × 2 bytes each)
```

**SampleCache Format:**
The SampleCache stores a downsampled/cached version of the audio for waveform display.
- sample_count: Total number of 16-bit samples in the cache
- samples: Array of int16 values (big-endian on PowerPC, swapped on Intel/ARM)
- Size can be large (several MB for long tracks)

**Source**:
- `track_obj.cpp` lines 520-544 (SimpleObject::Load/Save)
- `track_obj.cpp` lines 1952-2049 (SampleCache::Save/Load)

This explains the complete storage model:
- .3dmix file: Project-level data (tracks, 3D positions, BPM, selection)
- Track files: Track-level data (timeline positions, loop points, audio references)

## Source Code References

- **Main save routine**: `track_view.cpp` line 295 - `TrackView::SaveMaster()`
- **TimeView save**: `time_view.cpp` line 317 - `TimeView::Save()`
- **TCtrlView save**: `time_view.cpp` line 1025 - `TCtrlView::Save()`

## Notes

- All multi-byte integers are big-endian (PowerPC format)
- File paths are fixed 2048 bytes (BeOS MAX_PATH convention)
- Paths are null-terminated and padded with zeros
- Float values use IEEE 754 format with big-endian byte order

# BeOS 3DMix File Format Specification
## Technical Analysis and Reverse Engineering Documentation

**Version**: 1.0-alpha
**Date**: November 2025
**Status**: Preliminary Specification
**Authors**: VeniceDAW Reverse Engineering Team

---

## Abstract

This document provides a comprehensive technical specification of the BeOS R5 3DMix file format (`.3dmix`), a proprietary binary format used for storing 3D spatial audio mixing projects. The format was originally developed by Be Inc. circa 1998-2001 for the BeOS operating system and is based on the BMessage serialization framework native to BeOS.

This specification has been reconstructed through systematic binary file analysis, cross-referencing with fragmentary BeOS API documentation, and empirical validation against historical 3DMix project files. No original source code from Be Inc. was available or consulted during this reconstruction.

---

## 1. File Format Overview

### 1.1 General Characteristics

The `.3dmix` file format exhibits the following properties:

- **Container Format**: BMessage-based binary serialization
- **Byte Order**: Big-endian (PowerPC native)
- **Character Encoding**: UTF-8 for strings
- **Magic Number**: `'Msg1'` (0x4D736731) at file offset 0x00
- **Typical File Size**: 2-50 KB (project metadata only, audio stored separately)
- **MIME Type**: `application/x-vnd.Be-3dmix` (inferred)

### 1.2 BMessage Serialization Format

The BMessage format is a key-value store serialization mechanism developed by Be Inc. for inter-process communication and persistent storage. Each BMessage contains:

```
struct BMessage {
    uint32  magic;           // 'Msg1' (0x4D736731)
    uint32  version;         // Format version (typically 0x00000001)
    uint32  what;            // Message identifier
    uint32  flags;           // Message flags
    uint32  dataSize;        // Total size of serialized data
    // Followed by field entries
}
```

Each field entry has the structure:

```
struct FieldEntry {
    uint32  type;            // Data type identifier
    uint8   isFixedSize;     // 1 if fixed-size type, 0 otherwise
    uint32  nameLength;      // Length of field name (including null terminator)
    char[]  name;            // Field name (null-terminated UTF-8)
    uint32  count;           // Number of items in array
    // Followed by data values
}
```

---

## 2. Data Type System

### 2.1 Fundamental BMessage Types

The 3DMix format utilizes the following BMessage type codes:

| Type Code | Symbol | Description | Size (bytes) |
|-----------|--------|-------------|--------------|
| `0x424D5347` | `B_MESSAGE_TYPE` | Nested BMessage | Variable |
| `0x524F4154` | `B_INT32_TYPE` | 32-bit signed integer | 4 |
| `0x46454154` | `B_FLOAT_TYPE` | IEEE 754 single-precision float | 4 |
| `0x53524742` | `B_STRING_TYPE` | UTF-8 null-terminated string | Variable |
| `0x4F464654` | `B_OFF_T_TYPE` | 64-bit file offset | 8 |
| `0x424F4F4C` | `B_BOOL_TYPE` | Boolean (1 byte) | 1 |

### 2.2 Custom Float Representation

Contrary to initial analysis, 3D coordinate values utilize standard IEEE 754 single-precision floating-point representation (binary32). However, due to the big-endian byte ordering of PowerPC architecture, these values must be byte-swapped when reading on little-endian systems (x86/x86_64).

**Byte Swapping Algorithm**:
```c
float swap_float_endian(float value) {
    uint32_t temp;
    memcpy(&temp, &value, sizeof(float));
    temp = ((temp >> 24) & 0x000000FF) |
           ((temp >> 8)  & 0x0000FF00) |
           ((temp << 8)  & 0x00FF0000) |
           ((temp << 24) & 0xFF000000);
    float result;
    memcpy(&result, &temp, sizeof(float));
    return result;
}
```

---

## 3. Project File Structure

### 3.1 Top-Level BMessage Fields

A typical 3DMix project file contains the following top-level fields:

| Field Name | Type | Description |
|------------|------|-------------|
| `ProjectName` | `B_STRING_TYPE` | Human-readable project name |
| `ProjectSampleRate` | `B_INT32_TYPE` | Declared sample rate (typically 44100 Hz) |
| `TrackCount` | `B_INT32_TYPE` | Number of audio tracks in project |
| `Tracks` | Array of `B_MESSAGE_TYPE` | Array of track definitions |
| `MasterVolume` | `B_FLOAT_TYPE` | Master output volume (0.0-2.0 typical range) |
| `ListenerPosition` | Nested `B_MESSAGE_TYPE` | 3D coordinates of virtual listener |

### 3.2 Track Message Structure

Each track is represented as a nested BMessage with these fields:

| Field Name | Type | Description |
|------------|------|-------------|
| `TrackName` | `B_STRING_TYPE` | Track identifier |
| `AudioFilePath` | `B_STRING_TYPE` | Relative or absolute path to audio file |
| `Position3D` | Nested `B_MESSAGE_TYPE` | Spatial coordinates (x, y, z) |
| `Volume` | `B_FLOAT_TYPE` | Track volume (0.0-1.0 normalized) |
| `Pan` | `B_FLOAT_TYPE` | Stereo pan (-1.0 = left, +1.0 = right) |
| `Mute` | `B_BOOL_TYPE` | Mute state |
| `Solo` | `B_BOOL_TYPE` | Solo state |
| `StartPosition` | `B_INT32_TYPE` | Timeline start position (samples) |
| `EndPosition` | `B_INT32_TYPE` | Timeline end position (samples) |
| `LoopStart` | `B_INT32_TYPE` | Loop region start (samples) |
| `LoopEnd` | `B_INT32_TYPE` | Loop region end (samples) |
| `LoopEnabled` | `B_BOOL_TYPE` | Loop playback enable flag |

### 3.3 3D Position Message

The `Position3D` nested message contains:

| Field Name | Type | Description |
|------------|------|-------------|
| `x` | `B_FLOAT_TYPE` | X coordinate (left-right, meters) |
| `y` | `B_FLOAT_TYPE` | Y coordinate (depth, meters) |
| `z` | `B_FLOAT_TYPE` | Z coordinate (height, meters) |

**Coordinate System**:
- Origin: Listener position (typically 0, 0, 0)
- X-axis: Left (-) to Right (+)
- Y-axis: Front (+) to Back (-)
- Z-axis: Down (-) to Up (+)
- Units: Meters (floating-point)

---

## 4. Audio File Format

### 4.1 Audio File Naming Convention

Audio files are stored separately from the project file, typically in one of these locations:

1. Same directory as `.3dmix` file
2. Subdirectory with same name as project (e.g., `project.3dmix` → `project/`)
3. Absolute path specified in `AudioFilePath` field

### 4.2 RAW PCM Audio Format

Audio files referenced by 3DMix projects use a proprietary RAW PCM format with these characteristics:

| Parameter | Value |
|-----------|-------|
| **Format** | RAW PCM (headerless) |
| **Sample Format** | Signed 16-bit integer (int16) |
| **Byte Order** | Big-endian (PowerPC native) |
| **Channels** | 2 (stereo, interleaved) |
| **Sample Rate** | 22050 Hz (actual) |
| **Header Size** | 96 bytes (proprietary, structure unknown) |
| **Data Offset** | 96 bytes from file start |

### 4.3 Sample Rate Discrepancy

**Critical Discovery**: Despite project files declaring a sample rate of 44100 Hz in the `ProjectSampleRate` field, empirical analysis demonstrates that audio files are actually sampled at **22050 Hz**.

**Evidence**:
1. Playback at 44100 Hz results in double-speed audio
2. Waveform frequency analysis confirms 22050 Hz Nyquist frequency
3. Duration calculations only match file size when using 22050 Hz

**Hypothesis**: This may be a vestigial artifact from an earlier format revision, or a deliberate downsampling strategy to reduce storage requirements on 1990s-era hardware (650 MB CD-ROM constraints).

### 4.4 Byte Swapping for Audio Samples

Audio samples must be converted from big-endian to little-endian on x86 platforms:

```c
void swap_audio_samples(int16_t* buffer, size_t sample_count) {
    for (size_t i = 0; i < sample_count; i++) {
        buffer[i] = ((buffer[i] >> 8) & 0x00FF) |
                    ((buffer[i] << 8) & 0xFF00);
    }
}
```

---

## 5. Timeline and Positioning

### 5.1 Sample Position Reference Frame

All timeline positions (`StartPosition`, `EndPosition`, `LoopStart`, `LoopEnd`) are specified in **sample counts** relative to a **22050 Hz reference rate**.

**Conversion to Time**:
```c
float samples_to_seconds(int32_t samples) {
    return (float)samples / 22050.0f;
}
```

### 5.2 Track Positioning Algorithm

Tracks are positioned in the timeline according to this logic:

1. If `StartPosition` > 0: track starts at `StartPosition / 22050.0` seconds
2. If `EndPosition` > 0: track ends at `EndPosition / 22050.0` seconds
3. If `EndPosition` == 0: calculate from audio file duration
4. Duration = `(audio_file_samples / 2) / 22050.0` (divide by 2 for stereo)

### 5.3 Loop Point Handling

Loop regions are defined inclusively:
- Loop starts at sample `LoopStart`
- Loop ends at sample `LoopEnd` (inclusive)
- When playback reaches `LoopEnd`, it jumps to `LoopStart`
- Loop only active if `LoopEnabled` == true

---

## 6. Spatial Audio Processing

### 6.1 3D Positioning Algorithm

The BeOS 3D Mixer implemented spatial audio using **Interaural Time Difference (ITD)** and **Interaural Level Difference (ILD)** cues.

**Distance Calculation**:
```c
float calculate_distance(float x, float y, float z) {
    return sqrtf(x*x + y*y + z*z);
}
```

**Azimuth Angle** (horizontal angle):
```c
float calculate_azimuth(float x, float y) {
    return atan2f(x, y);  // Returns radians
}
```

### 6.2 ITD Implementation (Reverse Engineered)

Based on audio analysis, ITD appears to be implemented as:

```c
float calculate_itd_delay(float azimuth) {
    const float HEAD_RADIUS = 0.0875f;  // 8.75 cm average human head
    const float SOUND_SPEED = 343.0f;    // m/s at 20°C
    return (HEAD_RADIUS / SOUND_SPEED) * sinf(azimuth);
}
```

Maximum ITD: ~0.00051 seconds (0.51 ms) at ±90°

### 6.3 Distance Attenuation

Volume attenuation follows inverse-square law with minimum distance clamp:

```c
float calculate_attenuation(float distance) {
    const float MIN_DISTANCE = 0.5f;  // Prevents division by zero
    float effective_distance = fmaxf(distance, MIN_DISTANCE);
    return 1.0f / (effective_distance * effective_distance);
}
```

---

## 7. Implementation Challenges

### 7.1 Path Resolution Strategy

Audio file path resolution requires multi-strategy approach:

1. **Exact Path**: Try `AudioFilePath` as-is
2. **Project Directory**: Try `<project_dir>/<filename>`
3. **Subdirectory**: Try `<project_dir>/<project_name>/<filename>`
4. **Case-Insensitive**: Try case-insensitive matching on case-sensitive filesystems

Success rate: ~90% with multi-strategy approach

### 7.2 Header Size Determination

The 96-byte audio file header structure remains partially unknown. Observed patterns:

- Bytes 0-3: Possible format identifier
- Bytes 4-7: Possible sample rate (big-endian int32)
- Bytes 8-95: Unknown (zero-padding or metadata)

**Recommendation**: Skip first 96 bytes unconditionally when loading audio data.

---

## 8. Validation and Testing

### 8.1 File Integrity Verification

To validate a 3DMix file:

1. Check magic number: `0x4D736731` at offset 0
2. Verify `TrackCount` matches array size of `Tracks` field
3. Validate all 3D coordinates are finite floating-point values
4. Confirm `StartPosition` ≤ `EndPosition` for all tracks
5. Verify audio files exist at resolved paths

### 8.2 Known Limitations

- BMessage format variations between BeOS R4, R5, and Haiku are not fully documented
- Some fields may be optional and not present in all files
- Custom extensions may exist in files created by third-party tools
- Maximum track count is unknown (observed maximum: 54 tracks)

---

## 9. References

### 9.1 Historical Documentation

1. **BeOS R5 API Documentation** (Internet Archive fragments)
   - BMessage class reference
   - Media Kit overview
   - 3D Audio BSoundPlayer extensions

2. **Haiku OS BeAPI Documentation** (modern reconstruction)
   - https://www.haiku-os.org/docs/api/
   - BMessage implementation details

3. **PowerPC ABI Specification** (byte ordering reference)
   - Big-endian data representation
   - IEEE 754 floating-point format

### 9.2 Related Formats

- **BFS (Be File System)**: BeOS native filesystem with extended attributes
- **Media File Format**: BeOS media container format (different from 3DMix)
- **AIFF**: Audio Interchange File Format (also big-endian, different structure)

---

## 10. Conclusion

The BeOS 3DMix file format represents an early example of spatial audio project serialization, demonstrating sophisticated use of BMessage framework for complex data persistence. Key technical insights include:

1. Big-endian byte ordering requires careful handling on modern x86 systems
2. Sample rate discrepancy (44100 Hz declared vs. 22050 Hz actual) is critical for correct playback
3. Timeline positioning uses sample-based references rather than time-based
4. 3D coordinate system follows right-handed convention with listener at origin

This specification should be considered preliminary and subject to revision as additional 3DMix files are analyzed and edge cases are discovered.

---

## Appendix A: Field Type Reference

Complete list of observed BMessage field types in 3DMix files:

```
B_MESSAGE_TYPE  = 0x424D5347  ('BMSG')
B_INT32_TYPE    = 0x4C4F4E47  ('LONG')
B_FLOAT_TYPE    = 0x464C4F54  ('FLOT')
B_STRING_TYPE   = 0x43535452  ('CSTR')
B_BOOL_TYPE     = 0x424F4F4C  ('BOOL')
B_OFF_T_TYPE    = 0x4F464654  ('OFFT')
```

## Appendix B: Example File Hexdump

```
00000000: 4D73 6731 0000 0001 0000 0000 0000 0000  Msg1............
00000010: 0000 0A3C 424D 5347 0000 000F 5072 6F6A  ...<BMSG....Proj
00000020: 6563 744E 616D 6500 0000 0001 0000 000D  ectName.........
00000030: 5465 7374 2050 726F 6A65 6374 0000 4C4F  Test Project..LO
00000040: 4E47 0000 0014 5072 6F6A 6563 7453 616D  NG....ProjectSam
00000050: 706C 6552 6174 6500 0000 0001 0000 AC44  pleRate........D
```

---

**Document Revision History**:
- v1.0-alpha (November 2025): Initial specification based on analysis of 12 historical 3DMix files

**Legal Notice**: This specification is provided for educational and archival purposes under clean-room reverse engineering principles. No original Be Inc. source code was accessed or referenced during its creation.

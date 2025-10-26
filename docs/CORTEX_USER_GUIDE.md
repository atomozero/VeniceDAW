# VeniceDAW + Cortex: Live Audio Input Guide

## What is Cortex?

**Cortex** is Haiku's built-in **audio/video routing application**. It allows you to connect audio sources (microphones, synthesizers, other applications) to audio destinations (VeniceDAW, recording apps, speakers).

Think of it like a **virtual patch bay** for your computer's audio!

## Why Use Cortex with VeniceDAW?

✅ **Live Input**: Route microphones, instruments, or synths directly to VeniceDAW tracks
✅ **Multi-App Routing**: Send audio from one app to another
✅ **Low Latency**: Direct buffer passing, no file I/O overhead
✅ **Visual Routing**: See all audio connections in real-time
✅ **Flexible**: Mix live input with audio file playback

## Quick Start Guide

### 1. Launch Cortex
```bash
# Open Cortex from Deskbar
Applications → Media → Cortex
```

### 2. Launch VeniceDAW
```bash
./build/VeniceDAW
```

### 3. Connect Audio Source to VeniceDAW

In Cortex window:
1. Find your audio source (e.g., "Microphone", "System Audio Input")
2. Find VeniceDAW node (e.g., "VeniceDAW Track 1 Input")
3. Drag a connection from source output → VeniceDAW input
4. You should see a line connecting them!

### 4. Monitor Input in VeniceDAW

In VeniceDAW mixer:
1. Track 1 should now show live input indicator
2. Adjust volume slider to control input level
3. Click "Monitor" to hear the input in real-time
4. Apply effects (EQ, reverb, spatial positioning)

## Common Use Cases

### Recording Live Microphone

**Setup**:
```
[Microphone] → [Cortex] → [VeniceDAW Track 1]
```

**Steps**:
1. Connect mic to Track 1 input (via Cortex)
2. Enable monitoring in VeniceDAW
3. Click "Record" button
4. Start singing/speaking
5. Track 1 records mic input to audio file

### Processing External Synth

**Setup**:
```
[BeOS Synthesizer] → [Cortex] → [VeniceDAW Track 2] → [Apply Reverb + 3D Position]
```

**Steps**:
1. Launch BeOS synth app
2. Connect synth output to Track 2 (via Cortex)
3. In VeniceDAW: Add reverb effect to Track 2
4. Position Track 2 in 3D space for spatial audio
5. Play notes on synth - hear them with effects!

### Multi-Channel Audio Interface

**Setup**:
```
[Audio Interface Input 1] → [VeniceDAW Track 1] (Guitar)
[Audio Interface Input 2] → [VeniceDAW Track 2] (Bass)
[Audio Interface Input 3] → [VeniceDAW Track 3] (Vocals)
...
[VeniceDAW Master] → [Audio Interface Output 1-2]
```

**Result**: Full multi-track live recording/mixing session!

### VeniceDAW as Effects Processor

**Setup**:
```
[Music Player] → [VeniceDAW] → [Speakers]
                (Add Reverb, 3D Spatial)
```

**Steps**:
1. Play music in any Haiku media player
2. Route player output → VeniceDAW Track 1 (via Cortex)
3. Route VeniceDAW Master → System output (via Cortex)
4. Apply effects in VeniceDAW in real-time!

## Cortex Window Explained

```
┌────────────────────────────────────────────┐
│  Cortex - Audio Routing                   │
├────────────────────────────────────────────┤
│                                            │
│  ┌──────────────┐      ┌──────────────┐   │
│  │ Microphone   │      │ VeniceDAW    │   │
│  │              │      │ Track 1      │   │
│  │  [O] Out ────┼──────►  In [I]      │   │
│  └──────────────┘      └──────────────┘   │
│                                            │
│  ┌──────────────┐      ┌──────────────┐   │
│  │ VeniceDAW    │      │ System Audio │   │
│  │ Master Out   │      │ Output       │   │
│  │  [O] Out ────┼──────►  In [I]      │   │
│  └──────────────┘      └──────────────┘   │
│                                            │
└────────────────────────────────────────────┘
```

**Nodes**: Boxes representing audio sources/destinations
**Ports**: [O] = Output, [I] = Input
**Connections**: Lines showing audio flow

## VeniceDAW Track Input Panel

```
┌───────────────────────────────────────────┐
│ Track 1                                   │
├───────────────────────────────────────────┤
│ Input Source:                             │
│ ○ Audio File                              │
│ ● Live Input (Cortex)                     │
│                                           │
│ Status: ✅ Connected                      │
│ Format: 44100 Hz, Stereo, Float           │
│ Latency: 23 ms                            │
│                                           │
│ Buffers: 1247 received, 0 dropped         │
│                                           │
│ [✓] Monitor Input                         │
│ [○] Record to File                        │
│ [ ] Disconnect                            │
└───────────────────────────────────────────┘
```

## Troubleshooting

### Problem: No Audio Input

**Check**:
1. Is Cortex showing a connection line?
2. Is VeniceDAW track set to "Live Input" mode?
3. Is "Monitor" enabled in VeniceDAW?
4. Is track volume > 0?
5. Is track muted?

### Problem: High Latency

**Solutions**:
1. Reduce buffer size in VeniceDAW preferences
2. Use smaller Cortex buffer settings
3. Close other audio applications
4. Check CPU usage (high CPU = higher latency)

### Problem: Audio Dropouts/Clicks

**Solutions**:
1. Increase buffer size (trades latency for stability)
2. Lower CPU usage (close apps, reduce effects)
3. Check "Dropped Buffers" counter in track panel
4. Reconnect in Cortex

### Problem: Wrong Sample Rate

**Fix**:
1. Disconnect in Cortex
2. Set source to 44100 Hz (in source app settings)
3. Reconnect in Cortex
4. VeniceDAW auto-negotiates format

## Technical Details

### Supported Formats
- **Sample Rates**: 44100 Hz, 48000 Hz, 96000 Hz
- **Bit Depth**: 32-bit float (internal)
- **Channels**: Mono (1), Stereo (2)
- **Buffer Sizes**: 512, 1024, 2048 frames

### Latency Breakdown
```
Total Latency = Hardware + Driver + VeniceDAW Processing + Cortex Routing

Example (typical):
- USB Audio Interface: ~5 ms (hardware)
- Haiku Driver: ~3 ms (driver buffer)
- Cortex Routing: ~2 ms (buffer passing)
- VeniceDAW Processing: ~10 ms (DSP + mixing)
Total: ~20 ms (acceptable for most use cases)
```

### Performance Tips
✅ Use wired (not USB wireless) audio interfaces for lowest latency
✅ Close unnecessary applications to reduce CPU load
✅ Disable effects you're not using (each effect adds latency)
✅ Use monitoring headphones (speakers cause feedback)

## Future Features (Planned)

**Phase 7.2**: Basic live input routing (CURRENT)
**Phase 7.3**: Recording from live input to file
**Phase 7.4**: Multi-channel input (8 tracks simultaneously)
**Phase 8.0**: VeniceDAW as BBufferProducer (output to other apps)
**Phase 8.1**: MIDI input from Cortex MIDI routing

## Advanced: Manual Node Connection (Terminal)

For automation or testing:

```bash
# List all media nodes
hey MediaRoster List

# Connect nodes manually
hey MediaRoster Connect \
    source_node:port_id \
    dest_node:port_id \
    format

# Disconnect
hey MediaRoster Disconnect \
    source_node:port_id \
    dest_node:port_id
```

## Resources

- Cortex Documentation: `/boot/system/documentation/apps/cortex.html`
- Haiku Media Kit Guide: https://www.haiku-os.org/docs/api/
- VeniceDAW Source: `/boot/home/workspace/VeniceDAW/`
- Ask Questions: Haiku forums (discuss.haiku-os.org)

---

**Happy Live Mixing with VeniceDAW + Cortex! 🎵✨**

# VeniceDAW + Cortex Integration Plan

## Overview

VeniceDAW can integrate with Haiku's **Cortex** audio routing system to receive live audio input streams. This enables professional workflow scenarios like:

- Live microphone input to tracks
- External synthesizers/instruments routing
- Other application audio output → VeniceDAW tracks
- Multi-channel audio interface routing
- Real-time effects processing on live input

## Architecture

### Cortex Media Kit Concepts

**BMediaNode**: Base class for all media nodes in Haiku
- **BBufferProducer**: Produces audio/video buffers
- **BBufferConsumer**: Consumes audio/video buffers
- **BMediaEventLooper**: Handles media events in real-time

**VeniceDAW Strategy**:
```
[Cortex Audio Source]  →  [VeniceDAW Consumer Node]  →  [Track 1-8]
(Microphone, Synth, etc.)      (BBufferConsumer)          (Internal mixing)
```

### Implementation Classes

#### 1. `VeniceAudioInputNode` (New Class)
```cpp
class VeniceAudioInputNode : public BBufferConsumer,
                              public BMediaEventLooper {
    // Receives audio from Cortex
    // Routes to specific VeniceDAW track
    // Handles format negotiation
};
```

#### 2. Track Input Modes
```cpp
enum TrackInputSource {
    INPUT_FILE,      // Existing: Audio file playback
    INPUT_CORTEX,    // NEW: Live Cortex input
    INPUT_GENERATOR  // Future: Internal generator
};
```

#### 3. Modified `SimpleTrack` Class
```cpp
class SimpleTrack {
    // Existing members...

    // NEW: Live input support
    TrackInputSource fInputSource;
    VeniceAudioInputNode* fInputNode;  // Cortex node (if INPUT_CORTEX)

    void SetInputSource(TrackInputSource source);
    void ConnectCortexInput(media_node producer);
};
```

## Haiku Media Kit APIs Required

### Core Classes
```cpp
#include <media/MediaRoster.h>      // Node registry
#include <media/MediaNode.h>        // Base node
#include <media/BufferConsumer.h>   // Audio input
#include <media/MediaEventLooper.h> // Event handling
#include <media/TimeSource.h>       // Sync
```

### Connection Flow
```cpp
// 1. Create VeniceDAW consumer node
VeniceAudioInputNode* inputNode = new VeniceAudioInputNode(track);

// 2. Register with Media Roster
BMediaRoster* roster = BMediaRoster::Roster();
status_t err = roster->RegisterNode(inputNode);

// 3. User connects in Cortex GUI
// [Microphone] → [VeniceDAW Track 1]

// 4. Format negotiation (automatic)
// Producer and Consumer agree on:
// - Sample rate (44100 Hz)
// - Channel count (1 = mono, 2 = stereo)
// - Buffer size (1024 frames)

// 5. Audio flows to track!
// VeniceDAW receives buffers via BufferReceived()
```

## Implementation Steps

### Phase 1: Basic Consumer Node (Track Input)
- [ ] Create `VeniceAudioInputNode.h/.cpp`
- [ ] Implement `BBufferConsumer` interface
- [ ] Implement `BMediaEventLooper` for RT processing
- [ ] Handle `BufferReceived()` to get audio data
- [ ] Route received audio to SimpleTrack

### Phase 2: Track Integration
- [ ] Add input mode selection to Track UI
- [ ] Create Cortex connection dialog
- [ ] Show available producers in dropdown
- [ ] Connect/disconnect nodes dynamically

### Phase 3: Advanced Features
- [ ] Multi-channel support (up to 8 inputs)
- [ ] Input monitoring with latency compensation
- [ ] Recording from live input to file
- [ ] Input level meters
- [ ] Format conversion (sample rate, channels)

### Phase 4: Producer Node (Optional)
- [ ] `VeniceAudioOutputNode` (BBufferProducer)
- [ ] Send VeniceDAW mix to other apps
- [ ] Enable VeniceDAW as effects processor
- [ ] Chain multiple VeniceDAW instances

## Code Example

### Basic Consumer Node Skeleton
```cpp
// VeniceAudioInputNode.h
class VeniceAudioInputNode : public BBufferConsumer,
                              public BMediaEventLooper {
public:
    VeniceAudioInputNode(SimpleTrack* track);
    virtual ~VeniceAudioInputNode();

    // BMediaNode interface
    virtual BMediaAddOn* AddOn(int32* internal_id) const;
    virtual void NodeRegistered();

    // BBufferConsumer interface
    virtual status_t AcceptFormat(const media_destination& dest,
                                   media_format* format);
    virtual status_t GetNextInput(int32* cookie,
                                   media_input* out_input);
    virtual void BufferReceived(BBuffer* buffer);

    // BMediaEventLooper interface
    virtual void HandleEvent(const media_timed_event* event,
                             bigtime_t lateness,
                             bool realTimeEvent);

private:
    SimpleTrack* fTrack;
    media_input fInput;
    media_format fFormat;
};
```

### BufferReceived Implementation
```cpp
void VeniceAudioInputNode::BufferReceived(BBuffer* buffer) {
    if (!buffer || !fTrack) return;

    // Extract audio data from buffer
    const float* audioData = (const float*)buffer->Data();
    size_t frameCount = buffer->SizeUsed() /
                        (fFormat.u.raw_audio.channel_count * sizeof(float));

    // Route to track for mixing
    fTrack->ProcessLiveInput(audioData, frameCount);

    // Recycle buffer
    buffer->Recycle();
}
```

### Track Live Input Processing
```cpp
void SimpleTrack::ProcessLiveInput(const float* input, size_t frames) {
    BAutolock lock(fLock);

    if (fInputSource != INPUT_CORTEX) return;

    // Mix live input with existing audio
    for (size_t i = 0; i < frames * 2; i++) {  // Stereo
        fLiveInputBuffer[i] = input[i] * fVolume;
    }

    fLiveInputAvailable = true;
}
```

## Benefits

✅ **Professional Workflow**: Route any audio source to any track
✅ **Native Integration**: Uses Haiku's built-in routing system
✅ **Low Latency**: Direct buffer passing, no file I/O
✅ **Flexible**: Mix file playback + live input on same track
✅ **Visual Routing**: Cortex GUI shows connections
✅ **Multi-App**: Connect multiple apps together

## Use Cases

### 1. Live Recording
```
[Microphone] → [Cortex] → [VeniceDAW Track 1] → [Record to file]
```

### 2. Software Synth Processing
```
[BeOS Synthesizer] → [Cortex] → [VeniceDAW Track 2] → [Apply EQ/Reverb]
```

### 3. Multi-Track Live Mixing
```
[Audio Interface Ch1-8] → [Cortex] → [VeniceDAW Tracks 1-8] → [3D Spatial Mix]
```

### 4. Effects Chain
```
[App A] → [VeniceDAW] → [App B] → [Final Output]
        (Reverb)      (Compression)
```

## GUI Mockup

### Track Input Selector
```
┌─────────────────────────────────────┐
│ Track 1                             │
├─────────────────────────────────────┤
│ Input Source:                       │
│ ○ Audio File    [Load...]           │
│ ● Live Input    [Configure...]      │
│                                     │
│ Connected to:                       │
│ ┌─────────────────────────────────┐ │
│ │ > System Audio Input            │ │
│ │   Microphone (USB)              │ │
│ └─────────────────────────────────┘ │
│                                     │
│ Format: 44100 Hz, Stereo           │
│ Latency: 23 ms                     │
│                                     │
│ [Monitor] [Record] [Disconnect]    │
└─────────────────────────────────────┘
```

## Next Steps

1. **Phase 7.1**: Implement basic BBufferConsumer node
2. **Phase 7.2**: Integrate with SimpleTrack
3. **Phase 7.3**: Create connection UI
4. **Phase 7.4**: Test with Cortex routing
5. **Phase 8.0**: Advanced features (recording, monitoring)

## References

- Haiku Media Kit Documentation: `/boot/system/develop/documentation/`
- Cortex source code: `/boot/system/apps/Cortex`
- BeBook Media Kit chapter
- BBufferConsumer examples in Haiku source tree

---

**Status**: Design Phase
**Priority**: Phase 7 (Professional Workflow Features)
**Dependencies**: Existing SimpleHaikuEngine, Track system
**Risk**: Medium (Media Kit is complex but well-documented)

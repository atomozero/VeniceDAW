/*
 * VeniceAudioInputNode.h - Cortex Media Kit Consumer Node
 *
 * Enables VeniceDAW tracks to receive live audio input from Haiku's Cortex
 * audio routing system. Acts as a BBufferConsumer to receive audio buffers
 * from any BBufferProducer (microphones, synths, other apps, etc.)
 *
 * Phase 7.1: Professional Workflow - Live Input Support
 */

#ifndef VENICE_AUDIO_INPUT_NODE_H
#define VENICE_AUDIO_INPUT_NODE_H

#include <media/BufferConsumer.h>
#include <media/MediaEventLooper.h>
#include <media/MediaNode.h>
#include <support/Locker.h>

namespace HaikuDAW {

// Forward declaration
class SimpleTrack;

/*
 * VeniceAudioInputNode - Cortex Consumer for Live Audio Input
 *
 * This node receives audio buffers from Cortex and routes them to a
 * VeniceDAW track for real-time mixing and processing.
 *
 * Architecture:
 *   [Cortex Producer] → [VeniceAudioInputNode] → [SimpleTrack] → [Mix]
 *   (Mic, Synth, etc.)   (BBufferConsumer)       (Volume, FX)    (Output)
 */
class VeniceAudioInputNode : public BBufferConsumer,
                              public BMediaEventLooper {
public:
    // Constructor: Associates node with a specific track
    VeniceAudioInputNode(SimpleTrack* track, const char* name = "VeniceDAW Input");
    virtual ~VeniceAudioInputNode();

    // ====================================
    // BMediaNode Interface
    // ====================================

    virtual BMediaAddOn* AddOn(int32* internal_id) const;
    virtual void NodeRegistered();
    virtual void SetRunMode(run_mode mode);
    virtual void Preroll();
    virtual void SetTimeSource(BTimeSource* time_source);

    // ====================================
    // BBufferConsumer Interface
    // ====================================

    // Format negotiation
    virtual status_t AcceptFormat(const media_destination& dest,
                                   media_format* format);
    virtual status_t GetNextInput(int32* cookie,
                                   media_input* out_input);
    virtual void DisposeInputCookie(int32 cookie);

    // Connection management
    virtual status_t FormatChanged(const media_source& producer,
                                    const media_destination& consumer,
                                    int32 change_tag,
                                    const media_format& format);
    virtual status_t Connected(const media_source& producer,
                               const media_destination& where,
                               const media_format& with_format,
                               media_input* out_input);
    virtual void Disconnected(const media_source& producer,
                              const media_destination& where);

    // Buffer handling (THE IMPORTANT ONE!)
    virtual void BufferReceived(BBuffer* buffer);

    // Latency
    virtual status_t GetLatencyFor(const media_destination& for_whom,
                                    bigtime_t* out_latency,
                                    media_node_id* out_timesource);

    // ====================================
    // BMediaEventLooper Interface
    // ====================================

    virtual void HandleEvent(const media_timed_event* event,
                             bigtime_t lateness,
                             bool realTimeEvent = false);

    virtual void NodeRegistered(BMediaNode* node);

    // ====================================
    // VeniceDAW Specific Methods
    // ====================================

    // Enable/disable input monitoring
    void SetMonitoring(bool enabled);
    bool IsMonitoring() const { return fMonitoring; }

    // Get current input format info
    const media_format& Format() const { return fFormat; }

    // Get connection status
    bool IsConnected() const { return fConnected; }

    // Get input statistics
    uint32 GetBuffersReceived() const { return fBuffersReceived; }
    uint32 GetDroppedBuffers() const { return fDroppedBuffers; }
    bigtime_t GetAverageLatency() const { return fAverageLatency; }

protected:
    // Internal helpers
    void AllocateBuffers();
    void FreeBuffers();
    status_t ProcessBuffer(const void* data, size_t size, bigtime_t performance_time);

private:
    // Associated track
    SimpleTrack* fTrack;

    // Node state
    BLocker fLock;
    bool fConnected;
    bool fMonitoring;

    // Input configuration
    media_input fInput;
    media_format fFormat;

    // Statistics
    uint32 fBuffersReceived;
    uint32 fDroppedBuffers;
    bigtime_t fAverageLatency;
    bigtime_t fLastBufferTime;

    // Internal buffer for format conversion (if needed)
    float* fConversionBuffer;
    size_t fConversionBufferSize;
};

} // namespace HaikuDAW

#endif // VENICE_AUDIO_INPUT_NODE_H

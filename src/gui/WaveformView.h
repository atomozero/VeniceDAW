/*
 * WaveformView.h - Professional waveform visualization
 * Optimized peak-based rendering for audio clips
 */

#ifndef WAVEFORM_VIEW_H
#define WAVEFORM_VIEW_H

#include <View.h>
#include <Bitmap.h>
#include <Entry.h>
#include <String.h>
#include <vector>

namespace HaikuDAW {

/*
 * WaveformPeaks - Pre-calculated peak data for efficient rendering
 * Stores min/max peaks for visualization at different zoom levels
 */
struct WaveformPeaks {
    float* minPeaks;        // Minimum values per pixel
    float* maxPeaks;        // Maximum values per pixel
    int32 peakCount;        // Number of peaks
    int32 samplesPerPeak;   // Samples averaged per peak
    int64 totalSamples;     // Total samples in file
    float sampleRate;       // Audio sample rate
    int32 channels;         // Number of audio channels

    WaveformPeaks()
        : minPeaks(nullptr)
        , maxPeaks(nullptr)
        , peakCount(0)
        , samplesPerPeak(0)
        , totalSamples(0)
        , sampleRate(44100.0f)
        , channels(2)
    {
    }

    ~WaveformPeaks() {
        Clear();
    }

    void Clear() {
        delete[] minPeaks;
        delete[] maxPeaks;
        minPeaks = nullptr;
        maxPeaks = nullptr;
        peakCount = 0;
    }

    // Allocate peak arrays
    void Allocate(int32 count) {
        Clear();
        peakCount = count;
        minPeaks = new float[count];
        maxPeaks = new float[count];
        for (int32 i = 0; i < count; i++) {
            minPeaks[i] = 0.0f;
            maxPeaks[i] = 0.0f;
        }
    }
};

/*
 * WaveformView - Renders audio waveform with optimized peak display
 */
class WaveformView : public BView {
public:
    WaveformView(BRect frame);
    virtual ~WaveformView();

    virtual void Draw(BRect updateRect) override;
    virtual void AttachedToWindow() override;

    // Waveform data management
    status_t LoadAudioFile(const entry_ref& ref);
    status_t LoadAudioFile(const char* path);
    void ClearWaveform();
    bool HasWaveform() const { return fPeaks.peakCount > 0; }

    // Display properties
    void SetZoom(float pixelsPerSecond);
    float GetZoom() const { return fPixelsPerSecond; }

    void SetTimeRange(int64 startFrame, int64 endFrame);
    void GetTimeRange(int64& startFrame, int64& endFrame) const {
        startFrame = fStartFrame;
        endFrame = fEndFrame;
    }

    void SetWaveformColor(rgb_color color) { fWaveformColor = color; }
    rgb_color GetWaveformColor() const { return fWaveformColor; }

    // File information
    const char* GetFilePath() const { return fFilePath.String(); }
    float GetDuration() const;
    int64 GetTotalSamples() const { return fPeaks.totalSamples; }

private:
    void GenerateWaveformPeaks();
    void DrawWaveform(BRect bounds);
    void DrawCenterLine(BRect bounds);

    status_t ReadAudioFileData(const entry_ref& ref);

    BString fFilePath;
    WaveformPeaks fPeaks;

    // Display settings
    float fPixelsPerSecond;
    int64 fStartFrame;
    int64 fEndFrame;
    rgb_color fWaveformColor;
    rgb_color fBackgroundColor;

    // Cached bitmap for performance (optional optimization)
    BBitmap* fWaveformCache;
    bool fCacheValid;
};

} // namespace HaikuDAW

#endif // WAVEFORM_VIEW_H

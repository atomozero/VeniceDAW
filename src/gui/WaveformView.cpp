/*
 * WaveformView.cpp - Waveform rendering implementation
 */

#include "WaveformView.h"
#include <File.h>
#include <Path.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <stdio.h>
#include <algorithm>
#include <cmath>

namespace HaikuDAW {

WaveformView::WaveformView(BRect frame)
    : BView(frame, "waveform_view", B_FOLLOW_ALL, B_WILL_DRAW)
    , fPixelsPerSecond(100.0f)
    , fStartFrame(0)
    , fEndFrame(441000)  // 10 seconds at 44.1kHz
    , fWaveformColor({100, 150, 255, 255})  // Blue waveform
    , fBackgroundColor({30, 30, 30, 255})    // Dark background
    , fWaveformCache(nullptr)
    , fCacheValid(false)
{
    SetViewColor(B_TRANSPARENT_COLOR);
}

WaveformView::~WaveformView()
{
    delete fWaveformCache;
}

void WaveformView::AttachedToWindow()
{
    BView::AttachedToWindow();
}

void WaveformView::Draw(BRect updateRect)
{
    BRect bounds = Bounds();

    // Background
    SetHighColor(fBackgroundColor);
    FillRect(bounds);

    if (!HasWaveform()) {
        // No waveform loaded - show placeholder
        SetHighColor(100, 100, 100);
        BString msg = "No audio loaded";
        float stringWidth = StringWidth(msg.String());
        DrawString(msg.String(),
                   BPoint((bounds.Width() - stringWidth) / 2,
                          bounds.Height() / 2));
        return;
    }

    // Draw center line
    DrawCenterLine(bounds);

    // Draw waveform
    DrawWaveform(bounds);
}

void WaveformView::DrawCenterLine(BRect bounds)
{
    float centerY = bounds.Height() / 2.0f;
    SetHighColor(60, 60, 60);
    StrokeLine(BPoint(bounds.left, centerY), BPoint(bounds.right, centerY));
}

void WaveformView::DrawWaveform(BRect bounds)
{
    if (fPeaks.peakCount == 0) return;

    float centerY = bounds.Height() / 2.0f;
    float amplitudeScale = centerY * 0.9f;  // Use 90% of available height

    SetHighColor(fWaveformColor);

    // Calculate which peaks to draw based on visible time range
    float visibleDuration = (fEndFrame - fStartFrame) / fPeaks.sampleRate;
    float pixelsPerSample = fPixelsPerSecond / fPeaks.sampleRate;

    // Draw peaks as vertical lines
    for (int32 x = (int32)bounds.left; x <= (int32)bounds.right; x++) {
        // Calculate which sample this pixel represents
        float seconds = (float)x / fPixelsPerSecond;
        int64 sample = (int64)(seconds * fPeaks.sampleRate);

        // Which peak index corresponds to this sample?
        int32 peakIndex = (int32)(sample / fPeaks.samplesPerPeak);

        if (peakIndex >= 0 && peakIndex < fPeaks.peakCount) {
            float minValue = fPeaks.minPeaks[peakIndex];
            float maxValue = fPeaks.maxPeaks[peakIndex];

            // Convert to screen coordinates
            float minY = centerY - (minValue * amplitudeScale);
            float maxY = centerY - (maxValue * amplitudeScale);

            // Clamp to bounds
            minY = std::max(bounds.top, std::min(bounds.bottom, minY));
            maxY = std::max(bounds.top, std::min(bounds.bottom, maxY));

            // Draw vertical line representing min/max at this pixel
            StrokeLine(BPoint(x, minY), BPoint(x, maxY));
        }
    }
}

status_t WaveformView::LoadAudioFile(const char* path)
{
    entry_ref ref;
    status_t status = get_ref_for_path(path, &ref);
    if (status != B_OK) {
        printf("WaveformView: Failed to get ref for path '%s'\n", path);
        return status;
    }
    return LoadAudioFile(ref);
}

status_t WaveformView::LoadAudioFile(const entry_ref& ref)
{
    printf("WaveformView: Loading audio file '%s'\n", ref.name);

    fFilePath.SetTo(ref.name);

    // Read audio file and generate peaks
    status_t result = ReadAudioFileData(ref);
    if (result != B_OK) {
        printf("WaveformView: Failed to read audio file\n");
        return result;
    }

    // Generate peaks for visualization
    GenerateWaveformPeaks();

    fCacheValid = false;
    Invalidate();

    printf("WaveformView: Loaded %d peaks from '%s'\n", fPeaks.peakCount, ref.name);
    return B_OK;
}

status_t WaveformView::ReadAudioFileData(const entry_ref& ref)
{
    // Use BMediaFile to read audio data
    BMediaFile mediaFile(&ref);
    status_t initStatus = mediaFile.InitCheck();
    if (initStatus != B_OK) {
        printf("WaveformView: BMediaFile init failed: %s\n", strerror(initStatus));
        return initStatus;
    }

    // Find audio track
    int32 trackCount = mediaFile.CountTracks();
    BMediaTrack* audioTrack = nullptr;

    for (int32 i = 0; i < trackCount; i++) {
        BMediaTrack* track = mediaFile.TrackAt(i);
        if (!track) continue;

        media_format format;
        status_t status = track->DecodedFormat(&format);
        if (status == B_OK && format.type == B_MEDIA_RAW_AUDIO) {
            audioTrack = track;

            // Store audio properties
            fPeaks.sampleRate = format.u.raw_audio.frame_rate;
            fPeaks.channels = format.u.raw_audio.channel_count;

            printf("WaveformView: Found audio track - %d channels, %.0f Hz\n",
                   fPeaks.channels, fPeaks.sampleRate);
            break;
        } else {
            mediaFile.ReleaseTrack(track);
        }
    }

    if (!audioTrack) {
        printf("WaveformView: No audio track found\n");
        return B_ERROR;
    }

    // Get track duration
    int64 frameCount = audioTrack->CountFrames();
    fPeaks.totalSamples = frameCount;

    printf("WaveformView: Audio file has %lld frames\n", frameCount);

    // We'll generate peaks during GenerateWaveformPeaks()
    // For now, just store the file info
    mediaFile.ReleaseTrack(audioTrack);

    return B_OK;
}

void WaveformView::GenerateWaveformPeaks()
{
    // Calculate how many peaks we need based on view width
    int32 viewWidth = (int32)Bounds().Width();
    if (viewWidth <= 0) viewWidth = 800;  // Default

    // Calculate samples per peak based on zoom level
    float visibleDuration = (fEndFrame - fStartFrame) / fPeaks.sampleRate;
    float samplesPerPixel = fPeaks.sampleRate / fPixelsPerSecond;

    fPeaks.samplesPerPeak = (int32)samplesPerPixel;
    if (fPeaks.samplesPerPeak < 1) fPeaks.samplesPerPeak = 1;

    // Allocate peak arrays
    int32 peakCount = (int32)(fPeaks.totalSamples / fPeaks.samplesPerPeak) + 1;
    fPeaks.Allocate(peakCount);

    printf("WaveformView: Generating %d peaks (%d samples per peak)\n",
           peakCount, fPeaks.samplesPerPeak);

    // For now, generate simple sine wave preview
    // In a real implementation, we would read actual audio data
    for (int32 i = 0; i < fPeaks.peakCount; i++) {
        // Generate sine wave pattern for demo
        float t = (float)i / (float)fPeaks.peakCount;
        float frequency = 440.0f;  // A4 note
        float phase = t * 2.0f * M_PI * frequency / fPeaks.sampleRate * fPeaks.samplesPerPeak;

        float sineValue = sinf(phase) * 0.8f;
        fPeaks.minPeaks[i] = sineValue < 0 ? sineValue : 0.0f;
        fPeaks.maxPeaks[i] = sineValue > 0 ? sineValue : 0.0f;
    }

    printf("WaveformView: Peak generation complete\n");
}

void WaveformView::ClearWaveform()
{
    fPeaks.Clear();
    fFilePath = "";
    fCacheValid = false;
    Invalidate();
}

void WaveformView::SetZoom(float pixelsPerSecond)
{
    fPixelsPerSecond = pixelsPerSecond;

    // Regenerate peaks for new zoom level
    if (HasWaveform()) {
        GenerateWaveformPeaks();
    }

    fCacheValid = false;
    Invalidate();
}

void WaveformView::SetTimeRange(int64 startFrame, int64 endFrame)
{
    fStartFrame = startFrame;
    fEndFrame = endFrame;
    fCacheValid = false;
    Invalidate();
}

float WaveformView::GetDuration() const
{
    if (fPeaks.sampleRate <= 0.0f) return 0.0f;
    return (float)fPeaks.totalSamples / fPeaks.sampleRate;
}

} // namespace HaikuDAW

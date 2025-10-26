/*
 * TrackColors.cpp - Implementation
 */

#include "TrackColors.h"

namespace HaikuDAW {

const TrackColor& TrackColorManager::GetColorByIndex(int index)
{
    // Clamp index to valid range
    if (index < 0) index = 0;
    if (index >= COLOR_COUNT) index = COLOR_COUNT - 1;

    return kTrackColors[index];
}

const TrackColor& TrackColorManager::GetColor(TrackColorPreset preset)
{
    return kTrackColors[(int)preset];
}

TrackColorPreset TrackColorManager::GetAutomaticColor(int trackIndex)
{
    // Cycle through colors, skipping grey/dark grey/white for better visibility
    // Use first 13 colors (all vibrant colors)
    const int kVibriantColorCount = 13;
    int colorIndex = trackIndex % kVibriantColorCount;

    return (TrackColorPreset)colorIndex;
}

const char* TrackColorManager::GetColorName(int index)
{
    if (index < 0 || index >= COLOR_COUNT) {
        return "Unknown";
    }
    return kTrackColors[index].name;
}

const char* TrackColorManager::GetColorName(TrackColorPreset preset)
{
    return kTrackColors[(int)preset].name;
}

} // namespace HaikuDAW

/*
 * TrackColors.h - Professional track color coding system
 * Provides visual organization inspired by Ableton Live and Logic Pro
 */

#ifndef TRACK_COLORS_H
#define TRACK_COLORS_H

#include <interface/GraphicsDefs.h>

namespace HaikuDAW {

/*
 * Track color structure with three variants:
 * - normal: Default track appearance
 * - selected: Highlighted when track is selected
 * - muted: Dimmed when track is muted
 */
struct TrackColor {
    rgb_color normal;
    rgb_color selected;
    rgb_color muted;
    const char* name;
};

/*
 * Predefined professional color palette (16 colors)
 * Inspired by Ableton Live's color coding system
 */
enum TrackColorPreset {
    COLOR_RED = 0,
    COLOR_ORANGE,
    COLOR_YELLOW,
    COLOR_LIME,
    COLOR_GREEN,
    COLOR_MINT,
    COLOR_CYAN,
    COLOR_SKY,
    COLOR_BLUE,
    COLOR_PURPLE,
    COLOR_MAGENTA,
    COLOR_PINK,
    COLOR_BROWN,
    COLOR_GREY,
    COLOR_DARK_GREY,
    COLOR_WHITE,

    COLOR_COUNT = 16
};

// Color palette definition
const TrackColor kTrackColors[COLOR_COUNT] = {
    // Red
    {{255, 100, 100, 255}, {255, 150, 150, 255}, {100, 50, 50, 255}, "Red"},

    // Orange
    {{255, 180, 100, 255}, {255, 210, 150, 255}, {100, 80, 50, 255}, "Orange"},

    // Yellow
    {{255, 255, 100, 255}, {255, 255, 150, 255}, {100, 100, 50, 255}, "Yellow"},

    // Lime
    {{180, 255, 100, 255}, {210, 255, 150, 255}, {80, 100, 50, 255}, "Lime"},

    // Green
    {{100, 255, 100, 255}, {150, 255, 150, 255}, {50, 100, 50, 255}, "Green"},

    // Mint
    {{100, 255, 180, 255}, {150, 255, 210, 255}, {50, 100, 80, 255}, "Mint"},

    // Cyan
    {{100, 255, 255, 255}, {150, 255, 255, 255}, {50, 100, 100, 255}, "Cyan"},

    // Sky
    {{100, 180, 255, 255}, {150, 210, 255, 255}, {50, 80, 100, 255}, "Sky"},

    // Blue
    {{100, 100, 255, 255}, {150, 150, 255, 255}, {50, 50, 100, 255}, "Blue"},

    // Purple
    {{180, 100, 255, 255}, {210, 150, 255, 255}, {80, 50, 100, 255}, "Purple"},

    // Magenta
    {{255, 100, 255, 255}, {255, 150, 255, 255}, {100, 50, 100, 255}, "Magenta"},

    // Pink
    {{255, 100, 180, 255}, {255, 150, 210, 255}, {100, 50, 80, 255}, "Pink"},

    // Brown
    {{180, 140, 100, 255}, {210, 180, 150, 255}, {80, 60, 50, 255}, "Brown"},

    // Grey
    {{160, 160, 160, 255}, {190, 190, 190, 255}, {80, 80, 80, 255}, "Grey"},

    // Dark Grey
    {{100, 100, 100, 255}, {130, 130, 130, 255}, {50, 50, 50, 255}, "Dark Grey"},

    // White
    {{240, 240, 240, 255}, {255, 255, 255, 255}, {120, 120, 120, 255}, "White"}
};

/*
 * TrackColorManager - Utility class for track color management
 */
class TrackColorManager {
public:
    // Get color by preset index
    static const TrackColor& GetColorByIndex(int index);

    // Get color by preset enum
    static const TrackColor& GetColor(TrackColorPreset preset);

    // Assign colors to tracks automatically (cycling through palette)
    static TrackColorPreset GetAutomaticColor(int trackIndex);

    // Get total number of available colors
    static int GetColorCount() { return COLOR_COUNT; }

    // Get color name for display in UI
    static const char* GetColorName(int index);
    static const char* GetColorName(TrackColorPreset preset);
};

} // namespace HaikuDAW

#endif // TRACK_COLORS_H

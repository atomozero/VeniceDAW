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

// Color palette definition - Professional DAW colors from VeniceTheme
const TrackColor kTrackColors[COLOR_COUNT] = {
    // Red - Bright and saturated
    {{220, 80, 80, 255}, {255, 120, 120, 255}, {100, 40, 40, 255}, "Red"},

    // Orange - Warm tone
    {{220, 140, 60, 255}, {255, 170, 100, 255}, {100, 70, 30, 255}, "Orange"},

    // Yellow - Clear visibility
    {{210, 190, 50, 255}, {245, 220, 90, 255}, {100, 95, 25, 255}, "Yellow"},

    // Lime - Fresh green-yellow
    {{130, 190, 70, 255}, {170, 220, 110, 255}, {65, 95, 35, 255}, "Lime"},

    // Green - Professional forest
    {{60, 180, 100, 255}, {100, 220, 140, 255}, {30, 90, 50, 255}, "Green"},

    // Teal - Cool cyan-green
    {{50, 170, 160, 255}, {90, 210, 200, 255}, {25, 85, 80, 255}, "Mint"},

    // Sky Blue - Clear professional
    {{60, 150, 220, 255}, {100, 190, 255, 255}, {30, 75, 110, 255}, "Cyan"},

    // Sky - Haiku-inspired
    {{60, 150, 220, 255}, {100, 190, 255, 255}, {30, 75, 110, 255}, "Sky"},

    // Blue - Deep professional
    {{100, 120, 220, 255}, {140, 160, 255, 255}, {50, 60, 110, 255}, "Blue"},

    // Purple - Rich violet
    {{150, 90, 210, 255}, {190, 130, 245, 255}, {75, 45, 105, 255}, "Purple"},

    // Magenta - Vibrant pink-purple
    {{200, 80, 170, 255}, {235, 120, 210, 255}, {100, 40, 85, 255}, "Magenta"},

    // Pink - Soft rose
    {{210, 90, 120, 255}, {245, 130, 160, 255}, {105, 45, 60, 255}, "Pink"},

    // Brown - Warm earth
    {{160, 120, 80, 255}, {200, 160, 120, 255}, {80, 60, 40, 255}, "Brown"},

    // Grey - Neutral professional
    {{160, 160, 160, 255}, {200, 200, 200, 255}, {80, 80, 80, 255}, "Grey"},

    // Dark Grey - Subtle contrast
    {{100, 100, 100, 255}, {140, 140, 140, 255}, {50, 50, 50, 255}, "Dark Grey"},

    // White - Maximum contrast
    {{230, 230, 230, 255}, {255, 255, 255, 255}, {115, 115, 115, 255}, "White"}
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

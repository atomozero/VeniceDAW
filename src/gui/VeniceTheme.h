/*
 * VeniceTheme.h - Unified professional theme system for VeniceDAW
 * Ensures Haiku HIG compliance and consistent visual design
 */

#ifndef VENICE_THEME_H
#define VENICE_THEME_H

#include <InterfaceDefs.h>
#include <GraphicsDefs.h>

namespace VeniceDAW {

class VeniceTheme {
public:
    // ===== Haiku-Compliant Base Colors =====

    // Use native Haiku colors for consistency
    static rgb_color PanelBackground() {
        return ui_color(B_PANEL_BACKGROUND_COLOR);
    }

    static rgb_color ControlBackground() {
        return ui_color(B_CONTROL_BACKGROUND_COLOR);
    }

    static rgb_color DocumentBackground() {
        return ui_color(B_DOCUMENT_BACKGROUND_COLOR);
    }

    static rgb_color TextColor() {
        return ui_color(B_PANEL_TEXT_COLOR);
    }

    static rgb_color ControlTextColor() {
        return ui_color(B_CONTROL_TEXT_COLOR);
    }

    // Haiku accent color (blue)
    static rgb_color AccentColor() {
        rgb_color color = {12, 148, 214, 255};  // #0C94D6 - Haiku blue
        return color;
    }

    // ===== Professional DAW Track Colors =====
    // 12 distinct, saturated colors for clear visual distinction

    static rgb_color TrackColor(int index) {
        static const rgb_color palette[] = {
            {220, 80, 80, 255},    // Red
            {220, 140, 60, 255},   // Orange
            {210, 190, 50, 255},   // Yellow
            {130, 190, 70, 255},   // Lime
            {60, 180, 100, 255},   // Green
            {50, 170, 160, 255},   // Teal
            {60, 150, 220, 255},   // Sky Blue
            {100, 120, 220, 255},  // Blue
            {150, 90, 210, 255},   // Purple
            {200, 80, 170, 255},   // Magenta
            {210, 90, 120, 255},   // Pink
            {160, 160, 160, 255}   // Gray
        };

        return palette[index % 12];
    }

    // Slightly darker version for track headers
    static rgb_color TrackHeaderColor(int index) {
        rgb_color base = TrackColor(index);
        return make_color(
            (uint8)(base.red * 0.8f),
            (uint8)(base.green * 0.8f),
            (uint8)(base.blue * 0.8f),
            255
        );
    }

    // ===== Professional Audio Meter Colors =====
    // Standard DAW metering colors (EBU scale)

    static rgb_color MeterGreen() {
        rgb_color color = {50, 200, 80, 255};  // Safe level: -24 to -12 dB
        return color;
    }

    static rgb_color MeterYellow() {
        rgb_color color = {220, 200, 50, 255};  // Caution: -12 to -3 dB
        return color;
    }

    static rgb_color MeterRed() {
        rgb_color color = {220, 60, 60, 255};  // Danger: -3 to 0 dB
        return color;
    }

    static rgb_color MeterClipRed() {
        rgb_color color = {255, 0, 0, 255};  // Clipping: > 0 dB
        return color;
    }

    static rgb_color MeterBackground() {
        rgb_color color = {40, 40, 40, 255};  // Dark background for meters
        return color;
    }

    // ===== Haiku HIG Spacing Constants =====

    static constexpr float PADDING = 5.0f;      // Internal padding
    static constexpr float SPACING = 3.0f;      // Space between controls
    static constexpr float MARGIN = 10.0f;      // Window margins
    static constexpr float INSET = 8.0f;        // Group inset

    // Control sizes (Haiku standard)
    static constexpr float BUTTON_HEIGHT = 24.0f;
    static constexpr float SLIDER_WIDTH = 200.0f;
    static constexpr float TEXTBOX_HEIGHT = 20.0f;

    // DAW-specific sizes - Ultra compact professional layout
    static constexpr float CHANNEL_STRIP_WIDTH = 60.0f;   // Very compact mixer strip
    static constexpr float FADER_HEIGHT = 180.0f;         // Taller vertical fader
    static constexpr float METER_WIDTH = 4.0f;            // Thin VU meter bar
    static constexpr float KNOB_SIZE = 40.0f;             // Rotary knob

    // ===== Utility Functions =====

    // Tint color (lighter/darker)
    static rgb_color Tint(rgb_color color, float amount) {
        return tint_color(color, amount);
    }

    // Blend two colors
    static rgb_color Blend(rgb_color a, rgb_color b, float ratio) {
        return make_color(
            (uint8)(a.red * (1.0f - ratio) + b.red * ratio),
            (uint8)(a.green * (1.0f - ratio) + b.green * ratio),
            (uint8)(a.blue * (1.0f - ratio) + b.blue * ratio),
            255
        );
    }

    // Dim color (reduce opacity visually)
    static rgb_color Dim(rgb_color color, float factor) {
        return make_color(
            (uint8)(color.red * factor),
            (uint8)(color.green * factor),
            (uint8)(color.blue * factor),
            color.alpha
        );
    }
};

} // namespace VeniceDAW

#endif // VENICE_THEME_H

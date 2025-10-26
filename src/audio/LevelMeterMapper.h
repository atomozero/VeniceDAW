/*
 * LevelMeterMapper.h - Pre-calculated lookup tables for level meter visualization
 * Eliminates runtime calculations for dB-to-color conversion
 */

#ifndef LEVEL_METER_MAPPER_H
#define LEVEL_METER_MAPPER_H

#include <stdint.h>

namespace HaikuDAW {

/*
 * LevelMeterMapper - Fast color mapping for audio level meters
 *
 * Purpose:
 * - Pre-calculate dB → color conversions at startup
 * - Zero runtime overhead (single array lookup)
 * - Smooth gradients instead of harsh color steps
 *
 * Color scheme (professional metering standard):
 * - < -60 dB: Black (silence)
 * - -60 to -18 dB: Green (safe headroom)
 * - -18 to -6 dB: Green → Yellow (caution)
 * - -6 to 0 dB: Yellow → Orange (warning)
 * - > 0 dB: Red (clipping!)
 */
class LevelMeterMapper {
public:
    // Lookup table size (256 entries = 1 byte index)
    static constexpr int TABLE_SIZE = 256;

    // Color structure (RGB)
    struct LevelColor {
        uint8_t r;
        uint8_t g;
        uint8_t b;

        LevelColor() : r(0), g(0), b(0) {}
        LevelColor(uint8_t red, uint8_t green, uint8_t blue)
            : r(red), g(green), b(blue) {}
    };

    // Get color for linear audio level [0.0 - 1.0+]
    LevelColor GetColor(float linearLevel) const;

    // Get color for dB level (alternative API)
    LevelColor GetColorFromDB(float dB) const;

    // Singleton access
    static LevelMeterMapper& GetInstance();

private:
    LevelMeterMapper();
    ~LevelMeterMapper() = default;

    // Non-copyable
    LevelMeterMapper(const LevelMeterMapper&) = delete;
    LevelMeterMapper& operator=(const LevelMeterMapper&) = delete;

    // Pre-calculated lookup table
    LevelColor fColorTable[TABLE_SIZE];

    // Helper: Build lookup table at construction
    void _BuildColorTable();

    // Helper: Interpolate between two colors
    static LevelColor _Interpolate(const LevelColor& c1, const LevelColor& c2, float t);
};

} // namespace HaikuDAW

#endif // LEVEL_METER_MAPPER_H

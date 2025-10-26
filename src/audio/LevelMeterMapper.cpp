/*
 * LevelMeterMapper.cpp - Implementation of level meter color mapping
 */

#include "LevelMeterMapper.h"
#include <math.h>
#include <stdio.h>

namespace HaikuDAW {

LevelMeterMapper::LevelMeterMapper()
{
    _BuildColorTable();
    printf("LevelMeterMapper: Initialized with %d-entry lookup table\n", TABLE_SIZE);
}

void LevelMeterMapper::_BuildColorTable()
{
    // Define color breakpoints (professional metering standard)
    const float DB_SILENCE = -60.0f;
    const float DB_GREEN = -18.0f;
    const float DB_YELLOW = -6.0f;
    const float DB_CLIP = 0.0f;

    // Define colors
    const LevelColor BLACK(0, 0, 0);
    const LevelColor GREEN(0, 255, 0);
    const LevelColor YELLOW(255, 255, 0);
    const LevelColor ORANGE(255, 127, 0);
    const LevelColor RED(255, 0, 0);

    for (int i = 0; i < TABLE_SIZE; i++) {
        // Map table index to linear level [0.0 - 2.0]
        // (2.0 = 200% for clipping visualization)
        float linearLevel = (float)i / (float)(TABLE_SIZE - 1) * 2.0f;

        // Convert linear to dB (avoid log(0) with small epsilon)
        float dB;
        if (linearLevel < 0.0001f) {
            dB = -100.0f;  // Very quiet
        } else {
            dB = 20.0f * log10f(linearLevel);
        }

        // Map dB to color based on breakpoints
        if (dB < DB_SILENCE) {
            // Silence zone: black
            fColorTable[i] = BLACK;
        }
        else if (dB < DB_GREEN) {
            // -60 to -18 dB: black → green gradient
            float t = (dB - DB_SILENCE) / (DB_GREEN - DB_SILENCE);
            fColorTable[i] = _Interpolate(BLACK, GREEN, t);
        }
        else if (dB < DB_YELLOW) {
            // -18 to -6 dB: green → yellow gradient
            float t = (dB - DB_GREEN) / (DB_YELLOW - DB_GREEN);
            fColorTable[i] = _Interpolate(GREEN, YELLOW, t);
        }
        else if (dB < DB_CLIP) {
            // -6 to 0 dB: yellow → orange gradient
            float t = (dB - DB_YELLOW) / (DB_CLIP - DB_YELLOW);
            fColorTable[i] = _Interpolate(YELLOW, ORANGE, t);
        }
        else {
            // > 0 dB: CLIPPING! Red zone
            fColorTable[i] = RED;
        }
    }
}

LevelMeterMapper::LevelColor LevelMeterMapper::GetColor(float linearLevel) const
{
    // Clamp to valid range [0.0 - 2.0]
    if (linearLevel <= 0.0f) return fColorTable[0];
    if (linearLevel >= 2.0f) return fColorTable[TABLE_SIZE - 1];

    // Convert to table index
    int index = (int)(linearLevel / 2.0f * (TABLE_SIZE - 1));
    if (index < 0) index = 0;
    if (index >= TABLE_SIZE) index = TABLE_SIZE - 1;

    return fColorTable[index];
}

LevelMeterMapper::LevelColor LevelMeterMapper::GetColorFromDB(float dB) const
{
    // Convert dB to linear level
    float linearLevel = powf(10.0f, dB / 20.0f);
    return GetColor(linearLevel);
}

LevelMeterMapper::LevelColor LevelMeterMapper::_Interpolate(
    const LevelColor& c1,
    const LevelColor& c2,
    float t)
{
    // Clamp interpolation factor
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    // Linear interpolation in RGB space
    LevelColor result;
    result.r = (uint8_t)(c1.r + (c2.r - c1.r) * t);
    result.g = (uint8_t)(c1.g + (c2.g - c1.g) * t);
    result.b = (uint8_t)(c1.b + (c2.b - c1.b) * t);

    return result;
}

LevelMeterMapper& LevelMeterMapper::GetInstance()
{
    static LevelMeterMapper instance;
    return instance;
}

} // namespace HaikuDAW

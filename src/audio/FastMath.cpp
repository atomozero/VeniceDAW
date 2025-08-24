/*
 * FastMath.cpp - Fast math utilities implementation
 */

#include "FastMath.h"
#include <cmath>

namespace HaikuDAW {

// Initialize static members
float* FastMath::sSinTable = nullptr;
bool FastMath::sInitialized = false;

void FastMath::Initialize()
{
    if (sInitialized) return;
    
    // Allocate lookup table (4K entries for 0-2π)
    const int tableSize = 4096;
    sSinTable = new float[tableSize];
    
    // Fill lookup table
    for (int i = 0; i < tableSize; i++) {
        float angle = (2.0f * M_PI * i) / tableSize;
        sSinTable[i] = sinf(angle);
    }
    
    sInitialized = true;
}

void FastMath::Cleanup()
{
    if (sSinTable) {
        delete[] sSinTable;
        sSinTable = nullptr;
    }
    sInitialized = false;
}

float FastMath::Sin(float angle)
{
    if (!sInitialized) Initialize();
    
    // Normalize angle to 0-2π
    while (angle < 0) angle += 2.0f * M_PI;
    while (angle >= 2.0f * M_PI) angle -= 2.0f * M_PI;
    
    // Convert to table index
    const int tableSize = 4096;
    float indexFloat = (angle / (2.0f * M_PI)) * tableSize;
    int index = (int)indexFloat;
    float frac = indexFloat - index;
    
    // Linear interpolation between table entries
    int nextIndex = (index + 1) % tableSize;
    return sSinTable[index] * (1.0f - frac) + sSinTable[nextIndex] * frac;
}

float FastMath::Cos(float angle)
{
    // cos(x) = sin(x + π/2)
    return Sin(angle + M_PI / 2.0f);
}

} // namespace HaikuDAW
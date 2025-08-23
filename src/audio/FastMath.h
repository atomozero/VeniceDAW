/*
 * FastMath.h - Optimized math functions for real-time audio
 */

#ifndef FAST_MATH_H
#define FAST_MATH_H

#include <cmath>
#include <cstdint>

namespace HaikuDAW {

class FastMath {
public:
    // Sine lookup table for fast sine wave generation
    static constexpr int SINE_TABLE_SIZE = 4096;
    static float sineTable[SINE_TABLE_SIZE];
    static bool initialized;
    
    static void Initialize() {
        if (initialized) return;
        
        for (int i = 0; i < SINE_TABLE_SIZE; i++) {
            float phase = (float)i * 2.0f * M_PI / SINE_TABLE_SIZE;
            sineTable[i] = sinf(phase);
        }
        initialized = true;
    }
    
    // Fast sine using lookup table with linear interpolation
    static inline float FastSin(float phase) {
        if (!initialized) Initialize();
        
        // Normalize phase to [0, 2*PI)
        while (phase < 0) phase += 2.0f * M_PI;
        while (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        
        // Convert to table index
        float indexFloat = phase * SINE_TABLE_SIZE / (2.0f * M_PI);
        int index0 = (int)indexFloat;
        int index1 = (index0 + 1) & (SINE_TABLE_SIZE - 1);
        float fract = indexFloat - index0;
        
        // Linear interpolation for smoother output
        return sineTable[index0] * (1.0f - fract) + sineTable[index1] * fract;
    }
    
    // Fast phase wrapping without expensive modulo
    static inline float WrapPhase(float phase) {
        static constexpr float INV_TWO_PI = 1.0f / (2.0f * M_PI);
        return phase - floorf(phase * INV_TWO_PI) * 2.0f * M_PI;
    }
    
    // Pre-calculate gain values for panning
    struct StereoGain {
        float left;
        float right;
    };
    
    static inline StereoGain CalculatePanGains(float pan) {
        // Pan: -1.0 (full left) to +1.0 (full right)
        // Use constant power panning for better perceived balance
        float angle = (pan + 1.0f) * 0.25f * M_PI; // 0 to PI/2
        return {
            cosf(angle),  // Left gain
            sinf(angle)   // Right gain
        };
    }
};

// Static member initialization
inline float FastMath::sineTable[FastMath::SINE_TABLE_SIZE];
inline bool FastMath::initialized = false;

} // namespace HaikuDAW

#endif // FAST_MATH_H
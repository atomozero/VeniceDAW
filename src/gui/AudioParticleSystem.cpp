/*
 * AudioParticleSystem.cpp - Implementation of 3D particle visualization
 */

#include "AudioParticleSystem.h"
#include <stdlib.h>
#include <math.h>
#include <algorithm>

namespace HaikuDAW {

AudioParticleSystem::AudioParticleSystem()
    : fEnabled(false)
    , fMaxParticles(2048)  // Same as BeOS R6 3d_mixer
    , fParticleLifetime(2.0f)  // 2 seconds
    , fEmissionRate(30.0f)  // 30 particles/second per active track
    , fGravity(-0.5f)  // Gentle downward drift
    , fEmissionAccumulator(0.0f)
{
    // Reserve space to avoid allocations during emission
    fParticles.reserve(fMaxParticles);
}

void AudioParticleSystem::EmitFromTrack(int trackIndex, float x, float y, float z, float audioLevel)
{
    if (!fEnabled) return;
    if (audioLevel < 0.05f) return;  // Only emit for audible signal

    // Get track-specific color
    uint8_t r, g, b;
    _GetTrackColor(trackIndex, r, g, b);

    // Modulate emission rate by audio level
    float emissionCount = fEmissionRate * audioLevel;

    // Emit particles (may be fractional, accumulated over time)
    fEmissionAccumulator += emissionCount;
    int particlesToEmit = (int)fEmissionAccumulator;
    fEmissionAccumulator -= particlesToEmit;

    for (int i = 0; i < particlesToEmit; i++) {
        _EmitParticle(x, y, z, audioLevel, r, g, b);
    }
}

void AudioParticleSystem::EmitBurst(float x, float y, float z, int count, float intensity)
{
    if (!fEnabled) return;

    for (int i = 0; i < count; i++) {
        _EmitParticle(x, y, z, intensity, 255, 255, 255);
    }
}

void AudioParticleSystem::Update(float deltaTime)
{
    if (!fEnabled) return;

    // Update all particles
    for (auto& particle : fParticles) {
        _UpdateParticle(particle, deltaTime);
    }

    // Remove dead particles
    _RemoveDeadParticles();
}

void AudioParticleSystem::Render()
{
    if (!fEnabled || fParticles.empty()) return;

    // OpenGL state for particle rendering
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);  // Particles always visible
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive blending

    glPointSize(2.0f);
    glBegin(GL_POINTS);

    for (const auto& p : fParticles) {
        // Alpha based on lifetime (fade out)
        float alpha = p.life;

        // Size variation based on lifetime
        float sizeMultiplier = 0.5f + p.life * 0.5f;

        glColor4f(p.r / 255.0f, p.g / 255.0f, p.b / 255.0f, alpha);
        glVertex3f(p.x, p.y, p.z);
    }

    glEnd();

    // Restore OpenGL state
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void AudioParticleSystem::Clear()
{
    fParticles.clear();
    fEmissionAccumulator = 0.0f;
}

// Private methods

void AudioParticleSystem::_EmitParticle(float x, float y, float z, float audioLevel,
                                        uint8_t r, uint8_t g, uint8_t b)
{
    // Don't exceed max particles
    if ((int)fParticles.size() >= fMaxParticles) {
        return;
    }

    // Random velocity based on audio level
    float speed = 0.5f + audioLevel * 2.0f;

    // Random direction (spherical distribution)
    float theta = (rand() % 360) * (M_PI / 180.0f);
    float phi = (rand() % 180) * (M_PI / 180.0f);

    float vx = speed * sinf(phi) * cosf(theta);
    float vy = speed * sinf(phi) * sinf(theta);
    float vz = speed * cosf(phi);

    // Add upward bias (particles rise from tracks)
    vy += 1.0f;

    // Random size variation
    float size = 1.0f + (rand() % 100) / 100.0f;

    fParticles.emplace_back(x, y, z, vx, vy, vz, r, g, b, size);
}

void AudioParticleSystem::_UpdateParticle(Particle& p, float deltaTime)
{
    // Update position
    p.x += p.dx * deltaTime;
    p.y += p.dy * deltaTime;
    p.z += p.dz * deltaTime;

    // Apply gravity
    p.dy += fGravity * deltaTime;

    // Decay velocity (air resistance)
    float drag = 0.98f;
    p.dx *= drag;
    p.dy *= drag;
    p.dz *= drag;

    // Decrease lifetime
    p.life -= deltaTime / fParticleLifetime;
    if (p.life < 0.0f) p.life = 0.0f;
}

void AudioParticleSystem::_RemoveDeadParticles()
{
    // Remove particles with life <= 0
    fParticles.erase(
        std::remove_if(fParticles.begin(), fParticles.end(),
            [](const Particle& p) { return p.life <= 0.0f; }),
        fParticles.end()
    );
}

void AudioParticleSystem::_GetTrackColor(int trackIndex, uint8_t& r, uint8_t& g, uint8_t& b)
{
    // Cycle through rainbow colors based on track index
    static const uint8_t colors[][3] = {
        {255, 100, 100},  // Red
        {100, 255, 100},  // Green
        {100, 100, 255},  // Blue
        {255, 255, 100},  // Yellow
        {255, 100, 255},  // Magenta
        {100, 255, 255},  // Cyan
        {255, 200, 100},  // Orange
        {200, 100, 255}   // Purple
    };

    int colorIndex = trackIndex % 8;
    r = colors[colorIndex][0];
    g = colors[colorIndex][1];
    b = colors[colorIndex][2];
}

} // namespace HaikuDAW

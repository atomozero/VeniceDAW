/*
 * AudioParticleSystem.h - 3D particle effects for audio visualization
 * Inspired by BeOS R6 3d_mixer particle system
 */

#ifndef AUDIO_PARTICLE_SYSTEM_H
#define AUDIO_PARTICLE_SYSTEM_H

#include <vector>
#include <GL/gl.h>

namespace HaikuDAW {

/*
 * AudioParticleSystem - Dynamic particle effects for 3D audio visualization
 *
 * Purpose:
 * - Emit particles from active audio tracks during playback
 * - Visual feedback for audio activity and intensity
 * - Inspired by BeOS R6 3d_mixer particle rendering
 *
 * Features:
 * - Particles emanate from track positions in 3D space
 * - Velocity-based motion with gravity/decay
 * - Color-coded based on track or audio level
 * - Automatic lifecycle management (emit → update → cleanup)
 *
 * Usage:
 *   AudioParticleSystem particles;
 *   particles.SetEnabled(true);
 *
 *   // In audio callback:
 *   particles.EmitFromTrack(trackIndex, position, audioLevel);
 *
 *   // In render loop:
 *   particles.Update(deltaTime);
 *   particles.Render();
 */
class AudioParticleSystem {
public:
    AudioParticleSystem();
    ~AudioParticleSystem() = default;

    // Control
    void SetEnabled(bool enabled) { fEnabled = enabled; }
    bool IsEnabled() const { return fEnabled; }

    // Particle emission
    void EmitFromTrack(int trackIndex, float x, float y, float z, float audioLevel);
    void EmitBurst(float x, float y, float z, int count, float intensity);

    // Lifecycle
    void Update(float deltaTime);  // Update positions, remove dead particles
    void Render();                 // OpenGL rendering
    void Clear();                  // Remove all particles

    // Configuration
    void SetMaxParticles(int max) { fMaxParticles = max; }
    void SetParticleLifetime(float seconds) { fParticleLifetime = seconds; }
    void SetEmissionRate(float particlesPerSecond) { fEmissionRate = particlesPerSecond; }
    void SetGravity(float gravity) { fGravity = gravity; }

    // Statistics
    int GetActiveParticleCount() const { return fParticles.size(); }

private:
    struct Particle {
        // Position in 3D space
        float x, y, z;

        // Velocity
        float dx, dy, dz;

        // Lifetime (0.0 = dead, 1.0 = just born)
        float life;

        // Color (RGB)
        uint8_t r, g, b;

        // Size
        float size;

        Particle(float px, float py, float pz, float vx, float vy, float vz,
                 uint8_t cr, uint8_t cg, uint8_t cb, float sz = 1.0f)
            : x(px), y(py), z(pz)
            , dx(vx), dy(vy), dz(vz)
            , life(1.0f)
            , r(cr), g(cg), b(cb)
            , size(sz)
        {}
    };

    // Particle pool
    std::vector<Particle> fParticles;

    // Configuration
    bool fEnabled;
    int fMaxParticles;
    float fParticleLifetime;  // Seconds
    float fEmissionRate;      // Particles/second
    float fGravity;           // Downward acceleration

    // Emission tracking
    float fEmissionAccumulator;  // For sub-frame emission timing

    // Internal methods
    void _EmitParticle(float x, float y, float z, float audioLevel, uint8_t r, uint8_t g, uint8_t b);
    void _UpdateParticle(Particle& p, float deltaTime);
    void _RemoveDeadParticles();
    void _GetTrackColor(int trackIndex, uint8_t& r, uint8_t& g, uint8_t& b);
};

} // namespace HaikuDAW

#endif // AUDIO_PARTICLE_SYSTEM_H

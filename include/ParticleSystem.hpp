// =============================================================================
// ParticleSystem.hpp
//
// Short-lived visual effects: juice splashes when a fruit is sliced and an
// explosion when a bomb is hit.
//
// A particle is just a small colored circle with a position, a velocity and a
// lifetime. Every frame particles move, fall under gravity, fade out and
// shrink; when their lifetime runs out they are removed.
// =============================================================================
#pragma once

#include <raylib.h>   // Vector2, Color
#include <vector>

// A plain "aggregate" struct (no constructors, all public members), so it can
// be created with brace initialization: Particle{ pos, vel, color, ... }.
// The values must be given in the same order as the members are declared.
struct Particle {
    Vector2 position;    // pixels
    Vector2 velocity;    // pixels per second
    Color color;
    float radius;        // pixels, at full life
    float alpha{1.0f};   // opacity, recomputed from lifeTime every frame
    float lifeTime;      // seconds left before the particle disappears
    float maxLifeTime;   // the starting lifetime, used to compute alpha
};

class ParticleSystem {
public:
    // `= default` asks the compiler to generate the default constructor.
    // (It would be generated anyway here; writing it just documents intent.)
    ParticleSystem() = default;

    // Burst of `count` particles of the fruit's color flying in all directions.
    void spawnJuiceSplash(Vector2 position, Color color, int count = 25);

    // Bigger, faster burst of orange/red particles for a bomb.
    void spawnBombExplosion(Vector2 position, int count = 45);

    // Move, fade and remove expired particles. dt is in seconds.
    void update(float dt);

    // Render all live particles.
    void draw() const;

    // Remove all particles immediately (used when the game restarts).
    void clear();

private:
    // All live particles. std::vector stores them contiguously in memory,
    // which is very cache-friendly when we loop over all of them every frame.
    std::vector<Particle> m_particles;
};

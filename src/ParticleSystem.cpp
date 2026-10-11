// =============================================================================
// ParticleSystem.cpp - spawning, simulating and drawing particles.
// See ParticleSystem.hpp for an overview.
// =============================================================================
#include "ParticleSystem.hpp"
#include <raymath.h>    // DEG2RAD
#include <algorithm>    // std::clamp, std::remove_if

void ParticleSystem::spawnJuiceSplash(Vector2 position, Color color, int count) {
    for (int i = 0; i < count; ++i) {
        // Pick a random direction (0-360 degrees, converted to radians because
        // cosf/sinf work in radians) and a random speed.
        float angle = static_cast<float>(GetRandomValue(0, 360)) * DEG2RAD;
        float speed = static_cast<float>(GetRandomValue(120, 480));

        // Turn (angle, speed) into a velocity vector: this is the conversion
        // from polar coordinates to Cartesian (x, y) coordinates.
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };

        float size = static_cast<float>(GetRandomValue(3, 8));                 // radius 3-8 px
        float duration = static_cast<float>(GetRandomValue(40, 90)) / 100.0f;  // 0.4-0.9 s

        // Aggregate initialization - the values follow the member order of
        // Particle: position, velocity, color, radius, alpha, lifeTime, maxLifeTime.
        // (With C++20 designated initializers you could write
        //  { .position = position, .velocity = vel, ... } which is clearer.)
        m_particles.push_back({ position, vel, color, size, 1.0f, duration, duration });
    }
}

void ParticleSystem::spawnBombExplosion(Vector2 position, int count) {
    // Same idea as the juice splash, but faster, bigger and longer-lived.
    for (int i = 0; i < count; ++i) {
        float angle = static_cast<float>(GetRandomValue(0, 360)) * DEG2RAD;
        float speed = static_cast<float>(GetRandomValue(180, 650));
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
        float size = static_cast<float>(GetRandomValue(4, 11));
        float duration = static_cast<float>(GetRandomValue(50, 110)) / 100.0f;  // 0.5-1.1 s

        // Alternate orange and red so the explosion looks like fire.
        Color fireColor = (i % 2 == 0) ? ORANGE : RED;
        m_particles.push_back({ position, vel, fireColor, size, 1.0f, duration, duration });
    }
}

void ParticleSystem::update(float dt) {
    // Lighter than the fruits' gravity (820), so juice "floats" a bit longer.
    constexpr float gravity = 550.0f;

    for (auto& p : m_particles) {
        // Same Euler integration as Fruit::update.
        p.velocity.y += gravity * dt;
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;

        // Count down the lifetime and derive the opacity from it:
        // 1.0 when just spawned, 0.0 when it expires.
        // std::clamp (C++17) limits a value to the range [low, high].
        p.lifeTime -= dt;
        p.alpha = std::clamp(p.lifeTime / p.maxLifeTime, 0.0f, 1.0f);
    }

    // The "erase-remove idiom":
    //   1. std::remove_if moves all particles we want to KEEP to the front of
    //      the vector and returns an iterator to the start of the leftovers.
    //   2. erase() then deletes everything from that iterator to the end.
    // This removes many elements in a single pass, which is much faster than
    // calling erase() once per dead particle.
    //
    // The `[](const Particle& p) { ... }` part is a lambda: a small unnamed
    // function written inline. `[]` means it captures no outside variables.
    //
    // (C++20 shortcut: `std::erase_if(m_particles, predicate);` does both steps.)
    m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(),
        [](const Particle& p) { return p.lifeTime <= 0.0f; }),
        m_particles.end());
}

void ParticleSystem::draw() const {
    for (const auto& p : m_particles) {
        // Particles both fade AND shrink as they die, since both the radius
        // and the color's opacity are multiplied by alpha.
        DrawCircleV(p.position, p.radius * p.alpha, ColorAlpha(p.color, p.alpha));
    }
}

void ParticleSystem::clear() {
    m_particles.clear();
}

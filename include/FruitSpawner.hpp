// =============================================================================
// FruitSpawner.hpp
//
// Decides WHEN and WHERE new fruits appear.
//
// Fruits are launched in "waves":
//   - Every m_spawnInterval seconds (randomized between 1.7s and 2.5s) a new
//     wave is scheduled, containing 1-3 objects.
//   - The objects of a wave are not launched all at once; they come out one
//     by one, m_fruitSpawnDelay seconds apart, which feels more natural.
// =============================================================================
#pragma once

#include "Fruit.hpp"
#include <vector>

class FruitSpawner {
public:
    // The spawner needs the screen size to choose valid spawn positions.
    FruitSpawner(int screenWidth, int screenHeight);

    // Advance the timers by dt seconds, and append any newly spawned fruits
    // to `fruits`.
    //
    // `fruits` is passed by non-const reference (`&`): the function modifies
    // the caller's vector directly instead of working on a copy.
    void update(float dt, std::vector<Fruit>& fruits);

    // Restore the initial timer values (used when the game restarts).
    void reset();

private:
    int m_screenWidth;
    int m_screenHeight;

    float m_timer{0.0f};          // time since the last wave was scheduled
    float m_spawnInterval{1.6f};  // time until the next wave (first wave: 1.6s)
    float m_fruitSpawnTimer{0.0f};// time since the last fruit of the wave was launched
    int m_pendingFruits{0};       // fruits scheduled but not launched yet

    // `static constexpr`: one value shared by all FruitSpawner objects,
    // known at compile time. It takes no space inside each object.
    static constexpr float m_fruitSpawnDelay{0.20f}; // seconds between fruits in a wave
};

// =============================================================================
// FruitSpawner.cpp - timing and randomization of new fruits.
// See FruitSpawner.hpp for an overview of how waves work.
// =============================================================================
#include "FruitSpawner.hpp"

FruitSpawner::FruitSpawner(int screenWidth, int screenHeight)
    : m_screenWidth(screenWidth), m_screenHeight(screenHeight) {}

void FruitSpawner::update(float dt, std::vector<Fruit>& fruits) {
    // ---- Step 1: schedule a new wave when the wave timer runs out ----
    m_timer += dt;
    if (m_timer >= m_spawnInterval) {
        // Subtract instead of resetting to 0, so leftover time is not lost
        // (keeps the timing accurate even with uneven frame times).
        m_timer -= m_spawnInterval;

        // raylib's GetRandomValue(min, max) returns an int in [min, max],
        // both ends included. Each wave has 1-3 objects.
        m_pendingFruits += GetRandomValue(1, 3);

        // spawn interval between 1.7 and 2.5 seconds
        // (pick an int 17..25 and divide by 10 to get one decimal place)
        m_spawnInterval = static_cast<float>(GetRandomValue(17, 25)) / 10.0f;
    }

    // ---- Step 2: launch the scheduled fruits one at a time ----
    if (m_pendingFruits > 0) {
        m_fruitSpawnTimer += dt;
        if (m_fruitSpawnTimer >= m_fruitSpawnDelay) {
            m_fruitSpawnTimer -= m_fruitSpawnDelay;

            // Random horizontal position, keeping a 220px margin from the
            // left/right edges so fruits stay on screen.
            float spawnX = static_cast<float>(GetRandomValue(220, m_screenWidth - 220));
            // Random sideways drift, left or right.
            float velX = static_cast<float>(GetRandomValue(-170, 170));
            // Strong upward launch (negative = up). With gravity 820 px/s^2,
            // a launch of 700-820 px/s peaks about 300-410 px above the spawn
            // point (peak height = v^2 / (2 * g)).
            float velY = static_cast<float>(GetRandomValue(-820, -700));

            // 1/10 is probability of spawning a bomb, otherwise spawn a random fruit
            bool isBomb = (GetRandomValue(1, 10) == 1);
            // Converting an int to an enum class requires an explicit cast.
            // 0, 1, 2 map to Watermelon, Orange, Apple (see Types.hpp).
            FruitType type = isBomb ? FruitType::Bomb : static_cast<FruitType>(GetRandomValue(0, 2));

            // emplace_back constructs the Fruit DIRECTLY inside the vector,
            // forwarding these arguments to Fruit's constructor. push_back
            // would first create a temporary Fruit and then move/copy it.
            // The fruit starts 40px below the bottom edge, out of view.
            fruits.emplace_back(Vector2{ spawnX, static_cast<float>(m_screenHeight + 40) },
                                Vector2{ velX, velY }, type);
            --m_pendingFruits;
        }
    } else {
        // Nothing pending: reset the per-fruit timer so the first fruit of the
        // next wave waits the full delay.
        m_fruitSpawnTimer = 0.0f;
    }
}

void FruitSpawner::reset() {
    // Must match the default member initializers in FruitSpawner.hpp.
    // (Alternative modern trick: `*this = FruitSpawner(m_screenWidth, m_screenHeight);`
    // which can never get out of sync with the header.)
    m_timer = 0.0f;
    m_spawnInterval = 1.6f;
    m_fruitSpawnTimer = 0.0f;
    m_pendingFruits = 0;
}

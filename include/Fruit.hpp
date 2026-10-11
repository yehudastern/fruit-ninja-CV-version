// =============================================================================
// Fruit.hpp
//
// A single object flying across the screen: a fruit or a bomb.
//
// Lifecycle of a Fruit:
//   1. Spawned below the bottom edge with an upward velocity (FruitSpawner).
//   2. Flies in a parabola under gravity (update()).
//   3a. If the blade crosses it -> it becomes "sliced" and splits into two
//       halves that fly apart and fall (checkSlice()).
//   3b. If it falls back below the screen without being sliced -> "missed"
//       (isMissed()), which costs the player a strike (bombs excluded).
//   4. Once it (or both halves) leaves the screen, main.cpp removes it
//      (isOffScreen()).
// =============================================================================
#pragma once

#include "Types.hpp"   // FruitType
#include <raylib.h>    // Vector2, Color

class Fruit {
public:
    // spawnPos: starting position in pixels (usually just below the screen).
    // velocity: initial velocity in pixels per second (negative y = upwards,
    //           because in screen coordinates y grows downwards).
    // type:     which fruit (or bomb) this is.
    // radius:   size of the fruit in pixels; also used for collision.
    Fruit(Vector2 spawnPos, Vector2 velocity, FruitType type, float radius = 42.0f);

    // Advance the physics simulation by dt seconds.
    void update(float dt);

    // Render the fruit (whole, or as two halves if it was sliced).
    void draw() const;

    // Test whether the line segment p1 -> p2 (the blade's latest movement)
    // crosses this fruit's circle. If it does, the fruit becomes sliced and
    // the function returns true. Returns false if there was no hit or if the
    // fruit was already sliced, so each fruit can only be scored once.
    //
    // Not `const` because a successful hit changes the fruit's state.
    bool checkSlice(Vector2 p1, Vector2 p2);

    // True once the fruit (or both of its halves) has fallen far enough below
    // the screen that it can be deleted.
    bool isOffScreen(int screenHeight) const;

    // True if an un-sliced, non-bomb fruit fell below the screen,
    // i.e. the player missed it and should receive a strike.
    bool isMissed(int screenHeight) const;

    // Small accessors defined inline in the header. Functions defined inside
    // the class body are implicitly `inline`, which lets the compiler
    // optimize the call away completely.
    bool isSliced() const { return m_sliced; }
    bool isBomb() const { return m_type == FruitType::Bomb; }
    Vector2 getPosition() const { return m_pos; }

    // The color of the juice particles spawned when this fruit is sliced.
    Color getSplashColor() const;

private:
    // Helpers used by draw().
    void drawWhole() const;
    // Draws one half of a sliced fruit as a half-circle ("sector").
    // rot is the cut angle in degrees; isLeft picks which of the two halves.
    void drawHalf(Vector2 pos, float rot, bool isLeft) const;

    // ---- State of the whole (un-sliced) fruit ----
    Vector2 m_pos;          // center position in pixels
    Vector2 m_velocity;     // pixels per second
    float m_radius;         // pixels
    FruitType m_type;
    float m_rotation{0.0f}; // degrees. NOTE: updated but not used when drawing yet.
    float m_rotSpeed{140.0f}; // degrees per second

    // ---- State of the sliced fruit ----
    // These members are only meaningful when m_sliced == true.
    // (A good modern C++ exercise: group them in a struct and store it in a
    // std::optional, so "sliced" and "has halves" can never disagree.)
    bool m_sliced{false};
    Vector2 m_half1Pos;
    Vector2 m_half1Vel;
    Vector2 m_half2Pos;
    Vector2 m_half2Vel;
    float m_sliceAngle{0.0f};  // angle of the cut, in degrees
};

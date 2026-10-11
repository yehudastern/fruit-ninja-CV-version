// =============================================================================
// Types.hpp
//
// Small, dependency-free types shared across the whole game.
// Keeping them in their own header means any file can use them without
// pulling in raylib or Winsock.
// =============================================================================

// `#pragma once` tells the compiler to include this file only once per
// translation unit (.cpp file), even if it is #included several times.
// It replaces the classic `#ifndef X / #define X / #endif` include guard.
#pragma once

// A 2D point in *normalized* coordinates, as sent by the Python vision server.
// Both x and y are in the range [0, 1], where (0, 0) is the top-left of the
// camera image and (1, 1) is the bottom-right. main.cpp multiplies them by the
// window size to get screen pixels.
//
// The `{0.0f}` syntax is a "default member initializer" (C++11): any Point2D
// created without explicit values starts at (0, 0) instead of garbage memory.
struct Point2D {
    float x{0.0f};
    float y{0.0f};
};

// The kinds of objects that can be thrown onto the screen.
//
// `enum class` (a "scoped enumeration", C++11) is preferred over plain `enum`:
//   - names must be qualified (FruitType::Apple), so they don't leak into the
//     surrounding scope and clash with other names;
//   - it does NOT implicitly convert to int, which prevents accidental
//     arithmetic or comparisons with unrelated numbers.
//
// NOTE: FruitSpawner relies on the order of the first three values
// (it does `static_cast<FruitType>(GetRandomValue(0, 2))`), so adding a new
// fruit means updating the spawner too.
enum class FruitType {
    Watermelon,
    Orange,
    Apple,
    Bomb
};

// The top-level state machine of the game loop in main.cpp.
//   Playing  -> fruits spawn, slicing scores points, misses/bombs add strikes.
//   GameOver -> the world freezes and we wait for a restart (key R or a swipe).
enum class GameState {
    Playing,
    GameOver
};

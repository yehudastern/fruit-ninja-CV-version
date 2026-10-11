// =============================================================================
// main.cpp - entry point and game loop.
//
// The program is a classic real-time game loop. Every frame (~60 times a
// second) it does three things:
//
//   1. INPUT   - read the newest fingertip position from the network and add
//                it to the blade trail.
//   2. UPDATE  - spawn fruits, move everything, check for slices and misses,
//                update score / strikes / game state.
//   3. RENDER  - draw fruits, particles, the blade and the HUD.
//
// All game state currently lives as local variables inside main().
// (A good refactoring exercise: move it into a `Game` class with
//  update() / draw() / reset() methods.)
// =============================================================================
#include "NetworkReceiver.hpp"
#include "Blade.hpp"
#include "Fruit.hpp"
#include "FruitSpawner.hpp"
#include "ParticleSystem.hpp"
#include <raylib.h>
#include <algorithm>   // std::remove_if
#include <vector>
#include <iostream>    // std::cerr

// Draw the score (top-left) and the strike counter (top-right).
// NOTE: the number 4 (max strikes) is also hard-coded in main() below; a
// shared `constexpr int maxStrikes = 4;` would keep them in sync.
void DrawHUD(int score, int strikes, int screenWidth) {
    // TextFormat works like printf: %04d pads the score with zeros to 4 digits.
    // It returns a pointer to an internal raylib buffer, valid only until the
    // next TextFormat call.
    DrawText(TextFormat("SCORE: %04d", score), 30, 25, 32, WHITE);

    // draw strikes (X marks) at the top-right corner:
    // red X for each strike used, gray X for each one remaining.
    for (int i = 0; i < 4; ++i) {
        int xPos = screenWidth - 160 + (i * 40);
        int yPos = 30;
        if (i < strikes) {
            DrawText("X", xPos, yPos, 36, RED);
        } else {
            DrawText("X", xPos, yPos, 36, DARKGRAY);
        }
    }
}

// Draw a dark overlay with "GAME OVER", the final score and restart hint.
void DrawGameOverScreen(int score, int screenWidth, int screenHeight) {
    // Semi-transparent black rectangle over the whole screen dims the game
    // underneath it.
    DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, 0.75f));

    // To center text horizontally: measure its width in pixels, then start
    // drawing at (screenWidth - textWidth) / 2.
    const char* overText = "GAME OVER";
    int overW = MeasureText(overText, 64);
    DrawText(overText, (screenWidth - overW) / 2, screenHeight / 2 - 80, 64, RED);

    const char* finalScoreText = TextFormat("FINAL SCORE: %d", score);
    int scoreW = MeasureText(finalScoreText, 32);
    DrawText(finalScoreText, (screenWidth - scoreW) / 2, screenHeight / 2 + 10, 32, WHITE);

    const char* restartText = "Swipe middle or Press 'R' to Restart";
    int restartW = MeasureText(restartText, 22);
    DrawText(restartText, (screenWidth - restartW) / 2, screenHeight / 2 + 70, 22, LIGHTGRAY);
}

int main() {
    // `constexpr` - known at compile time. This lets them be captured by
    // lambdas and used anywhere a constant expression is required.
    constexpr int screenWidth = 1280;
    constexpr int screenHeight = 720;

    // Start listening for the Python vision server BEFORE opening the window,
    // so we can fail fast with a clear message (e.g. the port is in use).
    NetworkReceiver receiver(5005);
    if (!receiver.initialize()) {
        std::cerr << "Failed to start Network Receiver!\n";
        return -1;
    }

    // Create the window and OpenGL context. Most raylib functions (drawing,
    // input, timing) only work after InitWindow.
    InitWindow(screenWidth, screenHeight, "CV Fruit Ninja");
    // Cap the frame rate; raylib sleeps at the end of each frame as needed.
    SetTargetFPS(60);

    // ---- Game objects ----
    Blade blade;                                       // the finger trail
    ParticleSystem particles;                          // splash/explosion effects
    FruitSpawner spawner(screenWidth, screenHeight);   // decides when fruits appear
    std::vector<Fruit> fruits;                         // every fruit/bomb on screen

    // ---- Game state ----
    GameState state = GameState::Playing;
    int score = 0;
    int strikes = 0;   // misses + bombs hit; 4 strikes = game over

    // The smoothed finger position in screen pixels (starts at the center).
    Vector2 fingerScreenPos = { screenWidth / 2.0f, screenHeight / 2.0f };
    // Becomes true once the first point arrives, so we don't slice with a
    // "blade" before a hand was ever seen.
    // NOTE: it is never set back to false when the hand leaves the camera.
    bool fingerDetected = false;

    // WindowShouldClose() returns true when the user presses ESC or clicks
    // the window's close button.
    while (!WindowShouldClose()) {
        // Time in seconds since the previous frame (~0.0167 at 60 FPS).
        // Every movement is multiplied by dt so the game runs at the same
        // speed regardless of the actual frame rate.
        float dt = GetFrameTime();

        // =====================================================================
        // 1. INPUT
        // =====================================================================

        // get latest finger position from network
        // `auto` deduces the type: std::optional<Point2D>.
        auto pointOpt = receiver.receiveLatestPoint();
        if (pointOpt.has_value()) {
            fingerDetected = true;

            // Normalized [0..1] camera coordinates -> screen pixels.
            // `pointOpt->x` accesses the Point2D stored inside the optional.
            float targetX = pointOpt->x * screenWidth;
            float targetY = pointOpt->y * screenHeight;

            // Exponential smoothing (a simple low-pass filter): move only half
            // way toward the new target each time. This removes the jitter of
            // the hand tracker at the cost of a little lag.
            // 0.0 = never move, 1.0 = no smoothing at all.
            constexpr float smoothing = 0.5f;
            fingerScreenPos.x += (targetX - fingerScreenPos.x) * smoothing;
            fingerScreenPos.y += (targetY - fingerScreenPos.y) * smoothing;

            blade.addPoint(fingerScreenPos);
        }

        // These run in both game states, so the blade and particles keep
        // animating on the game-over screen.
        blade.update(dt);
        particles.update(dt);

        // The blade's movement this frame, as a line segment prevTip -> tip.
        Vector2 tip = blade.getTip();
        Vector2 prevTip = blade.getPreviousTip();

        // =====================================================================
        // 2. UPDATE (game logic)
        // =====================================================================
        if (state == GameState::Playing) {
            spawner.update(dt, fruits);

            // Iterate with an explicit iterator (not a range-for loop) because
            // we may ERASE elements while looping. vector::erase() invalidates
            // the iterator we pass in, but returns a valid iterator to the
            // element after the erased one - so we assign it back to `fruit`
            // and skip the `++fruit` at the bottom.
            for (auto fruit = fruits.begin(); fruit != fruits.end();) {
                fruit->update(dt);

                // A fruit fell off the bottom without being sliced -> strike.
                if (fruit->isMissed(screenHeight)) {
                    strikes++;
                    if (strikes >= 4) {
                        state = GameState::GameOver;
                    }
                    fruit = fruits.erase(fruit);
                    if (state == GameState::GameOver) {
                        break;   // stop processing the rest of the fruits
                    }
                    continue;    // `fruit` already points to the next element
                }

                // check for slicing if the finger is detected and moving fast
                if (fingerDetected && blade.isMovingFast()) {
                    if (fruit->checkSlice(prevTip, tip)) {
                        if (fruit->isBomb()) {
                            // Hitting a bomb also costs a strike.
                            particles.spawnBombExplosion(fruit->getPosition());
                            strikes++;
                            if (strikes >= 4) {
                                state = GameState::GameOver;
                            }
                            // NOTE: unlike the "missed" branch above, there is
                            // no `break` here, so other fruits can still be
                            // sliced and scored in this same frame after the
                            // game is already over.
                        } else {
                            particles.spawnJuiceSplash(fruit->getPosition(), fruit->getSplashColor());
                            score += 10;
                        }
                    }
                }
                ++fruit;
            }

            // Remove fruits (and sliced halves) that have left the screen,
            // using the erase-remove idiom (see ParticleSystem::update).
            // The lambda captures screenHeight by value (`[screenHeight]`) so
            // it can use it inside.
            fruits.erase(std::remove_if(fruits.begin(), fruits.end(),
                [screenHeight](const Fruit& f) { return f.isOffScreen(screenHeight); }),
                fruits.end());

        } else if (state == GameState::GameOver) {
            // restart game if 'R' is pressed or if the player swipes in the middle of the screen
            // IsKeyPressed is true only on the frame the key goes down
            // (unlike IsKeyDown, which stays true while it is held).
            bool restartKey = IsKeyPressed(KEY_R);
            // The "restart zone" is a 70px circle just below the screen center,
            // around the restart hint text.
            bool restartSlice = fingerDetected && blade.isMovingFast() &&
                                CheckCollisionPointCircle(tip, { screenWidth / 2.0f, screenHeight / 2.0f + 50.0f }, 70.0f);

            if (restartKey || restartSlice) {
                // Reset everything to a fresh game.
                score = 0;
                strikes = 0;
                fruits.clear();
                particles.clear();
                spawner.reset();
                state = GameState::Playing;
            }
        }

        // =====================================================================
        // 3. RENDER
        // =====================================================================
        // Everything between BeginDrawing() and EndDrawing() is drawn into a
        // hidden back buffer; EndDrawing() shows it on screen all at once
        // ("double buffering"), which prevents flickering.
        // Things drawn later appear ON TOP of things drawn earlier.
        BeginDrawing();
        ClearBackground(Color{ 18, 20, 26, 255 });   // dark blue-gray background

        // Fruits and particles
        // `const auto&` - read-only reference, avoids copying each Fruit.
        for (const auto& fruit : fruits) {
            fruit.draw();
        }
        particles.draw();

        // Blade trail (drawn over the fruits so it is always visible)
        blade.draw();

        // UI is drawn last, on top of everything.
        if (state == GameState::Playing) {
            DrawHUD(score, strikes, screenWidth);
        } else {
            DrawGameOverScreen(score, screenWidth, screenHeight);
        }

        // Uncomment to show the current frame rate (useful for debugging).
        // DrawFPS(20, screenHeight - 35);
        EndDrawing();
    }

    // Destroy the window and OpenGL context.
    // `receiver` is destroyed automatically after this, when main() returns
    // (RAII: its destructor closes the socket).
    CloseWindow();
    return 0;
}

// =============================================================================
// Blade.hpp
//
// The "sword" the player controls with their index finger.
//
// The blade is a short trail of recent finger positions. Each point fades out
// over time, which produces the glowing streak you see on screen. The two most
// recent points form a line segment that is used to test whether a fruit has
// been cut (see Fruit::checkSlice).
// =============================================================================
#pragma once

#include <raylib.h>   // Vector2, Color and the drawing functions
#include <deque>      // std::deque: fast insert/remove at BOTH ends

// One sample of the finger position plus how visible it still is.
struct BladePoint {
    Vector2 position;  // screen position in pixels
    float alpha;       // opacity: 1.0 = fully visible, 0.0 = invisible (removed)
};

class Blade {
public:
    // maxPoints: the maximum number of points kept in the trail (trail length).
    // fadeSpeed: how much alpha each point loses per second. At 3.5 a point
    //            disappears after roughly 1 / 3.5 ~= 0.29 seconds.
    //
    // NOTE: because both parameters have default values, this constructor can
    // be called with a single argument, which makes it an *implicit
    // conversion* from size_t to Blade (e.g. `Blade b = 5;` compiles).
    // Marking it `explicit` would prevent that.
    Blade(size_t maxPoints = 20, float fadeSpeed = 3.5f);

    // Add a new finger position to the front of the trail.
    // Called only when a new point arrives from the network.
    void addPoint(Vector2 pos);

    // Fade all points and drop the ones that are fully transparent.
    // Called once per frame; dt is the frame time in seconds.
    void update(float dt);

    // Render the trail. `const` at the end means this method promises not to
    // modify the Blade, so it can be called on a `const Blade&`.
    void draw() const;

    // True if the finger moved far enough between the last two samples to
    // count as a "swipe". Slow movement should not cut fruit.
    bool isMovingFast() const;

    // The newest point of the trail (the tip of the blade).
    // Returns (0, 0) if the trail is empty.
    Vector2 getTip() const;

    // The second-newest point. Together with getTip() it forms the segment
    // used for collision. If there is only one point, returns getTip().
    Vector2 getPreviousTip() const;

private:
    // The trail, newest point at the FRONT, oldest at the BACK.
    // std::deque is used because we push to the front and pop from the back
    // every frame; both are O(1) for a deque (a std::vector would need to
    // shift every element when inserting at the front).
    std::deque<BladePoint> m_points;

    size_t m_maxPoints;  // maximum trail length
    float m_fadeSpeed;   // alpha lost per second

    // Distance (in pixels) between the last two samples received.
    // NOTE: this is "pixels per network packet", not "pixels per second",
    // so it depends on the camera's frame rate. It is also only updated when
    // a new point arrives, so it keeps its last value if the hand disappears.
    float m_currentSpeed{0.0f};
};

// =============================================================================
// Blade.cpp - implementation of the finger-controlled blade trail.
// See Blade.hpp for an overview.
// =============================================================================
#include "Blade.hpp"

#include <raymath.h>   // Vector2Distance and other vector math helpers

// The member initializer list (`: m_maxPoints(...), ...`) initializes members
// directly, before the constructor body runs. This is preferred over assigning
// inside the body, and it is the only way to initialize const or reference
// members.
Blade::Blade(size_t maxPoints, float fadeSpeed)
    : m_maxPoints(maxPoints), m_fadeSpeed(fadeSpeed) {}

void Blade::addPoint(Vector2 pos) {
    // Measure how far the finger moved since the previous sample.
    // This distance is our "speed" (see the note in Blade.hpp about units).
    if (!m_points.empty()) {
        float dist = Vector2Distance(m_points.front().position, pos);
        m_currentSpeed = dist;
    }

    // Newest point goes to the front, fully visible.
    // `{pos, 1.0f}` creates a BladePoint with brace (aggregate) initialization.
    m_points.push_front({pos, 1.0f});

    // Keep the trail at most m_maxPoints long by dropping the oldest point.
    if (m_points.size() > m_maxPoints) {
        m_points.pop_back();
    }
}

void Blade::update(float dt) {
    // Fade every point. Multiplying by dt makes the fade speed independent of
    // the frame rate: the trail fades just as fast at 30 FPS as at 144 FPS.
    //
    // `auto&` - take each element by reference so we modify the point stored
    // in the deque, not a temporary copy.
    for (auto& pt : m_points) {
        pt.alpha -= m_fadeSpeed * dt;
        if (pt.alpha < 0.0f) pt.alpha = 0.0f;
    }

    // Remove fully faded points. The oldest points are at the back and they
    // fade first, so we only need to look at the back.
    while (!m_points.empty() && m_points.back().alpha <= 0.0f) {
        m_points.pop_back();
    }
}

void Blade::draw() const {
    // A line needs at least two points.
    if (m_points.size() < 2) return;

    // Draw a line segment between every pair of neighboring points.
    // Note `m_points.size() - 1`: size() is unsigned, so this is only safe
    // because we returned early above when size() < 2 (otherwise 0 - 1 would
    // wrap around to a huge number).
    for (size_t i = 0; i < m_points.size() - 1; ++i) {
        const auto& p1 = m_points[i];
        const auto& p2 = m_points[i + 1];

        // t goes from 0 at the tip to ~1 at the tail.
        // The trail is thickest at the tip (10px) and thin at the tail (~1px).
        float t = static_cast<float>(i) / static_cast<float>(m_points.size());
        float thickness = (1.0f - t) * 9.0f + 1.0f;

        // Outer blue glow: wider, semi-transparent line drawn first (underneath).
        DrawLineEx(p1.position, p2.position, thickness * 1.8f, ColorAlpha(SKYBLUE, p1.alpha * 0.4f));
        // Sharp white core drawn on top of the glow.
        DrawLineEx(p1.position, p2.position, thickness, ColorAlpha(WHITE, p1.alpha * 0.9f));
    }

    // Spark at the tip of the blade: a small white dot with a blue halo.
    if (!m_points.empty()) {
        DrawCircleV(m_points.front().position, 6.0f, WHITE);
        DrawCircleV(m_points.front().position, 14.0f, ColorAlpha(SKYBLUE, 0.4f));
    }
}

bool Blade::isMovingFast() const {
    // The finger must move more than 15 pixels between two samples to count as
    // a slicing swipe. This stops a hand that is just hovering over a fruit
    // from cutting it.
    return m_currentSpeed > 15.0f;
}

Vector2 Blade::getTip() const {
    // Ternary operator: condition ? value-if-true : value-if-false.
    return m_points.empty() ? Vector2{0, 0} : m_points.front().position;
}

Vector2 Blade::getPreviousTip() const {
    // m_points[1] is the second-newest point. If it does not exist, return the
    // tip itself; the segment tip->tip has zero length, so it cannot slice
    // anything (Fruit::checkSlice rejects zero-length segments).
    return m_points.size() > 1 ? m_points[1].position : getTip();
}

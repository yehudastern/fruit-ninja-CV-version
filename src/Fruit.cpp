// =============================================================================
// Fruit.cpp - physics, slicing and drawing of a single fruit or bomb.
// See Fruit.hpp for an overview of a fruit's lifecycle.
// =============================================================================
#include "Fruit.hpp"
#include <raymath.h>   // Vector2Subtract, Vector2DotProduct, RAD2DEG

// Members are initialized in the order they are DECLARED in the class, not in
// the order written here. Keeping both orders the same avoids surprises (and
// the compiler warning -Wreorder).
Fruit::Fruit(Vector2 spawnPos, Vector2 velocity, FruitType type, float radius)
    : m_pos(spawnPos), m_velocity(velocity), m_radius(radius), m_type(type) {}

void Fruit::update(float dt) {
    // Downward acceleration in pixels per second^2. Tuned by feel, not real
    // physics. `constexpr` = a compile-time constant.
    constexpr float gravity = 820.0f;

    // Simple "semi-implicit Euler" integration, run once per frame:
    //   velocity += acceleration * dt
    //   position += velocity * dt
    // Multiplying by dt (seconds since the last frame) keeps the motion
    // independent of the frame rate.
    if (!m_sliced) {
        m_velocity.y += gravity * dt;
        m_pos.x += m_velocity.x * dt;
        m_pos.y += m_velocity.y * dt;
        m_rotation += m_rotSpeed * dt;
    } else {
        // After slicing, simulate the two halves independently.
        // (m_pos/m_velocity of the whole fruit are frozen from now on.)
        m_half1Vel.y += gravity * dt;
        m_half2Vel.y += gravity * dt;
        m_half1Pos.x += m_half1Vel.x * dt;
        m_half1Pos.y += m_half1Vel.y * dt;
        m_half2Pos.x += m_half2Vel.x * dt;
        m_half2Pos.y += m_half2Vel.y * dt;
        m_rotation += m_rotSpeed * 1.5f * dt;  // halves spin faster
    }
}

// -----------------------------------------------------------------------------
// Segment-vs-circle intersection.
//
// The blade's movement this frame is the segment from p1 to p2. Any point on
// it can be written as
//         P(t) = p1 + t * d,      where d = p2 - p1 and 0 <= t <= 1.
//
// We want to know if some P(t) lies on the fruit's circle (center C, radius r):
//         |P(t) - C|^2 = r^2
//
// Let f = p1 - C. Expanding |f + t*d|^2 = r^2 gives a quadratic in t:
//         (d.d) t^2 + 2 (f.d) t + (f.f - r^2) = 0
//          \_a_/      \__b__/     \___c___/
//
// If the discriminant b^2 - 4ac is negative, the infinite LINE misses the
// circle. Otherwise the line enters the circle at t1 and leaves at t2. The
// SEGMENT touches the circle's edge if t1 or t2 lies in [0, 1].
//
// NOTE: if the whole segment is inside the circle (t1 < 0 and t2 > 1), this
// test reports no hit. In practice the blade moves fast enough that this
// rarely matters.
// -----------------------------------------------------------------------------
bool Fruit::checkSlice(Vector2 p1, Vector2 p2) {
    // A fruit can only be sliced once.
    if (m_sliced) return false;

    Vector2 d = Vector2Subtract(p2, p1);      // segment direction
    Vector2 f = Vector2Subtract(p1, m_pos);   // from circle center to segment start

    float a = Vector2DotProduct(d, d);
    // a == 0 means p1 == p2: a zero-length segment (the blade did not move).
    // We must stop here anyway to avoid dividing by zero below.
    if (a == 0.0f) return false;

    float b = 2.0f * Vector2DotProduct(f, d);
    float c = Vector2DotProduct(f, f) - (m_radius * m_radius);
    float discriminant = b * b - 4.0f * a * c;

    if (discriminant >= 0.0f) {
        // Quadratic formula: t = (-b -/+ sqrt(disc)) / 2a
        discriminant = sqrtf(discriminant);
        float t1 = (-b - discriminant) / (2.0f * a);  // entry point
        float t2 = (-b + discriminant) / (2.0f * a);  // exit point

        if ((t1 >= 0.0f && t1 <= 1.0f) || (t2 >= 0.0f && t2 <= 1.0f)) {
            // HIT! Switch to the "sliced" state.
            m_sliced = true;

            // The angle of the blade's movement, converted from radians
            // (what atan2f returns) to degrees (what raylib's drawing uses).
            // The halves are drawn split along this direction.
            m_sliceAngle = atan2f(d.y, d.x) * RAD2DEG;

            // Both halves start where the whole fruit was...
            m_half1Pos = m_pos;
            m_half2Pos = m_pos;

            // ...and inherit its velocity, plus a sideways push in opposite
            // directions and a small upward "pop" so they visibly fly apart.
            m_half1Vel = { m_velocity.x - 130.0f, m_velocity.y - 70.0f };
            m_half2Vel = { m_velocity.x + 130.0f, m_velocity.y - 40.0f };
            return true;
        }
    }
    return false;
}

Color Fruit::getSplashColor() const {
    // A `switch` over an enum class. Because every enumerator has a case and
    // there is no `default:`, the compiler can warn us if a new FruitType is
    // added and forgotten here.
    switch (m_type) {
        case FruitType::Watermelon: return Color{ 255, 45, 85, 255 };
        case FruitType::Orange:     return Color{ 255, 140, 0, 255 };
        case FruitType::Apple:      return Color{ 230, 30, 50, 255 };
        case FruitType::Bomb:       return Color{ 255, 80, 0, 255 };
    }
    // Unreachable for valid enum values, but required so every code path
    // returns something (an enum variable can technically hold other values).
    return WHITE;
}

void Fruit::draw() const {
    if (!m_sliced) {
        drawWhole();
    } else if (m_type != FruitType::Bomb) {
        // A sliced bomb is not drawn at all - it "exploded" (main.cpp spawns
        // the explosion particles). Sliced fruits are drawn as two halves.
        drawHalf(m_half1Pos, m_sliceAngle, true);
        drawHalf(m_half2Pos, m_sliceAngle, false);
    }
}

void Fruit::drawWhole() const {
    // Every fruit is drawn as a few concentric circles, from the outside
    // (skin) to the inside (flesh). Drawing order matters: later circles are
    // painted on top of earlier ones.
    //
    // Color{ r, g, b, a } - each channel is 0..255; a = 255 means opaque.

    if (m_type == FruitType::Bomb) {
        // Bomb: dark body, slightly lighter inner circle, a small white
        // highlight to make it look round, and a fuse on top.
        DrawCircleV(m_pos, m_radius, Color{ 35, 35, 40, 255 });
        DrawCircleV(m_pos, m_radius * 0.85f, Color{ 50, 50, 55, 255 });
        DrawCircle(static_cast<int>(m_pos.x - 10), static_cast<int>(m_pos.y - 10), 6, ColorAlpha(WHITE, 0.4f));

        // The fuse goes from the center to the top edge of the bomb.
        Vector2 fusePos = { m_pos.x, m_pos.y - m_radius };
        DrawLineEx(m_pos, fusePos, 4.0f, BROWN);
        // The spark randomly switches between yellow and orange every frame,
        // which makes it flicker.
        DrawCircleV(fusePos, 5.0f, (GetRandomValue(0, 1) == 0 ? YELLOW : ORANGE));
        return;
    }

    switch (m_type) {
        case FruitType::Watermelon:
            DrawCircleV(m_pos, m_radius, Color{ 34, 139, 34, 255 });            // dark green rind
            DrawCircleV(m_pos, m_radius * 0.90f, Color{ 60, 179, 113, 255 });   // light green rind
            DrawCircleV(m_pos, m_radius * 0.82f, Color{ 220, 245, 200, 255 });  // pale inner rind
            DrawCircleV(m_pos, m_radius * 0.74f, Color{ 255, 45, 85, 255 });    // red flesh
            break;

        case FruitType::Orange:
            DrawCircleV(m_pos, m_radius, Color{ 255, 140, 0, 255 });            // peel
            DrawCircleV(m_pos, m_radius * 0.88f, Color{ 255, 230, 180, 255 });  // white pith
            DrawCircleV(m_pos, m_radius * 0.80f, Color{ 255, 165, 0, 255 });   // flesh
            break;

        case FruitType::Apple:
            DrawCircleV(m_pos, m_radius, Color{ 220, 20, 60, 255 });            // red skin
            DrawCircleV(m_pos, m_radius * 0.88f, Color{ 255, 245, 215, 255 });  // cream flesh
            break;

        default:
            // Bomb was handled above.
            break;
    }
}

void Fruit::drawHalf(Vector2 pos, float rot, bool isLeft) const {
    // Each half is a 180-degree sector (half disc). The two halves together
    // cover the full 360 degrees: one from rot to rot+180, the other from
    // rot+180 to rot+360. Because the cut angle follows the blade direction,
    // the fruit appears to split along the line of the swipe.
    float startAngle = isLeft ? rot : rot + 180.0f;
    float endAngle   = startAngle + 180.0f;

    // Pick the skin and flesh colors with chained ternary operators.
    // (These colors repeat values from drawWhole() - a good candidate for a
    // single lookup table per FruitType.)
    Color skin = (m_type == FruitType::Watermelon) ? Color{ 34, 139, 34, 255 } :
                 (m_type == FruitType::Orange)     ? Color{ 255, 140, 0, 255 } :
                                                     Color{ 220, 20, 60, 255 };

    Color inner = (m_type == FruitType::Watermelon) ? Color{ 255, 45, 85, 255 } :
                  (m_type == FruitType::Orange)     ? Color{ 255, 165, 0, 255 } :
                                                      Color{ 255, 245, 215, 255 };

    // 32 = number of triangle segments used to approximate the curved edge.
    DrawCircleSector(pos, m_radius, startAngle, endAngle, 32, skin);
    DrawCircleSector(pos, m_radius * 0.80f, startAngle, endAngle, 32, inner);
}

bool Fruit::isOffScreen(int screenHeight) const {
    // Screen y grows downwards, so "below the screen" means y > screenHeight.
    // The +90 margin (about 2x the radius) makes sure the object is completely
    // out of view before we delete it, so it doesn't vanish visibly.
    // A sliced fruit is only gone when BOTH halves are below the screen.
    return (!m_sliced && m_pos.y > screenHeight + 90) ||
           (m_sliced && m_half1Pos.y > screenHeight + 90 && m_half2Pos.y > screenHeight + 90);
}

bool Fruit::isMissed(int screenHeight) const {
    // A miss is: not sliced, not a bomb (missing bombs is good!), below the
    // screen, AND moving downwards.
    //
    // The "moving downwards" check matters: fruits spawn at screenHeight + 40
    // moving UP, so without it a fruit could count as missed right after it
    // spawned. Using +50 (less than isOffScreen's +90) means main.cpp sees the
    // miss before the fruit is silently removed as off-screen.
    return !m_sliced && (m_type != FruitType::Bomb) && (m_pos.y > screenHeight + 50) && (m_velocity.y > 0.0f);
}

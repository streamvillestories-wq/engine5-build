#pragma once

#include "e5/gameplay/character_motor.hpp"

namespace e5::gameplay {

// Geometry of a bowstring: two straight segments from the limb anchors to the
// nock point (where the arrow sits and the fingers pull). Engine-agnostic so
// the same numbers can later drive arrow placement and hit timing.

struct BowStringShape {
    Vec3 top;    // anchor on the upper limb
    Vec3 nock;   // where the string is pulled
    Vec3 bottom; // anchor on the lower limb
};

// `draw` runs from 0 (at rest: the string is straight) to 1 (full draw: the
// nock point is `max_draw_distance` away from the rest line). Values outside
// 0..1 are clamped. `pull_direction` need not be normalised; a zero vector
// leaves the string at rest.
[[nodiscard]] BowStringShape bow_string_shape(const Vec3& top, const Vec3& bottom, const Vec3& pull_direction,
                                              float max_draw_distance, float draw) noexcept;

} // namespace e5::gameplay

#pragma once

#include "e5/gameplay/character_motor.hpp"

namespace e5::gameplay {

// A point mass under gravity. Engine-agnostic and deterministic, so a server
// can later replay the same flight to validate a hit.
struct Projectile {
    Vec3 position;
    Vec3 velocity;
};

// Advances one step (semi-implicit Euler: velocity first, then position).
// `gravity` is the downward acceleration in m/s^2 (positive number).
[[nodiscard]] Projectile step_projectile(const Projectile& projectile, float gravity, float delta_seconds) noexcept;

// Score of a hit on a round target with `rings` equal-width rings: the centre
// ring scores `rings`, the outermost 1, anything outside the radius 0.
[[nodiscard]] int target_ring_score(float distance_from_centre, float radius, int rings) noexcept;

} // namespace e5::gameplay

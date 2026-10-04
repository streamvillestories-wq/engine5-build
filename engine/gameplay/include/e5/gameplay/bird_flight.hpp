#pragma once

#include "e5/gameplay/character_motor.hpp"

namespace e5::gameplay {

// Flight of a summoned attack bird. Plain steering, no random numbers: the
// same inputs give the same flight on every machine.
//
//   Rise     just released: keeps the direction it was thrown in
//   Attack   steers at its target; touching it is a hit
//   Retreat  peels away after a hit to gain room for the next dive
//   Leave    its time is up: climbs away, then it is finished
//
// Without a target an attacking bird circles above `home` instead.
enum class BirdPhase : unsigned char { Rise, Attack, Retreat, Leave };

struct BirdState {
    Vec3 position;
    Vec3 velocity;
    BirdPhase phase = BirdPhase::Rise;
    float phase_seconds = 0.0F; // time spent in the current phase
    float age_seconds = 0.0F;
    float side = 1.0F; // +1 or -1: which way it peels off and circles
};

struct BirdParams {
    float speed = 9.0F;              // m/s
    float turn_acceleration = 60.0F; // m/s^2 available for changing direction
    float hit_distance = 0.35F;      // metres from the target that count as a hit
    float rise_seconds = 0.45F;
    float retreat_seconds = 0.4F;
    float lifetime_seconds = 10.0F; // from release until it leaves
    float leave_seconds = 1.2F;
    float circle_radius = 2.4F; // metres, when there is nothing to attack
    float circle_height = 2.6F; // metres above `home`
};

struct BirdStep {
    BirdState state;
    bool hit = false;      // touched the target on this step
    bool finished = false; // has left; remove it
};

// `target` is ignored unless `has_target` is true.
[[nodiscard]] BirdStep step_bird(const BirdState& state, bool has_target, const Vec3& target, const Vec3& home,
                                 const BirdParams& params, float delta_seconds) noexcept;

} // namespace e5::gameplay
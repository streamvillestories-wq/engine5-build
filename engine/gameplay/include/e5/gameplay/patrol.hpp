#pragma once

#include "e5/gameplay/character_motor.hpp"

#include <cstdint>

namespace e5::gameplay {

// Roaming about a place: a creature that keeps watch over something (the stone
// golem and the great tree) wanders freely in a ring around it. It picks a spot
// in the ring, walks there, stands a while, picks the next. It never enters the
// inner circle (where the thing it guards stands) and never leaves the outer one.

struct RoamParams {
    float center_x = 0.0F;
    float center_z = 0.0F;
    float inner_radius = 10.0F; // metres; it stays outside this
    float outer_radius = 20.0F; // metres; it stays inside this
    float speed = 2.0F;         // m/s
    float rest_min_seconds = 3.0F;
    float rest_max_seconds = 9.0F;
    float arrive_distance = 1.0F; // metres from its spot at which it has arrived
    // If it has not arrived after this long (something is in its way), it picks another spot.
    float give_up_seconds = 45.0F;
};

struct RoamState {
    float target_x = 0.0F;
    float target_z = 0.0F;
    bool has_target = false;
    bool resting = false;
    float seconds = 0.0F;               // time walking to the current spot, or resting
    float rest_seconds = 0.0F;          // how long this rest lasts
    std::uint32_t random = 0x9E3779B9U; // state of its random numbers; seed it per creature
};

struct RoamStep {
    RoamState state;
    // Velocity on the ground plane (y is always 0); zero while resting.
    Vec3 velocity;
};

// `x`, `z`: where it stands now.
[[nodiscard]] RoamStep step_roam(const RoamState& state, float x, float z, const RoamParams& params,
                                 float delta_seconds) noexcept;

} // namespace e5::gameplay

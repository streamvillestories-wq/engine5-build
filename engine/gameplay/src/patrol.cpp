#include "e5/gameplay/patrol.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace e5::gameplay {
namespace {

constexpr float pi = std::numbers::pi_v<float>;
// The next spot lies this far round the ring from where it stands, either way: far enough
// to be a walk, near enough that the straight way there does not cut deep through the middle.
constexpr float least_turn = 0.5F; // radians
constexpr float most_turn = 1.7F;
// Within this many metres of a bound it is steered away from it.
constexpr float bound_margin = 1.5F;

[[nodiscard]] float next_random(std::uint32_t& state) noexcept {
    state ^= state << 13U;
    state ^= state >> 17U;
    state ^= state << 5U;
    return static_cast<float>(state >> 8U) / 16777216.0F; // [0, 1)
}

void pick_target(RoamState& state, float x, float z, const RoamParams& params) noexcept {
    const float here = std::atan2(z - params.center_z, x - params.center_x);
    const float turn = std::lerp(least_turn, most_turn, next_random(state.random));
    const float angle = here + (next_random(state.random) < 0.5F ? -turn : turn);
    const float radius = std::lerp(params.inner_radius, params.outer_radius, next_random(state.random));
    state.target_x = params.center_x + std::cos(angle) * radius;
    state.target_z = params.center_z + std::sin(angle) * radius;
    state.has_target = true;
    state.seconds = 0.0F;
}

} // namespace

RoamStep step_roam(const RoamState& state, float x, float z, const RoamParams& params, float delta_seconds) noexcept {
    RoamStep step{.state = state};
    RoamState& next = step.state;
    if (next.random == 0) {
        next.random = 0x9E3779B9U; // the generator stays at zero for ever
    }
    next.seconds += delta_seconds;

    if (next.resting) {
        if (next.seconds < next.rest_seconds) {
            return step;
        }
        next.resting = false;
        next.has_target = false;
    }
    if (!next.has_target || next.seconds > params.give_up_seconds) {
        pick_target(next, x, z, params);
    }

    const float to_x = next.target_x - x;
    const float to_z = next.target_z - z;
    const float left = std::sqrt(to_x * to_x + to_z * to_z);
    if (left <= params.arrive_distance) {
        next.resting = true;
        next.seconds = 0.0F;
        next.rest_seconds = std::lerp(params.rest_min_seconds, params.rest_max_seconds, next_random(next.random));
        return step;
    }
    float vx = to_x / left * params.speed;
    float vz = to_z / left * params.speed;

    // The bounds: near the inner one it is pushed outward, near the outer one inward, so the
    // straight way to its spot bends round the middle instead of crossing it.
    float out_x = x - params.center_x;
    float out_z = z - params.center_z;
    const float distance = std::sqrt(out_x * out_x + out_z * out_z);
    if (distance > 0.001F) {
        out_x /= distance;
        out_z /= distance;
        const float inner_push = std::clamp((params.inner_radius + bound_margin - distance) / bound_margin, 0.0F, 2.0F);
        const float outer_push = std::clamp((distance - params.outer_radius + bound_margin) / bound_margin, 0.0F, 2.0F);
        const float push = (inner_push - outer_push) * params.speed;
        vx += out_x * push;
        vz += out_z * push;
        // Hard against the inner bound it does not move inward at all.
        const float inward = -(vx * out_x + vz * out_z);
        if (distance <= params.inner_radius && inward > 0.0F) {
            vx += out_x * inward;
            vz += out_z * inward;
        }
    }
    // Never faster than its pace.
    const float length = std::sqrt(vx * vx + vz * vz);
    if (length > params.speed && length > 0.0F) {
        vx *= params.speed / length;
        vz *= params.speed / length;
    }
    step.velocity = {.x = vx, .y = 0.0F, .z = vz};
    return step;
}

} // namespace e5::gameplay

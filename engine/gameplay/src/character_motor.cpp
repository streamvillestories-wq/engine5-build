#include "e5/gameplay/character_motor.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace e5::gameplay {
namespace {

float move_toward(float current, float target, float max_delta) noexcept {
    const float diff = target - current;
    if (std::abs(diff) <= max_delta) {
        return target;
    }
    return current + std::copysign(max_delta, diff);
}

} // namespace

Vec3 step_velocity(const MotorState& state, const MotorInput& input, float yaw_radians, const MotorParams& params,
                   float delta_seconds) noexcept {
    // Normalise so diagonal movement is not faster than straight movement.
    float local_x = input.move_right;
    float local_z = -input.move_forward;
    const float length = std::hypot(local_x, local_z);
    if (length > 1.0F) {
        local_x /= length;
        local_z /= length;
    }

    // Rotate the local wish direction into world space about +Y.
    const float sin_yaw = std::sin(yaw_radians);
    const float cos_yaw = std::cos(yaw_radians);
    const float speed = input.sprint ? params.sprint_speed : params.walk_speed;
    const float target_x = (local_x * cos_yaw + local_z * sin_yaw) * speed;
    const float target_z = (-local_x * sin_yaw + local_z * cos_yaw) * speed;

    const float acceleration = state.on_floor ? params.ground_acceleration : params.air_acceleration;
    const float max_delta = acceleration * delta_seconds;

    Vec3 velocity = state.velocity;
    velocity.x = move_toward(velocity.x, target_x, max_delta);
    velocity.z = move_toward(velocity.z, target_z, max_delta);

    if (state.on_floor) {
        velocity.y = input.jump ? params.jump_velocity : 0.0F;
    } else {
        velocity.y = std::max(velocity.y - params.gravity * delta_seconds, -params.terminal_fall_speed);
    }

    return velocity;
}

LookAngles apply_look(LookAngles angles, float delta_x, float delta_y, float radians_per_pixel) noexcept {
    constexpr float pitch_limit = std::numbers::pi_v<float> * 0.5F - 0.02F;
    constexpr float two_pi = std::numbers::pi_v<float> * 2.0F;

    angles.yaw = std::remainder(angles.yaw - delta_x * radians_per_pixel, two_pi);
    angles.pitch = std::clamp(angles.pitch - delta_y * radians_per_pixel, -pitch_limit, pitch_limit);
    return angles;
}

float facing_yaw(float direction_x, float direction_z) noexcept {
    return std::atan2(direction_x, direction_z);
}

float turn_toward(float current, float target, float max_step) noexcept {
    constexpr float two_pi = std::numbers::pi_v<float> * 2.0F;
    // std::remainder yields the signed shortest difference in [-pi, pi].
    const float difference = std::remainder(target - current, two_pi);
    const float step = std::clamp(difference, -max_step, max_step);
    return std::remainder(current + step, two_pi);
}

LocomotionState select_locomotion_state(float horizontal_speed, bool on_floor,
                                        const LocomotionThresholds& thresholds) noexcept {
    if (!on_floor) {
        return LocomotionState::Airborne;
    }
    if (horizontal_speed < thresholds.idle_below) {
        return LocomotionState::Idle;
    }
    return horizontal_speed < thresholds.run_above ? LocomotionState::Walk : LocomotionState::Run;
}

} // namespace e5::gameplay

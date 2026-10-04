#pragma once

namespace e5::gameplay {

// Coordinate convention matches Godot: right-handed, +Y up, -Z forward, metres.
struct Vec3 {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

struct MotorParams {
    float walk_speed = 4.5F;           // m/s
    float sprint_speed = 8.0F;         // m/s
    float ground_acceleration = 50.0F; // m/s^2
    float air_acceleration = 8.0F;     // m/s^2
    float jump_velocity = 4.8F;        // m/s
    float gravity = 12.0F;             // m/s^2, slightly above 9.81 for a less floaty feel
    float terminal_fall_speed = 55.0F; // m/s
};

struct MotorInput {
    float move_right = 0.0F;   // -1..1, local X
    float move_forward = 0.0F; // -1..1, positive walks along local -Z
    bool sprint = false;
    bool jump = false;
};

struct MotorState {
    Vec3 velocity;
    bool on_floor = false;
};

// Computes the velocity for the next physics step. Pure function: collision
// resolution stays with whichever physics engine consumes the result.
[[nodiscard]] Vec3 step_velocity(const MotorState& state, const MotorInput& input, float yaw_radians,
                                 const MotorParams& params, float delta_seconds) noexcept;

struct LookAngles {
    float yaw = 0.0F;   // radians, about +Y
    float pitch = 0.0F; // radians, about local +X, positive looks up
};

// Applies a mouse delta (pixels). Pitch is clamped short of straight up/down
// so the view basis never degenerates.
[[nodiscard]] LookAngles apply_look(LookAngles angles, float delta_x, float delta_y, float radians_per_pixel) noexcept;

// Yaw (about +Y) that makes a model whose front is +Z (the glTF convention)
// face along a horizontal direction.
[[nodiscard]] float facing_yaw(float direction_x, float direction_z) noexcept;

// Moves `current` toward `target` by at most `max_step` radians, always the
// short way round. The result is wrapped to (-pi, pi].
[[nodiscard]] float turn_toward(float current, float target, float max_step) noexcept;

enum class LocomotionState : unsigned char { Idle, Walk, Run, Airborne };

struct LocomotionThresholds {
    float idle_below = 0.25F; // m/s; below this the character counts as standing
    float run_above = 3.0F;   // m/s; at or above this the run cycle is used
};

// Chooses which animation state matches the character's motion.
[[nodiscard]] LocomotionState select_locomotion_state(float horizontal_speed, bool on_floor,
                                                      const LocomotionThresholds& thresholds = {}) noexcept;

} // namespace e5::gameplay

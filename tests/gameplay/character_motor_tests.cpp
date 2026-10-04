#include "e5/gameplay/character_motor.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <numbers>

using Catch::Approx;
using namespace e5::gameplay;

namespace {
constexpr MotorParams params{};
constexpr float dt = 1.0F / 60.0F;

// Runs enough steps for horizontal velocity to converge.
Vec3 settle(MotorInput input, float yaw, bool on_floor = true) {
    MotorState state{.velocity = {}, .on_floor = on_floor};
    for (int i = 0; i < 240; ++i) {
        state.velocity = step_velocity(state, input, yaw, params, dt);
    }
    return state.velocity;
}
} // namespace

TEST_CASE("forward input moves along -Z at walk speed", "[motor]") {
    const Vec3 v = settle({.move_forward = 1.0F}, 0.0F);
    CHECK(v.x == Approx(0.0F).margin(1e-4));
    CHECK(v.z == Approx(-params.walk_speed));
    CHECK(v.y == Approx(0.0F));
}

TEST_CASE("yaw rotates the movement direction", "[motor]") {
    // Yaw +90 degrees turns the character left: forward becomes world -X.
    const Vec3 v = settle({.move_forward = 1.0F}, std::numbers::pi_v<float> / 2.0F);
    CHECK(v.x == Approx(-params.walk_speed));
    CHECK(v.z == Approx(0.0F).margin(1e-4));
}

TEST_CASE("diagonal input is not faster than straight input", "[motor]") {
    const Vec3 v = settle({.move_right = 1.0F, .move_forward = 1.0F}, 0.0F);
    CHECK(std::hypot(v.x, v.z) == Approx(params.walk_speed));
}

TEST_CASE("sprint uses sprint speed", "[motor]") {
    const Vec3 v = settle({.move_forward = 1.0F, .sprint = true}, 0.0F);
    CHECK(v.z == Approx(-params.sprint_speed));
}

TEST_CASE("acceleration is rate limited per step", "[motor]") {
    const MotorState state{.velocity = {}, .on_floor = true};
    const Vec3 v = step_velocity(state, {.move_forward = 1.0F}, 0.0F, params, dt);
    CHECK(v.z == Approx(-params.ground_acceleration * dt));
}

TEST_CASE("releasing input decelerates to a stop", "[motor]") {
    MotorState state{.velocity = {.x = 0.0F, .y = 0.0F, .z = -params.walk_speed}, .on_floor = true};
    for (int i = 0; i < 60; ++i) {
        state.velocity = step_velocity(state, {}, 0.0F, params, dt);
    }
    CHECK(state.velocity.z == Approx(0.0F).margin(1e-5));
}

TEST_CASE("jump only applies on the floor", "[motor]") {
    const Vec3 grounded = step_velocity({.velocity = {}, .on_floor = true}, {.jump = true}, 0.0F, params, dt);
    CHECK(grounded.y == Approx(params.jump_velocity));

    const Vec3 airborne = step_velocity({.velocity = {}, .on_floor = false}, {.jump = true}, 0.0F, params, dt);
    CHECK(airborne.y == Approx(-params.gravity * dt));
}

TEST_CASE("falling speed is capped at terminal velocity", "[motor]") {
    MotorState state{.velocity = {}, .on_floor = false};
    for (int i = 0; i < 60 * 30; ++i) {
        state.velocity = step_velocity(state, {}, 0.0F, params, dt);
    }
    CHECK(state.velocity.y == Approx(-params.terminal_fall_speed));
}

TEST_CASE("look pitch is clamped and yaw wraps", "[look]") {
    constexpr float sensitivity = 0.002F;
    const LookAngles up = apply_look({}, 0.0F, -100000.0F, sensitivity);
    CHECK(up.pitch < std::numbers::pi_v<float> / 2.0F);
    CHECK(up.pitch > 1.5F);

    const LookAngles down = apply_look({}, 0.0F, 100000.0F, sensitivity);
    CHECK(down.pitch == Approx(-up.pitch));

    const LookAngles spun = apply_look({}, 100000.0F, 0.0F, sensitivity);
    CHECK(std::abs(spun.yaw) <= std::numbers::pi_v<float> + 1e-4F);

    // Mouse right turns right (negative yaw about +Y).
    CHECK(apply_look({}, 10.0F, 0.0F, sensitivity).yaw == Approx(-0.02F));
}

TEST_CASE("facing yaw points a +Z-forward model along the direction", "[facing]") {
    CHECK(facing_yaw(0.0F, 1.0F) == Approx(0.0F));
    CHECK(facing_yaw(1.0F, 0.0F) == Approx(std::numbers::pi_v<float> / 2.0F));
    CHECK(std::abs(facing_yaw(0.0F, -1.0F)) == Approx(std::numbers::pi_v<float>));
}

TEST_CASE("turn_toward is rate limited and takes the short way round", "[facing]") {
    constexpr float pi = std::numbers::pi_v<float>;
    CHECK(turn_toward(0.0F, 1.0F, 0.25F) == Approx(0.25F));
    CHECK(turn_toward(0.0F, 0.1F, 0.25F) == Approx(0.1F));
    // From 170 degrees to -170 degrees the short way crosses +-180, not zero.
    const float from = pi * 170.0F / 180.0F;
    const float to = -pi * 170.0F / 180.0F;
    const float stepped = turn_toward(from, to, pi * 5.0F / 180.0F);
    CHECK(stepped == Approx(pi * 175.0F / 180.0F));
    CHECK(std::abs(turn_toward(from, to, pi)) == Approx(pi * 170.0F / 180.0F));
}

TEST_CASE("locomotion state follows speed and ground contact", "[locomotion]") {
    CHECK(select_locomotion_state(0.0F, true) == LocomotionState::Idle);
    CHECK(select_locomotion_state(1.5F, true) == LocomotionState::Walk);
    CHECK(select_locomotion_state(4.5F, true) == LocomotionState::Run);
    CHECK(select_locomotion_state(4.5F, false) == LocomotionState::Airborne);
    CHECK(select_locomotion_state(0.0F, false) == LocomotionState::Airborne);
}

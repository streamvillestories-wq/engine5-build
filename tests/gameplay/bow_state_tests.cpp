#include "e5/gameplay/bow_state.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;
using namespace e5::gameplay;

namespace {
constexpr BowTimings timings{
    .draw_seconds = 1.0F, .release_seconds = 0.5F, .charge_seconds = 1.0F, .min_charge = 0.35F};
constexpr float dt = 0.1F;
constexpr BowInput hold{.aim_held = true, .cancel_pressed = false, .build_charge = false};
constexpr BowInput hold_charging{.aim_held = true, .cancel_pressed = false, .build_charge = true};
constexpr BowInput hold_and_cancel{.aim_held = true, .cancel_pressed = true, .build_charge = false};
constexpr BowInput let_go{};
constexpr BowState at_full_draw{.phase = BowPhase::Aiming, .phase_seconds = 1.0F, .charge = 0.0F};

// Advances with constant input and returns the last step.
BowStep run(BowState state, BowInput input, int steps) {
    BowStep step{.state = state};
    for (int i = 0; i < steps; ++i) {
        step = step_bow(step.state, input, timings, dt);
    }
    return step;
}
} // namespace

TEST_CASE("the bow stays lowered without input", "[bow_state]") {
    const BowStep step = run({}, let_go, 5);
    CHECK(step.state.phase == BowPhase::Lowered);
    CHECK(step.string_draw == Approx(0.0F));
    CHECK_FALSE(step.arrow_released);
}

TEST_CASE("holding aim draws, then aims at full draw", "[bow_state]") {
    const BowStep started = run({}, hold, 1);
    CHECK(started.state.phase == BowPhase::Drawing);

    const BowStep halfway = run(started.state, hold, 5);
    CHECK(halfway.state.phase == BowPhase::Drawing);
    CHECK(halfway.string_draw == Approx(0.5F));

    const BowStep drawn = run(halfway.state, hold, 6);
    CHECK(drawn.state.phase == BowPhase::Aiming);
    CHECK(drawn.string_draw == Approx(1.0F));
    CHECK_FALSE(drawn.arrow_released);
}

TEST_CASE("letting go before full draw lowers the bow without shooting", "[bow_state]") {
    const BowStep early = step_bow(run({}, hold, 4).state, let_go, timings, dt);
    CHECK(early.state.phase == BowPhase::Lowered);
    CHECK_FALSE(early.arrow_released);
}

TEST_CASE("letting go at full draw shoots exactly one normal arrow", "[bow_state]") {
    const BowStep shot = step_bow(at_full_draw, let_go, timings, dt);
    CHECK(shot.state.phase == BowPhase::Releasing);
    CHECK(shot.arrow_released);
    CHECK(shot.shot_power == Approx(0.0F));
    CHECK(shot.string_draw == Approx(0.0F));

    CHECK_FALSE(step_bow(shot.state, let_go, timings, dt).arrow_released);
}

TEST_CASE("cancel lowers the bow at any point without shooting", "[bow_state]") {
    const BowStep drawing = step_bow(run({}, hold, 4).state, hold_and_cancel, timings, dt);
    CHECK(drawing.state.phase == BowPhase::Lowered);
    CHECK_FALSE(drawing.arrow_released);

    const BowStep aiming = step_bow(at_full_draw, hold_and_cancel, timings, dt);
    CHECK(aiming.state.phase == BowPhase::Lowered);
    CHECK_FALSE(aiming.arrow_released);

    // Cancel also wins over a simultaneous release.
    const BowInput cancel_and_let_go{.aim_held = false, .cancel_pressed = true, .build_charge = false};
    CHECK_FALSE(step_bow(at_full_draw, cancel_and_let_go, timings, dt).arrow_released);
}

TEST_CASE("after the follow-through the next draw starts if aim is held", "[bow_state]") {
    const BowState releasing{.phase = BowPhase::Releasing, .phase_seconds = 0.0F, .charge = 0.0F};
    CHECK(run(releasing, hold, 3).state.phase == BowPhase::Releasing);
    CHECK(run(releasing, hold, 6).state.phase == BowPhase::Drawing);
    CHECK(run(releasing, let_go, 6).state.phase == BowPhase::Lowered);
}

TEST_CASE("a charging skill builds power while aim is held", "[bow_state]") {
    const BowStep half = run(at_full_draw, hold_charging, 5);
    CHECK(half.state.phase == BowPhase::Aiming);
    CHECK(half.state.charge == Approx(0.5F));
    CHECK_FALSE(half.arrow_released);

    CHECK(run(at_full_draw, hold_charging, 30).state.charge == Approx(1.0F));
    CHECK(run(at_full_draw, hold, 30).state.charge == Approx(0.0F));
}

TEST_CASE("charge only builds at full draw", "[bow_state]") {
    const BowStep drawing = run({}, hold_charging, 5);
    CHECK(drawing.state.phase == BowPhase::Drawing);
    CHECK(drawing.state.charge == Approx(0.0F));
}

TEST_CASE("letting go releases with the power reached", "[bow_state]") {
    const BowStep shot = step_bow(run(at_full_draw, hold_charging, 8).state, let_go, timings, dt);
    CHECK(shot.state.phase == BowPhase::Releasing);
    CHECK(shot.arrow_released);
    CHECK(shot.shot_power == Approx(0.8F));
    CHECK(shot.state.charge == Approx(0.0F));
}

TEST_CASE("too little charge shoots a normal arrow", "[bow_state]") {
    const BowStep shot = step_bow(run(at_full_draw, hold_charging, 2).state, let_go, timings, dt);
    CHECK(shot.arrow_released);
    CHECK(shot.shot_power == Approx(0.0F));
}

TEST_CASE("cancel drops the charge", "[bow_state]") {
    const BowStep lowered = step_bow(run(at_full_draw, hold_charging, 8).state, hold_and_cancel, timings, dt);
    CHECK(lowered.state.phase == BowPhase::Lowered);
    CHECK(lowered.state.charge == Approx(0.0F));
    CHECK_FALSE(lowered.arrow_released);
}

TEST_CASE("strafe direction follows the dominant axis", "[bow_state]") {
    CHECK(select_strafe_direction(1.0F, 0.2F) == StrafeDirection::Forward);
    CHECK(select_strafe_direction(-1.0F, 0.2F) == StrafeDirection::Back);
    CHECK(select_strafe_direction(0.1F, 1.0F) == StrafeDirection::Left);
    CHECK(select_strafe_direction(0.1F, -1.0F) == StrafeDirection::Right);
    CHECK(select_strafe_direction(1.0F, 1.0F) == StrafeDirection::Forward);
}
#include "e5/gameplay/bow_state.hpp"

#include <algorithm>
#include <cmath>

namespace e5::gameplay {
namespace {

BowStep enter(BowPhase phase, float string_draw) noexcept {
    return BowStep{.state = {.phase = phase, .phase_seconds = 0.0F, .charge = 0.0F},
                   .string_draw = string_draw,
                   .arrow_released = false,
                   .shot_power = 0.0F};
}

BowStep release(float shot_power) noexcept {
    BowStep step = enter(BowPhase::Releasing, 0.0F);
    step.arrow_released = true;
    step.shot_power = shot_power;
    return step;
}

BowStep stay(BowPhase phase, float phase_seconds, float charge, float string_draw) noexcept {
    return BowStep{.state = {.phase = phase, .phase_seconds = phase_seconds, .charge = charge},
                   .string_draw = string_draw,
                   .arrow_released = false,
                   .shot_power = 0.0F};
}

BowStep step_aiming(const BowState& state, const BowInput& input, const BowTimings& timings, float elapsed,
                    float delta_seconds) noexcept {
    if (input.cancel_pressed) {
        return enter(BowPhase::Lowered, 0.0F);
    }
    if (!input.aim_held) {
        // Letting go shoots. Too little charge counts as none.
        return release(state.charge >= timings.min_charge ? state.charge : 0.0F);
    }
    float charge = 0.0F;
    if (input.build_charge) {
        const float gain = timings.charge_seconds > 0.0F ? delta_seconds / timings.charge_seconds : 1.0F;
        charge = std::min(state.charge + gain, 1.0F);
    }
    return stay(BowPhase::Aiming, elapsed, charge, 1.0F);
}

} // namespace

BowStep step_bow(const BowState& state, const BowInput& input, const BowTimings& timings,
                 float delta_seconds) noexcept {
    const float elapsed = state.phase_seconds + delta_seconds;

    switch (state.phase) {
    case BowPhase::Lowered:
        return input.aim_held && !input.cancel_pressed ? enter(BowPhase::Drawing, 0.0F)
                                                       : enter(BowPhase::Lowered, 0.0F);

    case BowPhase::Drawing:
        if (!input.aim_held || input.cancel_pressed) {
            return enter(BowPhase::Lowered, 0.0F);
        }
        if (elapsed >= timings.draw_seconds) {
            return enter(BowPhase::Aiming, 1.0F);
        }
        return stay(BowPhase::Drawing, elapsed, 0.0F,
                    timings.draw_seconds > 0.0F ? elapsed / timings.draw_seconds : 1.0F);

    case BowPhase::Aiming:
        return step_aiming(state, input, timings, elapsed, delta_seconds);

    case BowPhase::Releasing:
        // The follow-through always completes: an interrupted release would
        // let rapid clicking cancel its own recovery.
        if (elapsed >= timings.release_seconds) {
            return input.aim_held ? enter(BowPhase::Drawing, 0.0F) : enter(BowPhase::Lowered, 0.0F);
        }
        return stay(BowPhase::Releasing, elapsed, 0.0F, 0.0F);
    }
    return enter(BowPhase::Lowered, 0.0F);
}

StrafeDirection select_strafe_direction(float forward_speed, float left_speed) noexcept {
    // Ties go to forward/back: diagonal movement reads better with those clips.
    if (std::abs(forward_speed) >= std::abs(left_speed)) {
        return forward_speed >= 0.0F ? StrafeDirection::Forward : StrafeDirection::Back;
    }
    return left_speed > 0.0F ? StrafeDirection::Left : StrafeDirection::Right;
}

} // namespace e5::gameplay
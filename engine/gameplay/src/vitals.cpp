#include "e5/gameplay/vitals.hpp"

#include <algorithm>

namespace e5::gameplay {

Vitals full_vitals(const VitalsParams& params) noexcept {
    return {.health = params.max_health};
}

VitalsStep step_vitals(const Vitals& state, float damage, const VitalsParams& params, float delta_seconds) noexcept {
    VitalsStep step{.state = state};
    if (state.dead) {
        step.state.seconds_dead += delta_seconds;
        if (step.state.seconds_dead >= params.respawn_seconds) {
            step.state = full_vitals(params);
            step.respawned = true;
        }
        return step;
    }

    if (damage > 0.0F) {
        step.hurt = true;
        step.state.health = std::max(state.health - damage, 0.0F);
        step.state.seconds_since_hurt = 0.0F;
        if (step.state.health <= 0.0F) {
            step.state.dead = true;
            step.state.seconds_dead = 0.0F;
            step.died = true;
        }
        return step;
    }

    step.state.seconds_since_hurt += delta_seconds;
    if (step.state.seconds_since_hurt >= params.regen_delay_seconds) {
        step.state.health = std::min(state.health + params.regen_per_second * delta_seconds, params.max_health);
    }
    return step;
}

} // namespace e5::gameplay

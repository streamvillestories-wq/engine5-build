#include "e5/gameplay/enemy.hpp"

#include <algorithm>

namespace e5::gameplay {

EnemyState spawn_enemy(const EnemyParams& params) noexcept {
    return {.health = params.max_health,
            .special_cooldown_seconds = params.special_cooldown_seconds * params.special_first_share};
}

namespace {

constexpr float keep_distance_inner_share = 0.6F;

void enter(EnemyState& state, EnemyPhase phase) noexcept {
    state.phase = phase;
    state.phase_seconds = 0.0F;
}

// Takes the damage of this step. Returns true when that decided the step: it died or was staggered.
[[nodiscard]] bool take_damage(const EnemyState& before, const EnemyInput& input, const EnemyParams& params,
                               EnemyStep& step) noexcept {
    EnemyState& next = step.state;
    if (input.damage <= 0.0F) {
        return false;
    }
    next.health = std::max(before.health - input.damage, 0.0F);
    next.aggro = true; // whoever hurt it has its attention
    if (next.health <= 0.0F) {
        enter(next, EnemyPhase::Dead);
        step.died = true;
        return true;
    }
    if (input.heaviest_blow < params.stagger_damage) {
        return false;
    }
    // A cast that is knocked out of its hands is lost, and has to wait like any other.
    if (before.phase == EnemyPhase::Cast) {
        next.cast_cooldown_seconds = params.cast_cooldown_seconds;
    }
    if (before.phase == EnemyPhase::Special) {
        next.special_cooldown_seconds = params.special_cooldown_seconds;
    }
    enter(next, EnemyPhase::Hit);
    return true;
}

// Running at the player: strike when in reach, throw a spell from a distance, give up when too far.
void chase(const EnemyInput& input, const EnemyParams& params, EnemyStep& step) noexcept {
    EnemyState& next = step.state;
    if (!input.has_player || input.distance_to_player > params.give_up_range) {
        next.aggro = false;
        enter(next, EnemyPhase::Idle);
        return;
    }
    const bool in_reach = input.distance_to_player <= params.attack_range;
    const bool in_cast_range = params.cast_range > 0.0F && input.distance_to_player <= params.cast_range &&
                               input.distance_to_player >= params.cast_min_range;
    const bool special_ready = params.special_range > 0.0F && input.distance_to_player <= params.special_range &&
                               next.special_cooldown_seconds <= 0.0F;
    if (special_ready) {
        enter(next, EnemyPhase::Special);
        ++next.specials_started;
        step.special_started = true;
    } else if (in_reach && next.attack_cooldown_seconds <= 0.0F) {
        enter(next, EnemyPhase::Attack);
        step.attack_started = true;
    } else if (in_cast_range && next.cast_cooldown_seconds <= 0.0F) {
        enter(next, EnemyPhase::Cast);
        next.cast_released = false;
        step.cast_started = true;
    } else if (params.keep_distance > 0.0F) {
        // Between the two bounds it stays where it is: no dithering back and forth.
        step.moving = input.distance_to_player > params.keep_distance;
        step.retreating = input.distance_to_player < params.keep_distance * keep_distance_inner_share;
    } else {
        step.moving = !in_reach;
    }
}

void cast(const EnemyParams& params, EnemyStep& step) noexcept {
    EnemyState& next = step.state;
    if (!next.cast_released && next.phase_seconds >= params.cast_seconds * params.cast_release_share) {
        next.cast_released = true;
        step.cast_released = true;
    }
    if (next.phase_seconds >= params.cast_seconds) {
        next.cast_cooldown_seconds = params.cast_cooldown_seconds;
        enter(next, EnemyPhase::Chase);
    }
}

} // namespace

EnemyStep step_enemy(const EnemyState& state, const EnemyInput& input, const EnemyParams& params,
                     float delta_seconds) noexcept {
    EnemyStep step{.state = state};
    EnemyState& next = step.state;
    next.phase_seconds += delta_seconds;
    if (state.phase == EnemyPhase::Dead) {
        return step;
    }
    next.attack_cooldown_seconds = std::max(state.attack_cooldown_seconds - delta_seconds, 0.0F);
    next.cast_cooldown_seconds = std::max(state.cast_cooldown_seconds - delta_seconds, 0.0F);
    if (state.phase != EnemyPhase::Idle) {
        next.special_cooldown_seconds = std::max(state.special_cooldown_seconds - delta_seconds, 0.0F);
    }
    if (take_damage(state, input, params, step)) {
        return step;
    }

    switch (next.phase) {
    case EnemyPhase::Idle:
        if (input.has_player && input.distance_to_player <= params.aggro_range) {
            next.aggro = true;
        }
        if (next.aggro && input.has_player) {
            enter(next, EnemyPhase::Chase);
        }
        break;
    case EnemyPhase::Chase:
        chase(input, params, step);
        break;
    case EnemyPhase::Attack:
        if (next.phase_seconds >= params.attack_seconds) {
            next.attack_cooldown_seconds = params.attack_cooldown_seconds;
            enter(next, EnemyPhase::Chase);
        }
        break;
    case EnemyPhase::Cast:
        cast(params, step);
        break;
    case EnemyPhase::Hit:
        if (next.phase_seconds >= params.hit_seconds) {
            enter(next, EnemyPhase::Chase);
        }
        break;
    case EnemyPhase::Special:
        if (next.phase_seconds >= params.special_seconds) {
            next.special_cooldown_seconds = params.special_cooldown_seconds;
            enter(next, EnemyPhase::Chase);
        }
        break;
    case EnemyPhase::Dead:
        break;
    }
    return step;
}

} // namespace e5::gameplay

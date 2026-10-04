#include "e5/gameplay/enemy.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;
using namespace e5::gameplay;

namespace {

constexpr float dt = 1.0F / 60.0F;

EnemyInput player_at(float distance) {
    return {.has_player = true, .distance_to_player = distance};
}

EnemyInput blow(float damage, float distance = 10.0F) {
    return {.has_player = true, .distance_to_player = distance, .damage = damage, .heaviest_blow = damage};
}

} // namespace

TEST_CASE("an enemy ignores a distant player and notices a near one", "[enemy]") {
    const EnemyParams params;
    EnemyState enemy = spawn_enemy(params);
    CHECK(enemy.health == Approx(params.max_health));

    enemy = step_enemy(enemy, player_at(12.0F), params, dt).state;
    CHECK(enemy.phase == EnemyPhase::Idle);
    CHECK_FALSE(enemy.aggro);

    enemy = step_enemy(enemy, player_at(4.0F), params, dt).state;
    CHECK(enemy.phase == EnemyPhase::Chase);
    const EnemyStep walking = step_enemy(enemy, player_at(4.0F), params, dt);
    CHECK(walking.moving);
}

TEST_CASE("an enemy that reaches the player attacks, rests, and attacks again", "[enemy]") {
    const EnemyParams params;
    EnemyState enemy = spawn_enemy(params);
    int attacks = 0;
    int steps_moving_in_reach = 0;
    for (int i = 0; i < 60 * 5; ++i) {
        const EnemyStep step = step_enemy(enemy, player_at(1.0F), params, dt);
        attacks += step.attack_started ? 1 : 0;
        steps_moving_in_reach += step.moving ? 1 : 0;
        enemy = step.state;
    }
    // One attack lasts 1.0 s plus 0.6 s rest, so they start at about 0, 1.6, 3.2 and 4.8 s.
    CHECK(attacks == 4);
    CHECK(steps_moving_in_reach == 0);
}

TEST_CASE("damage reduces health, provokes, and a heavy blow staggers", "[enemy]") {
    const EnemyParams params;
    EnemyState enemy = spawn_enemy(params);

    // A light peck: hurt and provoked, but not staggered.
    EnemyStep step = step_enemy(enemy, blow(6.0F), params, dt);
    CHECK(step.state.health == Approx(94.0F));
    CHECK(step.state.aggro);
    CHECK(step.state.phase != EnemyPhase::Hit);
    CHECK_FALSE(step.died);

    // An arrow: staggered, and it cannot move while staggered.
    step = step_enemy(step.state, blow(20.0F), params, dt);
    CHECK(step.state.phase == EnemyPhase::Hit);
    CHECK_FALSE(step.moving);
    enemy = step.state;
    for (int i = 0; i < 40; ++i) {
        enemy = step_enemy(enemy, player_at(10.0F), params, dt).state;
    }
    CHECK(enemy.phase == EnemyPhase::Chase);
    CHECK(enemy.health == Approx(74.0F));
}

TEST_CASE("an enemy dies when its health is used up and stays dead", "[enemy]") {
    const EnemyParams params;
    EnemyStep step = step_enemy(spawn_enemy(params), blow(250.0F), params, dt);
    CHECK(step.died);
    CHECK(step.state.phase == EnemyPhase::Dead);
    CHECK(step.state.health == Approx(0.0F));

    step = step_enemy(step.state, blow(20.0F, 1.0F), params, dt);
    CHECK_FALSE(step.died); // only once
    CHECK(step.state.phase == EnemyPhase::Dead);
    CHECK_FALSE(step.moving);
    CHECK_FALSE(step.attack_started);
}

TEST_CASE("an enemy gives up when the player is gone or far away", "[enemy]") {
    const EnemyParams params;
    EnemyState enemy = step_enemy(spawn_enemy(params), player_at(3.0F), params, dt).state;
    REQUIRE(enemy.phase == EnemyPhase::Chase);
    enemy = step_enemy(enemy, player_at(30.0F), params, dt).state;
    CHECK(enemy.phase == EnemyPhase::Idle);
    CHECK_FALSE(enemy.aggro);

    enemy = step_enemy(enemy, player_at(3.0F), params, dt).state;
    REQUIRE(enemy.phase == EnemyPhase::Chase);
    enemy = step_enemy(enemy, {}, params, dt).state;
    CHECK(enemy.phase == EnemyPhase::Idle);
}
namespace {

// Steps until `seconds` have passed and returns how many spells left its hand.
int casts_within(EnemyState& state, const EnemyParams& params, float distance, float seconds) {
    int released = 0;
    const float tick = 1.0F / 60.0F;
    const int ticks = static_cast<int>(seconds / tick);
    for (int index = 0; index < ticks; ++index) {
        const EnemyStep step = step_enemy(state, {.has_player = true, .distance_to_player = distance}, params, tick);
        state = step.state;
        released += step.cast_released ? 1 : 0;
    }
    return released;
}

} // namespace

TEST_CASE("an enemy without magic never casts", "[enemy]") {
    const EnemyParams params; // cast_range 0
    EnemyState state = spawn_enemy(params);
    state.aggro = true;
    CHECK(casts_within(state, params, 4.5F, 20.0F) == 0);
}

TEST_CASE("a caster throws from a distance, waits, and throws again", "[enemy]") {
    const EnemyParams params{.aggro_range = 12.0F, .cast_range = 12.0F};
    EnemyState state = spawn_enemy(params);

    // It notices the player and starts at once.
    EnemyStep step = step_enemy(state, {.has_player = true, .distance_to_player = 8.0F}, params, 0.1F);
    step = step_enemy(step.state, {.has_player = true, .distance_to_player = 8.0F}, params, 0.1F);
    CHECK(step.cast_started);
    CHECK(step.state.phase == EnemyPhase::Cast);
    CHECK_FALSE(step.moving);
    state = step.state;

    // One spell per cast, part-way through it; the next after the cooldown.
    CHECK(casts_within(state, params, 8.0F, params.cast_seconds + 0.1F) == 1);
    CHECK(state.phase == EnemyPhase::Chase);
    CHECK(casts_within(state, params, 8.0F, params.cast_cooldown_seconds - 0.7F) == 0);
    CHECK(casts_within(state, params, 8.0F, 0.7F + params.cast_seconds) == 1);
}

TEST_CASE("up close a caster uses its fists, far away it runs closer", "[enemy]") {
    const EnemyParams params{.aggro_range = 30.0F, .cast_range = 12.0F};
    EnemyState state = spawn_enemy(params);
    state.aggro = true;
    state.phase = EnemyPhase::Chase;

    CHECK(casts_within(state, params, 2.0F, 6.0F) == 0);

    EnemyState far = spawn_enemy(params);
    far.aggro = true;
    far.phase = EnemyPhase::Chase;
    const EnemyStep step = step_enemy(far, {.has_player = true, .distance_to_player = 20.0F}, params, 0.1F);
    CHECK(step.moving);
    CHECK_FALSE(step.cast_started);
}

TEST_CASE("a heavy blow before the spell leaves the hand stops the cast", "[enemy]") {
    const EnemyParams params{.aggro_range = 12.0F, .cast_range = 12.0F};
    EnemyState state = spawn_enemy(params);
    state.aggro = true;
    state.phase = EnemyPhase::Chase;
    EnemyStep step = step_enemy(state, {.has_player = true, .distance_to_player = 8.0F}, params, 0.1F);
    REQUIRE(step.cast_started);

    step = step_enemy(step.state,
                      {.has_player = true, .distance_to_player = 8.0F, .damage = 20.0F, .heaviest_blow = 20.0F}, params,
                      0.1F);
    CHECK(step.state.phase == EnemyPhase::Hit);
    CHECK_FALSE(step.cast_released);
    state = step.state;
    // Nothing flies afterwards either, until the cooldown is over.
    CHECK(casts_within(state, params, 8.0F, params.cast_cooldown_seconds - 0.5F) == 0);

    // A light blow does not stop it.
    EnemyState calm = spawn_enemy(params);
    calm.aggro = true;
    calm.phase = EnemyPhase::Chase;
    step = step_enemy(calm, {.has_player = true, .distance_to_player = 8.0F}, params, 0.1F);
    step =
        step_enemy(step.state, {.has_player = true, .distance_to_player = 8.0F, .damage = 6.0F, .heaviest_blow = 6.0F},
                   params, 0.1F);
    CHECK(step.state.phase == EnemyPhase::Cast);
}

TEST_CASE("an enemy that keeps its distance comes closer, holds, and backs off", "[enemy]") {
    const EnemyParams params{.aggro_range = 30.0F, .attack_range = 0.0F, .keep_distance = 8.0F};
    EnemyState state = spawn_enemy(params);
    state.aggro = true;
    state.phase = EnemyPhase::Chase;

    const EnemyStep far = step_enemy(state, {.has_player = true, .distance_to_player = 15.0F}, params, 0.1F);
    CHECK(far.moving);
    CHECK_FALSE(far.retreating);

    const EnemyStep right = step_enemy(state, {.has_player = true, .distance_to_player = 6.5F}, params, 0.1F);
    CHECK_FALSE(right.moving);
    CHECK_FALSE(right.retreating);

    const EnemyStep close = step_enemy(state, {.has_player = true, .distance_to_player = 2.0F}, params, 0.1F);
    CHECK_FALSE(close.moving);
    CHECK(close.retreating);
    // It never strikes: its attack range is zero.
    CHECK_FALSE(close.attack_started);
}

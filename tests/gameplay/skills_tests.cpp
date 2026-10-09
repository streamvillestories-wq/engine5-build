#include "e5/gameplay/skills.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>

using Catch::Approx;
using namespace e5::gameplay;

TEST_CASE("the skill bar starts with the archer's skills in order", "[skills]") {
    const SkillBar bar;
    CHECK(bar.selected() == SkillId::Shot);
    CHECK(bar.slot(0) == SkillId::Shot);
    CHECK(bar.slot(1) == SkillId::PowerShot);
    CHECK(bar.slot(2) == SkillId::ArrowRain);
    CHECK(bar.slot(3) == SkillId::FrostFan);
    CHECK(bar.slot(4) == SkillId::FireArrow);
    CHECK(bar.slot(5) == SkillId::ThunderKick);
    CHECK(bar.slot(6) == SkillId::Kingfishers);
    CHECK(bar.slot(7) == SkillId::VenomArrow);
    CHECK(bar.slot(8) == SkillId::GaleArrow);
    CHECK(bar.slot(9) == SkillId::StormArrow);
    CHECK(bar.slot(10) == SkillId::BrambleArrow);
    CHECK(bar.slot(11) == SkillId::None);
    CHECK(bar.slot(99) == SkillId::None);
}

TEST_CASE("selecting changes the active skill", "[skills]") {
    SkillBar bar;
    CHECK(bar.select(2));
    CHECK(bar.selected() == SkillId::ArrowRain);
    CHECK(bar.selected_index() == 2);
    CHECK_FALSE(bar.select(2)); // already selected
}

TEST_CASE("empty and out-of-range slots cannot be selected", "[skills]") {
    SkillBar bar;
    CHECK_FALSE(bar.select(11));
    CHECK_FALSE(bar.select(12));
    CHECK(bar.selected() == SkillId::Shot);
}

TEST_CASE("only the power shot charges", "[skills]") {
    CHECK_FALSE(skill_info(SkillId::Shot).charges);
    CHECK(skill_info(SkillId::PowerShot).charges);
    CHECK_FALSE(skill_info(SkillId::ArrowRain).charges);
    CHECK(skill_info(SkillId::None).name.empty());
    CHECK(skill_info(SkillId::ArrowRain).name == "Arrow Rain");
}

TEST_CASE("the kick and the summon are used without drawing the bow", "[skills]") {
    CHECK(skill_info(SkillId::ThunderKick).kind == SkillKind::Instant);
    CHECK(skill_info(SkillId::Kingfishers).kind == SkillKind::Instant);
    for (const SkillId skill : {SkillId::Shot, SkillId::PowerShot, SkillId::ArrowRain, SkillId::FrostFan,
                                SkillId::FireArrow, SkillId::None}) {
        CHECK(skill_info(skill).kind == SkillKind::Bow);
        CHECK_FALSE((skill != SkillId::PowerShot && skill_info(skill).charges));
    }
}

TEST_CASE("every skill does damage, and charging a power shot triples it", "[skills]") {
    for (const SkillId skill : {SkillId::Shot, SkillId::PowerShot, SkillId::ArrowRain, SkillId::FrostFan,
                                SkillId::FireArrow, SkillId::ThunderKick, SkillId::Kingfishers}) {
        CHECK(skill_damage(skill) > 0.0F);
    }
    CHECK(skill_damage(SkillId::None) == Approx(0.0F));
    CHECK(skill_damage(SkillId::PowerShot, 0.0F) == Approx(skill_damage(SkillId::Shot)));
    CHECK(skill_damage(SkillId::PowerShot, 1.0F) == Approx(3.0F * skill_damage(SkillId::Shot)));
    CHECK(skill_damage(SkillId::PowerShot, 7.0F) == Approx(skill_damage(SkillId::PowerShot, 1.0F)));
    // Charge only matters for the power shot.
    CHECK(skill_damage(SkillId::Shot, 1.0F) == Approx(skill_damage(SkillId::Shot)));
}

TEST_CASE("a fan of arrows is symmetric and covers the spread", "[skills]") {
    constexpr float spread = 0.4F;
    CHECK(fan_yaw_offset(0, 5, spread) == Approx(-0.2F));
    CHECK(fan_yaw_offset(2, 5, spread) == Approx(0.0F).margin(1e-6));
    CHECK(fan_yaw_offset(4, 5, spread) == Approx(0.2F));
    CHECK(fan_yaw_offset(1, 5, spread) == Approx(-fan_yaw_offset(3, 5, spread)));
    // A single arrow, or a bad index, flies straight.
    CHECK(fan_yaw_offset(0, 1, spread) == Approx(0.0F));
    CHECK(fan_yaw_offset(5, 5, spread) == Approx(0.0F));
    CHECK(fan_yaw_offset(-1, 5, spread) == Approx(0.0F));
}

TEST_CASE("distance to a disc is measured to its nearest point", "[skills]") {
    constexpr float radius = 2.0F;
    CHECK(distance_to_disc({.x = 0.5F, .y = 1.0F, .z = 0.0F}, radius) == Approx(0.0F));
    CHECK(distance_to_disc({.x = 0.0F, .y = 0.0F, .z = -1.5F}, radius) == Approx(1.5F)); // above the face
    CHECK(distance_to_disc({.x = 5.0F, .y = 0.0F, .z = 0.0F}, radius) == Approx(3.0F));  // beside the rim
    CHECK(distance_to_disc({.x = 0.0F, .y = 5.0F, .z = 4.0F}, radius) == Approx(5.0F));  // 3 out, 4 up
}

TEST_CASE("an instant action runs once and strikes once", "[skills]") {
    const ActionTimings timings{.duration_seconds = 1.0F, .strike_at_seconds = 0.4F};
    constexpr float dt = 1.0F / 60.0F;

    ActionState state;
    CHECK_FALSE(step_action(state, false, timings, dt).state.active);

    ActionStep step = step_action(state, true, timings, dt);
    CHECK(step.started);
    CHECK(step.state.active);
    CHECK_FALSE(step.strike);
    state = step.state;

    int strikes = 0;
    int restarts = 0;
    int steps = 0;
    float strike_time = 0.0F;
    while (state.active && steps < 1000) {
        // Asking again while it runs must not restart it.
        step = step_action(state, true, timings, dt);
        restarts += step.started ? 1 : 0;
        if (step.strike) {
            ++strikes;
            strike_time = state.elapsed + dt;
        }
        state = step.state;
        ++steps;
    }
    CHECK(strikes == 1);
    CHECK(restarts == 0);
    CHECK(strike_time == Approx(0.4F).margin(dt));
    CHECK(steps == Approx(60).margin(1));
}

TEST_CASE("an action's blow lands even in one very long step", "[skills]") {
    const ActionTimings timings{.duration_seconds = 1.0F, .strike_at_seconds = 0.4F};
    const ActionStep step = step_action({.active = true, .elapsed = 0.0F}, false, timings, 5.0F);
    CHECK(step.strike);
    CHECK_FALSE(step.state.active);
}

TEST_CASE("rain arrows land inside the radius and are spread out", "[skills]") {
    constexpr int count = 24;
    constexpr float radius = 3.0F;
    float furthest = 0.0F;
    int in_inner_half_of_area = 0;
    for (int i = 0; i < count; ++i) {
        const Vec3 offset = rain_arrow_offset(i, count, radius);
        const float distance = std::hypot(offset.x, offset.z);
        CHECK(offset.y == Approx(0.0F));
        CHECK(distance <= radius);
        furthest = std::max(furthest, distance);
        if (distance <= radius * std::sqrt(0.5F)) {
            ++in_inner_half_of_area;
        }
    }
    CHECK(furthest > radius * 0.9F);
    // Evenly spread by area: half of the arrows in the inner half of the disc.
    CHECK(in_inner_half_of_area == count / 2);
}

TEST_CASE("the rain pattern is deterministic and rejects bad indices", "[skills]") {
    const Vec3 first = rain_arrow_offset(7, 24, 3.0F);
    const Vec3 again = rain_arrow_offset(7, 24, 3.0F);
    CHECK(first.x == again.x);
    CHECK(first.z == again.z);

    CHECK(rain_arrow_offset(-1, 24, 3.0F).x == Approx(0.0F));
    CHECK(rain_arrow_offset(24, 24, 3.0F).x == Approx(0.0F));
    CHECK(rain_arrow_offset(0, 0, 3.0F).x == Approx(0.0F));
}

TEST_CASE("the wizard has his own skills on the bar", "[skills]") {
    const e5::gameplay::SkillBar bar(e5::gameplay::SkillSet::Wizard);

    CHECK(bar.slot(0) == e5::gameplay::SkillId::ArcaneBolt);
    CHECK(bar.slot(1) == e5::gameplay::SkillId::Fireball);
    CHECK(bar.slot(2) == e5::gameplay::SkillId::FrostNova);
    CHECK(bar.slot(3) == e5::gameplay::SkillId::ChainLightning);
    CHECK(bar.slot(4) == e5::gameplay::SkillId::Meteor);
    CHECK(bar.slot(5) == e5::gameplay::SkillId::StarBarrage);
    CHECK(bar.slot(6) == e5::gameplay::SkillId::BlackHole);
    CHECK(bar.slot(7) == e5::gameplay::SkillId::None);
    CHECK(bar.selected() == e5::gameplay::SkillId::ArcaneBolt);
    for (const e5::gameplay::SkillId spell :
         {e5::gameplay::SkillId::ArcaneBolt, e5::gameplay::SkillId::Fireball, e5::gameplay::SkillId::FrostNova,
          e5::gameplay::SkillId::ChainLightning, e5::gameplay::SkillId::Meteor, e5::gameplay::SkillId::StarBarrage,
          e5::gameplay::SkillId::BlackHole}) {
        // None of them needs a bow, all of them hurt, and all have a name for the bar.
        CHECK(e5::gameplay::skill_info(spell).kind == e5::gameplay::SkillKind::Instant);
        CHECK(e5::gameplay::skill_damage(spell) > 0.0F);
        CHECK_FALSE(e5::gameplay::skill_info(spell).name.empty());
    }
    // The slow, big spell hits harder than the quick one.
    CHECK(e5::gameplay::skill_damage(e5::gameplay::SkillId::Fireball) >
          e5::gameplay::skill_damage(e5::gameplay::SkillId::ArcaneBolt));
}

TEST_CASE("the warrior has her own skills on the bar", "[skills]") {
    using e5::gameplay::SkillId;
    const e5::gameplay::SkillBar bar(e5::gameplay::SkillSet::Warrior);

    CHECK(bar.slot(0) == SkillId::Slash);
    CHECK(bar.slot(1) == SkillId::FlameBlade);
    CHECK(bar.slot(2) == SkillId::FrostEdge);
    CHECK(bar.slot(3) == SkillId::ThunderCleave);
    CHECK(bar.slot(4) == SkillId::StarWhirl);
    CHECK(bar.slot(5) == SkillId::None);
    for (const SkillId skill :
         {SkillId::Slash, SkillId::FlameBlade, SkillId::FrostEdge, SkillId::ThunderCleave, SkillId::StarWhirl}) {
        CHECK(e5::gameplay::skill_info(skill).kind == e5::gameplay::SkillKind::Instant);
        CHECK(e5::gameplay::skill_damage(skill) > 0.0F);
        CHECK_FALSE(e5::gameplay::skill_info(skill).name.empty());
    }
    // Every special blow hits harder than a plain one.
    CHECK(e5::gameplay::skill_damage(SkillId::FrostEdge) > e5::gameplay::skill_damage(SkillId::Slash));
}

TEST_CASE("the sword combo goes on while she keeps striking, and starts over when she waits", "[skills]") {
    using e5::gameplay::next_combo_step;
    constexpr float window = 0.7F;

    CHECK(next_combo_step(-1, 0.0F, window) == 0); // never struck before
    CHECK(next_combo_step(0, 0.2F, window) == 1);
    CHECK(next_combo_step(1, 0.6F, window) == 2);
    CHECK(next_combo_step(2, 0.1F, window) == 0); // after the finisher
    CHECK(next_combo_step(0, 0.9F, window) == 0); // waited too long
    CHECK(next_combo_step(1, 5.0F, window) == 0);

    // The finisher is the hardest blow, and steps beyond the combo count as it.
    CHECK(e5::gameplay::combo_damage_factor(0) == 1.0F);
    CHECK(e5::gameplay::combo_damage_factor(2) > e5::gameplay::combo_damage_factor(1));
    CHECK(e5::gameplay::combo_damage_factor(9) == e5::gameplay::combo_damage_factor(2));
}

TEST_CASE("the dwarf has his own skills on the bar", "[skills]") {
    using e5::gameplay::SkillId;
    const e5::gameplay::SkillBar bar(e5::gameplay::SkillSet::Dwarf);

    CHECK(bar.slot(0) == SkillId::AxeCombo);
    CHECK(bar.slot(1) == SkillId::Whirlwind);
    CHECK(bar.slot(2) == SkillId::Earthbreaker);
    CHECK(bar.slot(3) == SkillId::LeapStrike);
    CHECK(bar.slot(4) == SkillId::Battlecry);
    CHECK(bar.slot(5) == SkillId::None);
    for (const SkillId skill :
         {SkillId::AxeCombo, SkillId::Whirlwind, SkillId::Earthbreaker, SkillId::LeapStrike, SkillId::Battlecry}) {
        CHECK(e5::gameplay::skill_info(skill).kind == e5::gameplay::SkillKind::Instant);
        CHECK(e5::gameplay::skill_damage(skill) > 0.0F);
        CHECK_FALSE(e5::gameplay::skill_info(skill).name.empty());
    }
    // His axe hits harder than her sword, blow for blow.
    CHECK(e5::gameplay::skill_damage(SkillId::AxeCombo) > e5::gameplay::skill_damage(SkillId::Slash));
    // The cry takes a full charge, so it must be worth more than a blow of the axe; but it is
    // still about throwing them back, not his hardest hit.
    CHECK(e5::gameplay::skill_damage(SkillId::Battlecry) > e5::gameplay::skill_damage(SkillId::AxeCombo));
    CHECK(e5::gameplay::skill_damage(SkillId::Battlecry) < e5::gameplay::skill_damage(SkillId::Earthbreaker));
}

TEST_CASE("the sword combo carries her forward by each blow's distance", "[skills]") {
    using e5::gameplay::combo_advance_distance;
    using e5::gameplay::combo_advance_speed;
    for (int step = 0; step < e5::gameplay::combo_length; ++step) {
        // Standing at the start and at the end of a blow.
        CHECK(combo_advance_speed(step, 0.0F) == 0.0F);
        CHECK(combo_advance_speed(step, 1.4F) == 0.0F);
        // The speed, summed over the blow in small steps, is the distance.
        float travelled = 0.0F;
        constexpr float dt = 0.001F;
        for (float seconds = 0.0F; seconds < 1.5F; seconds += dt) {
            travelled += combo_advance_speed(step, seconds) * dt;
        }
        CHECK(travelled == Catch::Approx(combo_advance_distance(step)).margin(0.005));
    }
    // The dwarf's axe has steps of its own, and they add up the same way.
    for (int step = 0; step < e5::gameplay::combo_length; ++step) {
        float travelled = 0.0F;
        constexpr float dt = 0.001F;
        for (float seconds = 0.0F; seconds < 1.5F; seconds += dt) {
            travelled += combo_advance_speed(step, seconds, true) * dt;
        }
        CHECK(travelled == Catch::Approx(combo_advance_distance(step, true)).margin(0.005));
    }
    // The finisher is a leap: it goes furthest.
    CHECK(combo_advance_distance(2) > combo_advance_distance(0));
}

TEST_CASE("the shield is raised while held, comes down after its time and then cools down", "[skills]") {
    using e5::gameplay::BlockParams;
    using e5::gameplay::BlockState;
    using e5::gameplay::step_block;
    const BlockParams params{.max_hold_seconds = 2.0F, .cooldown_seconds = 5.0F};

    auto step = step_block(BlockState{}, true, true, true, params, 0.1F);
    CHECK(step.raised_now);
    CHECK(step.state.raised);

    // Held: it stays up, until its time is over.
    step = step_block(step.state, false, true, true, params, 1.0F);
    CHECK(step.state.raised);
    step = step_block(step.state, false, true, true, params, 1.1F);
    CHECK(step.lowered_now);
    CHECK_FALSE(step.state.raised);
    CHECK(step.state.cooldown_left == Catch::Approx(5.0F));

    // Still held, and pressed again too early: nothing.
    step = step_block(step.state, true, true, true, params, 2.0F);
    CHECK_FALSE(step.state.raised);
    CHECK(step.state.cooldown_left == Catch::Approx(3.0F));
    // Holding the key through the cooldown does not raise it: it takes a new press.
    step = step_block(step.state, false, true, true, params, 3.5F);
    CHECK_FALSE(step.state.raised);
    step = step_block(step.state, true, true, true, params, 0.1F);
    CHECK(step.raised_now);

    // Let go early: down at once, and the cooldown starts.
    step = step_block(step.state, false, false, true, params, 0.1F);
    CHECK(step.lowered_now);
    CHECK(step.state.cooldown_left == Catch::Approx(5.0F));

    // She cannot block in the middle of a blow.
    step = step_block(BlockState{}, true, true, false, params, 0.1F);
    CHECK_FALSE(step.state.raised);
}

TEST_CASE("a raised shield covers the front and the sides, not the back", "[skills]") {
    using e5::gameplay::shield_covers;
    const float forward = 0.0F;                       // looking along +z
    CHECK(shield_covers(forward, 0.0F, 2.0F));        // straight ahead
    CHECK(shield_covers(forward, 2.0F, 0.5F));        // ahead and to one side
    CHECK(shield_covers(forward, -2.0F, 0.5F));       // and to the other
    CHECK_FALSE(shield_covers(forward, 0.0F, -2.0F)); // behind
    CHECK_FALSE(shield_covers(forward, 1.0F, -1.0F));
    // Turned a quarter: what was beside her is ahead now.
    const float quarter = 1.5707963F; // looking along +x
    CHECK(shield_covers(quarter, 2.0F, 0.0F));
    CHECK_FALSE(shield_covers(quarter, -2.0F, 0.0F));
}

TEST_CASE("standard attacks are always ready, specials cool down, one skill a hero takes a charge", "[skills]") {
    using e5::gameplay::skill_cooldown_seconds;
    using e5::gameplay::skill_needs_charge;
    using e5::gameplay::SkillId;
    for (const SkillId standard : {SkillId::Shot, SkillId::ArcaneBolt, SkillId::Slash, SkillId::AxeCombo}) {
        CHECK(skill_cooldown_seconds(standard) == 0.0F);
        CHECK_FALSE(skill_needs_charge(standard));
    }
    // One for each hero, and none of them also has a cooldown.
    for (const SkillId charged : {SkillId::Kingfishers, SkillId::BlackHole, SkillId::StarWhirl, SkillId::Battlecry}) {
        CHECK(skill_needs_charge(charged));
        CHECK(skill_cooldown_seconds(charged) == 0.0F);
    }
    // Every other special has one, between four and ten seconds.
    for (const SkillId special :
         {SkillId::PowerShot, SkillId::ArrowRain, SkillId::FrostFan, SkillId::FireArrow, SkillId::ThunderKick,
          SkillId::Fireball, SkillId::FrostNova, SkillId::ChainLightning, SkillId::Meteor, SkillId::StarBarrage,
          SkillId::FlameBlade, SkillId::FrostEdge, SkillId::ThunderCleave, SkillId::Whirlwind, SkillId::Earthbreaker,
          SkillId::LeapStrike}) {
        CHECK_FALSE(skill_needs_charge(special));
        CHECK(skill_cooldown_seconds(special) >= 4.0F);
        CHECK(skill_cooldown_seconds(special) <= 10.0F);
    }
}

TEST_CASE("a skill that takes a charge is worth more than the same hero's skills on a cooldown", "[skills]") {
    using e5::gameplay::skill_damage;
    using e5::gameplay::SkillId;
    // The warrior's: before the balance of 2026-10-05 her whirl was weaker than her cleave.
    CHECK(skill_damage(SkillId::StarWhirl) > skill_damage(SkillId::ThunderCleave));
    // And no special is weaker, blow for blow, than the standard attack of the hero who has it.
    CHECK(skill_damage(SkillId::FrostFan) * 5.0F > skill_damage(SkillId::Shot));
    CHECK(skill_damage(SkillId::Whirlwind) > skill_damage(SkillId::AxeCombo));
    CHECK(skill_damage(SkillId::LeapStrike) > skill_damage(SkillId::AxeCombo));
}

TEST_CASE("the charge fills from kills and a little from damage, and no further than full", "[skills]") {
    using e5::gameplay::charge_after;
    CHECK(charge_after(0.0F, 0.0F, true) == Catch::Approx(0.1F));
    CHECK(charge_after(0.0F, 100.0F, false) == Catch::Approx(0.04F));
    // An enemy of a hundred points, killed alone: both.
    CHECK(charge_after(0.5F, 100.0F, true) == Catch::Approx(0.64F));
    CHECK(charge_after(0.97F, 100.0F, true) == Catch::Approx(1.0F));
    // Healing an enemy, should that ever happen, takes nothing away.
    CHECK(charge_after(0.5F, -40.0F, false) == Catch::Approx(0.5F));
}

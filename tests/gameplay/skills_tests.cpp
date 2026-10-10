#include "e5/gameplay/skills.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>

using Catch::Approx;
using namespace e5::gameplay;

TEST_CASE("the archer has her shot, the skills wished for her and slots to fill", "[skills]") {
    SkillBar bar;
    CHECK(bar.slot(0) == SkillId::Shot);
    CHECK(bar.slot(1) == SkillId::VineTower);
    for (std::size_t slot = 2; slot < SkillBar::slot_count; ++slot) {
        CHECK(bar.slot(slot) == SkillId::None);
    }
    CHECK_FALSE(bar.select(2));
    CHECK(open_slot_count(SkillSet::Archer) == 9);
    CHECK(open_slot_count(SkillSet::ArcherFull) == 0);
    CHECK(open_slot_count(SkillSet::Wizard) == 0);
}

TEST_CASE("the skills the archer had are kept, in order", "[skills]") {
    const SkillBar bar(SkillSet::ArcherFull);
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
    CHECK(bar.slot(11) == SkillId::DaggerCombo);
    CHECK(bar.slot(12) == SkillId::None);
    CHECK(bar.slot(99) == SkillId::None);
}

TEST_CASE("selecting changes the active skill", "[skills]") {
    SkillBar bar(SkillSet::ArcherFull);
    CHECK(bar.select(2));
    CHECK(bar.selected() == SkillId::ArrowRain);
    CHECK(bar.selected_index() == 2);
    CHECK_FALSE(bar.select(2)); // already selected
}

TEST_CASE("empty and out-of-range slots cannot be selected", "[skills]") {
    SkillBar bar(SkillSet::ArcherFull);
    // The archer's old bar is full; the wizard's is not.
    CHECK_FALSE(SkillBar(SkillSet::Wizard).select(8));
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

TEST_CASE("the new warrior has her sword's combo, the skills wished for her and slots to fill", "[skills]") {
    using e5::gameplay::SkillId;
    e5::gameplay::SkillBar bar(e5::gameplay::SkillSet::Blade);

    CHECK(bar.slot(0) == SkillId::Slash);
    CHECK(bar.slot(1) == SkillId::CounterAttack);
    CHECK(bar.slot(2) == SkillId::BladeWhirl);
    CHECK(bar.slot(3) == SkillId::JumpAttack);
    CHECK(bar.slot(4) == SkillId::SeismicSlash);
    CHECK(bar.slot(5) == SkillId::Enrage);
    CHECK(bar.slot(6) == SkillId::NeverGiveUp);
    CHECK(bar.slot(7) == SkillId::Stampede);
    for (std::size_t slot = 8; slot < e5::gameplay::SkillBar::slot_count; ++slot) {
        CHECK(bar.slot(slot) == SkillId::None);
    }
    // An empty slot cannot be chosen.
    CHECK_FALSE(bar.select(8));
    CHECK(bar.selected() == SkillId::Slash);
    CHECK(bar.select(1));
    CHECK(e5::gameplay::open_slot_count(e5::gameplay::SkillSet::Blade) == 9);
}

TEST_CASE("the counter attack is a stance within the warrior's range", "[skills]") {
    using e5::gameplay::SkillId;
    CHECK(e5::gameplay::skill_is_stance(SkillId::CounterAttack));
    CHECK_FALSE(e5::gameplay::skill_is_stance(SkillId::Slash));
    CHECK(e5::gameplay::skill_info(SkillId::CounterAttack).kind == e5::gameplay::SkillKind::Instant);
    CHECK(e5::gameplay::skill_info(SkillId::CounterAttack).name == "Counter Attack");
    // One answer is less than a blow of her sword; with its wound, less than her finisher twice.
    const float answer = e5::gameplay::skill_damage(SkillId::CounterAttack);
    const float wound = e5::gameplay::counter_bleed_tick_damage * e5::gameplay::counter_bleed_seconds /
                        e5::gameplay::counter_bleed_tick_seconds;
    CHECK(answer > 0.0F);
    CHECK(answer < e5::gameplay::skill_damage(SkillId::Slash));
    CHECK(answer + wound < 2.0F * e5::gameplay::skill_damage(SkillId::Slash) * e5::gameplay::combo_damage_factor(2));
    // "Now and then": 8 to 12 seconds, and she does not stand ready all the time.
    CHECK(e5::gameplay::skill_cooldown_seconds(SkillId::CounterAttack) >= 8.0F);
    CHECK(e5::gameplay::skill_cooldown_seconds(SkillId::CounterAttack) <= 12.0F);
    CHECK(e5::gameplay::counter_seconds < e5::gameplay::skill_cooldown_seconds(SkillId::CounterAttack));
    CHECK_FALSE(e5::gameplay::skill_needs_charge(SkillId::CounterAttack));
}

TEST_CASE("the new warrior's whirlwind is a channel within her range", "[skills]") {
    using e5::gameplay::SkillId;
    CHECK(e5::gameplay::skill_is_channel(SkillId::BladeWhirl));
    CHECK_FALSE(e5::gameplay::skill_is_channel(SkillId::CounterAttack));
    CHECK_FALSE(e5::gameplay::skill_is_stance(SkillId::BladeWhirl));
    CHECK(e5::gameplay::skill_info(SkillId::BladeWhirl).name == "Whirlwind");
    CHECK(e5::gameplay::skill_info(SkillId::BladeWhirl).kind == e5::gameplay::SkillKind::Instant);
    // All of it on one enemy: more than her combo's finisher, less than the old warrior's star whirl.
    const float in_all = e5::gameplay::skill_damage(SkillId::BladeWhirl) * e5::gameplay::whirl_seconds /
                         e5::gameplay::whirl_tick_seconds;
    CHECK(in_all > e5::gameplay::skill_damage(SkillId::Slash) * e5::gameplay::combo_damage_factor(2));
    CHECK(in_all < e5::gameplay::skill_damage(SkillId::StarWhirl));
    // "Rarely": 20 to 30 seconds.
    CHECK(e5::gameplay::skill_cooldown_seconds(SkillId::BladeWhirl) >= 20.0F);
    CHECK(e5::gameplay::skill_cooldown_seconds(SkillId::BladeWhirl) <= 30.0F);
    CHECK(e5::gameplay::whirl_radius < e5::gameplay::whirl_pull_radius);
    // Its ticks are counted like a wound's: eight in four seconds.
    CHECK(e5::gameplay::step_bleed(
              e5::gameplay::open_wound({}, e5::gameplay::whirl_seconds, e5::gameplay::whirl_tick_seconds),
              e5::gameplay::whirl_tick_seconds, 60.0F)
              .ticks == 8);
}

TEST_CASE("the jump attack's leap lands where it is aimed", "[skills]") {
    using e5::gameplay::SkillId;
    CHECK(e5::gameplay::skill_info(SkillId::JumpAttack).name == "Jump Attack");
    CHECK(e5::gameplay::skill_cooldown_seconds(SkillId::JumpAttack) >= 8.0F);
    CHECK(e5::gameplay::skill_cooldown_seconds(SkillId::JumpAttack) <= 12.0F);
    // Harder than a blow of her sword, not harder than the old warrior's heaviest on a cooldown.
    CHECK(e5::gameplay::skill_damage(SkillId::JumpAttack) > e5::gameplay::skill_damage(SkillId::Slash));
    CHECK(e5::gameplay::skill_damage(SkillId::JumpAttack) <= e5::gameplay::skill_damage(SkillId::ThunderCleave));

    for (const float distance : {0.0F, 4.0F, 15.0F, 40.0F}) {
        const e5::gameplay::LeapArc arc = e5::gameplay::leap_arc(distance);
        const float way = std::min(distance, e5::gameplay::leap_max_distance);
        // Stepped as the game steps it: she is back at the height she left from when the time
        // is up, has gone the distance (never further than the longest leap), and was as high
        // as the apex in between.
        float height = 0.0F;
        float highest = 0.0F;
        float rise = arc.rise_speed;
        float gone = 0.0F;
        const float dt = 1.0F / 120.0F;
        for (float time = 0.0F; time < arc.seconds; time += dt) {
            rise -= arc.gravity * dt;
            height += rise * dt;
            gone += arc.forward_speed * dt;
            highest = std::max(highest, height);
        }
        CHECK(gone == Catch::Approx(way).margin(0.2));
        CHECK(height == Catch::Approx(0.0F).margin(0.45)); // (stepped coarsely)
        CHECK(highest == Catch::Approx(e5::gameplay::leap_apex_height(way)).margin(0.2));
        CHECK(arc.seconds <= 1.05F);
    }
    CHECK(e5::gameplay::leap_apex_height(15.0F) > e5::gameplay::leap_apex_height(2.0F));
}

TEST_CASE("the new warrior's enrage is a frenzy within her range", "[skills]") {
    using e5::gameplay::SkillId;
    CHECK(e5::gameplay::skill_info(SkillId::Enrage).name == "Enrage");
    CHECK(e5::gameplay::skill_info(SkillId::Enrage).kind == e5::gameplay::SkillKind::Instant);
    CHECK_FALSE(e5::gameplay::skill_is_stance(SkillId::Enrage));
    CHECK_FALSE(e5::gameplay::skill_is_channel(SkillId::Enrage));
    CHECK_FALSE(e5::gameplay::skill_needs_charge(SkillId::Enrage));
    // "Now and then": 8 to 12 seconds, and longer than the frenzy itself.
    CHECK(e5::gameplay::skill_cooldown_seconds(SkillId::Enrage) >= 8.0F);
    CHECK(e5::gameplay::skill_cooldown_seconds(SkillId::Enrage) <= 12.0F);
    CHECK(e5::gameplay::skill_cooldown_seconds(SkillId::Enrage) > e5::gameplay::enrage_seconds);
    // Faster blows, each weaker: what she deals in a second is more than with her sword alone,
    // but no more than twice that.
    const float pace = e5::gameplay::enrage_attack_speed * e5::gameplay::skill_damage(SkillId::Enrage) /
                       e5::gameplay::skill_damage(SkillId::Slash);
    CHECK(pace > 1.0F);
    CHECK(pace <= 2.0F);
    CHECK(e5::gameplay::enrage_move_share > 1.0F);
    CHECK(e5::gameplay::enrage_move_share <= 1.25F);
}

TEST_CASE("never give up is a stance that protects and heals within reason", "[skills]") {
    using e5::gameplay::SkillId;
    CHECK(e5::gameplay::skill_info(SkillId::NeverGiveUp).name == "Never Give Up");
    CHECK(e5::gameplay::skill_is_stance(SkillId::NeverGiveUp));
    CHECK_FALSE(e5::gameplay::skill_is_channel(SkillId::NeverGiveUp));
    CHECK(e5::gameplay::skill_damage(SkillId::NeverGiveUp) == 0.0F);
    // "Rarely": 20 to 30 seconds.
    CHECK(e5::gameplay::skill_cooldown_seconds(SkillId::NeverGiveUp) >= 20.0F);
    CHECK(e5::gameplay::skill_cooldown_seconds(SkillId::NeverGiveUp) <= 30.0F);
    // Less of a blow gets through, but not nothing; and all of it heals less than half of her.
    CHECK(e5::gameplay::resolve_damage_share < 1.0F);
    CHECK(e5::gameplay::resolve_damage_share >= 0.5F);
    CHECK(e5::gameplay::resolve_heal_share * e5::gameplay::resolve_seconds <= 0.5F);
}

TEST_CASE("the vine tower grows by how long it was charged", "[skills]") {
    using e5::gameplay::SkillId;
    using e5::gameplay::tower_height;
    CHECK(e5::gameplay::skill_info(SkillId::VineTower).name == "Vine Tower");
    CHECK(e5::gameplay::skill_info(SkillId::VineTower).kind == e5::gameplay::SkillKind::Instant);
    // The players' archer has it beside her shot; the bar kept for tests is as it was.
    CHECK(e5::gameplay::SkillBar(e5::gameplay::SkillSet::Archer).slot(1) == SkillId::VineTower);
    CHECK(e5::gameplay::SkillBar(e5::gameplay::SkillSet::Archer).slot(2) == SkillId::None);
    CHECK(e5::gameplay::SkillBar(e5::gameplay::SkillSet::ArcherFull).slot(1) == SkillId::PowerShot);
    // "Rarely"; let go too soon, sooner.
    CHECK(e5::gameplay::skill_cooldown_seconds(SkillId::VineTower) >= 20.0F);
    CHECK(e5::gameplay::skill_cooldown_seconds(SkillId::VineTower) <= 30.0F);
    CHECK(e5::gameplay::tower_cancel_cooldown_seconds < e5::gameplay::skill_cooldown_seconds(SkillId::VineTower));
    CHECK(tower_height(0.0F) == 0.0F);
    CHECK(tower_height(0.9F) == 0.0F);
    CHECK(tower_height(1.0F) == Catch::Approx(e5::gameplay::tower_lowest));
    CHECK(tower_height(3.0F) == Catch::Approx((e5::gameplay::tower_lowest + e5::gameplay::tower_tallest) * 0.5F));
    CHECK(tower_height(5.0F) == Catch::Approx(e5::gameplay::tower_tallest));
    CHECK(tower_height(60.0F) == Catch::Approx(e5::gameplay::tower_tallest));
}

TEST_CASE("the new warrior's sprintsz is a stance weaker than her whirlwind", "[skills]") {
    using e5::gameplay::SkillId;
    CHECK(e5::gameplay::skill_info(SkillId::Stampede).name == "Sprintsz");
    CHECK(e5::gameplay::skill_is_stance(SkillId::Stampede));
    CHECK(e5::gameplay::skill_cooldown_seconds(SkillId::Stampede) >= 20.0F);
    CHECK(e5::gameplay::skill_cooldown_seconds(SkillId::Stampede) <= 30.0F);
    // She can fight on while it lasts: less in all, and less far, than the whirl that holds her.
    const float in_all = e5::gameplay::skill_damage(SkillId::Stampede) * e5::gameplay::stampede_seconds /
                         e5::gameplay::stampede_tick_seconds;
    const float whirl = e5::gameplay::skill_damage(SkillId::BladeWhirl) * e5::gameplay::whirl_seconds /
                        e5::gameplay::whirl_tick_seconds;
    CHECK(in_all > 0.0F);
    CHECK(in_all < whirl);
    CHECK(e5::gameplay::stampede_radius <= e5::gameplay::whirl_pull_radius);
    CHECK(e5::gameplay::stampede_move_share > 1.0F);
    CHECK(e5::gameplay::stampede_move_share <= 1.3F);
}

TEST_CASE("the seismic slash catches what is in its wedge", "[skills]") {
    using e5::gameplay::in_wedge;
    using e5::gameplay::SkillId;
    CHECK(e5::gameplay::skill_info(SkillId::SeismicSlash).name == "Seismic Slash");
    CHECK(e5::gameplay::skill_damage(SkillId::SeismicSlash) < e5::gameplay::skill_damage(SkillId::Slash));
    CHECK(e5::gameplay::skill_cooldown_seconds(SkillId::SeismicSlash) >= 20.0F);
    CHECK(e5::gameplay::skill_cooldown_seconds(SkillId::SeismicSlash) <= 30.0F);

    const float length = e5::gameplay::seismic_length;
    const float half = e5::gameplay::seismic_half_angle;
    const float near = e5::gameplay::seismic_near;
    // She strikes along +Z.
    CHECK(in_wedge(0.0F, 8.0F, 0.0F, 1.0F, length, half, near));
    CHECK(in_wedge(0.0F, 15.0F, 0.0F, 2.0F, length, half, near));
    CHECK_FALSE(in_wedge(0.0F, 15.5F, 0.0F, 1.0F, length, half, near));
    CHECK_FALSE(in_wedge(0.0F, -8.0F, 0.0F, 1.0F, length, half, near));
    // 30 degrees to either side: at 10 m ahead that is 5.8 m aside.
    CHECK(in_wedge(5.5F, 10.0F, 0.0F, 1.0F, length, half, near));
    CHECK(in_wedge(-5.5F, 10.0F, 0.0F, 1.0F, length, half, near));
    CHECK_FALSE(in_wedge(6.5F, 10.0F, 0.0F, 1.0F, length, half, near));
    // A wide body reaches in from beside it.
    CHECK(in_wedge(6.5F, 10.0F, 0.0F, 1.0F, length, half, near, 1.0F));
    // Right at her feet the wedge is as wide as she is: beside her counts, behind her does not.
    CHECK(in_wedge(1.2F, 0.3F, 0.0F, 1.0F, length, half, near));
    CHECK_FALSE(in_wedge(0.5F, -1.0F, 0.0F, 1.0F, length, half, near));
    // Along another way.
    CHECK(in_wedge(7.0F, 0.5F, 1.0F, 0.0F, length, half, near));
    CHECK_FALSE(in_wedge(0.5F, 7.0F, 1.0F, 0.0F, length, half, near));
}

TEST_CASE("a wound bleeds once a tick until its time is up", "[skills]") {
    using e5::gameplay::BleedState;
    BleedState wound = e5::gameplay::open_wound({}, 3.0F, 1.0F);
    int ticks = 0;
    for (int frame = 0; frame < 60 * 5; ++frame) {
        const e5::gameplay::BleedStep step = e5::gameplay::step_bleed(wound, 1.0F, 1.0F / 60.0F);
        wound = step.state;
        ticks += step.ticks;
    }
    CHECK(ticks == 3);
    CHECK(wound.seconds_left == 0.0F);
    // Nothing more comes of a wound that has closed.
    CHECK(e5::gameplay::step_bleed(wound, 1.0F, 10.0F).ticks == 0);

    // One long step: every tick that fell due in it, and none after the end.
    CHECK(e5::gameplay::step_bleed(e5::gameplay::open_wound({}, 3.0F, 1.0F), 1.0F, 60.0F).ticks == 3);

    // Struck again while it bleeds: the time starts anew, the beat goes on.
    BleedState again = e5::gameplay::step_bleed(e5::gameplay::open_wound({}, 3.0F, 1.0F), 1.0F, 0.6F).state;
    again = e5::gameplay::open_wound(again, 3.0F, 1.0F);
    CHECK(again.seconds_left == 3.0F);
    CHECK(again.until_tick < 0.5F);
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

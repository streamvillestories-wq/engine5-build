#include "e5/gameplay/skills.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace e5::gameplay {

SkillInfo skill_info(SkillId skill) noexcept {
    switch (skill) {
    case SkillId::Shot:
        return {.name = "Shot", .charges = false, .kind = SkillKind::Bow};
    case SkillId::PowerShot:
        return {.name = "Power Shot", .charges = true, .kind = SkillKind::Bow};
    case SkillId::ArrowRain:
        return {.name = "Arrow Rain", .charges = false, .kind = SkillKind::Bow};
    case SkillId::FrostFan:
        return {.name = "Frost Fan", .charges = false, .kind = SkillKind::Bow};
    case SkillId::FireArrow:
        return {.name = "Fire Arrow", .charges = false, .kind = SkillKind::Bow};
    case SkillId::ThunderKick:
        return {.name = "Thunder Kick", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Kingfishers:
        return {.name = "Kingfishers", .charges = false, .kind = SkillKind::Instant};
    case SkillId::ArcaneBolt:
        return {.name = "Arcane Bolt", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Fireball:
        return {.name = "Fireball", .charges = false, .kind = SkillKind::Instant};
    case SkillId::FrostNova:
        return {.name = "Frost Nova", .charges = false, .kind = SkillKind::Instant};
    case SkillId::ChainLightning:
        return {.name = "Chain Lightning", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Meteor:
        return {.name = "Meteor", .charges = false, .kind = SkillKind::Instant};
    case SkillId::StarBarrage:
        return {.name = "Star Barrage", .charges = false, .kind = SkillKind::Instant};
    case SkillId::BlackHole:
        return {.name = "Black Hole", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Slash:
        return {.name = "Sword Combo", .charges = false, .kind = SkillKind::Instant};
    case SkillId::FlameBlade:
        return {.name = "Flame Blade", .charges = false, .kind = SkillKind::Instant};
    case SkillId::FrostEdge:
        return {.name = "Frost Edge", .charges = false, .kind = SkillKind::Instant};
    case SkillId::ThunderCleave:
        return {.name = "Thunder Cleave", .charges = false, .kind = SkillKind::Instant};
    case SkillId::StarWhirl:
        return {.name = "Star Whirl", .charges = false, .kind = SkillKind::Instant};
    case SkillId::AxeCombo:
        return {.name = "Axe Combo", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Whirlwind:
        return {.name = "Whirlwind", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Earthbreaker:
        return {.name = "Earthbreaker", .charges = false, .kind = SkillKind::Instant};
    case SkillId::LeapStrike:
        return {.name = "Leap Strike", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Battlecry:
        return {.name = "Battle Cry", .charges = false, .kind = SkillKind::Instant};
    case SkillId::None:
        break;
    }
    return {.name = "", .charges = false, .kind = SkillKind::Bow};
}

float skill_damage(SkillId skill, float power) noexcept {
    switch (skill) {
    case SkillId::Shot:
        return 20.0F;
    case SkillId::PowerShot:
        // A full charge triples the arrow.
        return 20.0F * (1.0F + 2.0F * std::clamp(power, 0.0F, 1.0F));
    case SkillId::ArrowRain:
        return 12.0F;
    case SkillId::FrostFan:
        return 15.0F;
    case SkillId::FireArrow:
        return 45.0F;
    case SkillId::ThunderKick:
        return 30.0F;
    case SkillId::Kingfishers:
        return 6.0F;
    case SkillId::ArcaneBolt:
        return 22.0F;
    case SkillId::Fireball:
        return 50.0F;
    case SkillId::FrostNova:
        return 35.0F;
    case SkillId::ChainLightning:
        // What the bolt strikes first; each jump after that carries a little less (see lightning_jump_share).
        return 30.0F;
    case SkillId::Meteor:
        return 80.0F;
    case SkillId::StarBarrage:
        return 12.0F; // per star
    case SkillId::BlackHole:
        return 30.0F; // when it spits them out; while inside they take a little every second
    case SkillId::Slash:
        return 22.0F; // the first blow of the combo; see combo_damage_factor
    case SkillId::FlameBlade:
        return 46.0F;
    case SkillId::FrostEdge:
        return 36.0F;
    case SkillId::ThunderCleave:
        return 62.0F;
    case SkillId::StarWhirl:
        return 40.0F;
    case SkillId::AxeCombo:
        return 26.0F; // the first blow of the combo; see combo_damage_factor
    case SkillId::Whirlwind:
        return 40.0F;
    case SkillId::Earthbreaker:
        return 64.0F;
    case SkillId::LeapStrike:
        return 52.0F;
    case SkillId::Battlecry:
        return 10.0F; // and it throws everything around him back
    case SkillId::None:
        break;
    }
    return 0.0F;
}

float combo_damage_factor(int step) noexcept {
    constexpr std::array<float, combo_length> factors{1.0F, 1.15F, 1.7F};
    return factors.at(static_cast<std::size_t>(std::clamp(step, 0, combo_length - 1)));
}

int next_combo_step(int last_step, float seconds_since_last, float window_seconds) noexcept {
    if (last_step < 0 || last_step >= combo_length - 1 || seconds_since_last > window_seconds) {
        return 0;
    }
    return last_step + 1;
}

SkillBar::SkillBar(SkillSet set) noexcept {
    if (set == SkillSet::Dwarf) {
        slots_ = {SkillId::AxeCombo, SkillId::Whirlwind, SkillId::Earthbreaker, SkillId::LeapStrike,
                  SkillId::Battlecry};
    } else if (set == SkillSet::Warrior) {
        slots_ = {SkillId::Slash, SkillId::FlameBlade, SkillId::FrostEdge, SkillId::ThunderCleave, SkillId::StarWhirl};
    } else if (set == SkillSet::Wizard) {
        slots_ = {SkillId::ArcaneBolt, SkillId::Fireball,    SkillId::FrostNova, SkillId::ChainLightning,
                  SkillId::Meteor,     SkillId::StarBarrage, SkillId::BlackHole};
    } else {
        slots_ = {SkillId::Shot,      SkillId::PowerShot,   SkillId::ArrowRain,  SkillId::FrostFan,
                  SkillId::FireArrow, SkillId::ThunderKick, SkillId::Kingfishers};
    }
}

SkillId SkillBar::slot(std::size_t index) const noexcept {
    return index < slot_count ? slots_[index] : SkillId::None;
}

bool SkillBar::select(std::size_t index) noexcept {
    if (index >= slot_count || slots_[index] == SkillId::None || index == selected_) {
        return false;
    }
    selected_ = index;
    return true;
}

Vec3 rain_arrow_offset(int index, int count, float radius) noexcept {
    if (count <= 0 || index < 0 || index >= count) {
        return {};
    }
    // Golden-angle spiral: the square root spreads the points evenly by area.
    constexpr float golden_angle = std::numbers::pi_v<float> * (3.0F - 2.236068F);
    const float distance = radius * std::sqrt((static_cast<float>(index) + 0.5F) / static_cast<float>(count));
    const float angle = golden_angle * static_cast<float>(index);
    return {.x = distance * std::cos(angle), .y = 0.0F, .z = distance * std::sin(angle)};
}

float fan_yaw_offset(int index, int count, float spread_radians) noexcept {
    if (count <= 1 || index < 0 || index >= count) {
        return 0.0F;
    }
    const float t = static_cast<float>(index) / static_cast<float>(count - 1); // 0..1 across the fan
    return (t - 0.5F) * spread_radians;
}

float distance_to_disc(const Vec3& local_point, float disc_radius) noexcept {
    const float beyond_rim = std::max(std::hypot(local_point.x, local_point.y) - disc_radius, 0.0F);
    return std::hypot(beyond_rim, local_point.z);
}

ActionStep step_action(const ActionState& state, bool start_requested, const ActionTimings& timings,
                       float delta_seconds) noexcept {
    ActionStep step{.state = state};
    if (!state.active) {
        if (start_requested) {
            step.state = {.active = true, .elapsed = 0.0F};
            step.started = true;
        }
        return step;
    }
    const float before = state.elapsed;
    step.state.elapsed = before + delta_seconds;
    step.strike = before < timings.strike_at_seconds && step.state.elapsed >= timings.strike_at_seconds;
    if (step.state.elapsed >= timings.duration_seconds) {
        // A blow timed at or after the end still lands, on the last step.
        step.strike = step.strike || before < timings.strike_at_seconds;
        step.state = {};
    }
    return step;
}

} // namespace e5::gameplay

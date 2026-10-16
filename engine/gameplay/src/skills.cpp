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
    case SkillId::SoulBolt:
        return {.name = "Soul Bolt", .charges = false, .kind = SkillKind::Instant};
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
    case SkillId::VenomArrow:
        return {.name = "Venom Arrow", .charges = false, .kind = SkillKind::Bow};
    case SkillId::GaleArrow:
        return {.name = "Gale Arrow", .charges = false, .kind = SkillKind::Bow};
    case SkillId::StormArrow:
        return {.name = "Storm Arrow", .charges = false, .kind = SkillKind::Bow};
    case SkillId::BrambleArrow:
        return {.name = "Bramble Arrow", .charges = false, .kind = SkillKind::Bow};
    case SkillId::DaggerCombo:
        return {.name = "Dagger Combo", .charges = false, .kind = SkillKind::Instant};
    case SkillId::CounterAttack:
        return {.name = "Counter Attack", .charges = false, .kind = SkillKind::Instant};
    case SkillId::BladeWhirl:
        return {.name = "Whirlwind", .charges = false, .kind = SkillKind::Instant};
    case SkillId::JumpAttack:
        return {.name = "Jump Attack", .charges = false, .kind = SkillKind::Instant};
    case SkillId::SeismicSlash:
        return {.name = "Seismic Slash", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Enrage:
        return {.name = "Enrage", .charges = false, .kind = SkillKind::Instant};
    case SkillId::NeverGiveUp:
        return {.name = "Never Give Up", .charges = false, .kind = SkillKind::Instant};
    case SkillId::VineTower:
        return {.name = "Vine Tower", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Stampede:
        return {.name = "Sprintsz", .charges = false, .kind = SkillKind::Instant};
    case SkillId::CutInPieces:
        return {.name = "Cut in Pieces", .charges = false, .kind = SkillKind::Instant};
    case SkillId::GatlingGun:
        return {.name = "Gatling Arrow Gun", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Disguise:
        return {.name = "Disguise", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Firebolt:
        return {.name = "Firebolt", .charges = false, .kind = SkillKind::Instant};
    case SkillId::FrostArmor:
        return {.name = "Frost Armor", .charges = false, .kind = SkillKind::Instant};
    case SkillId::ChainBolt:
        return {.name = "Chain Lightning", .charges = false, .kind = SkillKind::Instant};
    case SkillId::PoisonHydra:
        return {.name = "Poison Hydra", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Teleport:
        return {.name = "Teleportation", .charges = false, .kind = SkillKind::Instant};
    case SkillId::RovingHole:
        return {.name = "Black Whole", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Blizzard:
        return {.name = "Blizzard", .charges = false, .kind = SkillKind::Instant};
    case SkillId::BearTrap:
        return {.name = "Bear Trap", .charges = false, .kind = SkillKind::Instant};
    case SkillId::ProtectionDome:
        return {.name = "Dome of Protection", .charges = false, .kind = SkillKind::Instant};
    case SkillId::MoltenFissure:
        return {.name = "Molten Fissure", .charges = false, .kind = SkillKind::Instant};
    case SkillId::RunicBulwark:
        return {.name = "Runic Bulwark", .charges = false, .kind = SkillKind::Instant};
    case SkillId::AncestralAnvil:
        return {.name = "Ancestral Anvil", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Stonebreaker:
        return {.name = "Stonebreaker", .charges = false, .kind = SkillKind::Instant};
    case SkillId::ShadowStep:
        return {.name = "Shadow Step", .charges = false, .kind = SkillKind::Instant};
    case SkillId::FanOfKnives:
        return {.name = "Fan of Knives", .charges = false, .kind = SkillKind::Instant};
    case SkillId::VeilOfShadows:
        return {.name = "Veil of Shadows", .charges = false, .kind = SkillKind::Instant};
    case SkillId::VenomEdge:
        return {.name = "Venom Edge", .charges = false, .kind = SkillKind::Instant};
    case SkillId::ShadowFlurry:
        return {.name = "Shadow Flurry", .charges = false, .kind = SkillKind::Instant};
    case SkillId::MirrorImage:
        return {.name = "Whose Ur Mommy?", .charges = false, .kind = SkillKind::Instant};
    case SkillId::None:
        break;
    }
    return {.name = "", .charges = false, .kind = SkillKind::Bow};
}

// Balanced on 2026-10-05 from a simulation of every hero's damage (docs/ASSET_PIPELINE.md, "The
// balance of the heroes"): against one enemy the ranged heroes do about 32 to 35 a second with
// their skills and the two who must stand in the fight 38 to 44.
float skill_damage(SkillId skill, float power) noexcept {
    switch (skill) {
    case SkillId::Shot:
        return 28.0F;
    case SkillId::PowerShot:
        // A full charge triples the arrow.
        return 28.0F * (1.0F + 2.0F * std::clamp(power, 0.0F, 1.0F));
    case SkillId::ArrowRain:
        return 12.0F;
    case SkillId::FrostFan:
        return 24.0F; // each of its five arrows
    case SkillId::FireArrow:
        return 45.0F;
    case SkillId::ThunderKick:
        return 30.0F;
    case SkillId::Kingfishers:
        return 6.0F;
    case SkillId::ArcaneBolt:
    case SkillId::SoulBolt: // his standard attack, as the wizard's is
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
        return 85.0F; // it takes a full charge: it has to be worth one
    case SkillId::AxeCombo:
        return 26.0F; // the first blow of the combo; see combo_damage_factor
    case SkillId::Whirlwind:
        return 56.0F;
    case SkillId::Earthbreaker:
        return 64.0F;
    case SkillId::LeapStrike:
        return 82.0F;
    case SkillId::Battlecry:
        return 45.0F; // and it throws everything around him back
    case SkillId::MoltenFissure:
        return 30.0F; // to each it catches; the stun and the burning (burn_tick_damage) do the rest
    case SkillId::AncestralAnvil:
        return 9.0F; // every pulse, to each within its reach: 72 over its eight seconds
    case SkillId::Stonebreaker:
        return 48.0F; // to each in its burst
    case SkillId::ShadowStep:
        return 44.0F; // the stab in the back: twice a dagger blow, for a skill that must find its one
    case SkillId::FanOfKnives:
        return 16.0F; // each knife: one in the face is less than a dagger blow, five into a crowd are more
    case SkillId::ShadowFlurry:
        return 13.0F; // each of its fourteen blows in three and a half seconds: 182 in all
    // The archer's arrows to choose from: not yet weighed against the others (the user picks
    // among them first).
    case SkillId::VenomArrow:
        return 14.0F; // the arrow; its cloud does the rest (venom_tick_damage)
    case SkillId::GaleArrow:
        return 38.0F; // to each one it goes through
    case SkillId::StormArrow:
        return 32.0F; // what it strikes; see storm_jump_share
    case SkillId::BrambleArrow:
        return 18.0F; // to everything it catches
    case SkillId::DaggerCombo:
        return 20.0F; // the first blow; see combo_damage_factor. Less than an arrow, but at once
    case SkillId::CounterAttack:
        return 18.0F; // each answer: four fifths of a sword blow; its wound adds counter_bleed_tick_damage
    case SkillId::BladeWhirl:
        return 12.0F; // every half second to all it reaches: 96 over its four seconds (8 at first; bug report 30)
    case SkillId::JumpAttack:
        return 33.0F; // one and a half sword blows, to everything where she lands
    case SkillId::SeismicSlash:
        return 11.0F; // half a sword blow to each: what it is for is the stun
    case SkillId::Enrage:
        return 11.0F; // the first of its blows, half a sword blow; see combo_damage_factor and enrage_attack_speed
    case SkillId::NeverGiveUp:
    case SkillId::VineTower:
    case SkillId::Disguise:
    case SkillId::FrostArmor:
    case SkillId::Teleport:
    case SkillId::MirrorImage: // (his copy's spells are his own, at clone_damage_share)
    case SkillId::ProtectionDome:
    case SkillId::RunicBulwark:
    case SkillId::VeilOfShadows: // (its ambush doubles the blow that ends it)
    case SkillId::VenomEdge:     // (its poison: venom_edge_tick_damage)
        return 0.0F;             // they strike nobody
    case SkillId::Stampede:
        return 5.0F; // every half second to all it reaches: 40 over its four seconds
    case SkillId::CutInPieces:
        return 12.0F; // each of its twenty blows in five seconds (ticket 57; 8.8 for twenty-four in six before)
    case SkillId::GatlingGun:
        return 8.0F; // each arrow, four a second: 32 a second while she stands at it and holds the button
    case SkillId::BearTrap:
        return 60.0F; // to the one that steps in: more than a fire arrow, less than a fully charged power shot
    case SkillId::Blizzard:
        return 4.4F; // every half second to each enemy in it: two fifths of his bolt a second, more the longer it stays
    case SkillId::RovingHole:
        return 5.5F; // every half second to all near it: half his bolt a second, 66 to what stays with it to the end
    case SkillId::PoisonHydra:
        return 13.2F; // each spit: six tenths of his bolt; its poison adds hydra_poison_damage over five seconds
    case SkillId::ChainBolt:
        // The first strike, less than his bolt: it is for a crowd. Each leap a fifth more, so five
        // enemies in a row take 119 between them; one alone takes this.
        return 16.0F;
    case SkillId::Firebolt:
        // To everything its burst reaches. The full charge doubles it: then it is the first wizard's
        // meteor, which is ready as often and needs no four seconds of standing still.
        return 40.0F * (1.0F + std::clamp(power, 0.0F, 1.0F));
    case SkillId::None:
        break;
    }
    return 0.0F;
}

float combo_damage_factor(int step) noexcept {
    constexpr std::array<float, combo_length> factors{1.0F, 1.15F, 1.7F};
    return factors.at(static_cast<std::size_t>(std::clamp(step, 0, combo_length - 1)));
}

namespace {

struct ComboAdvance {
    float distance; // metres
    float from;     // seconds into the blow
    float to;
};
// As "step" in tools/godot/warrior_combo.gd: change both together.
constexpr std::array<ComboAdvance, combo_length> combo_advances{
    ComboAdvance{.distance = 0.6F, .from = 0.16F, .to = 0.36F},
    ComboAdvance{.distance = 0.6F, .from = 0.1F, .to = 0.46F},
    ComboAdvance{.distance = 1.4F, .from = 0.22F, .to = 0.5F}};

// The dwarf's: "step" of DWARF_BLOWS in the same tool. In the warrior's measures, as the tool
// has them; the game carries him the share of that by which his legs are shorter.
constexpr std::array<ComboAdvance, combo_length> axe_advances{
    ComboAdvance{.distance = 0.5F, .from = 0.24F, .to = 0.44F},
    ComboAdvance{.distance = 0.35F, .from = 0.14F, .to = 0.34F},
    ComboAdvance{.distance = 0.6F, .from = 0.18F, .to = 0.38F}};

const ComboAdvance& advance_of(int step, bool axe) noexcept {
    const auto index = static_cast<std::size_t>(std::clamp(step, 0, combo_length - 1));
    return axe ? axe_advances.at(index) : combo_advances.at(index);
}

} // namespace

float combo_advance_distance(int step, bool axe) noexcept {
    return advance_of(step, axe).distance;
}

float combo_advance_speed(int step, float seconds, bool axe) noexcept {
    const ComboAdvance& advance = advance_of(step, axe);
    const float u = (seconds - advance.from) / (advance.to - advance.from);
    if (u <= 0.0F || u >= 1.0F) {
        return 0.0F;
    }
    // The way goes as a smooth step (3u^2 - 2u^3); this is how fast that is.
    return advance.distance * 6.0F * u * (1.0F - u) / (advance.to - advance.from);
}

float skill_cooldown_seconds(SkillId skill) noexcept {
    switch (skill) {
    // Quick ones.
    case SkillId::PowerShot:
        return 4.0F;
    case SkillId::ThunderKick:
    case SkillId::Fireball:
    case SkillId::FlameBlade:
    case SkillId::ChainBolt:
        return 5.0F;
    // In between.
    case SkillId::FireArrow:
    case SkillId::FrostFan:
    case SkillId::ChainLightning:
    case SkillId::Whirlwind:
    case SkillId::GaleArrow:
    case SkillId::Teleport: // (10 until ticket 53)
        return 6.0F;
    case SkillId::VenomArrow:
        return 7.0F;
    case SkillId::FrostEdge:
    case SkillId::LeapStrike:
        return 7.0F;
    case SkillId::FrostNova:
    case SkillId::Earthbreaker:
    case SkillId::StormArrow:
        return 8.0F;
    // The ones that clear a place.
    case SkillId::ArrowRain:
    case SkillId::StarBarrage:
    case SkillId::ThunderCleave:
        return 9.0F;
    case SkillId::Meteor:
    case SkillId::BrambleArrow:
    case SkillId::CounterAttack: // from the key: she stands ready for half of it
    case SkillId::Enrage:        // from the key too: the frenzy takes four of them
    case SkillId::Firebolt:      // from the throw
    case SkillId::BearTrap:      // from when it lies armed
    case SkillId::BladeWhirl:    // from the key: it lasts four of them (20 until ticket 59, which asked for 9)
        return 10.0F;
    case SkillId::JumpAttack:
    case SkillId::FrostArmor: // from the key: it lasts five of them
    case SkillId::RovingHole: // from the cast: it drifts for six of them
        return 12.0F;
    // Rarely.
    case SkillId::VineTower:
    case SkillId::GatlingGun: // from when it stands: ready again as it falls apart
    case SkillId::Disguise:   // from when it ends
    case SkillId::PoisonHydra:
    case SkillId::Blizzard:
        return 30.0F;
    case SkillId::Stampede:
    case SkillId::NeverGiveUp:
    case SkillId::SeismicSlash:
        return 20.0F;
    case SkillId::ProtectionDome:
        return 28.0F; // from the key: it stands six of them
    case SkillId::Stonebreaker:
        return 9.0F;
    case SkillId::MoltenFissure:
        return 12.0F;
    case SkillId::RunicBulwark:
        return 22.0F; // from the key: it lasts six of them
    case SkillId::AncestralAnvil:
        return 26.0F; // from the blow: it stands eight of them
    case SkillId::ShadowStep:
        return 8.0F;
    case SkillId::FanOfKnives:
        return 7.0F;
    case SkillId::VeilOfShadows:
        return 20.0F; // from the key: it lasts five of them
    case SkillId::VenomEdge:
        return 18.0F; // from the key: it lasts eight of them
    // The standard attacks, and the skills that take a charge instead.
    case SkillId::None:
    case SkillId::Shot:
    case SkillId::ArcaneBolt:
    case SkillId::SoulBolt:
    case SkillId::Slash:
    case SkillId::AxeCombo:
    case SkillId::DaggerCombo:
    case SkillId::Kingfishers:
    case SkillId::BlackHole:
    case SkillId::StarWhirl:
    case SkillId::Battlecry:
    case SkillId::CutInPieces:
    case SkillId::MirrorImage:
    case SkillId::ShadowFlurry:
        return 0.0F;
    }
    return 0.0F;
}

bool skill_needs_charge(SkillId skill) noexcept {
    return skill == SkillId::Kingfishers || skill == SkillId::BlackHole || skill == SkillId::StarWhirl ||
           skill == SkillId::Battlecry || skill == SkillId::CutInPieces || skill == SkillId::MirrorImage ||
           skill == SkillId::ShadowFlurry;
}

float firebolt_power(float charged_seconds) noexcept {
    return std::clamp((charged_seconds - firebolt_least_charge_seconds) /
                          (firebolt_full_charge_seconds - firebolt_least_charge_seconds),
                      0.0F, 1.0F);
}

float firebolt_radius(float power) noexcept {
    return firebolt_least_radius + (firebolt_full_radius - firebolt_least_radius) * std::clamp(power, 0.0F, 1.0F);
}

float charge_after(float charge, float damage_dealt, bool killed) noexcept {
    const float gained = std::max(damage_dealt, 0.0F) * charge_per_damage + (killed ? charge_per_kill : 0.0F);
    return std::clamp(charge + gained, 0.0F, 1.0F);
}

bool in_wedge(float dx, float dz, float ahead_x, float ahead_z, float length, float half_angle, float near,
              float allowance) noexcept {
    const float distance = std::hypot(dx, dz);
    const float ahead = std::hypot(ahead_x, ahead_z);
    if (distance > length + allowance || ahead <= 0.0F) {
        return false;
    }
    if (distance <= allowance) {
        return true; // where she stands
    }
    const float along = (dx * ahead_x + dz * ahead_z) / ahead; // how far ahead of her it is
    if (distance <= near + allowance) {
        return along >= 0.0F;
    }
    // Its body may reach into the wedge from beside it.
    const float aside = std::sqrt(std::max(distance * distance - along * along, 0.0F));
    return along > 0.0F && aside - allowance <= along * std::tan(half_angle);
}

GatlingStep step_gatling(float until_next, bool held, float dt) noexcept {
    GatlingStep step{.until_next = std::max(until_next, 0.0F) - std::max(dt, 0.0F)};
    if (!held) {
        step.until_next = std::max(step.until_next, 0.0F);
        return step;
    }
    constexpr float between = 1.0F / gatling_arrows_per_second;
    while (step.until_next <= 0.0F) {
        ++step.arrows;
        step.until_next += between;
    }
    return step;
}

float tower_height(float charged_seconds) noexcept {
    if (charged_seconds < tower_least_charge_seconds) {
        return 0.0F;
    }
    const float share = std::clamp((charged_seconds - tower_least_charge_seconds) /
                                       (tower_full_charge_seconds - tower_least_charge_seconds),
                                   0.0F, 1.0F);
    return tower_lowest + (tower_tallest - tower_lowest) * share;
}

float leap_apex_height(float distance) noexcept {
    // A hop on the spot is still a jump; a long leap goes high.
    return 1.5F + 0.15F * std::clamp(distance, 0.0F, leap_max_distance);
}

LeapArc leap_arc(float distance) noexcept {
    const float way = std::clamp(distance, 0.0F, leap_max_distance);
    LeapArc arc;
    arc.seconds = 0.55F + 0.03F * way;
    arc.forward_speed = way / arc.seconds;
    // Up to the apex in half the time and down again in the other half.
    arc.gravity = 8.0F * leap_apex_height(way) / (arc.seconds * arc.seconds);
    arc.rise_speed = arc.gravity * arc.seconds * 0.5F;
    return arc;
}

BleedState open_wound(const BleedState& state, float seconds, float tick_seconds) noexcept {
    // One that bleeds already keeps its beat; a fresh one hurts first after a whole tick.
    return {.seconds_left = seconds, .until_tick = state.seconds_left > 0.0F ? state.until_tick : tick_seconds};
}

BleedStep step_bleed(const BleedState& state, float tick_seconds, float dt) noexcept {
    BleedStep step{.state = state};
    if (state.seconds_left <= 0.0F || tick_seconds <= 0.0F || dt <= 0.0F) {
        return step;
    }
    // Only the time the wound still has counts: a long step does not bleed past its end.
    float passed = std::min(dt, state.seconds_left);
    step.state.seconds_left = state.seconds_left - passed;
    // (A hair of allowance: the last tick falls due exactly as the wound closes.)
    constexpr float allowance = 1e-4F;
    while (passed + allowance >= step.state.until_tick) {
        passed = std::max(passed - step.state.until_tick, 0.0F);
        step.state.until_tick = tick_seconds;
        ++step.ticks;
    }
    step.state.until_tick -= passed;
    return step;
}

BlockStep step_block(const BlockState& state, bool pressed, bool held, bool able, const BlockParams& params,
                     float dt) noexcept {
    BlockStep step{.state = state};
    if (step.state.raised) {
        step.state.held_seconds += dt;
        if (!held || !able || step.state.held_seconds >= params.max_hold_seconds) {
            step.state.raised = false;
            step.state.held_seconds = 0.0F;
            step.state.cooldown_left = params.cooldown_seconds;
            step.lowered_now = true;
        }
        return step;
    }
    step.state.cooldown_left = std::max(step.state.cooldown_left - dt, 0.0F);
    if (pressed && able && step.state.cooldown_left <= 0.0F) {
        step.state.raised = true;
        step.state.held_seconds = 0.0F;
        step.raised_now = true;
    }
    return step;
}

bool shield_covers(float facing_yaw, float dx, float dz) noexcept {
    // In front or beside, not behind. Something exactly where she stands has no side: covered.
    return std::sin(facing_yaw) * dx + std::cos(facing_yaw) * dz >= 0.0F;
}

int next_combo_step(int last_step, float seconds_since_last, float window_seconds) noexcept {
    if (last_step < 0 || last_step >= combo_length - 1 || seconds_since_last > window_seconds) {
        return 0;
    }
    return last_step + 1;
}

SkillBar::SkillBar(SkillSet set) noexcept {
    if (set == SkillSet::Dwarf) {
        slots_ = {SkillId::AxeCombo,     SkillId::Whirlwind,      SkillId::Earthbreaker,
                  SkillId::LeapStrike,   SkillId::Battlecry,      SkillId::MoltenFissure,
                  SkillId::RunicBulwark, SkillId::AncestralAnvil, SkillId::Stonebreaker};
    } else if (set == SkillSet::Blade) {
        slots_ = {SkillId::Slash,       SkillId::CounterAttack, SkillId::BladeWhirl,
                  SkillId::JumpAttack,  SkillId::SeismicSlash,  SkillId::Enrage,
                  SkillId::NeverGiveUp, SkillId::Stampede,      SkillId::CutInPieces};
    } else if (set == SkillSet::Archer) {
        // (The Gatling Arrow Gun was wished for key 6: the slots before it are still to be filled.)
        slots_ = {SkillId::Shot,     SkillId::VineTower, SkillId::Disguise,
                  SkillId::BearTrap, SkillId::None,      SkillId::GatlingGun};
    } else if (set == SkillSet::Warrior) {
        slots_ = {SkillId::Slash, SkillId::FlameBlade, SkillId::FrostEdge, SkillId::ThunderCleave, SkillId::StarWhirl};
    } else if (set == SkillSet::Mage) {
        slots_ = {SkillId::ArcaneBolt,  SkillId::Firebolt,      SkillId::FrostArmor, SkillId::ChainBolt,
                  SkillId::PoisonHydra, SkillId::Teleport,      SkillId::RovingHole, SkillId::Blizzard,
                  SkillId::MirrorImage, SkillId::ProtectionDome};
    } else if (set == SkillSet::Bone) {
        slots_ = {SkillId::SoulBolt};
    } else if (set == SkillSet::Shadow) {
        slots_ = {SkillId::DaggerCombo,   SkillId::ShadowStep, SkillId::FanOfKnives,
                  SkillId::VeilOfShadows, SkillId::VenomEdge,  SkillId::ShadowFlurry};
    } else if (set == SkillSet::Wizard) {
        slots_ = {SkillId::ArcaneBolt, SkillId::Fireball,    SkillId::FrostNova, SkillId::ChainLightning,
                  SkillId::Meteor,     SkillId::StarBarrage, SkillId::BlackHole};
    } else {
        slots_ = {SkillId::Shot,      SkillId::PowerShot,   SkillId::ArrowRain,    SkillId::FrostFan,
                  SkillId::FireArrow, SkillId::ThunderKick, SkillId::Kingfishers,  SkillId::VenomArrow,
                  SkillId::GaleArrow, SkillId::StormArrow,  SkillId::BrambleArrow, SkillId::DaggerCombo};
    }
}

SkillId SkillBar::slot(std::size_t index) const noexcept {
    return index < slot_count ? slots_[index] : SkillId::None;
}

std::size_t SkillBar::find(SkillId skill) const noexcept {
    if (skill == SkillId::None) {
        return slot_count;
    }
    const auto found = std::ranges::find(slots_, skill);
    return static_cast<std::size_t>(found - slots_.begin());
}

bool SkillBar::assign(std::size_t index, SkillId skill) noexcept {
    if (index == 0 || index >= slot_count || skill == SkillId::None || skill == slots_[0]) {
        return false;
    }
    const std::size_t was_at = find(skill);
    if (was_at == index) {
        return true;
    }
    if (was_at < slot_count) {
        slots_[was_at] = slots_[index];
    }
    slots_[index] = skill;
    if (slots_[selected_] == SkillId::None) {
        selected_ = 0; // the selection always names a usable skill
    }
    return true;
}

bool SkillBar::clear(std::size_t index) noexcept {
    if (index == 0 || index >= slot_count || slots_[index] == SkillId::None) {
        return false;
    }
    slots_[index] = SkillId::None;
    if (selected_ == index) {
        selected_ = 0;
    }
    return true;
}

std::span<const SkillId> skill_catalogue(SkillSet set) noexcept {
    // The Archer's: what was made from wishes, then the skills she had before (kept "in the
    // back" since 2026-10-10 and offered again in the book).
    static constexpr std::array archer{
        SkillId::Shot,      SkillId::VineTower,   SkillId::Disguise,     SkillId::GatlingGun,
        SkillId::BearTrap,  SkillId::PowerShot,   SkillId::ArrowRain,    SkillId::FrostFan,
        SkillId::FireArrow, SkillId::ThunderKick, SkillId::Kingfishers,  SkillId::VenomArrow,
        SkillId::GaleArrow, SkillId::StormArrow,  SkillId::BrambleArrow, SkillId::DaggerCombo,
    };
    static constexpr std::array blade{
        SkillId::Slash,  SkillId::CounterAttack, SkillId::BladeWhirl, SkillId::JumpAttack,  SkillId::SeismicSlash,
        SkillId::Enrage, SkillId::NeverGiveUp,   SkillId::Stampede,   SkillId::CutInPieces,
    };
    static constexpr std::array warrior{SkillId::Slash, SkillId::FlameBlade, SkillId::FrostEdge, SkillId::ThunderCleave,
                                        SkillId::StarWhirl};
    static constexpr std::array dwarf{SkillId::AxeCombo,     SkillId::Whirlwind,      SkillId::Earthbreaker,
                                      SkillId::LeapStrike,   SkillId::Battlecry,      SkillId::MoltenFissure,
                                      SkillId::RunicBulwark, SkillId::AncestralAnvil, SkillId::Stonebreaker};
    static constexpr std::array wizard{SkillId::ArcaneBolt,     SkillId::Fireball, SkillId::FrostNova,
                                       SkillId::ChainLightning, SkillId::Meteor,   SkillId::StarBarrage,
                                       SkillId::BlackHole};
    static constexpr std::array mage{
        SkillId::ArcaneBolt, SkillId::Firebolt,   SkillId::FrostArmor, SkillId::ChainBolt,   SkillId::PoisonHydra,
        SkillId::Teleport,   SkillId::RovingHole, SkillId::Blizzard,   SkillId::MirrorImage, SkillId::ProtectionDome};
    static constexpr std::array shadow{SkillId::DaggerCombo,   SkillId::ShadowStep, SkillId::FanOfKnives,
                                       SkillId::VeilOfShadows, SkillId::VenomEdge,  SkillId::ShadowFlurry};
    static constexpr std::array bone{SkillId::SoulBolt};
    switch (set) {
    case SkillSet::Bone:
        return bone;
    case SkillSet::Shadow:
        return shadow;
    case SkillSet::Mage:
        return mage;
    case SkillSet::Archer:
    case SkillSet::ArcherFull:
        return archer;
    case SkillSet::Blade:
        return blade;
    case SkillSet::Warrior:
        return warrior;
    case SkillSet::Dwarf:
        return dwarf;
    case SkillSet::Wizard:
        return wizard;
    }
    return {};
}

std::string_view skill_description(SkillId skill) noexcept {
    switch (skill) {
    case SkillId::None:
        return {};
    case SkillId::Shot:
        return "Her standard attack: draw the bow, let go at full draw. One clean arrow.";
    case SkillId::PowerShot:
        return "Hold the bow at full draw to gather power: the longer it is held, the harder the arrow hits, up to "
               "three times a plain shot.";
    case SkillId::ArrowRain:
        return "A volley shot into the sky that comes down on the place she aims at. Each arrow hurts what "
               "stands near where it lands.";
    case SkillId::FrostFan:
        return "Five frost arrows at once, spread like a fan. Each of them does this damage.";
    case SkillId::FireArrow:
        return "An arrow that explodes where it strikes and sets what the blast catches on fire: it burns "
               "on for a few seconds.";
    case SkillId::ThunderKick:
        return "A kick with a shockwave for what has come too close: it hits everything right in front of her.";
    case SkillId::Kingfishers:
        return "She calls a flock of kingfishers that hunt the enemies around her for a while. The damage is "
               "one peck of one bird; they peck many times.";
    case SkillId::ArcaneBolt:
        return "His standard attack: a bolt of arcane force, thrown at what the crosshair covers.";
    case SkillId::SoulBolt:
        return "His standard attack: a wailing soul torn from the skull in his lantern, thrown at what the "
               "crosshair covers.";
    case SkillId::Fireball:
        return "A ball of fire that explodes where it strikes.";
    case SkillId::FrostNova:
        return "A ring of frost bursts out from him and hits everything around him.";
    case SkillId::ChainLightning:
        return "Lightning that strikes one enemy and leaps on to others near it, a little weaker each time.";
    case SkillId::Meteor:
        return "A meteor falls on the place he aims at, after a moment of warning.";
    case SkillId::StarBarrage:
        return "A stream of small stars. The damage is that of one star.";
    case SkillId::BlackHole:
        return "A black hole that draws enemies in, holds them and spits them out again.";
    case SkillId::Slash:
        return "Her standard attack: three sword blows in a row when the button is pressed again in time. "
               "The damage is the first blow; the second and the finisher hit harder.";
    case SkillId::FlameBlade:
        return "A burning sword blow.";
    case SkillId::FrostEdge:
        return "A wide blow with a blade of frost.";
    case SkillId::ThunderCleave:
        return "A heavy blow from above with thunder in it.";
    case SkillId::StarWhirl:
        return "She whirls in a storm of stars that hits everything around her.";
    case SkillId::AxeCombo:
        return "His standard attack: three axe blows in a row. The damage is the first blow.";
    case SkillId::Whirlwind:
        return "He spins with his axe held out and hits everything around him.";
    case SkillId::Earthbreaker:
        return "A blow into the ground that breaks it open ahead of him.";
    case SkillId::LeapStrike:
        return "He leaps at the place he aims at and strikes where he lands.";
    case SkillId::Battlecry:
        return "A cry that hurts everything around him and throws it back.";
    case SkillId::MoltenFissure:
        return "He drives his axe low into the ground: a line of lava breaks open ahead of him. Enemies it "
               "catches are stunned for a moment and burn for a while after.";
    case SkillId::RunicBulwark:
        return "Golden runes of the deep halls cover him for six seconds: every blow does less than half "
               "its harm.";
    case SkillId::AncestralAnvil:
        return "He beats his chest, and an anvil forged with runes slams down where he stands. For eight "
               "seconds it pulses: enemies near it are hurt, heroes near it are healed, his friends too. "
               "The damage is one pulse.";
    case SkillId::Stonebreaker:
        return "He hurls a boulder at what he aims at. It bursts where it strikes and stuns what it "
               "catches.";
    case SkillId::ShadowStep:
        return "He vanishes in a puff of shadow and is behind the enemy he aims at, and drives his dagger "
               "into its back. With nobody near the crosshair, nothing happens.";
    case SkillId::FanOfKnives:
        return "Five daggers of shadow thrown in a fan. Each hurts the first thing it strikes; the damage "
               "is one knife.";
    case SkillId::VeilOfShadows:
        return "Shadows close round him for five seconds: enemies lose sight of him and he runs faster. "
               "The first blow he lands out of the veil does double damage and ends it.";
    case SkillId::VenomEdge:
        return "For eight seconds his blades drip venom: everything his blows, his Shadow Step and his "
               "knives strike is poisoned and goes on being hurt for a while.";
    case SkillId::ShadowFlurry:
        return "He flits from enemy to enemy for a few seconds, a dagger blow for each, and nothing can "
               "touch him meanwhile. The damage is one blow.";
    case SkillId::VenomArrow:
        return "An arrow that leaves a cloud of venom where it strikes. The damage is the arrow's; the cloud "
               "goes on hurting what stays in it.";
    case SkillId::GaleArrow:
        return "An arrow on a gust of wind: it goes through every enemy in its way and throws them back.";
    case SkillId::StormArrow:
        return "Lightning leaps on from what the arrow strikes to enemies near it, a little weaker each time.";
    case SkillId::BrambleArrow:
        return "Brambles shoot up where the arrow lands and hold what stands there in place for a few seconds.";
    case SkillId::DaggerCombo:
        return "Three quick dagger blows for what has come too close for the bow. The damage is the first "
               "blow; the next two hit harder.";
    case SkillId::CounterAttack:
        return "A stance: for a few seconds she answers every blow an enemy lands on her with a blow of her "
               "own that leaves a bleeding wound. She is still hit herself.";
    case SkillId::BladeWhirl:
        return "She whirls with her sword held out for a few seconds, walking slowly. Everything in reach is "
               "hit twice a second, and enemies a little further off are drawn in.";
    case SkillId::JumpAttack:
        return "Hold the button to aim: a mark shows where she will land. Let go and she leaps there and "
               "strikes everything where she lands; what is hit walks slower for a while.";
    case SkillId::SeismicSlash:
        return "A blow into the ground that breaks it open in a wedge ahead of her. Everything in the wedge "
               "is hit and stunned for a few seconds.";
    case SkillId::Enrage:
        return "A cry, and for a few seconds she fights by herself in a frenzy: she runs at the nearest enemy "
               "and strikes much faster than usual. The damage is one of those blows.";
    case SkillId::NeverGiveUp:
        return "A stance: for a few seconds blows hurt her far less and her wounds close while she fights on.";
    case SkillId::VineTower:
        return "Hold the button: she kneels and charges. Let go and a tower of vines grows under her and lifts "
               "her above the fight, higher the longer she charged. It stands a few seconds and sinks back.";
    case SkillId::Stampede:
        return "For a few seconds she runs faster, and the ground breaks under her feet: everything near her "
               "is hit twice a second.";
    case SkillId::CutInPieces:
        return "Takes a full charge. For a few seconds she is at one enemy after another, picked by chance "
               "among those near, with a quick blow for each. Nothing hurts her meanwhile.";
    case SkillId::GatlingGun:
        return "Hold the button: she kneels and builds a turret. Then she mans it and, with the left button "
               "held, shoots arrows very fast where she aims. E leaves it and mans it again. The damage is one arrow.";
    case SkillId::Disguise:
        return "Standing at a bush or a tree, she becomes it: enemies do not see her until she is hurt or it ends.";
    case SkillId::Firebolt:
        return "Hold the button: fire gathers at the tip of his wand, and he stands still meanwhile. Let go and "
               "he throws it where he aims; it bursts where it strikes and burns everything near. The longer "
               "it was held, the harder and the wider the burst. Let go too soon and it goes out.";
    case SkillId::FrostArmor:
        return "For a few seconds a barrier of frost takes the blows meant for him, until it has taken its fill, "
               "and enemies near him move slower.";
    case SkillId::ChainBolt:
        return "Lightning from his wand strikes the enemy he aims at and leaps on to others near it, one after "
               "another, harder with every leap. The damage is the first strike.";
    case SkillId::PoisonHydra:
        return "A three-headed hydra rises from the ground where he aims and spits venom at the enemies near it "
               "for a while. What it strikes is poisoned. The damage is one spit; the poison does more over time.";
    case SkillId::Teleport:
        return "He is at once where he aims, a short way off: out of a fight, over a gap, up onto a ledge.";
    case SkillId::RovingHole:
        return "A small black hole leaves his wand and drifts slowly straight ahead. Enemies near it are drawn "
               "after it and hurt twice a second for as long as they are near. The damage is one such tick.";
    case SkillId::Blizzard:
        return "Ice rains for a while on the place he aims at. Enemies that come into it are held fast for a "
               "moment, then slowed, and hurt twice a second, more the longer they stay. The damage is one such tick.";
    case SkillId::BearTrap:
        return "Hold the button: she kneels and pulls a bear trap open on the ground before her. It lies there "
               "and waits; the first enemy that steps into it is hurt badly. Only so many lie at a time: with "
               "one more, the oldest is gone.";
    case SkillId::ProtectionDome:
        return "A dome of pink light rises round him and goes with him for six seconds. Nothing hurts a hero "
               "who is inside it: him, and a friend who keeps close to him.";
    case SkillId::MirrorImage:
        return "Takes a full charge. A copy of him in all the rainbow's colours appears where he aims and casts "
               "those of his spells that do damage at the enemies near it for a while, weaker than his own. "
               "Enemies go for it as they go for him, and nothing hurts it.";
    }
    return {};
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

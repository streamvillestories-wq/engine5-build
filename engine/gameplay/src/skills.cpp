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
    case SkillId::BoneSpikes:
        return {.name = "Bone Spikes", .charges = false, .kind = SkillKind::Instant};
    case SkillId::SoulDrain:
        return {.name = "Soul Drain", .charges = false, .kind = SkillKind::Instant};
    case SkillId::CurseOfFrailty:
        return {.name = "Curse of Frailty", .charges = false, .kind = SkillKind::Instant};
    case SkillId::BoneArmor:
        return {.name = "Bone Armor", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Requiem:
        return {.name = "Requiem", .charges = false, .kind = SkillKind::Instant};
    case SkillId::RicochetArrow:
        return {.name = "Ricochet Arrow", .charges = false, .kind = SkillKind::Bow};
    case SkillId::HuntersMark:
        return {.name = "Hunter's Mark", .charges = false, .kind = SkillKind::Bow};
    case SkillId::SpiritWolf:
        return {.name = "Spirit Wolf", .charges = false, .kind = SkillKind::Instant};
    case SkillId::OwlSentinel:
        return {.name = "Owl Sentinel", .charges = false, .kind = SkillKind::Instant};
    case SkillId::EvasiveShot:
        return {.name = "Evasive Shot", .charges = false, .kind = SkillKind::Instant};
    case SkillId::ArcaneOrb:
        return {.name = "Arcane Orb", .charges = false, .kind = SkillKind::Instant};
    case SkillId::GravityWell:
        return {.name = "Gravity Well", .charges = false, .kind = SkillKind::Instant};
    case SkillId::LightningPylon:
        return {.name = "Lightning Pylon", .charges = false, .kind = SkillKind::Instant};
    case SkillId::IcePrison:
        return {.name = "Ice Prison", .charges = false, .kind = SkillKind::Instant};
    case SkillId::ArcaneMissiles:
        return {.name = "Arcane Missiles", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Charge:
        return {.name = "Charge", .charges = false, .kind = SkillKind::Instant};
    case SkillId::RainOfBlades:
        return {.name = "Rain of Blades", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Execute:
        return {.name = "Execute", .charges = false, .kind = SkillKind::Instant};
    case SkillId::WarBanner:
        return {.name = "War Banner", .charges = false, .kind = SkillKind::Instant};
    case SkillId::SwordThrow:
        return {.name = "Sword Throw", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Tremor:
        return {.name = "Tremor", .charges = false, .kind = SkillKind::Instant};
    case SkillId::DwarvenCannon:
        return {.name = "Dwarven Cannon", .charges = false, .kind = SkillKind::Instant};
    case SkillId::ThrowingAxe:
        return {.name = "Throwing Axe", .charges = false, .kind = SkillKind::Instant};
    case SkillId::StoneBrew:
        return {.name = "Stone Brew", .charges = false, .kind = SkillKind::Instant};
    case SkillId::PowderKeg:
        return {.name = "Powder Keg", .charges = false, .kind = SkillKind::Instant};
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
    case SkillId::SmokeBomb:
        return {.name = "Smoke Bomb", .charges = false, .kind = SkillKind::Instant};
    case SkillId::DeathMark:
        return {.name = "Death Mark", .charges = false, .kind = SkillKind::Instant};
    case SkillId::BladeDance:
        return {.name = "Blade Dance", .charges = false, .kind = SkillKind::Instant};
    case SkillId::BansheeWail:
        return {.name = "Banshee Wail", .charges = false, .kind = SkillKind::Instant};
    case SkillId::GraveGrasp:
        return {.name = "Grave Grasp", .charges = false, .kind = SkillKind::Instant};
    case SkillId::SpectralForm:
        return {.name = "Spectral Form", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Caltrops:
        return {.name = "Caltrops", .charges = false, .kind = SkillKind::Instant};
    case SkillId::ShadowDash:
        return {.name = "Shadow Dash", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Garrote:
        return {.name = "Garrote", .charges = false, .kind = SkillKind::Instant};
    case SkillId::CripplingStrike:
        return {.name = "Crippling Strike", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Assassinate:
        return {.name = "Assassinate", .charges = false, .kind = SkillKind::Instant};
    case SkillId::DeathCoil:
        return {.name = "Death Coil", .charges = false, .kind = SkillKind::Instant};
    case SkillId::BoneSentinel:
        return {.name = "Bone Sentinel", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Plague:
        return {.name = "Plague", .charges = false, .kind = SkillKind::Instant};
    case SkillId::GraveChill:
        return {.name = "Grave Chill", .charges = false, .kind = SkillKind::Instant};
    case SkillId::LichForm:
        return {.name = "Lich Form", .charges = false, .kind = SkillKind::Instant};
    case SkillId::ShieldBash:
        return {.name = "Shield Bash", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Cleave:
        return {.name = "Cleave", .charges = false, .kind = SkillKind::Instant};
    case SkillId::BattleShout:
        return {.name = "Battle Shout", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Hamstring:
        return {.name = "Hamstring", .charges = false, .kind = SkillKind::Instant};
    case SkillId::HeroicStrike:
        return {.name = "Heroic Strike", .charges = false, .kind = SkillKind::Instant};
    case SkillId::ForgeHammer:
        return {.name = "Forge Hammer", .charges = false, .kind = SkillKind::Instant};
    case SkillId::RuneTrap:
        return {.name = "Rune Trap", .charges = false, .kind = SkillKind::Instant};
    case SkillId::EarthenGrip:
        return {.name = "Earthen Grip", .charges = false, .kind = SkillKind::Instant};
    case SkillId::RallyHorn:
        return {.name = "Rally Horn", .charges = false, .kind = SkillKind::Instant};
    case SkillId::Avalanche:
        return {.name = "Avalanche", .charges = false, .kind = SkillKind::Instant};
    case SkillId::FlameWall:
        return {.name = "Flame Wall", .charges = false, .kind = SkillKind::Instant};
    case SkillId::ArcaneSurge:
        return {.name = "Arcane Surge", .charges = false, .kind = SkillKind::Instant};
    case SkillId::StaticTrap:
        return {.name = "Static Trap", .charges = false, .kind = SkillKind::Instant};
    case SkillId::MeteorShower:
        return {.name = "Meteor Shower", .charges = false, .kind = SkillKind::Instant};
    case SkillId::TimeWarp:
        return {.name = "Time Warp", .charges = false, .kind = SkillKind::Instant};
    case SkillId::PiercingVolley:
        return {.name = "Piercing Volley", .charges = false, .kind = SkillKind::Bow};
    case SkillId::SmokeArrow:
        return {.name = "Smoke Arrow", .charges = false, .kind = SkillKind::Bow};
    case SkillId::TrapLine:
        return {.name = "Trap Line", .charges = false, .kind = SkillKind::Bow};
    case SkillId::HawkEye:
        return {.name = "Hawk Eye", .charges = false, .kind = SkillKind::Instant};
    case SkillId::PhoenixArrow:
        return {.name = "Phoenix Arrow", .charges = false, .kind = SkillKind::Bow};
    case SkillId::BoneGolem:
        return {.name = "Bone Golem", .charges = false, .kind = SkillKind::Instant};
    case SkillId::BaloonParty:
        return {.name = "Baloon Party", .charges = false, .kind = SkillKind::Instant};
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
    case SkillId::BoneSpikes:
        return 26.0F; // to each it catches; the hold does the rest
    case SkillId::SoulDrain:
        return 34.0F; // to the one; half of it heals him
    case SkillId::Requiem:
        return 40.0F; // to each round him: it takes a full charge, and stuns besides
    case SkillId::RicochetArrow:
        return 26.0F; // what it strikes first; each bounce ricochet_share of the one before: 87 over five
    case SkillId::HuntersMark:
        return 22.0F; // the arrow; the mark makes everything of hers after it harder (mark_factor)
    case SkillId::SpiritWolf:
        return 28.0F; // each bite, one to every enemy it runs past, and the slow
    case SkillId::OwlSentinel:
        return 10.0F; // each of its little arrows: ten in its twelve seconds, 100 if it is never idle
    case SkillId::EvasiveShot:
        return 14.0F; // each of its three arrows: 42, and out of harm's way
    case SkillId::ArcaneOrb:
        return 30.0F; // to each enemy it passes through; its burst at the end orb_burst_share of that
    case SkillId::GravityWell:
        return 45.0F; // its implosion, to each it caught, and a stun: about two of his bolts
    case SkillId::LightningPylon:
        return 14.0F; // each strike, one every 0.8 s for ten seconds: the hydra's 17 a second, no poison
    case SkillId::IcePrison:
        return 20.0F; // the strike; three seconds frozen is the point of it
    case SkillId::ArcaneMissiles:
        return 9.0F; // each of five: 45, two of his bolts in one cast, and they find their mark
    case SkillId::Charge:
        return 32.0F; // to each she passes: about her Jump Attack, with 9 m of ground won besides
    case SkillId::RainOfBlades:
        return 14.0F; // each of eight swords, to what is where it lands: a crowd takes most of 112
    case SkillId::Execute:
        return 40.0F; // execute_factor times that (100) to an enemy below execute_threshold of its health
    case SkillId::SwordThrow:
        return 22.0F; // each time it passes one, out and back: a sword blow twice
    case SkillId::Tremor:
        return 18.0F; // each ring, to all it reaches: 54 to what stands close, struck by all three
    case SkillId::DwarvenCannon:
        return 22.0F; // each cannonball's burst: eight in its twelve seconds, about the Lightning Pylon's
    case SkillId::ThrowingAxe:
        return 24.0F; // each time it passes one, out and back: about an axe blow twice
    case SkillId::PowderKeg:
        return 55.0F; // to everything in its blast: slow to set off, so harder than his Whirlwind
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
    case SkillId::BladeDance:
        return 22.0F; // each of its three blows, to everyone round him: 66 in all
    case SkillId::BansheeWail:
        return 24.0F; // to each in the cone, and the stun
    case SkillId::GraveGrasp:
        return 20.0F; // to each held; the hold does the rest
    // The five more of every hero (2026-10-22).
    case SkillId::Caltrops:
        return 5.0F; // every half second to each in the field: 80 over its eight seconds
    case SkillId::ShadowDash:
        return 22.0F; // to each he passes
    case SkillId::Garrote:
        return 11.0F; // each of its five ticks: 55, while the victim is held
    case SkillId::Assassinate:
        return 40.0F; // or three times that on one below a third of its health
    case SkillId::DeathCoil:
        return 26.0F; // and half of it heals him
    case SkillId::BoneSentinel:
        return 12.0F; // each throw, every 1.2 s for twelve seconds: up to 120
    case SkillId::BoneGolem:
        return 9.0F; // each blow of its fist, for a whole minute
    case SkillId::BaloonParty:
        return 0.0F; // it only looks beautiful
    case SkillId::Plague:
        return 6.0F; // every second for six, on each it spreads to
    case SkillId::GraveChill:
        return 18.0F; // to each in the cone, and the slowing
    case SkillId::ShieldBash:
        return 24.0F; // and the stun
    case SkillId::Cleave:
        return 30.0F; // to each in the sweep
    case SkillId::HeroicStrike:
        return 90.0F; // three slashes in one, and a stun
    case SkillId::ForgeHammer:
        return 28.0F; // and the stun
    case SkillId::RuneTrap:
        return 30.0F; // to the one that springs it, and the stun
    case SkillId::EarthenGrip:
        return 20.0F; // to each held
    case SkillId::Avalanche:
        return 24.0F; // each boulder, to each it catches
    case SkillId::FlameWall:
        return 6.0F; // every half second to each in the wall: 72 over its six seconds
    case SkillId::StaticTrap:
        return 25.0F; // to each in it when it springs, and the stun
    case SkillId::MeteorShower:
        return 18.0F; // each meteor, to each it catches
    case SkillId::PiercingVolley:
        return 20.0F; // each of its three arrows
    case SkillId::SmokeArrow:
        return 15.0F; // to each in the burst; the stun is the point
    case SkillId::TrapLine:
        return 30.0F; // each trap, as the Bear Trap's
    case SkillId::PhoenixArrow:
        return 66.0F; // three shots in one, in a burst that burns
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
    case SkillId::CurseOfFrailty: // (it makes everyone's blows harder)
    case SkillId::BoneArmor:
    case SkillId::VeilOfShadows: // (its ambush doubles the blow that ends it)
    case SkillId::VenomEdge:     // (its poison: venom_edge_tick_damage)
    case SkillId::SmokeBomb:     // (it blinds and slows)
    case SkillId::DeathMark:     // (it makes his blows harder: death_mark_factor)
    case SkillId::SpectralForm:  // (it speeds him and spares him)
    case SkillId::CripplingStrike: // (it sharpens the next blows)
    case SkillId::Hamstring:       // (it arms the next blow: hamstring_bleed)
    case SkillId::LichForm:        // (the stances of the five more)
    case SkillId::BattleShout:
    case SkillId::RallyHorn:
    case SkillId::ArcaneSurge:
    case SkillId::TimeWarp:
    case SkillId::HawkEye:
    case SkillId::WarBanner:     // (her blows near it: banner_blow_factor)
    case SkillId::StoneBrew:     // (it hardens and heals him)
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
    case SkillId::SmokeBomb:
        return 16.0F;
    case SkillId::DeathMark:
        return 14.0F; // the mark lasts eight of them
    case SkillId::BladeDance:
        return 12.0F;
    case SkillId::BansheeWail:
        return 12.0F;
    case SkillId::GraveGrasp:
        return 14.0F;
    case SkillId::SpectralForm:
        return 20.0F; // from the key: it lasts six of them
    // The five more of every hero (2026-10-22).
    case SkillId::Caltrops:
        return 14.0F;
    case SkillId::ShadowDash:
        return 10.0F;
    case SkillId::Garrote:
        return 16.0F;
    case SkillId::CripplingStrike:
        return 15.0F;
    case SkillId::Assassinate:
        return 20.0F;
    case SkillId::DeathCoil:
        return 8.0F;
    case SkillId::BoneSentinel:
        return 24.0F;
    case SkillId::BoneGolem:
        return 30.0F; // from when it has gone
    case SkillId::BaloonParty:
        return 0.0F; // none, as wished: it hurts nobody
    case SkillId::Plague:
        return 18.0F;
    case SkillId::GraveChill:
        return 12.0F;
    case SkillId::LichForm:
        return 40.0F;
    case SkillId::ShieldBash:
        return 10.0F;
    case SkillId::Cleave:
        return 9.0F;
    case SkillId::BattleShout:
        return 24.0F;
    case SkillId::Hamstring:
        return 14.0F;
    case SkillId::HeroicStrike:
        return 25.0F;
    case SkillId::ForgeHammer:
        return 9.0F;
    case SkillId::RuneTrap:
        return 14.0F;
    case SkillId::EarthenGrip:
        return 16.0F;
    case SkillId::RallyHorn:
        return 28.0F;
    case SkillId::Avalanche:
        return 30.0F;
    case SkillId::FlameWall:
        return 16.0F;
    case SkillId::ArcaneSurge:
        return 26.0F;
    case SkillId::StaticTrap:
        return 18.0F;
    case SkillId::MeteorShower:
        return 22.0F;
    case SkillId::TimeWarp:
        return 36.0F;
    case SkillId::PiercingVolley:
        return 8.0F;
    case SkillId::SmokeArrow:
        return 14.0F;
    case SkillId::TrapLine:
        return 20.0F;
    case SkillId::HawkEye:
        return 26.0F;
    case SkillId::PhoenixArrow:
        return 30.0F;
    case SkillId::BoneSpikes:
        return 10.0F;
    case SkillId::SoulDrain:
        return 8.0F;
    case SkillId::CurseOfFrailty:
        return 16.0F; // it lasts eight of them
    case SkillId::BoneArmor:
        return 20.0F; // from the key: it lasts seven of them
    case SkillId::RicochetArrow:
        return 7.0F;
    case SkillId::HuntersMark:
        return 14.0F; // the mark lasts ten of them
    case SkillId::SpiritWolf:
        return 12.0F;
    case SkillId::OwlSentinel:
        return 24.0F; // it stands twelve of them
    case SkillId::EvasiveShot:
        return 10.0F;
    case SkillId::ArcaneOrb:
        return 8.0F;
    case SkillId::GravityWell:
        return 16.0F;
    case SkillId::LightningPylon:
        return 30.0F; // it stands ten of them, as his hydra
    case SkillId::IcePrison:
        return 15.0F;
    case SkillId::ArcaneMissiles:
        return 6.0F;
    case SkillId::Charge:
        return 10.0F;
    case SkillId::RainOfBlades:
        return 14.0F;
    case SkillId::Execute:
        return 8.0F;
    case SkillId::WarBanner:
        return 25.0F; // from the plant: it stands ten of them
    case SkillId::SwordThrow:
        return 7.0F;
    case SkillId::Tremor:
        return 14.0F;
    case SkillId::DwarvenCannon:
        return 28.0F; // from the setting down: it stands twelve of them
    case SkillId::ThrowingAxe:
        return 8.0F;
    case SkillId::StoneBrew:
        return 24.0F; // from the key: it lasts seven of them
    case SkillId::PowderKeg:
        return 16.0F;
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
    case SkillId::Requiem:
        return 0.0F;
    }
    return 0.0F;
}

bool skill_needs_charge(SkillId skill) noexcept {
    return skill == SkillId::Kingfishers || skill == SkillId::BlackHole || skill == SkillId::StarWhirl ||
           skill == SkillId::Battlecry || skill == SkillId::CutInPieces || skill == SkillId::MirrorImage ||
           skill == SkillId::ShadowFlurry || skill == SkillId::Requiem;
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
        slots_ = {SkillId::AxeCombo, SkillId::Whirlwind, SkillId::Earthbreaker, SkillId::LeapStrike, SkillId::Battlecry,
                  SkillId::MoltenFissure, SkillId::RunicBulwark, SkillId::AncestralAnvil, SkillId::Stonebreaker,
                  // The first three of his five of 2026-10-16 (the other two are in his book).
                  SkillId::Tremor, SkillId::DwarvenCannon, SkillId::ThrowingAxe};
    } else if (set == SkillSet::Blade) {
        slots_ = {SkillId::Slash, SkillId::CounterAttack, SkillId::BladeWhirl, SkillId::JumpAttack,
                  SkillId::SeismicSlash, SkillId::Enrage, SkillId::NeverGiveUp, SkillId::Stampede, SkillId::CutInPieces,
                  // The first three of her five of 2026-10-16 (the other two are in her book).
                  SkillId::Charge, SkillId::RainOfBlades, SkillId::Execute};
    } else if (set == SkillSet::Archer) {
        // (The Gatling Arrow Gun was wished for key 6. Her five of 2026-10-16 fill key 5 and follow it.)
        slots_ = {SkillId::Shot,          SkillId::VineTower,  SkillId::Disguise,    SkillId::BearTrap,
                  SkillId::RicochetArrow, SkillId::GatlingGun, SkillId::HuntersMark, SkillId::SpiritWolf,
                  SkillId::OwlSentinel,   SkillId::EvasiveShot, SkillId::PiercingVolley, SkillId::SmokeArrow};
    } else if (set == SkillSet::Warrior) {
        slots_ = {SkillId::Slash, SkillId::FlameBlade, SkillId::FrostEdge, SkillId::ThunderCleave, SkillId::StarWhirl};
    } else if (set == SkillSet::Mage) {
        // His wishes in keys 2 to 0; the first two of his five of 2026-10-16 after them (the other three
        // are in his book).
        slots_ = {SkillId::ArcaneBolt,  SkillId::Firebolt,       SkillId::FrostArmor, SkillId::ChainBolt,
                  SkillId::PoisonHydra, SkillId::Teleport,       SkillId::RovingHole, SkillId::Blizzard,
                  SkillId::MirrorImage, SkillId::ProtectionDome, SkillId::ArcaneOrb,  SkillId::GravityWell};
    } else if (set == SkillSet::Bone) {
        slots_ = {SkillId::SoulBolt,       SkillId::BoneSpikes, SkillId::SoulDrain,
                  SkillId::CurseOfFrailty, SkillId::BoneArmor,  SkillId::Requiem,
                  SkillId::BansheeWail,    SkillId::GraveGrasp, SkillId::SpectralForm,
                  SkillId::DeathCoil,      SkillId::BoneSentinel, SkillId::Plague};
    } else if (set == SkillSet::Shadow) {
        slots_ = {SkillId::DaggerCombo,   SkillId::ShadowStep, SkillId::FanOfKnives,
                  SkillId::VeilOfShadows, SkillId::VenomEdge,  SkillId::ShadowFlurry,
                  SkillId::SmokeBomb,     SkillId::DeathMark,  SkillId::BladeDance,
                  SkillId::Caltrops,      SkillId::ShadowDash, SkillId::Garrote};
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

int skill_level_needed(SkillId skill) noexcept {
    switch (skill) {
    case SkillId::Caltrops:
    case SkillId::DeathCoil:
    case SkillId::ShieldBash:
    case SkillId::ForgeHammer:
    case SkillId::FlameWall:
    case SkillId::PiercingVolley:
        return 3;
    case SkillId::ShadowDash:
    case SkillId::BoneSentinel:
    case SkillId::Cleave:
    case SkillId::RuneTrap:
    case SkillId::ArcaneSurge:
    case SkillId::SmokeArrow:
        return 6;
    case SkillId::Garrote:
    case SkillId::Plague:
    case SkillId::BattleShout:
    case SkillId::EarthenGrip:
    case SkillId::StaticTrap:
    case SkillId::TrapLine:
        return 9;
    case SkillId::CripplingStrike:
    case SkillId::GraveChill:
    case SkillId::Hamstring:
    case SkillId::RallyHorn:
    case SkillId::MeteorShower:
    case SkillId::HawkEye:
        return 12;
    case SkillId::Assassinate:
    case SkillId::LichForm:
    case SkillId::HeroicStrike:
    case SkillId::Avalanche:
    case SkillId::TimeWarp:
    case SkillId::PhoenixArrow:
        return 15;
    default:
        return 1;
    }
}

std::span<const SkillId> skill_catalogue(SkillSet set) noexcept {
    // The Archer's: what was made from wishes, then the skills she had before (kept "in the
    // back" since 2026-10-10 and offered again in the book).
    static constexpr std::array archer{
        SkillId::Shot,          SkillId::VineTower,   SkillId::Disguise,   SkillId::GatlingGun,  SkillId::BearTrap,
        SkillId::RicochetArrow, SkillId::HuntersMark, SkillId::SpiritWolf, SkillId::OwlSentinel, SkillId::EvasiveShot,
        SkillId::PowerShot,     SkillId::ArrowRain,   SkillId::FrostFan,   SkillId::FireArrow,   SkillId::ThunderKick,
        SkillId::Kingfishers,   SkillId::VenomArrow,  SkillId::GaleArrow,  SkillId::StormArrow,  SkillId::BrambleArrow,
        SkillId::DaggerCombo,   SkillId::PiercingVolley, SkillId::SmokeArrow, SkillId::TrapLine, SkillId::HawkEye,
        SkillId::PhoenixArrow,
    };
    static constexpr std::array blade{
        SkillId::Slash,        SkillId::CounterAttack, SkillId::BladeWhirl, SkillId::JumpAttack,  SkillId::SeismicSlash,
        SkillId::Enrage,       SkillId::NeverGiveUp,   SkillId::Stampede,   SkillId::CutInPieces, SkillId::Charge,
        SkillId::RainOfBlades, SkillId::Execute,       SkillId::WarBanner,  SkillId::SwordThrow,
        SkillId::ShieldBash,   SkillId::Cleave,        SkillId::BattleShout, SkillId::Hamstring,  SkillId::HeroicStrike,
    };
    static constexpr std::array warrior{SkillId::Slash, SkillId::FlameBlade, SkillId::FrostEdge, SkillId::ThunderCleave,
                                        SkillId::StarWhirl};
    static constexpr std::array dwarf{
        SkillId::AxeCombo,     SkillId::Whirlwind,     SkillId::Earthbreaker,  SkillId::LeapStrike,
        SkillId::Battlecry,    SkillId::MoltenFissure, SkillId::RunicBulwark,  SkillId::AncestralAnvil,
        SkillId::Stonebreaker, SkillId::Tremor,        SkillId::DwarvenCannon, SkillId::ThrowingAxe,
        SkillId::StoneBrew,    SkillId::PowderKeg,     SkillId::ForgeHammer,   SkillId::RuneTrap,
        SkillId::EarthenGrip,  SkillId::RallyHorn,     SkillId::Avalanche,
    };
    static constexpr std::array wizard{SkillId::ArcaneBolt,     SkillId::Fireball, SkillId::FrostNova,
                                       SkillId::ChainLightning, SkillId::Meteor,   SkillId::StarBarrage,
                                       SkillId::BlackHole};
    static constexpr std::array mage{SkillId::ArcaneBolt,     SkillId::Firebolt,    SkillId::FrostArmor,
                                     SkillId::ChainBolt,      SkillId::PoisonHydra, SkillId::Teleport,
                                     SkillId::RovingHole,     SkillId::Blizzard,    SkillId::MirrorImage,
                                     SkillId::ProtectionDome, SkillId::ArcaneOrb,   SkillId::GravityWell,
                                     SkillId::LightningPylon, SkillId::IcePrison,   SkillId::ArcaneMissiles,
                                     SkillId::FlameWall,      SkillId::ArcaneSurge, SkillId::StaticTrap,
                                     SkillId::MeteorShower,   SkillId::TimeWarp,    SkillId::BaloonParty};
    static constexpr std::array shadow{SkillId::DaggerCombo,   SkillId::ShadowStep, SkillId::FanOfKnives,
                                       SkillId::VeilOfShadows, SkillId::VenomEdge,  SkillId::ShadowFlurry,
                                       SkillId::SmokeBomb,     SkillId::DeathMark,  SkillId::BladeDance,
                                       SkillId::Caltrops,      SkillId::ShadowDash, SkillId::Garrote,
                                       SkillId::CripplingStrike, SkillId::Assassinate};
    static constexpr std::array bone{SkillId::SoulBolt,       SkillId::BoneSpikes, SkillId::SoulDrain,
                                     SkillId::CurseOfFrailty, SkillId::BoneArmor,  SkillId::Requiem,
                                     SkillId::BansheeWail,    SkillId::GraveGrasp, SkillId::SpectralForm,
                                     SkillId::DeathCoil,      SkillId::BoneSentinel, SkillId::Plague,
                                     SkillId::GraveChill,     SkillId::LichForm,   SkillId::BoneGolem};
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
    case SkillId::BoneSpikes:
        return "A line of bone spikes breaks out of the ground ahead of him. Enemies it catches are hurt "
               "and held where they stand for a moment.";
    case SkillId::SoulDrain:
        return "A beam of green light from his lantern tears at the soul of the enemy he aims at; half of "
               "the damage heals him.";
    case SkillId::BansheeWail:
        return "A wail from the skull of his lantern: every enemy in a cone ahead of him is hurt and stands "
               "stunned for two seconds. The damage is to each.";
    case SkillId::GraveGrasp:
        return "Skeletal hands break out of the ground where he aims: every enemy there is hurt and held "
               "where it stands for three seconds. The damage is to each.";
    case SkillId::SpectralForm:
        return "He is a wraith for six seconds: he moves faster, and only half of what would hurt him "
               "gets through.";
    // The five more of every hero (2026-10-22).
    case SkillId::Caltrops:
        return "He scatters spikes over the ground where he aims. For eight seconds everything that walks "
               "in them is slowed by half and cut every half second. The damage is one cut.";
    case SkillId::ShadowDash:
        return "He dashes ahead through his enemies as a shadow, cutting each he passes and leaving it "
               "stunned for a moment. The damage is to each.";
    case SkillId::Garrote:
        return "He seizes the enemy he aims at from behind and strangles it: held fast and hurt five times "
               "over two and a half seconds. The damage is one tick.";
    case SkillId::CripplingStrike:
        return "His next three dagger blows cut deeper, 40 % harder, and lame what they strike: slowed by "
               "more than half for three seconds.";
    case SkillId::Assassinate:
        return "A killing stab at the enemy he aims at: three times the damage on one below a third of its "
               "health, and it stands stunned for a moment. The damage is the stab itself.";
    case SkillId::DeathCoil:
        return "A coil of souls flies from the skull at what he aims at. It hurts what it strikes, and half "
               "of that heals him.";
    case SkillId::BaloonParty:
        return "A balloon of a random colour leaves his hand and rises slowly, drifting on the wind, for everyone "
               "to see. It does nothing at all but look beautiful. As often as you like.";
    case SkillId::BoneGolem:
        return "Hold for a second and let go: a skeleton three times his height rises beside him and fights "
               "for a minute. It follows him, goes for what comes near or what he strikes, punches, and now "
               "and then pounds the ground with both fists, hurting all round it. Ready again after it has "
               "gone. The damage is one punch.";
    case SkillId::BoneSentinel:
        return "A sentinel of bone rises where he aims and for twelve seconds throws bone at the nearest "
               "enemy, every second or so. The damage is one throw.";
    case SkillId::Plague:
        return "The enemy he aims at sickens: hurt every second for six, and every two seconds the plague "
               "passes to the enemies beside it. The damage is one second of it.";
    case SkillId::GraveChill:
        return "A breath of grave-cold from the lantern: every enemy in the cone ahead is hurt and slowed "
               "by more than half for four seconds. The damage is to each.";
    case SkillId::LichForm:
        return "He becomes a lich for eight seconds: his spells do half as much again, and blows on him do "
               "less than three quarters of their harm.";
    case SkillId::ShieldBash:
        return "A blow with the shield's edge at what stands before her: hurt and stunned for a second and "
               "a half.";
    case SkillId::Cleave:
        return "One wide sweep of the sword that cuts everything before her. The damage is to each.";
    case SkillId::BattleShout:
        return "She roars: for eight seconds her blows do a quarter more and she runs faster.";
    case SkillId::Hamstring:
        return "Her next sword blow cuts the legs: the victim is held where it stands for two seconds and "
               "bleeds for more.";
    case SkillId::HeroicStrike:
        return "One crushing blow, three slashes in one, at what stands before her; it leaves the victim "
               "stunned for a second.";
    case SkillId::ForgeHammer:
        return "He hurls a hammer glowing from the forge at what he aims at: hurt and stunned for a second.";
    case SkillId::RuneTrap:
        return "A rune trap at his feet. The first enemy to step on it is hurt and stands stunned for two "
               "seconds.";
    case SkillId::EarthenGrip:
        return "The earth seizes every enemy within five metres of him: held where it stands for two and a "
               "half seconds, and hurt. The damage is to each.";
    case SkillId::RallyHorn:
        return "He sounds his horn: for ten seconds his blows do a fifth more and his wounds close, three "
               "health a second.";
    case SkillId::Avalanche:
        return "Six boulders fall in a line ahead of him, one after another. Each is a blow to all it "
               "catches, and slows them. The damage is one boulder.";
    case SkillId::FlameWall:
        return "A wall of fire rises ahead of him for six seconds: everything that stands in it burns, "
               "every half second. The damage is one burn.";
    case SkillId::ArcaneSurge:
        return "His spells surge: for eight seconds everything he casts does a third more.";
    case SkillId::StaticTrap:
        return "A trap of lightning lies where he aims for ten seconds. The first enemy to walk into it "
               "springs it: everything in it is hurt and stands stunned for two seconds.";
    case SkillId::MeteorShower:
        return "Five meteors fall on the place he aims at over two and a half seconds, each a burst. The "
               "damage is one meteor, to each it catches.";
    case SkillId::TimeWarp:
        return "Time slows round him for six seconds: every enemy within eight metres crawls at less than "
               "half its pace, and he moves a third faster.";
    case SkillId::PiercingVolley:
        return "Three heavy arrows in a fan, the middle one at the crosshair. The damage is one arrow.";
    case SkillId::SmokeArrow:
        return "An arrow that bursts into smoke where it strikes: everything in the smoke is hurt a little "
               "and stands stunned for two seconds.";
    case SkillId::TrapLine:
        return "Three bear traps laid in a line before her, open. Each is a Bear Trap.";
    case SkillId::HawkEye:
        return "For eight seconds her arrows do a third more and fly faster and flatter.";
    case SkillId::PhoenixArrow:
        return "One burning arrow of great weight. Where it strikes it bursts in fire that catches everything "
               "near, and what it catches goes on burning.";
    case SkillId::SmokeBomb:
        return "Black smoke bursts round him: every enemy in it stands blind for a moment, and stumbles "
               "slowed for a while after. It hurts nobody.";
    case SkillId::DeathMark:
        return "The enemy he aims at is marked for eight seconds: every blow struck on it hurts half as much "
               "again. With nobody to mark the key does nothing.";
    case SkillId::BladeDance:
        return "He whirls with both blades: every enemy round him is cut three times in a moment. The damage "
               "is one cut, to each.";
    case SkillId::CurseOfFrailty:
        return "He curses the place he aims at: for eight seconds every enemy there takes more damage "
               "from his blows.";
    case SkillId::BoneArmor:
        return "Bones circle him for seven seconds and take the blows meant for him, until they have "
               "taken their share.";
    case SkillId::Requiem:
        return "Wailing souls burst out all round him: every enemy they reach is hurt and stunned, and "
               "each heals him a little. The damage is to each.";
    case SkillId::RicochetArrow:
        return "An arrow that bounces on from what it strikes to the enemies near it, one after another, a "
               "little weaker each time. The damage is the first strike.";
    case SkillId::HuntersMark:
        return "An arrow that marks what it strikes: for ten seconds her arrows and blows hurt it more. The "
               "mark helps nobody else.";
    case SkillId::SpiritWolf:
        return "A ghostly wolf bounds out from her over the ground where she aims. It bites every enemy it "
               "runs past once and leaves it slowed for a moment.";
    case SkillId::OwlSentinel:
        return "An owl on a post rises where she aims and for a while shoots small arrows at the nearest "
               "enemy near it. One at a time. The damage is one arrow.";
    case SkillId::EvasiveShot:
        return "She springs back out of harm's way, away from where she aims, and looses three quick arrows "
               "at the crosshair while in the air. Nothing hurts her as she springs. The damage is one arrow.";
    case SkillId::ArcaneOrb:
        return "A slow orb of violet light leaves his wand and flies straight on where he aims, through every "
               "enemy in its way, hurting each once; at the end of its way it bursts. The damage is to each it "
               "passes; the burst does half of it.";
    case SkillId::GravityWell:
        return "A dark vortex opens on the ground where he aims and draws the enemies near it in, slowed. After "
               "three seconds it implodes: everything at its heart is hurt and stunned for a moment.";
    case SkillId::LightningPylon:
        return "A crystal pylon rises where he aims and for ten seconds throws lightning at the nearest enemy "
               "near it. One at a time. The damage is one strike.";
    case SkillId::IcePrison:
        return "The enemy he aims at is frozen into a block of ice: hurt, and for three seconds it can neither "
               "move nor strike. Then the ice shatters.";
    case SkillId::ArcaneMissiles:
        return "Five small arcane missiles leave his wand one after another and curve onto the enemy he aims "
               "at. The damage is one missile.";
    case SkillId::Charge:
        return "She dashes a long way ahead where she aims, sword low; every enemy in her way is struck and "
               "flung aside. Nobody steers her meanwhile, and a wall stops her.";
    case SkillId::RainOfBlades:
        return "She raises her sword, and spectral swords fall from the sky onto the place she aims at, one "
               "after another. The damage is one sword, to everything where it lands.";
    case SkillId::Execute:
        return "One heavy blow at what is before her. An enemy already badly hurt is struck far harder: it is "
               "meant to end it.";
    case SkillId::WarBanner:
        return "She plants her war banner at her feet. While she stands near it, her blows hit harder and her "
               "wounds close. It helps nobody but her.";
    case SkillId::SwordThrow:
        return "She hurls a spinning copy of her sword straight ahead where she aims; it flies back to her "
               "hand, striking every enemy it passes on the way out and again on the way back.";
    case SkillId::Tremor:
        return "He brings his axe down into the ground: three rings of shock run out from him one after "
               "another, each further. Every ring hurts and briefly stuns the enemies it reaches; the damage "
               "is one ring.";
    case SkillId::DwarvenCannon:
        return "He sets down a small bronze cannon where he aims. For twelve seconds it turns to the nearest "
               "enemy and fires a cannonball at it again and again. The damage is one cannonball.";
    case SkillId::ThrowingAxe:
        return "He hurls a spinning copy of his axe straight ahead where he aims; it flies back to his hand, "
               "striking every enemy it passes on the way out and again on the way back.";
    case SkillId::StoneBrew:
        return "He drinks deep of a dark dwarven brew and his skin turns to stone for seven seconds: blows "
               "do half their harm and his wounds slowly close. It helps nobody but him.";
    case SkillId::PowderKeg:
        return "He kicks a keg of black powder out ahead of him. When its fuse has burnt down, or at once "
               "when an enemy comes near it, it blows up and hurls everything around it away.";
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
               "seconds it pulses: enemies near it are hurt, and he is healed while he stands near it. "
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
        return "She becomes a copy of whatever the crosshair covers, however large: a house, a statue, a villager, "
               "an enemy, a tree; or else the bush or tree she stands at. Enemies do not see her until she is hurt "
               "or it ends.";
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
        return "A dome of pink light rises round him and goes with him for six seconds. Nothing hurts him "
               "while it stands.";
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

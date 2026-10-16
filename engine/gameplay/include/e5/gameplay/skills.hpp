#pragma once

#include "e5/gameplay/character_motor.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <string_view>

namespace e5::gameplay {

enum class SkillId : unsigned char {
    None,
    // The archer's.
    Shot,
    PowerShot,
    ArrowRain,
    FrostFan,
    FireArrow,
    ThunderKick,
    Kingfishers,
    // The wizard's.
    ArcaneBolt,
    Fireball,
    FrostNova,
    ChainLightning,
    Meteor,
    StarBarrage,
    BlackHole,
    // The warrior's.
    Slash, // a plain sword blow; pressed again in time it becomes the next blow of a combo
    FlameBlade,
    FrostEdge,
    ThunderCleave,
    StarWhirl,
    // The dwarf's.
    AxeCombo, // like Slash: a plain blow that continues as a combo
    Whirlwind,
    Earthbreaker,
    LeapStrike,
    Battlecry,
    // More of the archer's, to try out and choose from (2026-10-09). At the end, so that the
    // numbers of the others stay what they were.
    VenomArrow,   // drips venom as it flies; where it strikes a cloud of it lingers
    GaleArrow,    // goes through every enemy in its way and throws them back
    StormArrow,   // lightning leaps on from what it strikes
    BrambleArrow, // brambles shoot up where it lands and hold what stands there
    // The archer's dagger, for what has come too close for the bow: like Slash, a blow that
    // continues as a combo. On a key of its own (F).
    DaggerCombo,
    // Made from players' skill wishes (docs/BUGS.md, "Skill wishes"), one by one. At the end.
    CounterAttack, // the new warrior's: for a few seconds she answers every blow that lands on her
    BladeWhirl,    // the new warrior's "Whirlwind": she whirls with her sword and draws enemies in
    JumpAttack,    // the new warrior's: a high leap onto the place aimed at, and a blow where she lands
    SeismicSlash,  // the new warrior's: a blow into the ground that breaks it open ahead of her and stuns
    Enrage,        // the new warrior's: a cry, and for a few seconds she fights by herself, in a frenzy
    NeverGiveUp,   // the new warrior's: a cry, and for a few seconds she is hurt less and her wounds close
    VineTower,     // the archer's: she kneels and charges; a tower of vines lifts her above the fight
    Stampede,      // the new warrior's "Sprintsz": for a few seconds she is faster and tramples what is near
    CutInPieces,   // the new warrior's, on a full charge: she is at one enemy after another, a blow for each
    GatlingGun,    // the archer's "Gatling Arrow Gun": she builds a turret, mans it and shoots arrows fast
    Disguise,      // the archer's: standing in a bush she becomes that bush; enemies do not see her
    Firebolt,      // the new wizard's: fire gathers at his wand while he holds; let go, a fireball that bursts
    FrostArmor,    // the new wizard's: for a few seconds a barrier of frost takes the blows, and what is near is slowed
    ChainBolt,     // the new wizard's "Chain Lightning": lightning that leaps from enemy to enemy, harder each time
    PoisonHydra,   // the new wizard's: a three-headed hydra rises where he aims and spits venom at what comes near
    Teleport,      // the new wizard's "Teleportation": he is at once where the crosshair points
    RovingHole, // the new wizard's "Black Whole": a small black hole that drifts ahead, draws enemies in and hurts them
    Blizzard,   // the new wizard's: ice rains on a place for a while; what stands there is held, then slowed, and hurt
    BearTrap,   // the archer's: she kneels and sets a trap; the enemy that steps into it is hurt badly
    MirrorImage,    // the new wizard's "Whose Ur Mommy?", on a full charge: a copy of him that casts his spells for a
                    // while
    ProtectionDome, // the new wizard's "Dome of Protection": a pink dome where he stands; nothing hurts a hero inside
                    // it
    // The new dwarf's (the keeper, 2026-10-15: "4 new state of the art modern skills").
    MoltenFissure, // a low blow into the ground: a line of lava breaks open ahead; what it catches is stunned and burns
    RunicBulwark,  // a stance: golden runes cover him, and blows do less than half their harm
    AncestralAnvil, // he beats his chest; a rune-forged anvil slams down and pulses: enemies hurt, heroes healed
    Stonebreaker,   // he hurls a boulder at what he aims at; it bursts and stuns what it catches
    // The Shadowblade's (the keeper, 2026-10-16: "give him 5 skills").
    ShadowStep,    // he is behind the enemy he aims at in a puff of shadow, and stabs it in the back
    FanOfKnives,   // five daggers of shadow thrown in a fan
    VeilOfShadows, // a stance: enemies lose sight of him, he runs faster, and his first blow out of it is doubled
    VenomEdge,     // a stance: his blades drip venom; every blow poisons what it strikes
    ShadowFlurry,  // on a full charge: he flits from enemy to enemy, a dagger blow for each
    // The Bonecaller's (2026-10-16): his standard attack, cast as the new wizard's Arcane Bolt is.
    SoulBolt, // a wailing soul thrown from the skull in his lantern at what the crosshair covers
    // The Bonecaller's five more (the keeper, 2026-10-16: "give him more skills").
    BoneSpikes,     // a line of bone spikes breaks out of the ground ahead; what it catches is held
    SoulDrain,      // a beam from his lantern tears at the enemy he aims at; he is healed by half of it
    CurseOfFrailty, // enemies where he aims are cursed: every blow hurts them more for a while
    BoneArmor,      // a stance: bones circle him and take the blows, until they have taken their share
    Requiem,        // on a full charge: wailing souls burst out round him; what they reach is hurt and stunned
    // The Archer's five (2026-10-16), on the players' bar beside her wished skills.
    RicochetArrow, // an arrow that bounces on from what it strikes to the enemies near it, weaker each time
    HuntersMark,   // an arrow that marks what it strikes: her blows hurt it more for a while
    SpiritWolf,    // a ghostly wolf bounds out ahead of her over the ground and bites what it passes
    OwlSentinel,   // an owl on a post where she aims shoots small arrows at the enemies near it for a while
    EvasiveShot,   // she springs back out of harm's way and looses three quick arrows while in the air
    // The new Wizard's five (2026-10-16), on his players' bar and in his skill book.
    ArcaneOrb,      // a slow violet orb flies straight on through the enemies, hurting each, and bursts at its end
    GravityWell,    // a vortex where he aims draws the enemies near it in, then implodes and stuns them
    LightningPylon, // a crystal pylon where he aims throws lightning at the nearest enemy for a while
    IcePrison,      // the enemy he aims at is frozen into a block of ice: it cannot move or strike for a while
    ArcaneMissiles, // five small missiles leave his wand one after another and curve onto the enemy he aims at
    // The new Warrior's five (2026-10-16), on her players' bar and in her skill book.
    Charge,       // she dashes ahead along where she aims; what she passes is struck and flung aside
    RainOfBlades, // spectral swords fall from the sky onto the place she aims at
    Execute,      // one heavy blow; an enemy already low on health is struck far harder
    WarBanner,    // she plants a banner: near it her own blows hit harder and her wounds close
    SwordThrow,   // she hurls a spinning copy of her sword ahead; it flies back to her, striking twice
    // The new Dwarf's five (2026-10-16), on his bar after Stonebreaker and in his skill book.
    Tremor,        // he strikes the ground: three rings of shock run outward, hurting and stunning what they reach
    DwarvenCannon, // he sets down a small bronze cannon where he aims; it fires at the nearest enemy for a while
    ThrowingAxe,   // he hurls a spinning copy of his axe ahead; it flies back to him, striking twice
    StoneBrew,     // a stance: he drinks, his skin turns to stone; blows do half their harm and his wounds close
    PowderKeg,     // he kicks a keg of black powder ahead; it blows up when its fuse burns down or an enemy comes near
};

// Room for this many skills in anything indexed by SkillId (the player's cooldowns). It was 64 until
// the Bonecaller's five took the count to 66 and every use of a skill threw: the check below fails
// the build instead. Name the LAST SkillId in it when adding one.
inline constexpr std::size_t skill_id_limit = 128;
static_assert(static_cast<std::size_t>(SkillId::PowderKeg) < skill_id_limit, "more skills than skill_id_limit");

// Which skills a character has on the bar.
// `Blade` is the new warrior (2026-10-10): her sword's combo and nothing else yet. Her other
// slots are empty until skills are made for her, one by one, from the players' skill wishes.
// `Archer` is, since 2026-10-10, her standard shot and nothing else: her slots are to be filled
// from the players' skill wishes too. The eleven skills she had are kept as `ArcherFull` ("in the
// back", the keeper's word): nothing of them is removed, test runs use them, and they can be put
// back by making `Archer` build that bar again.
// The numbers are sent between machines as which hero a player is (game/net/net.gd): add at the end.
// `Mage` is the new wizard (2026-10-11): his arcane bolt, thrown with his wand, and nothing else
// yet; his other slots are filled from the players' skill wishes, as the Archer's and the new
// Warrior's are. The first wizard's spells stay his (`Wizard`).
// `Shadow` is the Shadowblade (2026-10-16): the dagger combo with a dagger in each fist, and his other
// slots open for wishes, as the new Wizard's and the new Warrior's were at first.
// `Bone` is the Bonecaller (2026-10-16), a necromancer with a skull lantern: his Soul Bolt and his
// other slots open, as the Shadowblade's were at first.
// The Archer's players' bar (`Archer`) has her five of 2026-10-16 too, in the slots her wishes left.
// The new Wizard's (`Mage`) has the first two of his five of 2026-10-16 in the two slots after key 0;
// the other three are in his book.
// The new Warrior's (`Blade`) has the first three of her five of 2026-10-16 in the three slots after
// key 9 (key 0 and the two right of it); the other two are in her book.
// The new Dwarf's (`Dwarf`) has the first three of his five of 2026-10-16 after Stonebreaker (key 0 and
// the two right of it; his bar is full now); the other two are in his book.
enum class SkillSet : unsigned char { Archer, Wizard, Warrior, Dwarf, Blade, ArcherFull, Mage, Shadow, Bone };

// How many of a hero's quick slots are hers to fill: shown on the bar also while they are empty.
inline constexpr int open_slots = 11;
[[nodiscard]] constexpr int open_slot_count(SkillSet set) noexcept {
    return set == SkillSet::Blade || set == SkillSet::Archer || set == SkillSet::Mage || set == SkillSet::Shadow ||
                   set == SkillSet::Bone
               ? open_slots
               : 0;
}

// How a skill is used.
enum class SkillKind : unsigned char {
    Bow,     // hold aim to draw, let go at full draw
    Instant, // starts when aim is pressed and plays through on its own
};

struct SkillInfo {
    std::string_view name; // shown on the skill bar; empty for SkillId::None
    bool charges;          // builds power while the bow is held at full draw
    SkillKind kind;
};

// What the archer's newer arrows do beside the hit itself (skill_damage).
inline constexpr float venom_cloud_radius = 2.6F;   // metres
inline constexpr float venom_cloud_seconds = 6.0F;  //
inline constexpr float venom_tick_seconds = 0.5F;   // the cloud hurts this often ...
inline constexpr float venom_tick_damage = 5.0F;    // ... this much: 60 to what stays in it
inline constexpr float burn_seconds = 4.0F;         // what a Fire Arrow's blast caught burns this long ...
inline constexpr float burn_tick_seconds = 0.5F;    // ... and is hurt this often ...
inline constexpr float burn_tick_damage = 4.0F;     // ... this much: 32 in all
inline constexpr int gale_max_pierced = 6;          // enemies one gale arrow goes through
inline constexpr float gale_throw_speed = 7.0F;     // m/s along its flight, for each of them
inline constexpr int storm_jumps = 4;               // enemies the lightning leaps on to
inline constexpr float storm_jump_reach = 9.0F;     // metres from one to the next
inline constexpr float storm_jump_share = 0.7F;     // of the arrow's damage, for each of them
inline constexpr float bramble_radius = 4.0F;       // metres
inline constexpr float bramble_root_seconds = 5.0F; // how long what it caught cannot move

// The new warrior's Counter Attack (bug report 21, a player's wish): for `counter_seconds` after
// the key every blow an enemy lands on her from within `counter_reach` is answered at once with
// a blow of her own (skill_damage) that leaves a wound which bleeds. She is still hit herself.
inline constexpr float counter_seconds = 5.0F;            // how long she stands ready
inline constexpr float counter_reach = 5.0F;              // metres from her to the one who struck
inline constexpr float counter_bleed_seconds = 3.0F;      // the wound bleeds this long ...
inline constexpr float counter_bleed_tick_seconds = 1.0F; // ... and hurts this often ...
inline constexpr float counter_bleed_tick_damage = 4.0F;  // ... this much: 12 in all

// A stance is on at once on its key and stays on for a while: it has no clip that plays
// through and does not hold her where she stands.
[[nodiscard]] constexpr bool skill_is_stance(SkillId skill) noexcept {
    return skill == SkillId::CounterAttack || skill == SkillId::NeverGiveUp || skill == SkillId::Stampede ||
           skill == SkillId::FrostArmor || skill == SkillId::ProtectionDome || skill == SkillId::RunicBulwark ||
           skill == SkillId::VeilOfShadows || skill == SkillId::VenomEdge || skill == SkillId::BoneArmor ||
           skill == SkillId::StoneBrew;
}

// The Bonecaller's five (2026-10-16, the keeper: "give him more skills"), all cast with his Soul
// Bolt's throw of the lantern but Bone Armor.
// Bone Spikes: a line of bone breaks open ahead of him, `bone_spikes_length` long and narrow; what
// it catches is hurt and held where it stands (E5Enemy::root) for `bone_spikes_root_seconds`.
inline constexpr float bone_spikes_length = 11.0F;
inline constexpr float bone_spikes_half_angle = 0.30F;
inline constexpr float bone_spikes_near = 1.4F;
inline constexpr float bone_spikes_root_seconds = 2.0F;
// Soul Drain: the living enemy nearest to where the crosshair points (within `drain_aim_slack` of it,
// no further than `drain_reach` from him) is struck, and he gains `drain_heal_share` of the damage.
inline constexpr float drain_reach = 24.0F;
inline constexpr float drain_aim_slack = 3.0F;
inline constexpr float drain_heal_share = 0.5F;
// Curse of Frailty: every enemy within `curse_radius` of where the crosshair points takes
// `curse_factor` times HIS blows for `curse_seconds` (E5Enemy::curse). Nobody else gains by it (the
// keeper, 2026-10-16: no skill helps another player, "since we do PVP"): the curse lies only on his
// machine's copy of the enemy, which is where his blows are counted, and is told to no other machine.
inline constexpr float curse_radius = 4.0F;
inline constexpr float curse_seconds = 8.0F;
inline constexpr float curse_factor = 1.4F;
inline constexpr float curse_reach = 25.0F; // metres from him to where it falls, at most
// Bone Armor, a stance: for `bone_armor_seconds` a barrier takes what would hurt him until it has
// taken `bone_armor_share` of his full health (the new wizard's Frost Armor without its frost).
inline constexpr float bone_armor_seconds = 7.0F;
inline constexpr float bone_armor_share = 0.35F;
// Requiem, on a full charge: every enemy within `requiem_radius` is hurt (skill_damage), stunned for
// `requiem_stun_seconds`, and gives him `requiem_heal_share` of his full health, at most
// `requiem_heal_most` in all.
inline constexpr float requiem_radius = 7.0F;
inline constexpr float requiem_stun_seconds = 2.0F;
inline constexpr float requiem_heal_share = 0.05F;
inline constexpr float requiem_heal_most = 0.3F;

// The Archer's five (2026-10-16), keys 5 and 7 to 10 of the players' bar. Nothing of them helps
// another player (the keeper: "since we do PVP"): the mark lies only on her machine's copy of an
// enemy, as the Bonecaller's curse does.
// Ricochet Arrow (her bow): what the arrow strikes is hurt (skill_damage), and the arrow bounces on
// to up to `ricochet_bounces` more enemies, each the nearest not yet struck within `ricochet_reach`
// of the last, every bounce `ricochet_share` of the one before.
inline constexpr int ricochet_bounces = 4;
inline constexpr float ricochet_reach = 8.0F;
inline constexpr float ricochet_share = 0.8F;
// What the strike after `bounces` bounces does, of the first.
[[nodiscard]] constexpr float ricochet_damage_factor(int bounces) noexcept {
    float factor = 1.0F;
    for (int bounce = 0; bounce < bounces && bounce < ricochet_bounces; ++bounce) {
        factor *= ricochet_share;
    }
    return factor;
}
// Hunter's Mark (her bow): the enemy the arrow strikes takes `mark_factor` times HER blows and arrows
// for `mark_seconds` (E5Enemy::curse, on her machine alone).
inline constexpr float mark_seconds = 10.0F;
inline constexpr float mark_factor = 1.35F;
// Spirit Wolf: a ghostly wolf runs out from her along where she aims, `wolf_distance` over the ground
// in `wolf_seconds`; every enemy within `wolf_bite_radius` of it is bitten once (skill_damage) and goes
// `wolf_slow_share` slower for `wolf_slow_seconds`.
inline constexpr float wolf_distance = 18.0F;
inline constexpr float wolf_seconds = 1.6F;
inline constexpr float wolf_bite_radius = 1.3F;
inline constexpr float wolf_slow_share = 0.3F;
inline constexpr float wolf_slow_seconds = 2.0F;
[[nodiscard]] constexpr float wolf_speed() noexcept {
    return wolf_distance / wolf_seconds;
}
// Owl Sentinel: an owl on a post where the crosshair points (`owl_place_range` from her at most); for
// `owl_seconds` it shoots a small arrow (skill_damage) every `owl_shot_seconds` at the nearest enemy
// within `owl_range` of it. One at a time: a new one takes the old one's place.
inline constexpr float owl_seconds = 12.0F;
inline constexpr float owl_shot_seconds = 1.2F;
inline constexpr float owl_range = 16.0F;
inline constexpr float owl_place_range = 20.0F;
// Evasive Shot: she springs `evasive_distance` back, away from where she aims, in `evasive_seconds`,
// and looses `evasive_arrows` arrows (skill_damage each) at the crosshair meanwhile. Nothing hurts her
// while she springs, as in her roll.
inline constexpr float evasive_distance = 6.0F;
inline constexpr float evasive_seconds = 0.35F;
inline constexpr int evasive_arrows = 3;

// The new Wizard's five (2026-10-16), all thrown with his Arcane Bolt's cast from the wand's tip.
// Nothing of them helps another player (the keeper: "since we do PVP"): what holds, drags or stuns
// an enemy is done by his own machine only.
// Arcane Orb: a slow orb flies `orb_distance` level along where he aims at `orb_speed`, through the
// enemies: each within `orb_touch_radius` of it is hurt once (skill_damage); at its end (or at a
// wall) it bursts, `orb_burst_share` of that to everything within `orb_burst_radius`.
inline constexpr float orb_speed = 8.0F; // m/s
inline constexpr float orb_distance = 20.0F;
inline constexpr float orb_touch_radius = 1.0F;
inline constexpr float orb_burst_radius = 2.5F;
inline constexpr float orb_burst_share = 0.5F;
[[nodiscard]] constexpr float orb_seconds() noexcept {
    return orb_distance / orb_speed;
}
// Gravity Well: on the ground where he aims (`well_range` from him at most), for `well_seconds`
// every enemy within `well_pull_radius` is drawn in at `well_pull_speed` and goes `well_slow_share`
// slower; then it implodes: skill_damage to everything within `well_radius`, stunned for
// `well_stun_seconds`.
inline constexpr float well_seconds = 3.0F;
inline constexpr float well_pull_radius = 7.0F;
inline constexpr float well_pull_speed = 3.0F; // m/s towards its middle
inline constexpr float well_slow_share = 0.4F;
inline constexpr float well_radius = 3.5F;
inline constexpr float well_stun_seconds = 1.0F;
inline constexpr float well_range = 25.0F;
// Lightning Pylon: a crystal where he aims (`pylon_place_range` from him at most); for
// `pylon_seconds` it strikes the nearest enemy within `pylon_range` of it every `pylon_zap_seconds`
// (skill_damage). One at a time: a new one takes the old one's place.
inline constexpr float pylon_seconds = 10.0F;
inline constexpr float pylon_zap_seconds = 0.8F;
inline constexpr float pylon_range = 12.0F;
inline constexpr float pylon_place_range = 20.0F;
// Ice Prison: the enemy nearest where the crosshair points (within `prison_aim_slack` of it, no
// further than `prison_reach` from him) is struck (skill_damage) and frozen: stunned `prison_seconds`.
inline constexpr float prison_seconds = 3.0F;
inline constexpr float prison_reach = 25.0F;
inline constexpr float prison_aim_slack = 2.5F;
// Arcane Missiles: `missile_count` missiles (skill_damage each), `missile_interval` apart, curving onto
// the enemy nearest where the crosshair points (within `missile_aim_slack` of it), or onto that place.
inline constexpr int missile_count = 5;
inline constexpr float missile_interval = 0.12F;
inline constexpr float missile_aim_slack = 2.5F;

// The new Warrior's five (2026-10-16), all her sword's (it counts for every one of them). Nothing of
// them helps another player (the keeper: "since we do PVP"): her banner strengthens and heals only
// her, and what flings an enemy is done by her own machine only.
// Charge: she dashes `charge_distance` along where she aims in `charge_seconds` (nobody steers her;
// a wall stops her); every enemy that comes within `charge_reach` of her is struck once
// (skill_damage) and flung aside, out of her way and a little up.
inline constexpr float charge_distance = 9.0F;
inline constexpr float charge_seconds = 0.45F;
inline constexpr float charge_reach = 1.6F;
inline constexpr float charge_fling_speed = 6.0F; // m/s sideways, out of her way
inline constexpr float charge_fling_lift = 3.5F;  // m/s upwards
[[nodiscard]] constexpr float charge_speed() noexcept {
    return charge_distance / charge_seconds;
}
// Rain of Blades: `blade_rain_count` spectral swords fall over `blade_rain_seconds` onto the place she
// aims at (`blade_rain_range` from her at most), each within `blade_rain_spread` of it; each hurts
// everything within `blade_rain_radius` of where it lands (skill_damage).
inline constexpr int blade_rain_count = 8;
inline constexpr float blade_rain_seconds = 1.2F;
inline constexpr float blade_rain_spread = 3.5F;
inline constexpr float blade_rain_radius = 1.5F;
inline constexpr float blade_rain_range = 20.0F;
// Execute: one heavy blow (skill_damage) at the enemy nearest her within `execute_reach` (she turns
// to it); an enemy with less than `execute_threshold` of its full health left takes `execute_factor`
// times that.
inline constexpr float execute_reach = 2.2F; // metres from her to its body: a sword's length and a step
inline constexpr float execute_threshold = 0.3F;
inline constexpr float execute_factor = 2.5F;
[[nodiscard]] constexpr float execute_damage_factor(float health, float max_health) noexcept {
    return max_health > 0.0F && health > 0.0F && health < max_health * execute_threshold ? execute_factor : 1.0F;
}
// War Banner: a banner at her feet for `banner_seconds`. While SHE stands within `banner_radius` of
// her OWN banner her blows do `banner_blow_factor` times their damage and she gains
// `banner_heal_share` of her full health a second. Nobody else gains by it. One at a time.
inline constexpr float banner_seconds = 10.0F;
inline constexpr float banner_radius = 6.0F;
inline constexpr float banner_blow_factor = 1.25F;
inline constexpr float banner_heal_share = 0.02F;
// Sword Throw: a spinning copy of her sword flies `throw_distance` straight along where she aims at
// `throw_speed` (a wall turns it back sooner), then back to her; each enemy within `throw_touch_radius`
// of it is struck once on the way out and once on the way back (skill_damage each time).
inline constexpr float throw_distance = 14.0F;
inline constexpr float throw_speed = 24.0F; // m/s
inline constexpr float throw_touch_radius = 1.2F;
inline constexpr float throw_longest_seconds = 3.0F; // it is back in her hand by then, wherever she ran
[[nodiscard]] constexpr float throw_out_seconds() noexcept {
    return throw_distance / throw_speed;
}

// The Shadowblade's five (2026-10-16, the keeper: "give him 5 skills").
// Shadow Step: the living enemy nearest to where the crosshair points (within `shadow_step_aim_slack`
// of it, no further than `shadow_step_reach` from him) has him behind it at once, `shadow_step_behind`
// beyond its body, facing it; the lunge then strikes that one (skill_damage). With nobody there the
// key does nothing and nothing is spent.
inline constexpr float shadow_step_reach = 14.0F;
inline constexpr float shadow_step_aim_slack = 3.0F;
inline constexpr float shadow_step_behind = 0.75F;
// Fan of Knives: `fan_knife_count` daggers of shadow, `fan_knife_spread` radians apart, the middle one
// at the crosshair; each hurts the first thing it strikes (skill_damage).
inline constexpr int fan_knife_count = 5;
inline constexpr float fan_knife_spread = 0.16F;
inline constexpr float fan_knife_speed = 32.0F; // metres a second
// Veil of Shadows, a stance: for `veil_seconds` no enemy sees him (they pass him over as they pass
// over a disguised Archer), he is `veil_move_share` times as fast, and the first blow he lands out of
// it does `veil_ambush_factor` times its damage and ends the veil.
inline constexpr float veil_seconds = 5.0F;
inline constexpr float veil_move_share = 1.3F;
inline constexpr float veil_ambush_factor = 2.0F;
// Venom Edge, a stance: for `venom_edge_seconds` everything his blows, his Shadow Step and his knives
// strike is poisoned: `venom_edge_tick_damage` every `venom_edge_tick_seconds` for
// `venom_edge_poison_seconds` (24 in all), begun anew by every blow, never twice at once.
inline constexpr float venom_edge_seconds = 8.0F;
inline constexpr float venom_edge_poison_seconds = 4.0F;
inline constexpr float venom_edge_tick_seconds = 0.5F;
inline constexpr float venom_edge_tick_damage = 3.0F;
// Shadow Flurry, on a full charge: the new warrior's Cut in Pieces made his (the same steps, every
// pieces_tick_seconds), shorter, with his daggers. Nothing hurts him while it lasts.
inline constexpr float flurry_seconds = 3.5F;

// The new dwarf's four (2026-10-15, the keeper: "4 new state of the art modern skills").
// Molten Fissure: a line of lava breaks open ahead of him, `fissure_length` long and narrow; what
// it catches is hurt, stunned for `fissure_stun_seconds` and burns as a Fire Arrow's blast burns.
inline constexpr float fissure_length = 12.0F;     // metres
inline constexpr float fissure_half_angle = 0.30F; // radians to either side: a narrow wedge, 34 degrees
inline constexpr float fissure_near = 1.4F;        // metres: this near, whatever is not behind him is caught
inline constexpr float fissure_stun_seconds = 1.2F;
// Runic Bulwark, a stance: for `bulwark_seconds` only `bulwark_damage_share` of a blow gets through.
inline constexpr float bulwark_seconds = 6.0F;
inline constexpr float bulwark_damage_share = 0.45F;
// Ancestral Anvil: it stands `anvil_seconds` where it fell; every `anvil_tick_seconds` it hurts every
// enemy within `anvil_radius` (skill_damage) and gives him, while he is within it, `anvil_heal_share`
// of his full health. (Friends were healed too until 2026-10-16; the keeper: no skill helps another
// player, "since we do PVP".)
inline constexpr float anvil_seconds = 8.0F;
inline constexpr float anvil_radius = 6.0F;
inline constexpr float anvil_tick_seconds = 1.0F;
inline constexpr float anvil_heal_share = 0.04F;
// Stonebreaker: a boulder thrown at the crosshair; it bursts where it strikes, within
// `stone_blast_radius`, and stuns what it catches for `stone_stun_seconds`.
inline constexpr float stone_speed = 24.0F;       // metres a second
inline constexpr float stone_blast_radius = 3.2F; // metres
inline constexpr float stone_stun_seconds = 1.5F;

// The new Dwarf's five (2026-10-16), all his axe's (it counts for every one of them). Nothing of them
// helps another player (the keeper: "since we do PVP"): his Stone Brew hardens and heals only him, and
// what stuns or flings an enemy is done by his own machine only.
// Tremor: he strikes the ground and `tremor_rings` rings of shock run out from where he stands, one
// every `tremor_ring_interval`, the first `tremor_first_radius` wide and each further out by as much;
// each hurts every enemy within its radius (skill_damage: so what stands close is struck by all three)
// and stuns it for `tremor_stun_seconds`.
inline constexpr int tremor_rings = 3;
inline constexpr float tremor_ring_interval = 0.35F;
inline constexpr float tremor_first_radius = 3.0F;
inline constexpr float tremor_stun_seconds = 0.8F;
[[nodiscard]] constexpr float tremor_radius(int ring) noexcept {
    return tremor_first_radius * static_cast<float>(ring + 1);
}
// Dwarven Cannon: a small cannon set down on the ground where he aims (`cannon_place_range` from him
// at most) for `cannon_seconds`; every `cannon_shot_seconds` it fires a cannonball at the nearest enemy
// within `cannon_range`, which bursts where it strikes within `cannon_blast_radius` (skill_damage).
// One at a time.
inline constexpr float cannon_seconds = 12.0F;
inline constexpr float cannon_place_range = 18.0F;
inline constexpr float cannon_range = 18.0F;
inline constexpr float cannon_shot_seconds = 1.5F;
inline constexpr float cannon_blast_radius = 2.5F;
inline constexpr float cannonball_speed = 26.0F; // m/s
// Throwing Axe: the Sword Throw with his axe (the same flight): `axe_throw_distance` out along where he
// aims and back, each enemy within `throw_touch_radius` struck once each way (skill_damage).
inline constexpr float axe_throw_distance = 13.0F;
inline constexpr float axe_throw_speed = 22.0F; // m/s: a heavier thing than her sword
[[nodiscard]] constexpr float axe_throw_out_seconds() noexcept {
    return axe_throw_distance / axe_throw_speed;
}
// Stone Brew, a stance: for `stone_brew_seconds` only `stone_brew_damage_share` of a blow gets through
// and he gains `stone_brew_heal_share` of his full health a second. Only him.
inline constexpr float stone_brew_seconds = 7.0F;
inline constexpr float stone_brew_damage_share = 0.5F;
inline constexpr float stone_brew_heal_share = 0.015F;
// Powder Keg: a keg set `keg_ahead` before him; it blows up when its fuse has burnt `keg_fuse_seconds`,
// or at once when an enemy comes within `keg_trigger_radius` of it: everything within `keg_blast_radius`
// is hurt (skill_damage) and flung outward. One at a time.
inline constexpr float keg_ahead = 3.0F;
inline constexpr float keg_fuse_seconds = 3.0F;
inline constexpr float keg_trigger_radius = 2.0F;
inline constexpr float keg_blast_radius = 4.5F;
inline constexpr float keg_fling_speed = 7.0F; // m/s outward
inline constexpr float keg_fling_lift = 4.5F;  // m/s upwards

// The new wizard's Dome of Protection (the keeper's own wish, 2026-10-13: "he summons a pink dome
// of protection around him ... lasts for 6 seconds"), a stance: a dome rises round him and goes
// with him for `dome_seconds` (it stood where he cast it at first; asked, the keeper chose this:
// he is always inside). Nothing hurts him while it stands; it shields nobody else (a friend inside
// was shielded too until 2026-10-16: no skill helps another player). Enemies walk in and out.
inline constexpr float dome_seconds = 6.0F;
inline constexpr float dome_radius = 5.0F;

// The new wizard's Frost Armor (ticket 42, a player's wish), a stance: for `frost_armor_seconds`
// a barrier takes what would hurt him, until it has taken `frost_armor_share` of his full health;
// what is left of a blow after that gets through. For as long as the armour lasts, enemies within
// `frost_armor_radius` of him go `frost_armor_slow_share` slower. (The wish had half his health
// every ten seconds: more than the new warrior's Never Give Up keeps from her, for a hero who
// stands back from the fight. It is two fifths, every twelve.)
inline constexpr float frost_armor_seconds = 5.0F;
inline constexpr float frost_armor_share = 0.4F;        // of his full health: what the barrier takes
inline constexpr float frost_armor_radius = 5.0F;       // metres
inline constexpr float frost_armor_slow_share = 0.3F;   // of an enemy's pace, taken away while it is near
inline constexpr float frost_armor_tick_seconds = 0.5F; // how often those near are slowed anew
// The new wizard's Chain Lightning (ticket 43, a player's wish; `SkillId::ChainBolt`, the first
// wizard has a Chain Lightning of his own): lightning from his wand strikes the enemy the
// crosshair covers (the nearest within `chain_first_reach` of where it points) and leaps on,
// one after another every `chain_leap_seconds`, to up to `chain_leaps` more, each the nearest
// not yet struck within `chain_leap_reach` of the last. Every leap hits `chain_leap_gain` times
// as hard as the one before (skill_damage is the first strike), as wished.
inline constexpr int chain_leaps = 4;
inline constexpr float chain_first_reach = 2.5F; // metres from where the crosshair points
inline constexpr float chain_leap_reach = 5.0F;  // metres from one enemy to the next
inline constexpr float chain_leap_gain = 1.2F;
inline constexpr float chain_leap_seconds = 0.14F;
// What the strike after `leaps` leaps does, of the first (leaps beyond the last count as the last).
[[nodiscard]] constexpr float chain_damage_factor(int leaps) noexcept {
    float factor = 1.0F;
    for (int leap = 0; leap < leaps && leap < chain_leaps; ++leap) {
        factor *= chain_leap_gain;
    }
    return factor;
}

// The new wizard's Poison Hydra (ticket 44, a player's wish): a hydra rises from the ground where
// the crosshair points, `hydra_place_range` from him at most, and for `hydra_seconds` spits venom
// every `hydra_spit_seconds` at the nearest enemy within `hydra_range` of it. A spit does
// skill_damage (six tenths of his bolt, as wished) and poisons what it strikes: `hydra_poison_damage`
// (his bolt's, as wished) over `hydra_poison_seconds`. A poisoned enemy struck again is poisoned
// anew, not twice over. One hydra at a time: a new one takes the old one's place.
inline constexpr float hydra_seconds = 10.0F;
inline constexpr float hydra_spit_seconds = 1.0F;
inline constexpr float hydra_range = 14.0F;       // metres from the hydra
inline constexpr float hydra_place_range = 20.0F; // metres from him
inline constexpr float hydra_poison_damage = 22.0F;
inline constexpr float hydra_poison_seconds = 5.0F;
inline constexpr float hydra_poison_tick_seconds = 0.5F;
// What the poison does each time it ticks.
[[nodiscard]] constexpr float hydra_poison_tick_damage() noexcept {
    return hydra_poison_damage * hydra_poison_tick_seconds / hydra_poison_seconds;
}

// The new wizard's Teleportation (ticket 45, a player's wish; `SkillId::Teleport`): on the key he
// is at once on the ground where the crosshair points, `teleport_range` from where he stood at
// most (further off: that far towards it). With no ground there to stand on nothing happens and
// nothing is spent.
inline constexpr float teleport_range = 20.0F; // metres
// How far along the way to a place `distance` metres off he goes: all of it, or as far as he may.
[[nodiscard]] constexpr float teleport_share(float distance) noexcept {
    return distance <= teleport_range || distance <= 0.0F ? 1.0F : teleport_range / distance;
}

// The new wizard's Black Whole (ticket 46, a player's wish, and its spelling; `SkillId::RovingHole`,
// the first wizard has a Black Hole of his own): a small black hole leaves his wand and drifts
// straight ahead, `hole_distance` in `hole_seconds`, and is gone at the end. Enemies within
// `hole_pull_radius` of it are drawn towards it (slower than it flies: they trail after it), and
// every `hole_tick_seconds` everything within `hole_radius` takes skill_damage (half his bolt a
// second, as wished).
inline constexpr float hole_seconds = 6.0F;
inline constexpr float hole_distance = 15.0F;   // metres
inline constexpr float hole_pull_radius = 5.0F; // metres
inline constexpr float hole_radius = 3.0F;      // metres: what it hurts
inline constexpr float hole_pull_speed = 2.0F;  // m/s towards it
inline constexpr float hole_tick_seconds = 0.5F;
[[nodiscard]] constexpr float hole_speed() noexcept {
    return hole_distance / hole_seconds;
}

// The new wizard's Blizzard (ticket 47, a player's wish): for `blizzard_seconds` ice rains on the
// place the crosshair points at (`blizzard_range` from him at most), `blizzard_radius` round it
// ("a 10 m area"). An enemy that comes into it is held for `blizzard_root_seconds` (once: one that
// leaves and comes back is not held again) and goes `blizzard_slow_share` slower for
// `blizzard_slow_seconds` after that. Every `blizzard_tick_seconds` each enemy in it takes
// skill_damage (two fifths of his bolt a second, as wished), a tenth more for every whole second
// it has stayed in (`blizzard_damage_factor`).
inline constexpr float blizzard_seconds = 6.0F;
inline constexpr float blizzard_radius = 5.0F; // metres
inline constexpr float blizzard_range = 25.0F; // metres from him
inline constexpr float blizzard_root_seconds = 2.0F;
inline constexpr float blizzard_slow_share = 0.3F;
inline constexpr float blizzard_slow_seconds = 2.0F;
inline constexpr float blizzard_tick_seconds = 0.5F;
inline constexpr float blizzard_gain = 0.1F; // more damage for every second an enemy has stayed in it
[[nodiscard]] constexpr float blizzard_damage_factor(float seconds_inside) noexcept {
    const float capped =
        seconds_inside < 0.0F ? 0.0F : (seconds_inside > blizzard_seconds ? blizzard_seconds : seconds_inside);
    return 1.0F + blizzard_gain * static_cast<float>(static_cast<int>(capped));
}

// The archer's Bear Trap (ticket 48, a player's wish): she kneels with the button held and pulls a
// trap open on the ground before her; after `bear_trap_set_seconds` it lies armed, whether she
// lets go or not (let go sooner: it is gone, and nothing is spent). The first enemy on the ground
// that comes within `bear_trap_radius` of it springs it and takes skill_damage. It waits for as
// long as it takes. `bear_trap_limit` of them lie at a time: with one more, the oldest is gone.
// (The wish had thirty: laid out over five minutes, that is more damage ready and waiting than
// any hero's skills do in a fight.)
inline constexpr float bear_trap_set_seconds = 2.0F;
inline constexpr float bear_trap_radius = 0.9F; // metres
inline constexpr int bear_trap_limit = 10;
// Which of `lying` traps, the oldest first, must go so that one more may be laid: its place in
// the row, or -1 if there is room.
[[nodiscard]] constexpr int bear_trap_to_remove(int lying) noexcept {
    return lying >= bear_trap_limit ? 0 : -1;
}

// The new wizard's Whose Ur Mommy? (ticket 49, a player's wish, and its name; `SkillId::MirrorImage`),
// the skill of his that takes a full charge: a copy of him appears on the ground where the
// crosshair points, `clone_range` from him at most, and for `clone_seconds` casts his spells at
// the nearest enemy within `clone_reach` of it, one every `clone_cast_seconds`. What it casts
// does `clone_damage_share` of what his own does and does not fill his charge. Enemies go for
// it as they go for him. One at a time.
// Since ticket 55 (its wisher's change): nothing hurts it (at first what struck it counted three
// times against a life like his, and one blow of a Shroomling ended it after its first spell), it
// stands 8 seconds and not 10, does three tenths and not half, and casts only his spells that
// are there to do damage: not his bolt, his Frost Armor or his Teleportation.
inline constexpr float clone_seconds = 8.0F;
inline constexpr float clone_range = 20.0F;       // metres from him
inline constexpr float clone_reach = 20.0F;       // metres from it
inline constexpr float clone_cast_seconds = 1.3F; // from one cast to the next
inline constexpr float clone_damage_share = 0.3F; // as its wisher asked
inline constexpr int clone_spells = 6;
// What it casts as its `cast`-th spell (0, 1, 2 ...): the blizzard (which holds what it is cast
// at), the hydra, the firebolt (at its least), the chain lightning, the black hole, the firebolt;
// six casts are what its eight seconds hold.
[[nodiscard]] constexpr SkillId clone_spell(int cast) noexcept {
    constexpr std::array<SkillId, clone_spells> spells{SkillId::Blizzard,  SkillId::PoisonHydra, SkillId::Firebolt,
                                                       SkillId::ChainBolt, SkillId::RovingHole,  SkillId::Firebolt};
    return spells.at(static_cast<std::size_t>((cast % clone_spells + clone_spells) % clone_spells));
}

// What a barrier with `barrier` left takes of a blow of `amount`.
[[nodiscard]] constexpr float barrier_takes(float barrier, float amount) noexcept {
    const float room = barrier > 0.0F ? barrier : 0.0F;
    const float blow = amount > 0.0F ? amount : 0.0F;
    return blow < room ? blow : room;
}

// The new warrior's Never Give Up (bug report 26, a player's wish), a stance too: for
// `resolve_seconds` after her cry only `resolve_damage_share` of every blow gets through to
// her (the game has no armour: "40% more armour" is 40% less of each blow), and every second
// she gains `resolve_heal_share` of her full health, hurt a moment ago or not. (The wish had
// 40% of her health a second: more than all of it twice over. This is 40% over the four.)
inline constexpr float resolve_seconds = 4.0F;
inline constexpr float resolve_damage_share = 0.6F;
inline constexpr float resolve_heal_share = 0.1F; // of her full health, a second

// The new warrior's Sprintsz (bug report 28, a player's wish; `SkillId::Stampede`), a third stance:
// for `stampede_seconds` she is `stampede_move_share` times as fast on her feet, and every
// `stampede_tick_seconds` the ground she pounds hurts everything within `stampede_radius`
// (skill_damage). She fights on as she likes meanwhile, which is why it reaches less far and
// hits less hard than her Whirlwind, during which she can do nothing else.
inline constexpr float stampede_seconds = 4.0F;
inline constexpr float stampede_tick_seconds = 0.5F;
inline constexpr float stampede_radius = 3.5F; // metres
inline constexpr float stampede_move_share = 1.3F;

// The new warrior's Cut in Pieces (bug report 29, a player's wish), the skill of hers that takes
// a full charge: for `pieces_seconds` she is, every `pieces_tick_seconds`, beside an enemy picked
// by chance from those within `pieces_reach` and strikes it (skill_damage). Nobody steers her
// meanwhile, and nothing hurts her. With no enemy that near it does not start.
// (Bug report 38, a change its wisher asked for: four blows a second instead of two, and unhurt
// while it lasts. Each blow is half of what it was, so that all of it does what it did: the
// three tenths less that were asked would have made it two fifths stronger, and safe besides.)
inline constexpr float pieces_seconds = 5.0F; // (6 until ticket 57)
inline constexpr float pieces_tick_seconds = 0.25F;
inline constexpr float pieces_reach = 10.0F; // metres

// The new warrior's Whirlwind (bug report 22, a player's wish; `SkillId::BladeWhirl`, the dwarf
// has a Whirlwind of his own): for `whirl_seconds` she turns round and round with her sword
// held out. Every `whirl_tick_seconds` everything within `whirl_radius` takes skill_damage, and
// enemies within `whirl_pull_radius` are drawn towards her. She can walk meanwhile, slowly.
inline constexpr float whirl_seconds = 4.0F;
inline constexpr float whirl_tick_seconds = 0.5F;
inline constexpr float whirl_radius = 3.0F;           // metres
inline constexpr float whirl_pull_radius = 5.0F;      // metres
inline constexpr float whirl_pull_speed = 0.5F;       // m/s towards her
inline constexpr float whirl_move_speed = 2.86F;      // m/s: how fast she walks while she whirls (2.6 until ticket 59)
inline constexpr float whirl_turns_per_second = 4.4F; // (2.2 at first: "really slow", bug report 30)

// A channel is on at once on its key, like a stance, but it is what she does while it lasts:
// nothing else can be started, and it shows as the skill in use.
[[nodiscard]] constexpr bool skill_is_channel(SkillId skill) noexcept {
    return skill == SkillId::BladeWhirl;
}

// The new warrior's Jump Attack (bug report 23, a player's wish): she leaps in a high arc onto
// the place the crosshair covers, `leap_max_distance` at most, and strikes the ground there:
// skill_damage to everything within `leap_radius`, and enemies there walk slower for a while.
inline constexpr float leap_max_distance = 15.0F;   // metres
inline constexpr float leap_radius = 4.0F;          // metres round where she lands
inline constexpr float leap_slow_share = 0.2F;      // of an enemy's pace, taken away ...
inline constexpr float leap_slow_seconds = 3.0F;    // ... for so long
inline constexpr float leap_recover_seconds = 0.5F; // on the ground after the blow, before she moves again
// The arc of a leap over `distance` metres of level ground: how long it takes, how fast she
// goes forward and upward at the start, and the pull that brings her down again at its end.
struct LeapArc {
    float seconds = 0.0F;
    float forward_speed = 0.0F;
    float rise_speed = 0.0F;
    float gravity = 0.0F;
};
[[nodiscard]] LeapArc leap_arc(float distance) noexcept;
// How high above the start the middle of that leap is.
[[nodiscard]] float leap_apex_height(float distance) noexcept;

// The new warrior's Seismic Slash (bug report 24, a player's wish): she strikes the ground and
// it breaks open ahead of her, in a wedge that widens with the distance. Every enemy in the
// wedge takes skill_damage and is stunned: it neither moves nor strikes for a while.
inline constexpr float seismic_length = 15.0F;       // metres ahead of her
inline constexpr float seismic_half_angle = 0.5236F; // radians to either side: a wedge of 60 degrees
inline constexpr float seismic_near = 1.5F;          // metres: this near, whatever is not behind her is caught
inline constexpr float seismic_stun_seconds = 2.0F;
// Whether something at (dx, dz) from her is in such a wedge, she striking along (ahead_x, ahead_z)
// (need not be of length one). `allowance`: the width of its body.
[[nodiscard]] bool in_wedge(float dx, float dz, float ahead_x, float ahead_z, float length, float half_angle,
                            float near, float allowance = 0.0F) noexcept;

// The new warrior's Enrage (bug report 25, a player's wish): a cry, and for `enrage_seconds` she
// is beside herself. She goes for the nearest enemy within `enrage_reach` by herself, faster than
// she runs, and strikes the blows of her sword's combo one after another, `enrage_attack_speed`
// times as fast as they usually are; the player does not steer her meanwhile (with no enemy
// that near she is steered as always). Each of these blows is half a sword blow (skill_damage,
// times combo_damage_factor): three times as many at half the strength, so that the frenzy is
// worth about one of her stronger skills and not three.
inline constexpr float enrage_seconds = 4.0F;
inline constexpr float enrage_reach = 12.0F;       // metres: how far she goes for an enemy
inline constexpr float enrage_attack_speed = 3.0F; // of the combo's usual pace
inline constexpr float enrage_move_share = 1.2F;   // of her pace on foot

// The archer's Vine Tower (bug report 27, a player's wish): she kneels with a hand on the ground
// and charges for as long as the button is held, `tower_full_charge_seconds` at most. Let go
// before `tower_least_charge_seconds` and nothing grows (and it is ready again after
// `tower_cancel_cooldown_seconds`); otherwise a tower of vines grows under her and lifts her,
// higher the longer she charged, stands for `tower_stand_seconds` and sinks back with her.
// (The wish had it up to 30 m: that is twice the height of the island's tallest trees, where
// neither the camera nor an arrow's flight has been tried. `tower_tallest` is what was.)
inline constexpr float tower_least_charge_seconds = 1.0F;
inline constexpr float tower_full_charge_seconds = 5.0F;
inline constexpr float tower_lowest = 5.0F;   // metres, after the least charge
inline constexpr float tower_tallest = 12.0F; // metres, after the full one
inline constexpr float tower_stand_seconds = 5.0F;
inline constexpr float tower_cancel_cooldown_seconds = 5.0F;
// How high the tower grows after a charge of so many seconds; 0: it does not grow.
[[nodiscard]] float tower_height(float charged_seconds) noexcept;

// The archer's Gatling Arrow Gun (ticket 35, a player's wish): she kneels and builds a turret in
// front of her for as long as the button is held; after `gatling_build_seconds` it stands and she
// mans it by herself. Let go sooner and what was built falls apart (ready again after
// `gatling_cancel_cooldown_seconds`). Manning it she stands behind it, aims with the crosshair,
// and the left button, held, shoots `gatling_arrows_per_second` arrows (skill_damage each; the
// bow in her weapon slot adds nothing to them). E leaves it and mans it again. It stands for
// `gatling_stand_seconds` and then falls apart.
inline constexpr float gatling_build_seconds = 5.0F;
inline constexpr float gatling_cancel_cooldown_seconds = 5.0F;
inline constexpr float gatling_stand_seconds = 30.0F;
inline constexpr float gatling_arrows_per_second = 4.0F;
// How many arrows leave it in a step of `dt` seconds with the button held. `until_next`: seconds
// until the next one is due (0: at once), carried from step to step; let go, it runs down to 0
// and no further, so that a new press shoots at once but tapping is no faster than holding.
struct GatlingStep {
    float until_next = 0.0F;
    int arrows = 0;
};
[[nodiscard]] GatlingStep step_gatling(float until_next, bool held, float dt) noexcept;

// The archer's Disguise (ticket 36, a player's wish; trees too since ticket 41): standing in a bush (its foot within
// `disguise_reach` of her) she takes its place: the bush is gone from where it grew, she looks
// like it, and enemies do not see her. She walks and shoots her standard shot as always. It ends
// with E, when anything hurts her, when she begins another skill, or after `disguise_seconds`
// (the wish had no end: unseen for ever would outdo everything else she has); the bush is then
// back where it grew, and the cooldown runs from that moment.
inline constexpr float disguise_reach =
    2.5F; // metres (1.8 while it was bushes only: a tree's trunk keeps her further off)
inline constexpr float disguise_seconds = 15.0F;

// A wound that bleeds: it hurts once every `tick_seconds` until its time is up. A new wound
// on the same enemy starts the time anew and keeps the beat (no second wound beside it).
struct BleedState {
    float seconds_left = 0.0F;
    float until_tick = 0.0F;
};
struct BleedStep {
    BleedState state;
    int ticks = 0; // how many times it hurts in this step
};
[[nodiscard]] BleedState open_wound(const BleedState& state, float seconds, float tick_seconds) noexcept;
[[nodiscard]] BleedStep step_bleed(const BleedState& state, float tick_seconds, float dt) noexcept;

[[nodiscard]] SkillInfo skill_info(SkillId skill) noexcept;

// The new wizard's Firebolt (ticket 40, a player's wish): fire gathers at the tip of his wand for
// as long as the button is held, `firebolt_full_charge_seconds` at most; he stands meanwhile. Let
// go before `firebolt_least_charge_seconds` and it goes out, with nothing spent. Otherwise he
// throws a fireball at what the crosshair covers; it bursts where it strikes and hurts everything
// within `firebolt_radius`. Damage (skill_damage with the power) and radius grow with the charge:
// twice the damage after the full charge, as wished. (The wish had the radius grow by 5 m a
// second, 20 m in the end, and ticket 50 asked for 10 to 40 m: wider than a camp of enemies. It is
// `firebolt_full_radius`: 5 m at first, 7 since ticket 50, the widest burst any hero has, for the
// four seconds he stands still.)
inline constexpr float firebolt_least_charge_seconds = 1.0F;
inline constexpr float firebolt_full_charge_seconds = 4.0F;
inline constexpr float firebolt_least_radius = 3.0F; // metres, after the least charge
inline constexpr float firebolt_full_radius = 7.0F;  // metres, after the full one
// How far a charge of so many seconds has come, 0 (the least that is thrown, or less) to 1 (full).
[[nodiscard]] float firebolt_power(float charged_seconds) noexcept;
// How far the burst of a fireball of that power (0..1) reaches.
[[nodiscard]] float firebolt_radius(float power) noexcept;

// Every skill a hero has, whether it is on her bar or not: what the skill book (F5) shows and
// what can be put on the bar. Her standard attack is the first. A new skill for a hero is added
// here, and given a description below, or the book does not know it.
[[nodiscard]] std::span<const SkillId> skill_catalogue(SkillSet set) noexcept;
// What the skill does, in a sentence or two, for the player (the skill book's tooltip). The
// numbers the tooltip shows beside it (damage, cooldown) come from the functions below, so
// this text should name none that could change.
[[nodiscard]] std::string_view skill_description(SkillId skill) noexcept;
// Made from a player's wish (docs/BUGS.md, "Skill wishes"): the book lists these apart.
[[nodiscard]] constexpr bool skill_from_wish(SkillId skill) noexcept {
    return skill >= SkillId::CounterAttack;
}

// Damage of one hit of a skill: one arrow, one explosion, one kick, one peck of
// one bird. `power` is the charge of a power shot or a firebolt (0..1) and ignored otherwise.
[[nodiscard]] float skill_damage(SkillId skill, float power = 0.0F) noexcept;

// The warrior's sword combo: how many blows it has, and how much harder than the first
// each one hits (the last is the finisher). `step` beyond the combo counts as its last blow.
inline constexpr int combo_length = 3;
[[nodiscard]] float combo_damage_factor(int step) noexcept;
// Which blow comes next: the following one if the last blow ended no longer than
// `window_seconds` ago, the first one otherwise or after the finisher.
[[nodiscard]] int next_combo_step(int last_step, float seconds_since_last, float window_seconds) noexcept;
// The warrior steps forward with every blow of her combo, and leaps with the last. How fast
// she moves forward (metres a second) `seconds` into blow `step`: nothing, then a smooth
// push, then nothing. The clips are made for exactly these steps (tools/godot/warrior_combo.gd,
// "step": the same distances and times), so that her feet do not slide.
// `axe`: the dwarf's combo, which has steps of its own (shorter, and none of them a leap).
[[nodiscard]] float combo_advance_speed(int step, float seconds, bool axe = false) noexcept;
// How far blow `step` carries her in all.
[[nodiscard]] float combo_advance_distance(int step, bool axe = false) noexcept;

// How often a skill can be used. A hero's standard attack: always. Her special skills: again
// after their cooldown, longer for the stronger ones. And one skill for each hero, the
// strongest (Kingfishers, Black Hole, Star Whirl, Battle Cry), has no cooldown at all: it
// takes a full charge, which fighting fills and using it empties.
[[nodiscard]] float skill_cooldown_seconds(SkillId skill) noexcept;
[[nodiscard]] bool skill_needs_charge(SkillId skill) noexcept;
// The charge is a share, 0 to 1. A kill is worth a tenth and damage a little (a hundred
// points: a twenty-fifth), so that a hero who fights beside others and seldom lands the last
// blow fills it too: about seven enemies alone.
inline constexpr float charge_per_kill = 0.1F;
inline constexpr float charge_per_damage = 0.0004F;
[[nodiscard]] float charge_after(float charge, float damage_dealt, bool killed) noexcept;

// The warrior's shield. While a key is held she raises it, for a limited time; then, or when
// the key is let go, it comes down and cannot be raised again until it has cooled down. A
// raised shield stops everything that comes from the half-circle in front of her.
struct BlockParams {
    float max_hold_seconds = 2.0F;
    float cooldown_seconds = 5.0F;
};
struct BlockState {
    bool raised = false;
    float held_seconds = 0.0F;  // how long it has been up; meaningful while raised
    float cooldown_left = 0.0F; // seconds until it can be raised again
};
struct BlockStep {
    BlockState state;
    bool raised_now = false;
    bool lowered_now = false;
};
// `pressed`: the key went down in this step (holding it through the cooldown does not raise
// the shield again). `held`: it is down. `able`: she can block at all right now (on her feet,
// not in the middle of a blow, alive).
[[nodiscard]] BlockStep step_block(const BlockState& state, bool pressed, bool held, bool able,
                                   const BlockParams& params, float dt) noexcept;
// Whether a raised shield is between her and something at (dx, dz) from her, she looking
// along `facing_yaw` (as facing_yaw() gives it): anything not behind her.
[[nodiscard]] bool shield_covers(float facing_yaw, float dx, float dz) noexcept;

// The twelve quick slots (keys 1..9, 0 and the two keys to the right of 0). Selecting an empty slot is ignored,
// so the selection always names a usable skill.
class SkillBar {
public:
    static constexpr std::size_t slot_count = 12;

    explicit SkillBar(SkillSet set = SkillSet::Archer) noexcept;

    [[nodiscard]] SkillId slot(std::size_t index) const noexcept;
    [[nodiscard]] std::size_t selected_index() const noexcept { return selected_; }
    [[nodiscard]] SkillId selected() const noexcept { return slots_[selected_]; }

    // Returns true if the selection changed.
    bool select(std::size_t index) noexcept;

    // Rearranging the bar (the skill book, F5). The first slot is the hero's standard attack
    // and stays what it is. A skill is on the bar once at most: put into a slot while it is in
    // another, the two slots exchange what they hold.
    // Where a skill is on the bar; slot_count if it is not.
    [[nodiscard]] std::size_t find(SkillId skill) const noexcept;
    // Puts a skill into a slot. False if that cannot be (the first slot, no skill, the standard attack).
    bool assign(std::size_t index, SkillId skill) noexcept;
    // Empties a slot. False for the first slot or one that is empty already.
    bool clear(std::size_t index) noexcept;

private:
    std::array<SkillId, slot_count> slots_{};
    std::size_t selected_ = 0;
};

// Where the arrows of an arrow rain land, relative to its centre, as offsets
// in the ground plane (y is 0). A sunflower pattern: evenly spread over the
// disc, deterministic and without a random number generator, so every machine
// in a multiplayer game computes the same rain.
[[nodiscard]] Vec3 rain_arrow_offset(int index, int count, float radius) noexcept;

// Sideways angle, in radians, of arrow `index` in a fan of `count` arrows that
// covers `spread_radians` in total. Symmetric about 0; a single arrow flies straight.
[[nodiscard]] float fan_yaw_offset(int index, int count, float spread_radians) noexcept;

// Distance from a point to a filled disc of `disc_radius` that lies in the XY
// plane around the origin (the frame of a target's face). 0 on the disc.
[[nodiscard]] float distance_to_disc(const Vec3& local_point, float disc_radius) noexcept;

// An instant skill (a kick, later a dodge): once started it runs for a fixed
// time and lands its blow at one moment in between.
struct ActionState {
    bool active = false;
    float elapsed = 0.0F; // seconds since the start; meaningful while active
};

struct ActionTimings {
    float duration_seconds = 1.0F;
    float strike_at_seconds = 0.5F;
};

struct ActionStep {
    ActionState state;
    bool started = false; // true on the step the action begins
    bool strike = false;  // true on exactly one step per action
};

// `start_requested` is ignored while an action is running.
[[nodiscard]] ActionStep step_action(const ActionState& state, bool start_requested, const ActionTimings& timings,
                                     float delta_seconds) noexcept;

} // namespace e5::gameplay

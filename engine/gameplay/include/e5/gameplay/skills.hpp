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
};

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
enum class SkillSet : unsigned char { Archer, Wizard, Warrior, Dwarf, Blade, ArcherFull, Mage };

// How many of a hero's quick slots are hers to fill: shown on the bar also while they are empty.
inline constexpr int open_slots = 11;
[[nodiscard]] constexpr int open_slot_count(SkillSet set) noexcept {
    return set == SkillSet::Blade || set == SkillSet::Archer || set == SkillSet::Mage ? open_slots : 0;
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
           skill == SkillId::FrostArmor || skill == SkillId::ProtectionDome || skill == SkillId::RunicBulwark;
}

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
// enemy within `anvil_radius` (skill_damage) and gives every hero within it `anvil_heal_share` of
// their full health, him and his friends alike.
inline constexpr float anvil_seconds = 8.0F;
inline constexpr float anvil_radius = 6.0F;
inline constexpr float anvil_tick_seconds = 1.0F;
inline constexpr float anvil_heal_share = 0.04F;
// Stonebreaker: a boulder thrown at the crosshair; it bursts where it strikes, within
// `stone_blast_radius`, and stuns what it catches for `stone_stun_seconds`.
inline constexpr float stone_speed = 24.0F;       // metres a second
inline constexpr float stone_blast_radius = 3.2F; // metres
inline constexpr float stone_stun_seconds = 1.5F;

// The new wizard's Dome of Protection (the keeper's own wish, 2026-10-13: "he summons a pink dome
// of protection around him ... lasts for 6 seconds"), a stance: a dome rises round him and goes
// with him for `dome_seconds` (it stood where he cast it at first; asked, the keeper chose this:
// he is always inside). Nothing hurts a hero who is inside it, him or a friend who keeps near
// him. Enemies walk in and out as they like.
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

#pragma once

#include "e5/gameplay/character_motor.hpp"

#include <array>
#include <cstddef>
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
};

// Which skills a character has on the bar.
// `Blade` is the new warrior (2026-10-10): her sword's combo and nothing else yet. Her other
// slots are empty until skills are made for her, one by one, from the players' skill wishes.
// `Archer` is, since 2026-10-10, her standard shot and nothing else: her slots are to be filled
// from the players' skill wishes too. The eleven skills she had are kept as `ArcherFull` ("in the
// back", the keeper's word): nothing of them is removed, test runs use them, and they can be put
// back by making `Archer` build that bar again.
// The numbers are sent between machines as which hero a player is (game/net/net.gd): add at the end.
enum class SkillSet : unsigned char { Archer, Wizard, Warrior, Dwarf, Blade, ArcherFull };

// How many of a hero's quick slots are hers to fill: shown on the bar also while they are empty.
inline constexpr int open_slots = 9;
[[nodiscard]] constexpr int open_slot_count(SkillSet set) noexcept {
    return set == SkillSet::Blade || set == SkillSet::Archer ? open_slots : 0;
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
    return skill == SkillId::CounterAttack;
}

// The new warrior's Whirlwind (bug report 22, a player's wish; `SkillId::BladeWhirl`, the dwarf
// has a Whirlwind of his own): for `whirl_seconds` she turns round and round with her sword
// held out. Every `whirl_tick_seconds` everything within `whirl_radius` takes skill_damage, and
// enemies within `whirl_pull_radius` are drawn towards her. She can walk meanwhile, slowly.
inline constexpr float whirl_seconds = 4.0F;
inline constexpr float whirl_tick_seconds = 0.5F;
inline constexpr float whirl_radius = 3.0F;           // metres
inline constexpr float whirl_pull_radius = 5.0F;      // metres
inline constexpr float whirl_pull_speed = 0.5F;       // m/s towards her
inline constexpr float whirl_move_speed = 2.6F;       // m/s: how fast she walks while she whirls
inline constexpr float whirl_turns_per_second = 2.2F; //

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

// Damage of one hit of a skill: one arrow, one explosion, one kick, one peck of
// one bird. `power` is the charge of a power shot (0..1) and ignored otherwise.
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

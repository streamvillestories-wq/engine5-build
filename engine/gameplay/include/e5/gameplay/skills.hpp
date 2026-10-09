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
};

// Which skills a character has on the bar.
enum class SkillSet : unsigned char { Archer, Wizard, Warrior, Dwarf };

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
inline constexpr int gale_max_pierced = 6;          // enemies one gale arrow goes through
inline constexpr float gale_throw_speed = 7.0F;     // m/s along its flight, for each of them
inline constexpr int storm_jumps = 4;               // enemies the lightning leaps on to
inline constexpr float storm_jump_reach = 9.0F;     // metres from one to the next
inline constexpr float storm_jump_share = 0.7F;     // of the arrow's damage, for each of them
inline constexpr float bramble_radius = 4.0F;       // metres
inline constexpr float bramble_root_seconds = 3.5F; // how long what it caught cannot move

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

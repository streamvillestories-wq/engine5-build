// Hero growth (2026-10-21, the keeper: "give each hero a set of new skills, each one upgradable,
// think about a skill tree as a base"): experience from kills, a level and a point for each, and a
// tree of three branches a hero spends them in. Per match: a hero sets out at level 1 every time
// (the keeper's choice), nothing is saved. Pure rules, no Godot: the player controller keeps a
// `Growth` and asks `mods_of` what the chosen nodes do to its skills.
#pragma once

#include "e5/gameplay/skills.hpp"

#include <array>
#include <bitset>
#include <span>

namespace e5::gameplay {

inline constexpr int max_hero_level = 20;
inline constexpr int tree_node_limit = 256;

// What a node does. Skill ones name their skill; the others are the hero's.
enum class NodeEffect : unsigned char {
    Damage,      // the skill does `amount` more (a share: 0.12 = 12 %)
    Cooldown,    // the skill is ready `amount` sooner (SkillId::None: every skill)
    Reach,       // the skill reaches `amount` further (radius, length, distance)
    Count,       // the skill has `amount` more (knives, blows, missiles, bounces)
    Duration,    // the skill (a stance, a summon, a mark) lasts `amount` longer
    MaxHealth,   // `amount` more health
    Regen,       // `amount` more health a second
    MoveSpeed,   // `amount` faster on foot
    AllDamage,   // everything does `amount` more
    DamageTaken, // blows do `amount` less
    Lifesteal,   // `amount` of the damage done heals
    Experience,  // `amount` more experience from kills
    Twist,       // the skill does something new (`twist` says what); `amount` is its number, if any
};

// What a Twist node changes: read by the player controller where that skill does its work.
enum class TwistId : unsigned char {
    None,
    SpinalTap,      // Shadow Step's stab stuns its victim
    VenomFan,       // Fan of Knives always poisons
    DeeperShadows,  // the blow out of the veil does three times its damage
    SoulHarvest,    // Soul Drain heals for all of its damage
    GraspingHands,  // Grave Grasp drags its victims to its centre
    TerrifyingWail, // the Banshee Wail throws enemies back
    MortalStrike,   // Execute stuns what it does not kill
    Bloodbath,      // while enraged, a tenth of the damage done heals
    StandFast,      // Never Give Up also heals a fifth of full health
    ThunderLeap,    // Leap Strike stuns what it catches
    LivingRunes,    // the Runic Bulwark closes wounds while it stands
    ShrapnelKeg,    // the Powder Keg's blast also slows
    Overload,       // the Firebolt's burst also stuns
    Shatter,        // the Ice Prison's cold slows what stands near it
    EventHorizon,   // the Gravity Well's implosion stuns twice as long
    DoubleNock,     // the standard shot looses two arrows
    ManglingTrap,   // the Bear Trap also stuns
    AlphaWolf,      // the Spirit Wolf's bite also stuns
};

struct TreeNode {
    int id;           // 1..n, unique over every hero
    SkillSet set;     // whose tree
    int branch;       // 0..2
    int row;          // 0..3: the rows of 3, 3, 3 and the capstone
    const char* name;
    const char* text; // what it does, for the window
    int needs;        // the node that must be chosen first (0: none)
    NodeEffect effect;
    SkillId skill;    // for the skill effects; None otherwise
    float amount;
    TwistId twist = TwistId::None;
};

struct TreeBranch {
    SkillSet set;
    int branch;
    const char* name;
    const char* colour; // "#rrggbb", the branch's colour in the window
};

// A hero's growth in this match.
struct Growth {
    int xp = 0;     // towards the next level
    int level = 1;
    int points = 0; // unspent
    std::bitset<tree_node_limit> chosen;
};

// What the chosen nodes make of the hero's skills: factors by SkillId, and the hero's own.
struct SkillMods {
    std::array<float, skill_id_limit> damage{};
    std::array<float, skill_id_limit> cooldown{};
    std::array<float, skill_id_limit> reach{};
    std::array<float, skill_id_limit> duration{};
    std::array<int, skill_id_limit> count{};
    float max_health = 0.0F;
    float regen = 0.0F;
    float move = 1.0F;
    float all_damage = 1.0F;
    float damage_taken = 1.0F;
    float lifesteal = 0.0F;
    float experience = 1.0F;
    std::bitset<32> twists; // by TwistId
    [[nodiscard]] bool has(TwistId twist) const noexcept { return twists.test(static_cast<std::size_t>(twist)); }
};

[[nodiscard]] std::span<const TreeNode> all_tree_nodes() noexcept;
[[nodiscard]] std::span<const TreeBranch> all_tree_branches() noexcept;
[[nodiscard]] const TreeNode* tree_node(int id) noexcept;
[[nodiscard]] const TreeBranch* tree_branch(SkillSet set, int branch) noexcept;
[[nodiscard]] bool set_has_tree(SkillSet set) noexcept;

// The experience a level takes (from the one before it), and what a kill gives.
[[nodiscard]] int xp_for_level(int level) noexcept;
[[nodiscard]] int xp_for_kill(float enemy_max_health) noexcept;
// Adds experience; returns how many levels were gained (a point each).
int gain_xp(Growth& growth, int amount) noexcept;
[[nodiscard]] bool can_choose(const Growth& growth, SkillSet set, int id) noexcept;
bool choose(Growth& growth, SkillSet set, int id) noexcept;
[[nodiscard]] SkillMods mods_of(const Growth& growth, SkillSet set) noexcept;

} // namespace e5::gameplay

#pragma once

namespace e5::bridge {

// Action names used by the bridge. Defined in code (not only in project.godot)
// so a missing binding fails at one obvious place instead of silently reading 0.
namespace actions {
inline constexpr const char* move_forward = "e5_move_forward";
inline constexpr const char* move_back = "e5_move_back";
inline constexpr const char* move_left = "e5_move_left";
inline constexpr const char* move_right = "e5_move_right";
inline constexpr const char* jump = "e5_jump";
inline constexpr const char* sprint = "e5_sprint";
inline constexpr const char* emote = "e5_emote";         // a dance, for show
inline constexpr const char* interact = "e5_interact";   // gets on the horse she stands at, or off it
inline constexpr const char* dodge_alt = "e5_dodge_alt"; // the second dodge, while two are compared
inline constexpr const char* walk = "e5_walk";           // held: she walks instead of running
// Left mouse: the hero's standard attack (the first slot of the bar), whatever is selected.
inline constexpr const char* attack = "e5_attack";
// Right mouse: the skill selected on the bar.
inline constexpr const char* aim = "e5_aim";
inline constexpr const char* use_potion = "e5_use_potion";
// Held: the warrior raises her shield.
inline constexpr const char* block = "e5_block";
// Opens and closes the bag; the interface script listens for it.
inline constexpr const char* inventory = "e5_inventory";
// Skill slots are e5_skill_1 .. e5_skill_12 (keys 1..9, 0 and the two keys to the right of 0:
// - and = on an American keyboard, ß and ´ on a German one).
inline constexpr const char* skill_prefix = "e5_skill_";
inline constexpr int skill_slot_count = 12;
} // namespace actions

// Registers default key bindings for any action the project has not defined
// itself. Bindings set in the Godot editor's Input Map always take precedence.
void ensure_default_input_actions();

} // namespace e5::bridge

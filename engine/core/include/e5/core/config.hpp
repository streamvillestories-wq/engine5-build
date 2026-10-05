#pragma once

#include "e5/core/error.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace e5 {

// Runtime options supplied on the command line after Godot's `--` separator,
// e.g. `godot --path game -- --benchmark --benchmark-seconds=10`.
struct RuntimeConfig {
    bool benchmark = false;
    float benchmark_warmup_seconds = 2.0F;
    float benchmark_seconds = 10.0F;
    std::string benchmark_output;  // JSON report path; empty = do not write
    std::string screenshot_output; // PNG path; empty = no screenshot
    bool show_overlay = true;
    // Starts with the detailed statistics open (what F4 shows), for pictures of it.
    bool overlay_detail = false;
    // Holds "move forward" for the whole run, so unattended tests exercise
    // movement, collision and animation instead of a standing character.
    // One of "forward", "back", "left", "right"; empty = no automatic input.
    // Directions are relative to the camera, as for a human player.
    std::string auto_move;
    // Holds the aim button for the whole run (unattended archery checks).
    bool auto_aim = false;
    // Attacks repeatedly with the attack button (left mouse: the standard attack), like auto_fire.
    bool auto_attack = false;
    // Runs that repeat one skill (auto_fire, auto_charge, auto_skill) switch the cooldowns
    // and the charge off, or they would test waiting. This keeps them on.
    bool cooldowns = false;
    // Holds the block key (the warrior's shield) for the whole run.
    bool auto_block = false;
    // Shoots repeatedly: holds aim until the bow is drawn, then lets go.
    bool auto_fire = false;
    // Like auto_fire, but with the power shot selected and held until fully charged.
    bool auto_charge = false;
    // Skill slot (1..10) to select at start; unset leaves the default.
    std::optional<int> auto_skill;
    // Feeds steady sideways mouse movement through the normal input path, so
    // unattended runs can check that camera control works (e.g. while aiming).
    bool auto_turn = false;
    // Name of a Camera3D node to view the scene through instead of the player's
    // camera (fixed viewpoints for captures); empty = player camera.
    std::string camera_name;
    // Initial camera orbit angle in degrees (0 = behind the character,
    // 180 = facing it). Lets unattended captures look at the character's front.
    std::optional<float> camera_yaw_degrees;
    // Where the player starts instead of where the scene puts her (metres, x and z); for
    // tests that need a certain place. Both or neither.
    std::optional<float> start_x;
    std::optional<float> start_z;
    // Initial camera tilt in degrees, positive up (unattended checks of aiming high or at the ground).
    std::optional<float> camera_pitch_degrees;
    // Hour of the day (0..24) to start at, in scenes with a day and night cycle;
    // the clock is stopped there, so a capture shows exactly that hour.
    std::optional<float> time_of_day;
};

// Unknown options are errors: a typo must not silently run the wrong test.
[[nodiscard]] Result<RuntimeConfig> parse_runtime_args(std::span<const std::string_view> args);

} // namespace e5

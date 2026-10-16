#pragma once

#include <span>

namespace e5::gameplay {

// The crystal core match (the keeper, 2026-10-16): every player places one crystal core during a
// placement time; it marks his base (its radius is his respawn point, his alarm, later his building
// ground), and the last core standing wins. These are the rules that need no engine: the numbers,
// and whether a place may take a core. The match itself (phases, who is out) is decided by the
// game server, which runs no C++ (server/match_state.gd); the server sends its own numbers in every
// state message, so what is here is the default and what the game shows before it has heard.
struct CoreRules {
    float health = 3000.0F;            // about two minutes for one hero striking without pause
    float radius = 32.0F;              // metres: respawn point, alarm, the six anchor stones' ring
    float spacing = 80.0F;             // metres at least between two cores
    float placement_seconds = 90.0F;   // the placement time at the start of a match
    float respawn_seconds = 8.0F;      // at his own core
    float reveal_metres = 60.0F;       // an enemy core this near is found and marked on the map
    float protection_seconds = 300.0F; // the dome over a new base (until he strikes another's core)
    // What a place must be like (the core's plinth is about 2.6 m across).
    float max_slope = 0.21F;      // E5Terrain::slope_at (1 = 45 degrees): about 12 degrees
    float max_unevenness = 0.6F;  // metres between the highest and lowest ground under the plinth
    float min_height = 0.4F;      // metres over the sea: not on a beach's wet edge
    float road_clearance = 6.0F;  // metres from the middle of a road or trail
    float water_clearance = 6.0F; // metres from the river's bank and a lake's shore
};

// What the ground is like at a place, gathered by the engine (E5Terrain::judge_core_site).
struct CoreSiteProbe {
    float height = 0.0F; // the ground at the place
    float lowest = 0.0F; // the lowest and highest ground in a ring round it
    float highest = 0.0F;
    float slope = 0.0F;          // steepness at the place (1 = 45 degrees)
    float road_distance = 1e9F;  // to the middle of the nearest road or trail
    float river_distance = 1e9F; // to the river's bank (negative in it)
    float lake_distance = 1e9F;  // to the nearest lake's shore (negative inside)
    bool underground = false;    // in the dungeons under the island
    bool keep_out = false;       // in a monster camp, a clearing, at a landmark or a portal
    float nearest_core = 1e9F;   // to the nearest other core already placed
};

// Why a place may not take a core; `Ok` when it may.
enum class CoreSite : unsigned char { Ok, Underground, Water, Road, Uneven, Steep, KeepOut, TooClose };

// The first reason that applies, in that order (what a player can do least about first).
[[nodiscard]] CoreSite judge_core_site(const CoreSiteProbe& probe, const CoreRules& rules) noexcept;

// What the placement shows the player for a verdict.
[[nodiscard]] const char* core_site_reason(CoreSite site) noexcept;

// A core's place on the ground.
struct CoreSpot {
    float x = 0.0F;
    float z = 0.0F;
};
// How far the nearest of `others` is from `candidate` (1e9 when there is none): the probe's
// `nearest_core`, compared with `CoreRules::spacing`.
[[nodiscard]] float nearest_core_distance(CoreSpot candidate, std::span<const CoreSpot> others) noexcept;

} // namespace e5::gameplay

#pragma once

#include <cstdint>

namespace e5::gameplay {

// The shape of an island, as a function: give it a place, get the height of
// the ground there. Nothing is stored; the same parameters describe the same
// island on every machine, which is what a multiplayer server and its clients
// need to agree on where the ground is.
//
// The island is round-ish, centred on the origin: rolling hills inland, a
// flatter clearing in the middle, beaches at the coast, sea floor beyond.
// Heights are metres relative to sea level (0).
struct IslandParams {
    float radius = 160.0F;         // metres from the centre to the coast, on average
    float hill_height = 16.0F;     // metres the highest hills rise above the lowland
    float lowland_height = 2.5F;   // metres the land lies above the sea where it is flat
    float sea_floor = -8.0F;       // metres, far from the coast
    float hill_size = 70.0F;       // metres across a typical hill
    float clearing_radius = 26.0F; // metres around the centre kept nearly flat
    std::uint32_t seed = 1;
    // Mountains, standing on the island; height 0 = none. With a length they are a
    // range: a wandering crest with peaks and saddles; without, a single peak. They
    // keep out of the clearing and end at the coast like all land.
    float mountain_height = 0.0F;  // metres the highest peak rises above the lowland
    float mountain_radius = 70.0F; // metres from the crest to the foot
    float mountain_x = 0.0F;       // the middle of the range, or the peak
    float mountain_z = 0.0F;
    float mountain_length = 0.0F;    // metres from one end of the crest to the other
    float mountain_direction = 0.0F; // radians; 0 = the crest runs along x
};

// Height of the ground at (x, z).
[[nodiscard]] float island_height(const IslandParams& params, float x, float z) noexcept;

// How steep the ground is at (x, z): 0 = level, 1 = 45 degrees, measured over `step` metres.
[[nodiscard]] float island_slope(const IslandParams& params, float x, float z, float step = 1.0F) noexcept;

} // namespace e5::gameplay
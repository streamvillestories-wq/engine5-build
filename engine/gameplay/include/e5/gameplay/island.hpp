#pragma once

#include <cstdint>
#include <vector>

namespace e5::gameplay {

// The shape of an island, as a function: give it a place, get the height of
// the ground there. Nothing is stored; the same parameters describe the same
// island on every machine, which is what a multiplayer server and its clients
// need to agree on where the ground is.
//
// The island is round-ish, centred on the origin: rolling hills inland, a
// flatter clearing in the middle, beaches at the coast, sea floor beyond.
// Heights are metres relative to sea level (0).
// A place on the island's plan (x east, z south).
struct IslandPoint {
    float x = 0.0F;
    float z = 0.0F;
};
// A round place: a lake, an islet.
struct IslandDisc {
    float x = 0.0F;
    float z = 0.0F;
    float radius = 10.0F;
};
// A bay: the coast drawn in around one direction.
struct IslandBay {
    float direction = 0.0F; // radians from the centre: 0 = east, a quarter turn = south
    float width = 0.3F;     // radians to either side that it reaches
    float depth = 0.2F;     // share of the island's radius it cuts in
};

// A hill set down on the lowland where one is wanted.
struct IslandHill {
    float x = 0.0F;
    float z = 0.0F;
    float radius = 30.0F; // metres from its top to its foot
    float height = 10.0F; // metres
};
// A ridge along a line of points, or with a negative height a valley.
struct IslandRidge {
    std::vector<IslandPoint> points;
    float width = 30.0F;  // metres from the crest to the foot on either side
    float height = 10.0F; // metres; negative cuts a valley that deep
};

// What kind of country a part of the island is. Meadow is whatever no other region claims.
enum class IslandRegionKind : unsigned char { Meadow, Pine, Dry, Marsh };
// A region: its kind, and the round area it fills (its border wanders).
struct IslandRegion {
    IslandRegionKind kind = IslandRegionKind::Meadow;
    float x = 0.0F;
    float z = 0.0F;
    float radius = 50.0F;
};
// How much of each kind a place is, 0 to 1 each; what is left of 1 is meadow.
struct RegionShares {
    float pine = 0.0F;
    float dry = 0.0F;
    float marsh = 0.0F;
};

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
    // A pass: a gap in the range where the crest drops almost to the lowland. `pass_along` is
    // metres along the crest from its middle; a width of 0 means none.
    float pass_along = 0.0F;
    float pass_width = 0.0F;

    // Water inland. A river is a line of points, source first; lakes are discs. Both are
    // cut down to a little below the sea, so that the sea's own surface fills them: there
    // is one water level on the island, and no second surface to draw. Shallow enough to
    // wade through.
    std::vector<IslandPoint> river;
    float river_width = 9.0F; // metres of water across
    float water_depth = 1.0F; // metres below the sea, in river and lakes
    std::vector<IslandDisc> lakes;

    // The coast. Bays draw it in; between `cliff_from` and `cliff_to` (radians, as a bay's
    // direction, going round from the first to the second) the land ends in cliffs
    // `cliff_height` metres high instead of a beach; islets stand out at sea.
    std::vector<IslandBay> bays;
    float cliff_from = 0.0F;
    float cliff_to = 0.0F;
    float cliff_height = 0.0F; // 0 = beaches all round
    std::vector<IslandDisc> islets;
    float islet_height = 5.0F;

    // The regions. They colour the ground and decide what grows (the terrain's material and
    // the forests ask), and two of them shape it: a marsh lies flat at the water's level
    // with pools in it, and dry highland stands a few metres above the land around.
    std::vector<IslandRegion> regions;

    // The lowland's own relief, on top of the broad hills: hills and ridges where they are
    // put, valleys cut between, and a gentle rolling everywhere (`rolling` metres up and
    // down, a few tens of metres across). None of it reaches into the mountains or the
    // clearing, and a valley's floor stays above the water.
    std::vector<IslandHill> hills;
    std::vector<IslandRidge> ridges;
    float rolling = 0.0F;

    // Building ground: within its radius a site is level, at the height the land has at its
    // middle, and meets the land around over half the radius again. For whatever is too
    // large to stand on a slope: a fort, a ring of stones, a stronghold to come.
    std::vector<IslandDisc> sites;
};

[[nodiscard]] RegionShares island_regions(const IslandParams& params, float x, float z) noexcept;
// The kind that has the largest share of a place.
[[nodiscard]] IslandRegionKind island_region(const IslandParams& params, float x, float z) noexcept;

// Metres from a place to the middle of the river, and to the edge of the nearest lake
// (negative inside it); very far if there is none. For whatever must keep its feet dry,
// or wants to stand at the water.
[[nodiscard]] float island_river_distance(const IslandParams& params, float x, float z) noexcept;
[[nodiscard]] float island_lake_distance(const IslandParams& params, float x, float z) noexcept;

// Height of the ground at (x, z).
[[nodiscard]] float island_height(const IslandParams& params, float x, float z) noexcept;

// How steep the ground is at (x, z): 0 = level, 1 = 45 degrees, measured over `step` metres.
[[nodiscard]] float island_slope(const IslandParams& params, float x, float z, float step = 1.0F) noexcept;

} // namespace e5::gameplay
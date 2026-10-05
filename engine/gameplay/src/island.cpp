#include "e5/gameplay/island.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace e5::gameplay {
namespace {

std::uint32_t hash(std::uint32_t value) {
    value = value * 747796405U + 2891336453U;
    const std::uint32_t word = ((value >> ((value >> 28U) + 4U)) ^ value) * 277803737U;
    return (word >> 22U) ^ word;
}

// A fixed pseudo-random number in [0, 1) for a lattice point.
float lattice(std::uint32_t seed, int x, int z) {
    const std::uint32_t mixed = hash(
        seed ^ hash(static_cast<std::uint32_t>(x) * 0x9E3779B9U ^ hash(static_cast<std::uint32_t>(z) * 0x85EBCA6BU)));
    return static_cast<float>(mixed >> 8U) / 16777216.0F;
}

float smooth(float t) {
    return t * t * (3.0F - 2.0F * t);
}

float smooth_step(float from, float to, float value) {
    return smooth(std::clamp((value - from) / (to - from), 0.0F, 1.0F));
}

// Smoothly interpolated lattice noise in [0, 1).
float value_noise(std::uint32_t seed, float x, float z) {
    const float floor_x = std::floor(x);
    const float floor_z = std::floor(z);
    const int ix = static_cast<int>(floor_x);
    const int iz = static_cast<int>(floor_z);
    const float tx = smooth(x - floor_x);
    const float tz = smooth(z - floor_z);
    const float near_row = std::lerp(lattice(seed, ix, iz), lattice(seed, ix + 1, iz), tx);
    const float far_row = std::lerp(lattice(seed, ix, iz + 1), lattice(seed, ix + 1, iz + 1), tx);
    return std::lerp(near_row, far_row, tz);
}

// Four layers of noise, each half as large and half as strong as the one before; result in [0, 1).
float layered_noise(std::uint32_t seed, float x, float z) {
    float total = 0.0F;
    float strength = 0.5F;
    float scale = 1.0F;
    for (std::uint32_t layer = 0; layer < 4; ++layer) {
        total += strength * value_noise(seed + layer * 101U, x * scale, z * scale);
        strength *= 0.5F;
        scale *= 2.0F;
    }
    return total / 0.9375F;
}

// Lattice noise with its slope: value in [0, 1) and how fast it changes along x and z.
struct NoiseSample {
    float value = 0.0F;
    float slope_x = 0.0F;
    float slope_z = 0.0F;
};

NoiseSample value_noise_with_slope(std::uint32_t seed, float x, float z) {
    const float floor_x = std::floor(x);
    const float floor_z = std::floor(z);
    const int ix = static_cast<int>(floor_x);
    const int iz = static_cast<int>(floor_z);
    const float fx = x - floor_x;
    const float fz = z - floor_z;
    const float ux = smooth(fx);
    const float uz = smooth(fz);
    const float corner = lattice(seed, ix, iz);
    const float along_x = lattice(seed, ix + 1, iz) - corner;
    const float along_z = lattice(seed, ix, iz + 1) - corner;
    const float twist = corner - lattice(seed, ix + 1, iz) - lattice(seed, ix, iz + 1) + lattice(seed, ix + 1, iz + 1);
    return {.value = corner + along_x * ux + along_z * uz + twist * ux * uz,
            .slope_x = 6.0F * fx * (1.0F - fx) * (along_x + twist * uz),
            .slope_z = 6.0F * fz * (1.0F - fz) * (along_z + twist * ux)};
}

// Layered noise that looks worn by water: every layer counts for less where the layers
// before it are steep, so detail gathers on the flats and ridges and the slopes run
// out smooth, as on eroded ground. Result in about [0, 1].
float eroded_noise(std::uint32_t seed, float x, float z) {
    constexpr int layers = 5;
    constexpr float steepness_weight = 2.2F;
    float total = 0.0F;
    float strength = 0.5F;
    float slope_x = 0.0F;
    float slope_z = 0.0F;
    for (int layer = 0; layer < layers; ++layer) {
        const NoiseSample sample = value_noise_with_slope(seed + static_cast<std::uint32_t>(layer) * 131U, x, z);
        slope_x += sample.slope_x * steepness_weight;
        slope_z += sample.slope_z * steepness_weight;
        // Folded: the crests of the noise become sharp ridges.
        const float ridged = 1.0F - std::abs(2.0F * sample.value - 1.0F);
        total += strength * ridged / (1.0F + slope_x * slope_x + slope_z * slope_z);
        strength *= 0.5F;
        // Each layer twice as fine and turned, so the layers do not line up.
        const float turned_x = 1.6F * x - 1.2F * z;
        z = 1.2F * x + 1.6F * z;
        x = turned_x;
    }
    return std::min(total / 0.62F, 1.0F);
}

constexpr float pass_floor = 0.07F; // of the mountains' height that is left in a pass
constexpr float far_away = 1.0e9F;
constexpr float full_turn = 2.0F * std::numbers::pi_v<float>;

// The smallest turn from one direction to another, in radians, without its sign.
float turn_between(float a, float b) {
    return std::abs(std::remainder(a - b, full_turn));
}

// Metres from a place to a line between two points.
float distance_to_segment(float x, float z, const IslandPoint& a, const IslandPoint& b) {
    const float along_x = b.x - a.x;
    const float along_z = b.z - a.z;
    const float length_squared = along_x * along_x + along_z * along_z;
    const float t = length_squared > 0.0F
                        ? std::clamp(((x - a.x) * along_x + (z - a.z) * along_z) / length_squared, 0.0F, 1.0F)
                        : 0.0F;
    return std::hypot(x - (a.x + along_x * t), z - (a.z + along_z * t));
}

// The mountains alone, in metres above the land they stand on.
float mountain(const IslandParams& params, float x, float z) {
    if (params.mountain_height <= 0.0F) {
        return 0.0F;
    }
    const float radius = std::max(params.mountain_radius, 1.0F);
    const float half_length = std::max(params.mountain_length, 0.0F) * 0.5F;
    const bool range = half_length > 0.0F;
    const float along_x = std::cos(params.mountain_direction);
    const float along_z = std::sin(params.mountain_direction);
    const float along = (x - params.mountain_x) * along_x + (z - params.mountain_z) * along_z;
    float across = (z - params.mountain_z) * along_x - (x - params.mountain_x) * along_z;
    float crest = 1.0F;
    if (range) {
        // The crest does not run straight, and it rises to peaks and drops to saddles.
        across -= radius * 0.5F * (2.0F * value_noise(params.seed + 71U, along / (radius * 1.3F), 0.5F) - 1.0F);
        const float peaks = value_noise(params.seed + 89U, along / (radius * 0.6F), 3.5F);
        crest = 0.45F + 0.55F * smooth(peaks);
        // Lower towards both ends.
        crest *= std::lerp(0.55F, 1.0F, smooth_step(half_length, half_length * 0.5F, std::abs(along)));
        // A pass: the crest drops almost to the lowland, wide enough to walk through.
        if (params.pass_width > 0.0F) {
            const float in_pass =
                smooth_step(params.pass_width, params.pass_width * 0.35F, std::abs(along - params.pass_along));
            crest *= std::lerp(1.0F, pass_floor, in_pass);
        }
    }
    const float past_the_end = std::abs(along) - std::min(std::abs(along), half_length);
    const float from_peak = std::hypot(past_the_end, across) / radius;
    if (from_peak >= 1.0F) {
        return 0.0F;
    }
    // Half a pointed cone, half a bell with a rounded top, running out gently at the foot.
    const float inward = 1.0F - from_peak;
    const float bell = (1.0F - from_peak * from_peak) * (1.0F - from_peak * from_peak);
    const float cone = 0.5F * inward * inward * (0.55F + 0.45F * inward) + 0.5F * bell;
    // The relief: ridges, gullies and spurs, a third of the mountain's width across.
    const float relief = eroded_noise(params.seed + 31U, x / (radius * 0.55F), z / (radius * 0.55F));
    // A single peak keeps its top where it was put; on a range the crest itself is jagged.
    const float carved = range ? 1.0F : smooth_step(0.0F, 0.35F, from_peak);
    const float shape = cone * std::lerp(1.0F, 0.42F + 0.58F * relief, carved);
    return params.mountain_height * crest * shape;
}

} // namespace

RegionShares island_regions(const IslandParams& params, float x, float z) noexcept {
    RegionShares shares;
    if (params.regions.empty()) {
        return shares;
    }
    // One wandering of the border for all regions, so that two that touch do not overlap.
    const float wander = 2.0F * layered_noise(params.seed + 251U, x / 55.0F, z / 55.0F) - 1.0F;
    for (const IslandRegion& region : params.regions) {
        const float from_middle = std::hypot(x - region.x, z - region.z) + wander * region.radius * 0.28F;
        const float share = smooth_step(region.radius * 1.1F, region.radius * 0.8F, from_middle);
        switch (region.kind) {
        case IslandRegionKind::Pine:
            shares.pine = std::max(shares.pine, share);
            break;
        case IslandRegionKind::Dry:
            shares.dry = std::max(shares.dry, share);
            break;
        case IslandRegionKind::Marsh:
            shares.marsh = std::max(shares.marsh, share);
            break;
        case IslandRegionKind::Meadow:
            break;
        }
    }
    // Where two meet, the stronger keeps what the weaker leaves it.
    const float total = shares.pine + shares.dry + shares.marsh;
    if (total > 1.0F) {
        shares.pine /= total;
        shares.dry /= total;
        shares.marsh /= total;
    }
    return shares;
}

IslandRegionKind island_region(const IslandParams& params, float x, float z) noexcept {
    const RegionShares shares = island_regions(params, x, z);
    const float meadow = 1.0F - shares.pine - shares.dry - shares.marsh;
    const float most = std::max({meadow, shares.pine, shares.dry, shares.marsh});
    if (most == shares.pine) {
        return IslandRegionKind::Pine;
    }
    if (most == shares.dry) {
        return IslandRegionKind::Dry;
    }
    if (most == shares.marsh) {
        return IslandRegionKind::Marsh;
    }
    return IslandRegionKind::Meadow;
}

float island_river_distance(const IslandParams& params, float x, float z) noexcept {
    float nearest = far_away;
    for (std::size_t index = 1; index < params.river.size(); ++index) {
        nearest = std::min(nearest, distance_to_segment(x, z, params.river[index - 1], params.river[index]));
    }
    if (nearest < far_away) {
        // It does not run as straight as it was drawn.
        nearest += 3.0F * (2.0F * value_noise(params.seed + 211U, x / 23.0F, z / 23.0F) - 1.0F);
    }
    return nearest;
}

float island_lake_distance(const IslandParams& params, float x, float z) noexcept {
    float nearest = far_away;
    for (const IslandDisc& lake : params.lakes) {
        // Not a circle: the shore wanders in and out by a fifth.
        const float wobble =
            1.0F +
            0.2F * (2.0F * value_noise(params.seed + 223U, x / (lake.radius * 0.8F), z / (lake.radius * 0.8F)) - 1.0F);
        nearest = std::min(nearest, std::hypot(x - lake.x, z - lake.z) - lake.radius * wobble);
    }
    return nearest;
}

namespace {

// The lowland with its own relief added: `ground` is the land so far, `share` how much of
// the relief this place takes (none in the clearing, none on the mountains).
float with_lowland_relief(const IslandParams& params, float x, float z, float ground, float share) {
    if (share <= 0.0F) {
        return ground;
    }
    float rise = 0.0F;
    float cut = 0.0F;
    if (params.rolling > 0.0F) {
        rise += params.rolling * (2.0F * layered_noise(params.seed + 307U, x / 46.0F, z / 46.0F) - 1.0F);
    }
    // Neither a cone nor a wall: tops are uneven and outlines wander.
    const float uneven = 0.7F + 0.6F * value_noise(params.seed + 313U, x / 37.0F, z / 37.0F);
    const float wander = 2.0F * value_noise(params.seed + 311U, x / 41.0F, z / 41.0F) - 1.0F;
    for (const IslandHill& hill : params.hills) {
        const float away = std::hypot(x - hill.x, z - hill.z) + wander * hill.radius * 0.2F;
        if (away < hill.radius) {
            rise += hill.height * uneven * smooth_step(hill.radius, 0.0F, away);
        }
    }
    for (const IslandRidge& ridge : params.ridges) {
        float away = far_away;
        for (std::size_t index = 1; index < ridge.points.size(); ++index) {
            away = std::min(away, distance_to_segment(x, z, ridge.points[index - 1], ridge.points[index]));
        }
        away += wander * ridge.width * 0.22F;
        if (away < ridge.width) {
            const float shape = smooth_step(ridge.width, 0.0F, away);
            if (ridge.height >= 0.0F) {
                rise += ridge.height * uneven * shape;
            } else {
                cut = std::max(cut, -ridge.height * shape);
            }
        }
    }
    float height = ground + rise * share;
    if (cut > 0.0F) {
        // A valley does not go under water: its floor stays a stride above the sea.
        constexpr float driest = 1.2F;
        height = std::max(height - cut * share, std::min(height, driest));
    }
    return height;
}

// The land as hills, mountains and regions make it, before anything is levelled or cut
// into it, and how much of a place is land at all.
struct OpenGround {
    float inland = 0.0F;
    float land = 0.0F;       // 1 inland, 0 from the coast outwards
    float from_coast = 0.0F; // 0 at the centre, 1 at the coast
};

OpenGround open_ground(const IslandParams& params, float x, float z) noexcept {
    const float distance = std::hypot(x, z);
    const float hill_size = std::max(params.hill_size, 1.0F);
    const float direction = std::atan2(z, x);

    // The coast is not a circle: its distance from the centre varies with the direction.
    const float coast_noise = layered_noise(params.seed + 7U, x / (hill_size * 1.5F), z / (hill_size * 1.5F));
    float coast = params.radius * (0.82F + 0.36F * coast_noise);
    // Bays draw it in.
    for (const IslandBay& bay : params.bays) {
        coast *= 1.0F - bay.depth * smooth_step(bay.width, 0.0F, turn_between(direction, bay.direction));
    }
    // Cliffs: how much of one this stretch of coast is (0 a beach, 1 a cliff), fading over a
    // tenth of a turn at both ends of the stretch.
    float cliff = 0.0F;
    if (params.cliff_height > 0.0F) {
        const float span = std::fmod(params.cliff_to - params.cliff_from + 2.0F * full_turn, full_turn);
        const float into = std::fmod(direction - params.cliff_from + 2.0F * full_turn, full_turn);
        constexpr float fade = 0.3F;
        cliff = into < span ? std::min(smooth_step(0.0F, fade, into), smooth_step(span, span - fade, into)) : 0.0F;
    }
    const float from_coast = distance / std::max(coast, 1.0F);
    // 1 inland, falling to 0 at the coast and staying 0 out at sea. A cliff falls at once.
    const float land = smooth_step(1.0F, std::lerp(0.72F, 0.955F, cliff), from_coast);

    // Hills: squared noise gives broad lowlands with a few real hills.
    const float hills = layered_noise(params.seed, x / hill_size, z / hill_size);
    // A clearing in the middle, for arriving and for building.
    const float away_from_centre = smooth_step(params.clearing_radius, params.clearing_radius * 2.5F, distance);
    const float mountains = mountain(params, x, z);
    float inland = params.lowland_height + (params.hill_height * hills * hills + mountains) * away_from_centre;
    inland = with_lowland_relief(params, x, z, inland, away_from_centre * smooth_step(10.0F, 0.0F, mountains));
    if (!params.regions.empty()) {
        const RegionShares shares = island_regions(params, x, z);
        // Dry highland stands above the land around it, in two broad steps.
        if (shares.dry > 0.0F) {
            const float steps =
                5.0F + 3.0F * smooth_step(0.45F, 0.6F, value_noise(params.seed + 263U, x / 34.0F, z / 34.0F));
            inland += steps * shares.dry * away_from_centre;
        }
        // A marsh is flat and barely above the water, with pools where the noise dips.
        if (shares.marsh > 0.0F) {
            const float pools = layered_noise(params.seed + 271U, x / 16.0F, z / 16.0F);
            // Hummocks a man's height above the water at most, and pools between them.
            const float wet_ground = 0.5F + 3.2F * (pools - 0.45F);
            inland = std::lerp(inland, std::min(inland, wet_ground), shares.marsh);
        }
    }
    // The land rises towards a cliff's edge, so that it has something to fall from.
    inland += params.cliff_height * cliff * smooth_step(0.55F, 0.9F, from_coast);
    return {.inland = inland, .land = land, .from_coast = from_coast};
}

} // namespace

float island_height(const IslandParams& params, float x, float z) noexcept {
    const OpenGround open = open_ground(params, x, z);
    float land = open.land;
    const float from_coast = open.from_coast;
    float inland = open.inland;
    // Building ground is levelled before the water is cut, so that a river still runs
    // through a site laid across it.
    for (const IslandDisc& site : params.sites) {
        const float away = std::hypot(x - site.x, z - site.z);
        const float reach = site.radius * 1.5F;
        if (away < reach) {
            // Level means level: near the coast the land already falls towards the beach
            // (`land` below 1), and a site there sloped with it, a metre from one side to the
            // other: boulders set on it hung in the air. The fall is taken from its middle too.
            const OpenGround middle = open_ground(params, site.x, site.z);
            const float within = smooth_step(reach, site.radius, away);
            inland = std::lerp(inland, middle.inland, within);
            land = std::lerp(land, middle.land, within);
        }
    }

    // Water inland: cut down to below the sea. The banks are wider where the land is high,
    // so that a river through hills has a valley and not a trench.
    const float bed = -params.water_depth;
    if (inland > bed) {
        const float banks = 8.0F + std::max(inland, 0.0F) * 2.2F;
        float wet = 0.0F;
        if (!params.river.empty()) {
            const float half_width = params.river_width * 0.5F;
            wet = smooth_step(half_width + banks, half_width, island_river_distance(params, x, z));
        }
        if (!params.lakes.empty()) {
            wet = std::max(wet, smooth_step(banks, 0.0F, island_lake_distance(params, x, z)));
        }
        // The banks come down gently and only the last of the way goes under: the water is
        // as wide as it was drawn, not as wide as its valley.
        inland = std::lerp(inland, bed, wet * wet * wet);
    }

    // Beyond the coast the ground slopes on down to the sea floor.
    const float out_at_sea = smooth_step(1.0F, 1.5F, from_coast);
    const float shore = std::lerp(-0.6F, params.sea_floor, out_at_sea);
    float height = std::lerp(shore, inland, land);

    // Islets: small rounded bits of land out at sea, with a beach round each.
    for (const IslandDisc& islet : params.islets) {
        const float from_middle = std::hypot(x - islet.x, z - islet.z) / std::max(islet.radius, 1.0F);
        if (from_middle < 1.6F) {
            const float rise = smooth_step(1.6F, 0.25F, from_middle);
            const float rough = 0.75F + 0.5F * value_noise(params.seed + 239U, x / 9.0F, z / 9.0F);
            height = std::max(height, std::lerp(params.sea_floor, params.islet_height * rough, rise));
        }
    }
    return height;
}

float island_slope(const IslandParams& params, float x, float z, float step) noexcept {
    const float rise_x = island_height(params, x + step, z) - island_height(params, x - step, z);
    const float rise_z = island_height(params, x, z + step) - island_height(params, x, z - step);
    return std::hypot(rise_x, rise_z) / (2.0F * step);
}

} // namespace e5::gameplay
#include "e5/gameplay/island.hpp"

#include <algorithm>
#include <cmath>

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

float island_height(const IslandParams& params, float x, float z) noexcept {
    const float distance = std::hypot(x, z);
    const float hill_size = std::max(params.hill_size, 1.0F);

    // The coast is not a circle: its distance from the centre varies with the direction.
    const float coast_noise = layered_noise(params.seed + 7U, x / (hill_size * 1.5F), z / (hill_size * 1.5F));
    const float coast = params.radius * (0.82F + 0.36F * coast_noise);
    // 1 inland, falling to 0 at the coast and staying 0 out at sea.
    const float land = smooth_step(1.0F, 0.72F, distance / std::max(coast, 1.0F));

    // Hills: squared noise gives broad lowlands with a few real hills.
    const float hills = layered_noise(params.seed, x / hill_size, z / hill_size);
    // A clearing in the middle, for arriving and for building.
    const float away_from_centre = smooth_step(params.clearing_radius, params.clearing_radius * 2.5F, distance);
    const float inland =
        params.lowland_height + (params.hill_height * hills * hills + mountain(params, x, z)) * away_from_centre;

    // Beyond the coast the ground slopes on down to the sea floor.
    const float out_at_sea = smooth_step(1.0F, 1.5F, distance / std::max(coast, 1.0F));
    const float shore = std::lerp(-0.6F, params.sea_floor, out_at_sea);
    return std::lerp(shore, inland, land);
}

float island_slope(const IslandParams& params, float x, float z, float step) noexcept {
    const float rise_x = island_height(params, x + step, z) - island_height(params, x - step, z);
    const float rise_z = island_height(params, x, z + step) - island_height(params, x, z - step);
    return std::hypot(rise_x, rise_z) / (2.0F * step);
}

} // namespace e5::gameplay
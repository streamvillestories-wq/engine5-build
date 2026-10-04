#pragma once

#include <cstdint>
#include <vector>

namespace e5::gameplay {

// Where the plants of a forest stand. Deterministic: the same parameters give
// the same forest on every machine and on every run, without storing a single
// position. Plants sit on a jittered grid, so they never crowd each other.
struct ScatterParams {
    float width = 60.0F;          // metres along x, centred on the origin
    float depth = 60.0F;          // metres along z
    float spacing = 6.0F;         // metres between neighbours on average
    float jitter = 0.35F;         // 0 = a regular grid, 0.5 = as irregular as the spacing allows
    float clearing_radius = 0.0F; // metres around the origin kept free
    float scale_min = 0.85F;
    float scale_max = 1.2F;
    int species_count = 1; // kinds of plant to choose between
    std::uint32_t seed = 1;
};

struct ScatterPoint {
    float x = 0.0F;
    float z = 0.0F;
    float yaw = 0.0F;   // radians
    float scale = 1.0F; // uniform
    int species = 0;    // 0 .. species_count - 1
};

[[nodiscard]] std::vector<ScatterPoint> scatter_plants(const ScatterParams& params);

} // namespace e5::gameplay
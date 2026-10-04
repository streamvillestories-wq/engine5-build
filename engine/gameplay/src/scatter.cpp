#include "e5/gameplay/scatter.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace e5::gameplay {
namespace {

// A well-mixed integer hash (PCG output permutation); the same on every platform.
std::uint32_t hash(std::uint32_t value) {
    value = value * 747796405U + 2891336453U;
    const std::uint32_t word = ((value >> ((value >> 28U) + 4U)) ^ value) * 277803737U;
    return (word >> 22U) ^ word;
}

// A number in [0, 1) for a grid cell and a purpose (`channel`).
float unit(std::uint32_t seed, int column, int row, std::uint32_t channel) {
    const std::uint32_t mixed = hash(seed ^ hash(static_cast<std::uint32_t>(column) * 0x9E3779B9U ^
                                                 hash(static_cast<std::uint32_t>(row) * 0x85EBCA6BU ^ hash(channel))));
    return static_cast<float>(mixed >> 8U) / 16777216.0F;
}

} // namespace

std::vector<ScatterPoint> scatter_plants(const ScatterParams& params) {
    std::vector<ScatterPoint> points;
    if (params.spacing <= 0.0F || params.width <= 0.0F || params.depth <= 0.0F || params.species_count <= 0) {
        return points;
    }
    const int columns = std::max(static_cast<int>(std::floor(params.width / params.spacing)), 1);
    const int rows = std::max(static_cast<int>(std::floor(params.depth / params.spacing)), 1);
    const float jitter = std::clamp(params.jitter, 0.0F, 0.5F) * params.spacing;
    points.reserve(static_cast<std::size_t>(columns) * static_cast<std::size_t>(rows));

    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const float cell_x =
                (static_cast<float>(column) + 0.5F - static_cast<float>(columns) * 0.5F) * params.spacing;
            const float cell_z = (static_cast<float>(row) + 0.5F - static_cast<float>(rows) * 0.5F) * params.spacing;
            const float x = cell_x + (unit(params.seed, column, row, 0) * 2.0F - 1.0F) * jitter;
            const float z = cell_z + (unit(params.seed, column, row, 1) * 2.0F - 1.0F) * jitter;
            if (std::hypot(x, z) < params.clearing_radius) {
                continue;
            }
            const float scale_range = std::max(params.scale_max - params.scale_min, 0.0F);
            points.push_back({
                .x = x,
                .z = z,
                .yaw = unit(params.seed, column, row, 2) * 2.0F * std::numbers::pi_v<float>,
                .scale = params.scale_min + unit(params.seed, column, row, 3) * scale_range,
                .species = std::min(
                    static_cast<int>(unit(params.seed, column, row, 4) * static_cast<float>(params.species_count)),
                    params.species_count - 1),
            });
        }
    }
    return points;
}

} // namespace e5::gameplay
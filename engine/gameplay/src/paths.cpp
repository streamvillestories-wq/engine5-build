#include "e5/gameplay/paths.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace e5::gameplay {
namespace {

// One coordinate of a Catmull-Rom curve between `from` and `to`; `before` and `after` are their neighbours.
float curve(float before, float from, float to, float after, float t) {
    const float t2 = t * t;
    const float t3 = t2 * t;
    return 0.5F * ((2.0F * from) + (to - before) * t + (2.0F * before - 5.0F * from + 4.0F * to - after) * t2 +
                   (3.0F * from - before - 3.0F * to + after) * t3);
}

} // namespace

std::vector<PathPoint> smooth_path(std::span<const PathPoint> points, float step) {
    if (points.size() < 2) {
        return {points.begin(), points.end()};
    }
    std::vector<PathPoint> smoothed;
    const float spacing = std::max(step, 0.05F);
    for (std::size_t index = 0; index + 1 < points.size(); ++index) {
        // At the ends the missing neighbour is the end itself.
        const PathPoint& before = points[index == 0 ? 0 : index - 1];
        const PathPoint& from = points[index];
        const PathPoint& to = points[index + 1];
        const PathPoint& after = points[std::min(index + 2, points.size() - 1)];
        const float length = std::hypot(to.x - from.x, to.z - from.z);
        const int pieces = std::max(static_cast<int>(std::ceil(length / spacing)), 1);
        for (int piece = 0; piece < pieces; ++piece) {
            const float t = static_cast<float>(piece) / static_cast<float>(pieces);
            smoothed.push_back(
                {.x = curve(before.x, from.x, to.x, after.x, t), .z = curve(before.z, from.z, to.z, after.z, t)});
        }
    }
    smoothed.push_back(points.back());
    return smoothed;
}

float distance_to_path(std::span<const PathPoint> path, float x, float z) noexcept {
    if (path.size() < 2) {
        return std::numeric_limits<float>::infinity();
    }
    float nearest_squared = std::numeric_limits<float>::max();
    for (std::size_t index = 0; index + 1 < path.size(); ++index) {
        const float along_x = path[index + 1].x - path[index].x;
        const float along_z = path[index + 1].z - path[index].z;
        const float length_squared = along_x * along_x + along_z * along_z;
        // How far along this piece the nearest place lies, kept within the piece.
        const float t =
            length_squared > 0.0F
                ? std::clamp(((x - path[index].x) * along_x + (z - path[index].z) * along_z) / length_squared, 0.0F,
                             1.0F)
                : 0.0F;
        const float off_x = x - (path[index].x + along_x * t);
        const float off_z = z - (path[index].z + along_z * t);
        nearest_squared = std::min(nearest_squared, off_x * off_x + off_z * off_z);
    }
    return std::sqrt(nearest_squared);
}

} // namespace e5::gameplay

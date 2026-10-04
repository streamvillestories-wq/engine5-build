#pragma once

#include <span>
#include <vector>

namespace e5::gameplay {

// Paths on the ground: dirt roads, trails. A path is a line through a few
// hand-placed points; this turns it into a smooth curve and answers how far a
// place is from it. Like the island's shape, a path is a handful of numbers
// that give the same result on every machine.
struct PathPoint {
    float x = 0.0F;
    float z = 0.0F;
};

// A smooth curve through the given points (Catmull-Rom), as points about
// `step` metres apart. It passes through every given point. Fewer than two
// points give back what was given.
[[nodiscard]] std::vector<PathPoint> smooth_path(std::span<const PathPoint> points, float step);

// Metres from (x, z) to the nearest place on the line through `path`.
// A path of fewer than two points is infinitely far away.
[[nodiscard]] float distance_to_path(std::span<const PathPoint> path, float x, float z) noexcept;

} // namespace e5::gameplay

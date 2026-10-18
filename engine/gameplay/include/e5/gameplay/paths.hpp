#pragma once

#include <cstdint>
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

// Many paths at once, for asking often how far the nearest is (a forest asks for every tree it may
// plant). The pieces of every path are put into square cells; a question looks at the cells
// round the place, ring by ring, and stops as soon as nothing further out can be nearer. Gives the
// same answer as asking every path with distance_to_path (the island's roads made that a question
// of minutes for its forests once it had thirty of them).
class PathIndex {
public:
    // `paths` must outlive the index; `cell` is in metres.
    void build(std::span<const std::vector<PathPoint>> paths, float cell = 32.0F);
    // Metres to the nearest of the paths numbered below `path_limit` (all of them if negative);
    // infinitely far if there is none.
    [[nodiscard]] float nearest(float x, float z, int path_limit = -1) const noexcept;

private:
    struct Piece {
        std::uint32_t path = 0;
        std::uint32_t index = 0; // the piece from point `index` to `index + 1`
    };
    [[nodiscard]] float piece_distance(const Piece& piece, float x, float z) const noexcept;

    std::span<const std::vector<PathPoint>> paths_;
    std::vector<std::vector<Piece>> cells_;
    float cell_ = 32.0F;
    float left_ = 0.0F;
    float top_ = 0.0F;
    int columns_ = 0;
    int rows_ = 0;
};

} // namespace e5::gameplay

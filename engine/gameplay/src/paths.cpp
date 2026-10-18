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

void PathIndex::build(std::span<const std::vector<PathPoint>> paths, float cell) {
    paths_ = paths;
    cell_ = cell;
    cells_.clear();
    float left = std::numeric_limits<float>::max();
    float top = std::numeric_limits<float>::max();
    float right = std::numeric_limits<float>::lowest();
    float bottom = std::numeric_limits<float>::lowest();
    for (const std::vector<PathPoint>& path : paths) {
        for (const PathPoint& point : path) {
            left = std::min(left, point.x);
            right = std::max(right, point.x);
            top = std::min(top, point.z);
            bottom = std::max(bottom, point.z);
        }
    }
    if (left > right) {
        columns_ = 0;
        rows_ = 0;
        return;
    }
    left_ = left;
    top_ = top;
    columns_ = static_cast<int>((right - left) / cell_) + 1;
    rows_ = static_cast<int>((bottom - top) / cell_) + 1;
    cells_.resize(static_cast<std::size_t>(columns_) * static_cast<std::size_t>(rows_));
    for (std::size_t path = 0; path < paths.size(); ++path) {
        const std::vector<PathPoint>& points = paths[path];
        for (std::size_t index = 0; index + 1 < points.size(); ++index) {
            const PathPoint& a = points[index];
            const PathPoint& b = points[index + 1];
            const int from_column = static_cast<int>((std::min(a.x, b.x) - left_) / cell_);
            const int to_column = static_cast<int>((std::max(a.x, b.x) - left_) / cell_);
            const int from_row = static_cast<int>((std::min(a.z, b.z) - top_) / cell_);
            const int to_row = static_cast<int>((std::max(a.z, b.z) - top_) / cell_);
            for (int row = from_row; row <= to_row; ++row) {
                for (int column = from_column; column <= to_column; ++column) {
                    cells_[static_cast<std::size_t>(row) * static_cast<std::size_t>(columns_) +
                           static_cast<std::size_t>(column)]
                        .push_back(
                            {.path = static_cast<std::uint32_t>(path), .index = static_cast<std::uint32_t>(index)});
                }
            }
        }
    }
}

float PathIndex::piece_distance(const Piece& piece, float x, float z) const noexcept {
    const std::vector<PathPoint>& points = paths_[piece.path];
    const PathPoint& a = points[piece.index];
    const PathPoint& b = points[piece.index + 1];
    const float along_x = b.x - a.x;
    const float along_z = b.z - a.z;
    const float length_squared = along_x * along_x + along_z * along_z;
    const float t = length_squared > 0.0F
                        ? std::clamp(((x - a.x) * along_x + (z - a.z) * along_z) / length_squared, 0.0F, 1.0F)
                        : 0.0F;
    return std::hypot(x - (a.x + along_x * t), z - (a.z + along_z * t));
}

float PathIndex::nearest(float x, float z, int path_limit) const noexcept {
    float best = std::numeric_limits<float>::infinity();
    if (columns_ == 0) {
        return best;
    }
    const auto limit =
        path_limit < 0 ? std::numeric_limits<std::uint32_t>::max() : static_cast<std::uint32_t>(path_limit);
    // The cell the place is in, or the nearest one to it if it lies outside the grid. Either way,
    // a cell `ring + 1` rings from that one is at least `ring` cells' width away from the place.
    const int centre_column = std::clamp(static_cast<int>(std::floor((x - left_) / cell_)), 0, columns_ - 1);
    const int centre_row = std::clamp(static_cast<int>(std::floor((z - top_) / cell_)), 0, rows_ - 1);
    const int most = std::max(columns_, rows_);
    for (int ring = 0; ring <= most; ++ring) {
        for (int row = centre_row - ring; row <= centre_row + ring; ++row) {
            if (row < 0 || row >= rows_) {
                continue;
            }
            const bool edge_row = row == centre_row - ring || row == centre_row + ring;
            for (int column = centre_column - ring; column <= centre_column + ring;
                 column += edge_row ? 1 : std::max(2 * ring, 1)) {
                if (column < 0 || column >= columns_) {
                    continue;
                }
                for (const Piece& piece : cells_[static_cast<std::size_t>(row) * static_cast<std::size_t>(columns_) +
                                                 static_cast<std::size_t>(column)]) {
                    if (piece.path < limit) {
                        best = std::min(best, piece_distance(piece, x, z));
                    }
                }
            }
        }
        // Everything not yet looked at lies in a cell at least `ring` cells away from the centre cell.
        if (best <= static_cast<float>(ring) * cell_) {
            break;
        }
    }
    return best;
}

} // namespace e5::gameplay

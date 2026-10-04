#include "e5/gameplay/trails.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <queue>
#include <utility>

namespace e5::gameplay {
namespace {

float smooth_step(float low, float high, float value) noexcept {
    const float t = std::clamp((value - low) / (high - low), 0.0F, 1.0F);
    return t * t * (3.0F - 2.0F * t);
}

} // namespace

namespace {

// The heights along one trail. `start` and `end`, when given, are the heights its two ends
// must have (where it leaves or meets another trail); otherwise they keep the ground's.
std::vector<float> heights_along(std::span<const PathPoint> trail, const GroundHeight& ground, float max_grade,
                                 const float* start, const float* end) {
    const std::size_t count = trail.size();
    std::vector<float> uncut(count);
    for (std::size_t index = 0; index < count; ++index) {
        uncut[index] = ground(trail[index].x, trail[index].z);
    }
    if (count < 2) {
        return uncut;
    }
    if (start != nullptr) {
        uncut.front() = *start;
    }
    if (end != nullptr) {
        uncut.back() = *end;
    }
    if (count < 3) {
        return uncut;
    }
    // First evened out over a few metres, so that the trail does not follow every bump. The
    // stretch it is evened over reaches equally far both ways and so shrinks to nothing at
    // the ends, which keep their heights.
    {
        constexpr std::size_t reach = 4;
        const std::vector<float> as_found = uncut;
        for (std::size_t index = 0; index < count; ++index) {
            const std::size_t either_way = std::min({reach, index, count - 1 - index});
            float sum = 0.0F;
            for (std::size_t other = index - either_way; other <= index + either_way; ++other) {
                sum += as_found[other];
            }
            uncut[index] = sum / static_cast<float>(2 * either_way + 1);
        }
    }
    // Then walked once from each end, never rising or falling faster than allowed; the middle
    // of the two is cut into the heights and filled over the hollows about equally.
    std::vector<float> forward = uncut;
    std::vector<float> backward = uncut;
    for (std::size_t index = 1; index < count; ++index) {
        const float run = std::hypot(trail[index].x - trail[index - 1].x, trail[index].z - trail[index - 1].z);
        forward[index] =
            std::clamp(uncut[index], forward[index - 1] - max_grade * run, forward[index - 1] + max_grade * run);
    }
    for (std::size_t index = count - 1; index-- > 0;) {
        const float run = std::hypot(trail[index].x - trail[index + 1].x, trail[index].z - trail[index + 1].z);
        backward[index] =
            std::clamp(uncut[index], backward[index + 1] - max_grade * run, backward[index + 1] + max_grade * run);
    }
    std::vector<float> heights(count);
    for (std::size_t index = 0; index < count; ++index) {
        // An end that has to keep its height is where the walk from that end counts alone; an
        // end that is free takes the middle of the two, like the rest.
        const float along = static_cast<float>(index) / static_cast<float>(count - 1);
        float from_the_end = 0.5F;
        if (start != nullptr && end != nullptr) {
            from_the_end = along;
        } else if (start != nullptr) {
            from_the_end = std::min(along * 4.0F, 0.5F);
        } else if (end != nullptr) {
            from_the_end = std::max(1.0F - (1.0F - along) * 4.0F, 0.5F);
        }
        heights[index] = std::lerp(forward[index], backward[index], from_the_end);
    }
    return heights;
}

} // namespace

std::vector<float> trail_heights(std::span<const PathPoint> trail, const GroundHeight& ground, float max_grade) {
    return heights_along(trail, ground, max_grade, nullptr, nullptr);
}

void TrailField::build(std::span<const std::vector<PathPoint>> trails, const GroundHeight& ground, float side,
                       const TrailParams& params) {
    change_.clear();
    nodes_ = 0;
    if (trails.empty()) {
        return;
    }
    nodes_ = static_cast<int>(std::ceil(side)) + 1;
    half_ = static_cast<float>(nodes_ - 1) * 0.5F;
    const std::size_t total = static_cast<std::size_t>(nodes_) * static_cast<std::size_t>(nodes_);
    // For each node: how much the trails near it count there (the nearest one's share), and
    // the height they ask for. Where two stretches are both near (a junction, a hairpin) the
    // nearer counts far more, and the height passes smoothly from one to the other.
    std::vector<float> share(total, 0.0F);
    std::vector<float> weight(total, 0.0F);
    std::vector<float> wanted(total, 0.0F);
    const int reach = static_cast<int>(std::ceil(params.shoulder)) + 1;

    // A trail that starts or ends on an earlier one takes that one's height there.
    constexpr float junction_reach = 2.5F;
    std::vector<std::vector<float>> built;
    const auto height_on_earlier = [&](PathPoint place, float& found) {
        float nearest = junction_reach;
        bool any = false;
        for (std::size_t earlier = 0; earlier < built.size(); ++earlier) {
            for (std::size_t index = 0; index < trails[earlier].size(); ++index) {
                const float away = std::hypot(trails[earlier][index].x - place.x, trails[earlier][index].z - place.z);
                if (away < nearest) {
                    nearest = away;
                    found = built[earlier][index];
                    any = true;
                }
            }
        }
        return any;
    };

    for (const std::vector<PathPoint>& trail : trails) {
        if (trail.size() < 2) {
            built.emplace_back();
            continue;
        }
        float start_height = 0.0F;
        float end_height = 0.0F;
        const bool start_joins = height_on_earlier(trail.front(), start_height);
        const bool end_joins = height_on_earlier(trail.back(), end_height);
        const std::vector<float> heights = heights_along(
            trail, ground, params.max_grade, start_joins ? &start_height : nullptr, end_joins ? &end_height : nullptr);
        for (std::size_t index = 0; index + 1 < trail.size(); ++index) {
            const PathPoint a = trail[index];
            const PathPoint b = trail[index + 1];
            const float along_x = b.x - a.x;
            const float along_z = b.z - a.z;
            const float length_squared = std::max(along_x * along_x + along_z * along_z, 1.0e-6F);
            const int first_column = std::max(static_cast<int>(std::floor(std::min(a.x, b.x) + half_)) - reach, 0);
            const int last_column =
                std::min(static_cast<int>(std::ceil(std::max(a.x, b.x) + half_)) + reach, nodes_ - 1);
            const int first_row = std::max(static_cast<int>(std::floor(std::min(a.z, b.z) + half_)) - reach, 0);
            const int last_row = std::min(static_cast<int>(std::ceil(std::max(a.z, b.z) + half_)) + reach, nodes_ - 1);
            for (int row = first_row; row <= last_row; ++row) {
                for (int column = first_column; column <= last_column; ++column) {
                    const float x = static_cast<float>(column) - half_;
                    const float z = static_cast<float>(row) - half_;
                    const float t =
                        std::clamp(((x - a.x) * along_x + (z - a.z) * along_z) / length_squared, 0.0F, 1.0F);
                    const float away = std::hypot(x - (a.x + along_x * t), z - (a.z + along_z * t));
                    if (away >= params.shoulder) {
                        continue;
                    }
                    const std::size_t node = static_cast<std::size_t>(row) * static_cast<std::size_t>(nodes_) +
                                             static_cast<std::size_t>(column);
                    const float here = 1.0F - smooth_step(params.half_width, params.shoulder, away);
                    // To the sixth power: of two stretches the nearer all but decides.
                    const float counts = here * here * here * here * here * here + 1.0e-6F;
                    share[node] = std::max(share[node], here);
                    weight[node] += counts;
                    wanted[node] += counts * std::lerp(heights[index], heights[index + 1], t);
                }
            }
        }
        built.push_back(heights);
    }

    change_.assign(total, 0.0F);
    for (int row = 0; row < nodes_; ++row) {
        for (int column = 0; column < nodes_; ++column) {
            const std::size_t node =
                static_cast<std::size_t>(row) * static_cast<std::size_t>(nodes_) + static_cast<std::size_t>(column);
            if (weight[node] <= 0.0F) {
                continue;
            }
            const float uncut = ground(static_cast<float>(column) - half_, static_cast<float>(row) - half_);
            change_[node] = share[node] * (wanted[node] / weight[node] - uncut);
        }
    }
}

float TrailField::apply(float x, float z, float uncut) const noexcept {
    if (change_.empty()) {
        return uncut;
    }
    const float column = x + half_;
    const float row = z + half_;
    if (column < 0.0F || row < 0.0F || column >= static_cast<float>(nodes_ - 1) ||
        row >= static_cast<float>(nodes_ - 1)) {
        return uncut;
    }
    const auto column0 = static_cast<std::size_t>(column);
    const auto row0 = static_cast<std::size_t>(row);
    const float across = column - static_cast<float>(column0);
    const float down = row - static_cast<float>(row0);
    const std::size_t width = static_cast<std::size_t>(nodes_);
    const std::size_t node = row0 * width + column0;
    const float upper = std::lerp(change_[node], change_[node + 1], across);
    const float lower = std::lerp(change_[node + width], change_[node + width + 1], across);
    return uncut + std::lerp(upper, lower, down);
}

std::vector<PathPoint> plan_trail(const GroundHeight& ground, PathPoint from, PathPoint to, float side, float step,
                                  float max_grade, float spacing) {
    const int across = static_cast<int>(side / step) + 1;
    const float half = side * 0.5F;
    const auto cell_of = [&](PathPoint point) {
        return std::pair{std::clamp(static_cast<int>(std::lround((point.x + half) / step)), 0, across - 1),
                         std::clamp(static_cast<int>(std::lround((point.z + half) / step)), 0, across - 1)};
    };
    const auto place_of = [&](int column, int row) {
        return PathPoint{.x = static_cast<float>(column) * step - half, .z = static_cast<float>(row) * step - half};
    };
    const std::size_t total = static_cast<std::size_t>(across) * static_cast<std::size_t>(across);
    std::vector<float> heights(total);
    for (int row = 0; row < across; ++row) {
        for (int column = 0; column < across; ++column) {
            const PathPoint place = place_of(column, row);
            heights[static_cast<std::size_t>(row) * static_cast<std::size_t>(across) +
                    static_cast<std::size_t>(column)] = ground(place.x, place.z);
        }
    }

    // Planned on the broad shape of the ground, not on every bump: a trail is cut into the
    // heights and filled over the hollows, so what matters is the slope over some metres.
    // Water is told from the ground as it is.
    const std::vector<float> as_it_is = heights;
    for (int pass = 0; pass < 2; ++pass) {
        const std::vector<float> before = heights;
        for (int row = 0; row < across; ++row) {
            for (int column = 0; column < across; ++column) {
                float sum = 0.0F;
                int taken = 0;
                for (int other_row = std::max(row - 2, 0); other_row <= std::min(row + 2, across - 1); ++other_row) {
                    for (int other_column = std::max(column - 2, 0); other_column <= std::min(column + 2, across - 1);
                         ++other_column) {
                        sum += before[static_cast<std::size_t>(other_row) * static_cast<std::size_t>(across) +
                                      static_cast<std::size_t>(other_column)];
                        ++taken;
                    }
                }
                heights[static_cast<std::size_t>(row) * static_cast<std::size_t>(across) +
                        static_cast<std::size_t>(column)] = sum / static_cast<float>(taken);
            }
        }
    }

    // The ways on from each cell: to its eight neighbours, and long shallow steps of up to
    // six cells one way and one the other. Across a steep slope only a step nearly along the
    // contour is gentle enough, and the steeper the slope, the nearer to the contour it must be.
    std::vector<std::pair<int, int>> moves{{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    for (const int far : {2, 3, 4, 6}) {
        for (const int sign_far : {1, -1}) {
            for (const int sign_near : {1, -1}) {
                moves.emplace_back(far * sign_far, sign_near);
                moves.emplace_back(sign_near, far * sign_far);
            }
        }
    }
    // Which way a step goes, as a unit vector: turning costs, the more the sharper, so that
    // the way runs in long stretches with few hairpins rather than in a tight zigzag.
    const std::size_t ways = moves.size();
    std::vector<std::pair<float, float>> heading(ways);
    for (std::size_t way = 0; way < ways; ++way) {
        const float length = std::hypot(static_cast<float>(moves[way].first), static_cast<float>(moves[way].second));
        heading[way] = {static_cast<float>(moves[way].first) / length, static_cast<float>(moves[way].second) / length};
    }
    constexpr float hairpin_cost = 45.0F; // metres a full turn about is worth

    const auto [start_column, start_row] = cell_of(from);
    const auto [goal_column, goal_row] = cell_of(to);
    const auto cell_index = [across](int column, int row) {
        return static_cast<std::size_t>(row) * static_cast<std::size_t>(across) + static_cast<std::size_t>(column);
    };
    const std::size_t start = cell_index(start_column, start_row);
    const std::size_t goal = cell_index(goal_column, goal_row);

    // A state is a cell and the way it was entered by.
    constexpr float unreached = std::numeric_limits<float>::max();
    constexpr std::uint32_t nowhere = std::numeric_limits<std::uint32_t>::max();
    std::vector<float> cost(total * ways, unreached);
    std::vector<std::uint32_t> came_from(total * ways, nowhere);
    using Entry = std::pair<float, std::uint32_t>; // estimated total, state
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> open;
    const auto left_from = [&](int column, int row) {
        return std::hypot(static_cast<float>(goal_column - column), static_cast<float>(goal_row - row)) * step;
    };
    // From the start every way is open at no cost.
    for (std::size_t way = 0; way < ways; ++way) {
        cost[start * ways + way] = 0.0F;
    }
    open.emplace(left_from(start_column, start_row), static_cast<std::uint32_t>(start * ways));
    std::uint32_t arrived = nowhere;
    while (!open.empty()) {
        const auto [estimate, state] = open.top();
        open.pop();
        const std::size_t cell = state / ways;
        const std::size_t entered_by = state % ways;
        if (cell == goal) {
            arrived = state;
            break;
        }
        const int column = static_cast<int>(cell % static_cast<std::size_t>(across));
        const int row = static_cast<int>(cell / static_cast<std::size_t>(across));
        const float so_far = cost[state];
        if (estimate > so_far + left_from(column, row) + 0.01F) {
            continue; // an older, worse entry for this state
        }
        for (std::size_t way = 0; way < ways; ++way) {
            const int next_column = column + moves[way].first;
            const int next_row = row + moves[way].second;
            if (next_column < 0 || next_row < 0 || next_column >= across || next_row >= across) {
                continue;
            }
            const std::size_t next = cell_index(next_column, next_row);
            if (as_it_is[next] < 0.6F) {
                continue; // water
            }
            const float run =
                std::hypot(static_cast<float>(moves[way].first), static_cast<float>(moves[way].second)) * step;
            const float grade = std::abs(heights[next] - heights[cell]) / run;
            if (grade > max_grade) {
                continue;
            }
            // Gentle ground costs its length; the steeper, the dearer.
            const float relative = grade / max_grade;
            float through = so_far + run * (1.0F + 1.2F * relative * relative);
            if (cell != start) {
                const float same =
                    heading[way].first * heading[entered_by].first + heading[way].second * heading[entered_by].second;
                through += hairpin_cost * 0.5F * (1.0F - same);
            }
            const std::size_t next_state = next * ways + way;
            if (through < cost[next_state]) {
                cost[next_state] = through;
                came_from[next_state] = state;
                open.emplace(through + left_from(next_column, next_row), static_cast<std::uint32_t>(next_state));
            }
        }
    }
    if (arrived == nowhere) {
        return {};
    }
    std::vector<PathPoint> way;
    for (std::uint32_t state = arrived; state != nowhere; state = came_from[state]) {
        const std::size_t cell = state / ways;
        way.push_back(place_of(static_cast<int>(cell % static_cast<std::size_t>(across)),
                               static_cast<int>(cell / static_cast<std::size_t>(across))));
    }
    std::ranges::reverse(way);

    // Thinned to one point every `spacing` metres: a smooth curve is laid through them later.
    std::vector<PathPoint> thinned{from};
    float since = 0.0F;
    for (std::size_t index = 1; index + 1 < way.size(); ++index) {
        since += std::hypot(way[index].x - way[index - 1].x, way[index].z - way[index - 1].z);
        if (since >= spacing) {
            thinned.push_back(way[index]);
            since = 0.0F;
        }
    }
    thinned.push_back(to);
    return thinned;
}

} // namespace e5::gameplay

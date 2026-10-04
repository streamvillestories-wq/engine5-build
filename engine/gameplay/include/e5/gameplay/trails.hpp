#pragma once

#include "e5/gameplay/paths.hpp"

#include <functional>
#include <span>
#include <vector>

namespace e5::gameplay {

// Trails: paths that are cut into the ground, so that a slope too steep to
// climb can be walked along one. A path only colours the ground; a trail
// changes its shape: level from side to side, and never steeper along its
// length than `max_grade`.
//
// Like the island's shape this is worked out from a few numbers (the island's
// parameters and the points the trails run through), the same on every
// machine: nothing is stored or sent.

// The height of the uncut ground at (x, z).
using GroundHeight = std::function<float(float x, float z)>;

struct TrailParams {
    float half_width = 1.8F; // metres to each side of the middle that are level
    float shoulder = 6.5F;   // metres from the middle to where the cut has run out into the slope
    float max_grade = 0.34F; // rise over run along the trail; 0.34 is about 19 degrees
};

// What the trails do to the ground, kept on a grid of one value a metre over a
// square centred on the origin.
class TrailField {
public:
    // `trails`: each a line of points about a metre apart (see smooth_path).
    // `side`: metres across the square the field covers.
    void build(std::span<const std::vector<PathPoint>> trails, const GroundHeight& ground, float side,
               const TrailParams& params);

    // The height at (x, z) with the trails cut in, given the uncut height there.
    [[nodiscard]] float apply(float x, float z, float uncut) const noexcept;

    [[nodiscard]] bool empty() const noexcept { return change_.empty(); }

private:
    int nodes_ = 0; // along one side
    float half_ = 0.0F;
    std::vector<float> change_; // metres the ground is raised (or, negative, lowered) at each node
};

// The heights along one trail: the ground's, evened out so that no stretch is
// steeper than `max_grade`. One for each point of `trail`.
[[nodiscard]] std::vector<float> trail_heights(std::span<const PathPoint> trail, const GroundHeight& ground,
                                               float max_grade);

// Finds a way from one place to another that a trail can take: as short as it
// can be without any stretch steeper than `max_grade` and without entering
// water, preferring gentle ground and few sharp turns. It is planned on the
// broad shape of the ground (heights evened out over some metres), since a
// trail is cut and filled anyway. Looks on a grid `step` metres wide over a
// square `side` metres across centred on the origin. Returns points about
// `spacing` metres apart, first and last the places given; empty if there is
// no way.
[[nodiscard]] std::vector<PathPoint> plan_trail(const GroundHeight& ground, PathPoint from, PathPoint to, float side,
                                                float step, float max_grade, float spacing);

} // namespace e5::gameplay

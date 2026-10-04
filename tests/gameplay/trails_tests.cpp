#include "e5/gameplay/trails.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstddef>
#include <vector>

using Catch::Approx;
using e5::gameplay::PathPoint;
using e5::gameplay::plan_trail;
using e5::gameplay::smooth_path;
using e5::gameplay::trail_heights;
using e5::gameplay::TrailField;

namespace {

// A slope rising one metre for every metre towards +x: 45 degrees, too steep to walk.
float steep_slope(float x, float /*z*/) {
    return 5.0F + x;
}

// A straight line across that slope at an angle, a point a metre.
std::vector<PathPoint> line(PathPoint from, PathPoint to) {
    const std::vector<PathPoint> ends{from, to};
    return smooth_path(ends, 1.0F);
}

} // namespace

TEST_CASE("a trail is never steeper than allowed", "[trails]") {
    const std::vector<PathPoint> trail = line({.x = -20.0F, .z = -20.0F}, {.x = 20.0F, .z = 20.0F});
    const std::vector<float> heights = trail_heights(trail, steep_slope, 0.3F);

    REQUIRE(heights.size() == trail.size());
    for (std::size_t index = 0; index + 1 < trail.size(); ++index) {
        const float run = std::hypot(trail[index + 1].x - trail[index].x, trail[index + 1].z - trail[index].z);
        CHECK(std::abs(heights[index + 1] - heights[index]) <= 0.3F * run + 1.0e-3F);
    }
}

TEST_CASE("a trail is level from side to side and leaves the ground beyond its shoulder alone", "[trails]") {
    // Along the slope's contour: the trail itself is level, the ground beside it is not.
    const std::vector<std::vector<PathPoint>> trails{line({.x = 0.0F, .z = -30.0F}, {.x = 0.0F, .z = 30.0F})};
    TrailField field;
    field.build(trails, steep_slope, 96.0F, {.half_width = 2.0F, .shoulder = 6.0F, .max_grade = 0.3F});

    REQUIRE_FALSE(field.empty());
    // On the trail and a metre and a half to either side: the height of its middle.
    CHECK(field.apply(0.0F, 0.0F, steep_slope(0.0F, 0.0F)) == Approx(5.0F).margin(0.05F));
    CHECK(field.apply(1.5F, 0.0F, steep_slope(1.5F, 0.0F)) == Approx(5.0F).margin(0.1F));
    CHECK(field.apply(-1.5F, 0.0F, steep_slope(-1.5F, 0.0F)) == Approx(5.0F).margin(0.1F));
    // Past the shoulder: untouched.
    CHECK(field.apply(8.0F, 0.0F, steep_slope(8.0F, 0.0F)) == Approx(13.0F));
    CHECK(field.apply(-8.0F, 0.0F, steep_slope(-8.0F, 0.0F)) == Approx(-3.0F));
}

TEST_CASE("without trails the ground stays as it is", "[trails]") {
    const TrailField field;
    CHECK(field.empty());
    CHECK(field.apply(3.0F, 4.0F, 12.5F) == Approx(12.5F));
}

TEST_CASE("a planned trail winds up a slope too steep to climb straight", "[trails]") {
    // Straight up would be a grade of 1; the plan may not be steeper than 0.3 anywhere.
    const std::vector<PathPoint> way =
        plan_trail(steep_slope, {.x = 0.0F, .z = 0.0F}, {.x = 30.0F, .z = 0.0F}, 240.0F, 3.0F, 0.3F, 6.0F);

    REQUIRE(way.size() > 4);
    CHECK(way.front().x == Approx(0.0F));
    CHECK(way.back().x == Approx(30.0F));
    float length = 0.0F;
    for (std::size_t index = 0; index + 1 < way.size(); ++index) {
        length += std::hypot(way[index + 1].x - way[index].x, way[index + 1].z - way[index].z);
    }
    // Thirty metres up at 0.3 takes at least a hundred metres of trail.
    CHECK(length > 95.0F);
}

TEST_CASE("there is no trail through water", "[trails]") {
    const auto sea = [](float /*x*/, float /*z*/) { return -2.0F; };
    CHECK(plan_trail(sea, {.x = 0.0F, .z = 0.0F}, {.x = 30.0F, .z = 0.0F}, 120.0F, 3.0F, 0.3F, 6.0F).empty());
}

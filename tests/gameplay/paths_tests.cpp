#include "e5/gameplay/paths.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

using Catch::Approx;
using e5::gameplay::distance_to_path;
using e5::gameplay::PathPoint;
using e5::gameplay::smooth_path;

TEST_CASE("a smoothed path passes through its points and has no long steps", "[paths]") {
    const std::vector<PathPoint> points{
        {.x = 0.0F, .z = 0.0F}, {.x = 20.0F, .z = 5.0F}, {.x = 30.0F, .z = 30.0F}, {.x = 60.0F, .z = 25.0F}};
    const std::vector<PathPoint> path = smooth_path(points, 1.0F);

    REQUIRE(path.size() > 60);
    CHECK(path.front().x == Approx(0.0F));
    CHECK(path.back().x == Approx(60.0F));
    CHECK(path.back().z == Approx(25.0F));
    for (const PathPoint& point : points) {
        CHECK(distance_to_path(path, point.x, point.z) < 0.01F);
    }
    for (std::size_t index = 0; index + 1 < path.size(); ++index) {
        CHECK(std::hypot(path[index + 1].x - path[index].x, path[index + 1].z - path[index].z) < 1.6F);
    }
}

TEST_CASE("distance to a path is measured to the line, not to its points", "[paths]") {
    const std::vector<PathPoint> path{{.x = 0.0F, .z = 0.0F}, {.x = 10.0F, .z = 0.0F}, {.x = 10.0F, .z = 10.0F}};

    CHECK(distance_to_path(path, 5.0F, 3.0F) == Approx(3.0F));
    CHECK(distance_to_path(path, 5.0F, 0.0F) == Approx(0.0F).margin(0.0001));
    CHECK(distance_to_path(path, 13.0F, 14.0F) == Approx(5.0F)); // past the end: to the end point
    CHECK(distance_to_path(path, -4.0F, 0.0F) == Approx(4.0F));
}

TEST_CASE("too short a path is nowhere", "[paths]") {
    const std::vector<PathPoint> one{{.x = 3.0F, .z = 4.0F}};
    CHECK(std::isinf(distance_to_path(one, 3.0F, 4.0F)));
    CHECK(smooth_path(one, 1.0F).size() == 1);
    CHECK(smooth_path({}, 1.0F).empty());
}

TEST_CASE("the path index answers as asking every path does", "[paths]") {
    using e5::gameplay::distance_to_path;
    using e5::gameplay::PathIndex;
    using e5::gameplay::PathPoint;
    std::vector<std::vector<PathPoint>> paths;
    // A ring, a long straight road, a short spur and a single point (no line at all).
    std::vector<PathPoint> ring;
    for (int step = 0; step <= 64; ++step) {
        const float angle = static_cast<float>(step) * 6.2831853F / 64.0F;
        ring.push_back({.x = 60.0F * std::cos(angle), .z = 60.0F * std::sin(angle)});
    }
    paths.push_back(ring);
    paths.push_back({{.x = -400.0F, .z = 300.0F}, {.x = 380.0F, .z = 260.0F}});
    paths.push_back({{.x = 200.0F, .z = -150.0F}, {.x = 210.0F, .z = -140.0F}, {.x = 230.0F, .z = -160.0F}});
    paths.push_back({{.x = 5.0F, .z = 5.0F}});
    PathIndex index;
    index.build(paths, 32.0F);
    for (float x = -700.0F; x <= 700.0F; x += 37.0F) {
        for (float z = -700.0F; z <= 700.0F; z += 41.0F) {
            float all = std::numeric_limits<float>::infinity();
            float first_two = std::numeric_limits<float>::infinity();
            for (std::size_t path = 0; path < paths.size(); ++path) {
                const float d = distance_to_path(paths[path], x, z);
                all = std::min(all, d);
                if (path < 2) {
                    first_two = std::min(first_two, d);
                }
            }
            CHECK(index.nearest(x, z) == Catch::Approx(all).margin(0.001));
            CHECK(index.nearest(x, z, 2) == Catch::Approx(first_two).margin(0.001));
        }
    }
    PathIndex empty;
    empty.build({}, 32.0F);
    CHECK(empty.nearest(0.0F, 0.0F) > 1e30F);
}

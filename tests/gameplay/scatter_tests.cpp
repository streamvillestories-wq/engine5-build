#include "e5/gameplay/scatter.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

using Catch::Approx;
using namespace e5::gameplay;

TEST_CASE("a forest fills its area at the requested spacing", "[scatter]") {
    const ScatterParams params{.width = 60.0F, .depth = 30.0F, .spacing = 6.0F};
    const std::vector<ScatterPoint> points = scatter_plants(params);
    CHECK(points.size() == 50U); // 10 columns of 5
    for (const ScatterPoint& point : points) {
        CHECK(std::abs(point.x) <= 30.0F);
        CHECK(std::abs(point.z) <= 15.0F);
        CHECK(point.scale >= params.scale_min);
        CHECK(point.scale <= params.scale_max);
        CHECK(point.species == 0);
    }
}

TEST_CASE("plants keep their distance from each other", "[scatter]") {
    const ScatterParams params{.width = 48.0F, .depth = 48.0F, .spacing = 6.0F, .jitter = 0.35F};
    const std::vector<ScatterPoint> points = scatter_plants(params);
    float closest = 1000.0F;
    for (std::size_t a = 0; a < points.size(); ++a) {
        for (std::size_t b = a + 1; b < points.size(); ++b) {
            closest = std::min(closest, std::hypot(points[a].x - points[b].x, points[a].z - points[b].z));
        }
    }
    // Two neighbours can each move 0.35 of the spacing toward the other.
    CHECK(closest >= 6.0F * (1.0F - 2.0F * 0.35F) - 0.001F);
    // And they are not on a regular grid.
    CHECK(closest < 6.0F);
}

TEST_CASE("the clearing around the origin stays free", "[scatter]") {
    const ScatterParams params{.width = 80.0F, .depth = 80.0F, .spacing = 5.0F, .clearing_radius = 20.0F};
    const std::vector<ScatterPoint> points = scatter_plants(params);
    REQUIRE_FALSE(points.empty());
    for (const ScatterPoint& point : points) {
        CHECK(std::hypot(point.x, point.z) >= 20.0F);
    }
    CHECK(points.size() < 256U); // fewer than the full 16 x 16 grid
}

TEST_CASE("the same seed gives the same forest, another seed a different one", "[scatter]") {
    const ScatterParams params{.species_count = 3, .seed = 42};
    const std::vector<ScatterPoint> first = scatter_plants(params);
    const std::vector<ScatterPoint> again = scatter_plants(params);
    REQUIRE(first.size() == again.size());
    for (std::size_t i = 0; i < first.size(); ++i) {
        CHECK(first[i].x == again[i].x);
        CHECK(first[i].z == again[i].z);
        CHECK(first[i].yaw == again[i].yaw);
        CHECK(first[i].species == again[i].species);
    }

    ScatterParams other = params;
    other.seed = 43;
    const std::vector<ScatterPoint> different = scatter_plants(other);
    REQUIRE(different.size() == first.size());
    int moved = 0;
    for (std::size_t i = 0; i < first.size(); ++i) {
        moved += first[i].x != different[i].x ? 1 : 0;
    }
    CHECK(moved > static_cast<int>(first.size()) / 2);
}

TEST_CASE("every species is used, in roughly equal shares", "[scatter]") {
    const ScatterParams params{.width = 300.0F, .depth = 300.0F, .spacing = 6.0F, .species_count = 2, .seed = 7};
    const std::vector<ScatterPoint> points = scatter_plants(params);
    const auto first = std::ranges::count_if(points, [](const ScatterPoint& point) { return point.species == 0; });
    const double share = static_cast<double>(first) / static_cast<double>(points.size());
    CHECK(share == Approx(0.5).margin(0.05));
    for (const ScatterPoint& point : points) {
        CHECK(point.species >= 0);
        CHECK(point.species < 2);
    }
}

TEST_CASE("nonsense parameters give an empty forest", "[scatter]") {
    CHECK(scatter_plants({.spacing = 0.0F}).empty());
    CHECK(scatter_plants({.width = -5.0F}).empty());
    CHECK(scatter_plants({.species_count = 0}).empty());
}
#include "e5/gameplay/island.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>

using Catch::Approx;
using namespace e5::gameplay;

TEST_CASE("the middle of the island is dry, nearly flat land", "[island]") {
    const IslandParams params;
    CHECK(island_height(params, 0.0F, 0.0F) == Approx(params.lowland_height).margin(0.05));
    for (int step = -5; step <= 5; ++step) {
        const float offset = params.clearing_radius * 0.16F * static_cast<float>(step);
        CHECK(island_height(params, offset, 0.0F) > 1.0F);
        CHECK(island_slope(params, offset, 5.0F) < 0.1F);
    }
}

TEST_CASE("far from the centre there is only sea floor", "[island]") {
    const IslandParams params;
    for (const float angle : {0.0F, 1.0F, 2.5F, 4.0F, 5.5F}) {
        const float x = std::cos(angle) * params.radius * 2.0F;
        const float z = std::sin(angle) * params.radius * 2.0F;
        CHECK(island_height(params, x, z) == Approx(params.sea_floor).margin(0.01));
    }
}

TEST_CASE("walking outward from the centre always reaches the sea", "[island]") {
    const IslandParams params;
    for (int direction = 0; direction < 16; ++direction) {
        const float angle = static_cast<float>(direction) * 0.3927F;
        float coast = 0.0F;
        for (int metres = 0; metres < static_cast<int>(params.radius * 2.0F); ++metres) {
            const auto distance = static_cast<float>(metres);
            if (island_height(params, std::cos(angle) * distance, std::sin(angle) * distance) < 0.0F) {
                coast = distance;
                break;
            }
        }
        // The coast wanders, but stays within reach of the stated radius.
        CHECK(coast > params.radius * 0.5F);
        CHECK(coast < params.radius * 1.3F);
    }
}

TEST_CASE("the ground has no cliffs or holes: a short step is a small change in height", "[island]") {
    const IslandParams params;
    float steepest = 0.0F;
    for (int column = -54; column <= 54; ++column) {
        for (int row = -46; row <= 46; ++row) {
            // An uneven grid, so the samples do not all fall on whole metres.
            steepest = std::max(
                steepest, island_slope(params, static_cast<float>(column) * 3.7F, static_cast<float>(row) * 4.3F));
        }
    }
    CHECK(steepest < 1.5F);  // nowhere steeper than about 56 degrees
    CHECK(steepest > 0.15F); // and not a pancake either
}

TEST_CASE("there are real hills", "[island]") {
    const IslandParams params;
    float highest = 0.0F;
    for (int column = -30; column <= 30; ++column) {
        for (int row = -30; row <= 30; ++row) {
            highest = std::max(
                highest, island_height(params, static_cast<float>(column) * 5.0F, static_cast<float>(row) * 5.0F));
        }
    }
    CHECK(highest > params.lowland_height + params.hill_height * 0.4F);
    CHECK(highest <= params.lowland_height + params.hill_height + 0.01F);
}

TEST_CASE("the same seed is the same island, another seed another island", "[island]") {
    const IslandParams params{.seed = 5};
    CHECK(island_height(params, 60.5F, -40.25F) == island_height(params, 60.5F, -40.25F));

    const IslandParams other{.seed = 6};
    int different = 0;
    for (int column = -5; column <= 5; ++column) {
        const float x = static_cast<float>(column) * 20.0F;
        different += std::abs(island_height(params, x, 70.0F) - island_height(other, x, 70.0F)) > 0.05F ? 1 : 0;
    }
    CHECK(different >= 6);
}
TEST_CASE("a mountain range is a long crest with peaks, and ends where it should", "[island]") {
    const IslandParams flat{.hill_height = 0.0F};
    const IslandParams range{.hill_height = 0.0F,
                             .mountain_height = 90.0F,
                             .mountain_radius = 50.0F,
                             .mountain_x = 0.0F,
                             .mountain_z = -100.0F,
                             .mountain_length = 160.0F};

    // Walking the length of the range, the highest ground across it is always a mountain,
    // but not always the same height.
    float lowest_crest = 1000.0F;
    float highest_crest = 0.0F;
    for (int step = -6; step <= 6; ++step) {
        const float x = static_cast<float>(step) * 10.0F;
        float crest = 0.0F;
        for (int across = -40; across <= 40; across += 2) {
            crest = std::max(crest, island_height(range, x, -100.0F + static_cast<float>(across)));
        }
        lowest_crest = std::min(lowest_crest, crest);
        highest_crest = std::max(highest_crest, crest);
    }
    CHECK(lowest_crest > 12.0F);
    CHECK(highest_crest > 45.0F);
    CHECK(highest_crest <= flat.lowland_height + 90.0F + 0.01F);
    CHECK(highest_crest - lowest_crest > 8.0F);

    // To the side of it, and in the clearing, the island is as without it.
    CHECK(island_height(range, 0.0F, 20.0F) == island_height(flat, 0.0F, 20.0F));
    CHECK(island_height(range, 0.0F, 0.0F) == island_height(flat, 0.0F, 0.0F));
}

TEST_CASE("a mountain stands where it is put, and nowhere else", "[island]") {
    const IslandParams flat{.hill_height = 0.0F};
    const IslandParams peaked{.hill_height = 0.0F,
                              .mountain_height = 80.0F,
                              .mountain_radius = 60.0F,
                              .mountain_x = 50.0F,
                              .mountain_z = -70.0F};

    CHECK(island_height(peaked, 50.0F, -70.0F) == Approx(flat.lowland_height + 80.0F).margin(0.5));
    // Lower all the way around the peak.
    for (const float angle : {0.0F, 1.3F, 2.6F, 3.9F, 5.2F}) {
        CHECK(island_height(peaked, 50.0F + std::cos(angle) * 30.0F, -70.0F + std::sin(angle) * 30.0F) <
              island_height(peaked, 50.0F, -70.0F) - 15.0F);
    }
    // Beyond its foot, and in the clearing, the island is as without it.
    CHECK(island_height(peaked, -40.0F, 60.0F) == island_height(flat, -40.0F, 60.0F));
    CHECK(island_height(peaked, 0.0F, 0.0F) == island_height(flat, 0.0F, 0.0F));
}

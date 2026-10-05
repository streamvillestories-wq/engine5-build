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

TEST_CASE("a river and a lake are cut to below the sea, and the land beside them is dry", "[island]") {
    e5::gameplay::IslandParams params;
    params.radius = 200.0F;
    params.clearing_radius = 10.0F;
    params.river = {{.x = 40.0F, .z = -40.0F}, {.x = 40.0F, .z = 40.0F}};
    params.lakes = {{.x = -60.0F, .z = 0.0F, .radius = 20.0F}};
    // In the middle of the river and of the lake: under water, and no deeper than asked.
    for (const float z : {-30.0F, 0.0F, 30.0F}) {
        const float bed = e5::gameplay::island_height(params, 40.0F, z);
        CHECK(bed < 0.0F);
        CHECK(bed >= -params.water_depth - 0.01F);
    }
    CHECK(e5::gameplay::island_height(params, -60.0F, 0.0F) < 0.0F);
    // Well away from both: land, as without them.
    e5::gameplay::IslandParams dry = params;
    dry.river.clear();
    dry.lakes.clear();
    CHECK(e5::gameplay::island_height(params, 110.0F, 0.0F) ==
          Catch::Approx(e5::gameplay::island_height(dry, 110.0F, 0.0F)));
    CHECK(e5::gameplay::island_height(params, 110.0F, 0.0F) > 0.5F);
    // The distances say the same.
    CHECK(e5::gameplay::island_river_distance(params, 40.0F, 0.0F) < 5.0F);
    CHECK(e5::gameplay::island_river_distance(params, 110.0F, 0.0F) > 60.0F);
    CHECK(e5::gameplay::island_lake_distance(params, -60.0F, 0.0F) < 0.0F);
    CHECK(e5::gameplay::island_lake_distance(dry, 0.0F, 0.0F) > 1.0e6F);
}

TEST_CASE("a bay draws the coast in, a cliff ends the land high, an islet stands in the sea", "[island]") {
    e5::gameplay::IslandParams plain;
    plain.radius = 200.0F;
    // A bay to the south: where there was land near the coast there is water.
    e5::gameplay::IslandParams with_bay = plain;
    with_bay.bays = {{.direction = 1.5708F, .width = 0.35F, .depth = 0.35F}};
    CHECK(e5::gameplay::island_height(plain, 0.0F, 150.0F) > 0.5F);
    CHECK(e5::gameplay::island_height(with_bay, 0.0F, 150.0F) < 0.0F);
    // And it does not reach round to the other side of the island.
    CHECK(e5::gameplay::island_height(with_bay, 0.0F, -150.0F) ==
          Catch::Approx(e5::gameplay::island_height(plain, 0.0F, -150.0F)));

    // Cliffs to the west: close to the coast the land is far higher than a beach.
    e5::gameplay::IslandParams with_cliff = plain;
    with_cliff.cliff_from = 2.6F;
    with_cliff.cliff_to = 3.7F;
    with_cliff.cliff_height = 14.0F;
    float highest_near_coast = 0.0F;
    float highest_beach = 0.0F;
    for (float x = -150.0F; x > -215.0F; x -= 1.0F) {
        highest_near_coast = std::max(highest_near_coast, e5::gameplay::island_height(with_cliff, x, 0.0F));
        highest_beach = std::max(highest_beach, e5::gameplay::island_height(plain, x, 0.0F));
    }
    CHECK(highest_near_coast > highest_beach + 8.0F);
    // The east is as it was.
    CHECK(e5::gameplay::island_height(with_cliff, 150.0F, 0.0F) ==
          Catch::Approx(e5::gameplay::island_height(plain, 150.0F, 0.0F)));

    // An islet where there was only sea.
    e5::gameplay::IslandParams with_islet = plain;
    with_islet.islets = {{.x = 250.0F, .z = 0.0F, .radius = 14.0F}};
    CHECK(e5::gameplay::island_height(plain, 250.0F, 0.0F) < -3.0F);
    CHECK(e5::gameplay::island_height(with_islet, 250.0F, 0.0F) > 1.0F);
    CHECK(e5::gameplay::island_height(with_islet, 250.0F, 40.0F) < -3.0F);
}

TEST_CASE("a pass is a low way through the range", "[island]") {
    e5::gameplay::IslandParams params;
    params.radius = 200.0F;
    params.mountain_height = 120.0F;
    params.mountain_radius = 80.0F;
    params.mountain_z = -100.0F;
    params.mountain_length = 240.0F;
    const auto highest_across = [&params](float x) {
        float highest = 0.0F;
        for (float z = -170.0F; z < -30.0F; z += 2.0F) {
            highest = std::max(highest, e5::gameplay::island_height(params, x, z));
        }
        return highest;
    };
    const float before = highest_across(60.0F);
    params.pass_along = 60.0F;
    params.pass_width = 40.0F;
    // Through the pass the range is a fraction of what it was; away from it, unchanged.
    CHECK(highest_across(60.0F) < before * 0.3F);
    const float elsewhere = highest_across(-40.0F);
    params.pass_width = 0.0F;
    CHECK(highest_across(-40.0F) == Catch::Approx(elsewhere));
}

TEST_CASE("regions: each place belongs mostly to one, and a marsh is low and flat", "[island]") {
    e5::gameplay::IslandParams params;
    params.radius = 200.0F;
    params.regions = {{.kind = e5::gameplay::IslandRegionKind::Pine, .x = -100.0F, .z = 0.0F, .radius = 60.0F},
                      {.kind = e5::gameplay::IslandRegionKind::Marsh, .x = 90.0F, .z = 60.0F, .radius = 50.0F}};
    using e5::gameplay::IslandRegionKind;
    CHECK(e5::gameplay::island_region(params, -100.0F, 0.0F) == IslandRegionKind::Pine);
    CHECK(e5::gameplay::island_region(params, 90.0F, 60.0F) == IslandRegionKind::Marsh);
    CHECK(e5::gameplay::island_region(params, 0.0F, 120.0F) == IslandRegionKind::Meadow);
    // The shares never add up to more than the whole.
    for (float x = -180.0F; x <= 180.0F; x += 15.0F) {
        const e5::gameplay::RegionShares shares = e5::gameplay::island_regions(params, x, 30.0F);
        CHECK(shares.pine + shares.dry + shares.marsh <= 1.0001F);
    }
    // In the marsh the ground stays within a couple of metres of the water, everywhere.
    for (float x = 70.0F; x <= 110.0F; x += 4.0F) {
        for (float z = 40.0F; z <= 80.0F; z += 4.0F) {
            const float height = e5::gameplay::island_height(params, x, z);
            CHECK(height < 2.2F);
            CHECK(height > -1.2F);
        }
    }
    // Without regions nothing is claimed.
    const e5::gameplay::IslandParams plain;
    CHECK(e5::gameplay::island_region(plain, -100.0F, 0.0F) == IslandRegionKind::Meadow);
}

TEST_CASE("a site is level ground at the height of its middle", "[island]") {
    e5::gameplay::IslandParams params;
    params.radius = 200.0F;
    params.hill_height = 30.0F;
    // Somewhere on a slope.
    float x = 60.0F;
    const auto rise = [&](float at) {
        return std::abs(e5::gameplay::island_height(params, at + 8.0F, 70.0F) -
                        e5::gameplay::island_height(params, at, 70.0F));
    };
    while (rise(x) < 0.6F && x < 140.0F) {
        x += 3.0F;
    }
    const float before = e5::gameplay::island_height(params, x, 70.0F);
    REQUIRE(std::abs(e5::gameplay::island_height(params, x + 8.0F, 70.0F) - before) > 0.4F);
    params.sites = {{.x = x, .z = 70.0F, .radius = 12.0F}};
    for (float away = -11.0F; away <= 11.0F; away += 2.0F) {
        CHECK(e5::gameplay::island_height(params, x + away, 70.0F) == Catch::Approx(before).margin(0.01));
        CHECK(e5::gameplay::island_height(params, x, 70.0F + away) == Catch::Approx(before).margin(0.01));
    }
    // Beyond its edge the land is what it was.
    const e5::gameplay::IslandParams plain = [&] {
        e5::gameplay::IslandParams copy = params;
        copy.sites.clear();
        return copy;
    }();
    CHECK(e5::gameplay::island_height(params, x + 19.0F, 70.0F) ==
          e5::gameplay::island_height(plain, x + 19.0F, 70.0F));
}

TEST_CASE("hills, ridges and valleys shape the lowland where they are put", "[island]") {
    e5::gameplay::IslandParams plain;
    plain.radius = 300.0F;
    e5::gameplay::IslandParams shaped = plain;
    shaped.hills = {{.x = 120.0F, .z = 40.0F, .radius = 30.0F, .height = 12.0F}};
    shaped.ridges = {
        {.points = {{.x = -150.0F, .z = -40.0F}, {.x = -100.0F, .z = 20.0F}}, .width = 30.0F, .height = 10.0F},
        {.points = {{.x = 20.0F, .z = 150.0F}, {.x = 80.0F, .z = 170.0F}}, .width = 26.0F, .height = -30.0F}};
    const auto change = [&](float x, float z) {
        return e5::gameplay::island_height(shaped, x, z) - e5::gameplay::island_height(plain, x, z);
    };
    // The hill's top and the ridge's crest stand well above what was there.
    CHECK(change(120.0F, 40.0F) > 6.0F);
    CHECK(change(-125.0F, -10.0F) > 5.0F);
    // The valley is lower than the land was, and however deep it is asked to be, dry.
    CHECK(change(50.0F, 160.0F) < -0.5F);
    CHECK(e5::gameplay::island_height(shaped, 50.0F, 160.0F) >= 1.19F);
    // Away from all of them nothing changed, and nor did the clearing.
    CHECK(change(-20.0F, -120.0F) == 0.0F);
    CHECK(change(200.0F, -100.0F) == 0.0F);
    CHECK(change(0.0F, 0.0F) == 0.0F);
    // Rolling ground: up and down by no more than asked.
    e5::gameplay::IslandParams rolling = plain;
    rolling.rolling = 1.5F;
    float most = 0.0F;
    for (float x = -150.0F; x <= 150.0F; x += 7.0F) {
        const float moved =
            std::abs(e5::gameplay::island_height(rolling, x, 90.0F) - e5::gameplay::island_height(plain, x, 90.0F));
        most = std::max(most, moved);
        CHECK(moved <= 1.5F);
    }
    CHECK(most > 0.3F);
}

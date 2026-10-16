#include "e5/gameplay/crystal_core.hpp"

#include <array>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using e5::gameplay::core_site_reason;
using e5::gameplay::CoreRules;
using e5::gameplay::CoreSite;
using e5::gameplay::CoreSiteProbe;
using e5::gameplay::CoreSpot;
using e5::gameplay::judge_core_site;
using e5::gameplay::nearest_core_distance;

namespace {

// A meadow: level, dry, far from roads, camps and other cores.
CoreSiteProbe meadow() {
    return {.height = 4.0F,
            .lowest = 3.8F,
            .highest = 4.1F,
            .slope = 0.05F,
            .road_distance = 30.0F,
            .river_distance = 80.0F,
            .lake_distance = 120.0F,
            .underground = false,
            .keep_out = false,
            .nearest_core = 200.0F};
}

} // namespace

TEST_CASE("a level dry meadow takes a core", "[crystal_core]") {
    CHECK(judge_core_site(meadow(), CoreRules{}) == CoreSite::Ok);
}

TEST_CASE("each kind of bad place is refused with its own reason", "[crystal_core]") {
    const CoreRules rules{};
    CoreSiteProbe probe = meadow();
    probe.underground = true;
    CHECK(judge_core_site(probe, rules) == CoreSite::Underground);

    probe = meadow();
    probe.height = 0.2F;
    probe.lowest = 0.1F;
    CHECK(judge_core_site(probe, rules) == CoreSite::Water);
    probe = meadow();
    probe.lake_distance = 2.0F;
    CHECK(judge_core_site(probe, rules) == CoreSite::Water);
    probe = meadow();
    probe.river_distance = 3.0F;
    CHECK(judge_core_site(probe, rules) == CoreSite::Water);

    probe = meadow();
    probe.road_distance = 4.0F;
    CHECK(judge_core_site(probe, rules) == CoreSite::Road);

    probe = meadow();
    probe.keep_out = true;
    CHECK(judge_core_site(probe, rules) == CoreSite::KeepOut);

    probe = meadow();
    probe.slope = 0.4F;
    CHECK(judge_core_site(probe, rules) == CoreSite::Steep);

    probe = meadow();
    probe.highest = probe.lowest + 1.2F;
    CHECK(judge_core_site(probe, rules) == CoreSite::Uneven);

    probe = meadow();
    probe.nearest_core = 50.0F;
    CHECK(judge_core_site(probe, rules) == CoreSite::TooClose);
}

TEST_CASE("the reasons come in order: what can be helped least first", "[crystal_core]") {
    CoreSiteProbe probe = meadow();
    probe.underground = true;
    probe.slope = 1.0F;
    probe.nearest_core = 1.0F;
    CHECK(judge_core_site(probe, CoreRules{}) == CoreSite::Underground);
    probe.underground = false;
    CHECK(judge_core_site(probe, CoreRules{}) == CoreSite::Steep);
}

TEST_CASE("every verdict has words for the player", "[crystal_core]") {
    for (const CoreSite site : {CoreSite::Ok, CoreSite::Underground, CoreSite::Water, CoreSite::Road, CoreSite::Uneven,
                                CoreSite::Steep, CoreSite::KeepOut, CoreSite::TooClose}) {
        CHECK(core_site_reason(site)[0] != '\0');
    }
}

TEST_CASE("the spacing between cores is measured on the ground", "[crystal_core]") {
    const std::array<CoreSpot, 2> others{CoreSpot{.x = 0.0F, .z = 0.0F}, CoreSpot{.x = 100.0F, .z = 0.0F}};
    CHECK(nearest_core_distance({.x = 60.0F, .z = 0.0F}, others) == Catch::Approx(40.0F));
    CHECK(nearest_core_distance({.x = 0.0F, .z = 80.0F}, others) == Catch::Approx(80.0F));
    CHECK(nearest_core_distance({.x = 5.0F, .z = 5.0F}, std::span<const CoreSpot>{}) > 1e8F);
    // The rule's own number: 80 m apart is just enough.
    CoreSiteProbe probe = meadow();
    probe.nearest_core = CoreRules{}.spacing;
    CHECK(judge_core_site(probe, CoreRules{}) == CoreSite::Ok);
}

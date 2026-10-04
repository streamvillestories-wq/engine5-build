#include "e5/gameplay/cloth_rig.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string_view>

using namespace e5::gameplay;
using namespace std::string_view_literals;

TEST_CASE("cloth bone names are parsed", "[cloth]") {
    const auto bone = parse_cloth_bone_name("cloth_skirt_03_1");
    REQUIRE(bone.has_value());
    CHECK(bone->part == "skirt");
    CHECK(bone->chain == 3);
    CHECK(bone->segment == 1);

    const auto underscored = parse_cloth_bone_name("cloth_long_coat_12_0");
    REQUIRE(underscored.has_value());
    CHECK(underscored->part == "long_coat");
    CHECK(underscored->chain == 12);
}

TEST_CASE("names outside the convention are rejected", "[cloth]") {
    CHECK_FALSE(parse_cloth_bone_name("Hips").has_value());
    CHECK_FALSE(parse_cloth_bone_name("cloth_").has_value());
    CHECK_FALSE(parse_cloth_bone_name("cloth_skirt").has_value());
    CHECK_FALSE(parse_cloth_bone_name("cloth_skirt_1").has_value());
    CHECK_FALSE(parse_cloth_bone_name("cloth__01_0").has_value());
    CHECK_FALSE(parse_cloth_bone_name("cloth_skirt_a_0").has_value());
    CHECK_FALSE(parse_cloth_bone_name("cloth_skirt_01_x").has_value());
    CHECK_FALSE(parse_cloth_bone_name("cloth_skirt_01_").has_value());
    CHECK_FALSE(parse_cloth_bone_name("mycloth_skirt_01_0").has_value());
}

TEST_CASE("chains are grouped, ordered root to tip, and sorted", "[cloth]") {
    const std::array bones{"Hips"sv,
                           "cloth_skirt_01_1"sv,
                           "cloth_skirt_00_0"sv,
                           "cloth_ponytail_00_1"sv,
                           "cloth_skirt_01_0"sv,
                           "cloth_ponytail_00_0"sv,
                           "cloth_skirt_00_1"sv,
                           "Head"sv};
    const auto chains = find_cloth_chains(bones);
    REQUIRE(chains.size() == 3);

    CHECK(chains[0].part == "ponytail");
    CHECK(chains[0].bones == std::vector<int>{5, 3});
    CHECK(chains[1].part == "skirt");
    CHECK(chains[1].chain == 0);
    CHECK(chains[1].bones == std::vector<int>{2, 6});
    CHECK(chains[2].chain == 1);
    CHECK(chains[2].bones == std::vector<int>{4, 1});
}

TEST_CASE("broken chains are dropped", "[cloth]") {
    SECTION("gap in the segments") {
        const std::array bones{"cloth_cape_00_0"sv, "cloth_cape_00_2"sv};
        CHECK(find_cloth_chains(bones).empty());
    }
    SECTION("missing root segment") {
        const std::array bones{"cloth_cape_00_1"sv, "cloth_cape_00_2"sv};
        CHECK(find_cloth_chains(bones).empty());
    }
    SECTION("duplicate segment") {
        const std::array bones{"cloth_cape_00_0"sv, "cloth_cape_00_0"sv};
        CHECK(find_cloth_chains(bones).empty());
    }
    SECTION("a broken chain does not affect a good one") {
        const std::array bones{"cloth_cape_00_1"sv, "cloth_cape_01_0"sv};
        const auto chains = find_cloth_chains(bones);
        REQUIRE(chains.size() == 1);
        CHECK(chains[0].chain == 1);
    }
}

TEST_CASE("a skeleton without cloth bones yields no chains", "[cloth]") {
    const std::array bones{"Hips"sv, "Spine"sv};
    CHECK(find_cloth_chains(bones).empty());
    CHECK(find_cloth_chains({}).empty());
}

#include "e5/core/config.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string_view>

using namespace std::string_view_literals;

TEST_CASE("no arguments yields defaults", "[config]") {
    const auto config = e5::parse_runtime_args({});
    REQUIRE(config.has_value());
    CHECK_FALSE(config->benchmark);
    CHECK(config->show_overlay);
    CHECK(config->benchmark_output.empty());
}

TEST_CASE("benchmark options are parsed", "[config]") {
    const std::array args{"--benchmark"sv,
                          "--benchmark-seconds=3.5"sv,
                          "--benchmark-warmup=0"sv,
                          "--benchmark-output=out/report.json"sv,
                          "--screenshot=out/shot.png"sv,
                          "--no-overlay"sv,
                          "--overlay-detail"sv,
                          "--auto-move"sv,
                          "--auto-aim"sv,
                          "--auto-attack"sv,
                          "--auto-fire"sv,
                          "--auto-turn"sv};
    const auto config = e5::parse_runtime_args(args);
    REQUIRE(config.has_value());
    CHECK(config->benchmark);
    CHECK(config->benchmark_seconds == Catch::Approx(3.5F));
    CHECK(config->benchmark_warmup_seconds == Catch::Approx(0.0F));
    CHECK(config->benchmark_output == "out/report.json");
    CHECK(config->screenshot_output == "out/shot.png");
    CHECK_FALSE(config->show_overlay);
    CHECK(config->overlay_detail);
    CHECK(config->auto_move == "forward");
    CHECK(config->auto_aim);
    CHECK(config->auto_attack);
    CHECK(config->auto_fire);
    CHECK(config->auto_turn);
    CHECK_FALSE(config->camera_yaw_degrees.has_value());
}

TEST_CASE("invalid arguments are rejected with a message", "[config]") {
    SECTION("unknown option") {
        const std::array args{"--benchmrak"sv};
        const auto config = e5::parse_runtime_args(args);
        REQUIRE_FALSE(config.has_value());
        CHECK(config.error().code == e5::ErrorCode::InvalidArgument);
        CHECK(config.error().message.contains("--benchmrak"));
    }
    SECTION("non-numeric duration") {
        const std::array args{"--benchmark-seconds=fast"sv};
        CHECK_FALSE(e5::parse_runtime_args(args).has_value());
    }
    SECTION("trailing garbage in number") {
        const std::array args{"--benchmark-seconds=5s"sv};
        CHECK_FALSE(e5::parse_runtime_args(args).has_value());
    }
    SECTION("two decimal points") {
        const std::array args{"--benchmark-seconds=1.2.3"sv};
        CHECK_FALSE(e5::parse_runtime_args(args).has_value());
    }
    SECTION("only a decimal point") {
        const std::array args{"--benchmark-seconds=."sv};
        CHECK_FALSE(e5::parse_runtime_args(args).has_value());
    }
    SECTION("skill slot out of range") {
        const std::array args{"--auto-skill=11"sv};
        CHECK_FALSE(e5::parse_runtime_args(args).has_value());
    }
    SECTION("skill slot not a whole number") {
        const std::array args{"--auto-skill=2.5"sv};
        CHECK_FALSE(e5::parse_runtime_args(args).has_value());
    }
    SECTION("unknown auto-move direction") {
        const std::array args{"--auto-move=up"sv};
        CHECK_FALSE(e5::parse_runtime_args(args).has_value());
    }
    SECTION("negative duration") {
        const std::array args{"--benchmark-seconds=-1"sv};
        CHECK_FALSE(e5::parse_runtime_args(args).has_value());
    }
    SECTION("missing value") {
        const std::array args{"--benchmark-output="sv};
        CHECK_FALSE(e5::parse_runtime_args(args).has_value());
    }
}

TEST_CASE("camera pitch may be negative, skill slots are 1-based", "[config]") {
    const std::array args{"--camera-pitch=-25"sv, "--auto-skill=3"sv};
    const auto config = e5::parse_runtime_args(args);
    REQUIRE(config.has_value());
    CHECK(config->camera_pitch_degrees == Catch::Approx(-25.0F));
    CHECK(config->auto_skill == 3);
}

TEST_CASE("the time of day is an hour", "[config]") {
    const std::array good{"--time-of-day=21.5"sv};
    const auto config = e5::parse_runtime_args(good);
    REQUIRE(config.has_value());
    CHECK(config->time_of_day == Catch::Approx(21.5F));

    const std::array bad{"--time-of-day=25"sv};
    CHECK_FALSE(e5::parse_runtime_args(bad).has_value());
}

TEST_CASE("options for playing together are left to the game's network script", "[config]") {
    const std::array args{"--server=play.example.org:7777"sv, "--net-join=play.example.org:7777"sv, "--net-name=Ada"sv};
    const auto config = e5::parse_runtime_args(args);
    REQUIRE(config.has_value());
}

TEST_CASE("options for the menu are left to the menu", "[config]") {
    const std::array args{"--menu"sv, "--menu-select=3"sv, "--menu-play"sv};
    const auto config = e5::parse_runtime_args(args);
    REQUIRE(config.has_value());
}

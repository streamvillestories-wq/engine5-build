#include "e5/gameplay/day_cycle.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

using namespace e5::gameplay;
using Catch::Approx;

namespace {

float length(const Vec3& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

} // namespace

TEST_CASE("the hour wraps around midnight", "[day]") {
    CHECK(wrap_hour(25.5F) == Approx(1.5F));
    CHECK(wrap_hour(-1.0F) == Approx(23.0F));
    CHECK(wrap_hour(24.0F) == Approx(0.0F));
}

TEST_CASE("time passes, and faster at night", "[day]") {
    const DayParams params{.day_length_seconds = 240.0F, .night_speed = 2.0F};
    // 240 s for 24 h: ten seconds are one hour by day, two by night.
    CHECK(advance_hour(10.0F, 10.0F, params) == Approx(11.0F));
    CHECK(advance_hour(22.0F, 10.0F, params) == Approx(0.0F).margin(1.0e-4));
    CHECK(advance_hour(3.0F, 5.0F, params) == Approx(4.0F));
    // A stopped clock stays where it is.
    CHECK(advance_hour(10.0F, 10.0F, {.day_length_seconds = 0.0F}) == Approx(10.0F));
    CHECK(advance_hour(10.0F, -1.0F, params) == Approx(10.0F));
}

TEST_CASE("the sun rises in the east, stands high at noon and sets in the west", "[day]") {
    const DayParams params;
    const DaySky sunrise = day_sky(6.0F, params);
    CHECK(sunrise.sun_height == Approx(0.0F).margin(1.0e-4));
    CHECK(sunrise.sun_direction.x == Approx(1.0F));

    const DaySky noon = day_sky(12.0F, params);
    CHECK(noon.sun_height > 0.8F);
    CHECK(noon.sun_direction.z > 0.0F); // leaning south

    const DaySky sunset = day_sky(18.0F, params);
    CHECK(sunset.sun_height == Approx(0.0F).margin(1.0e-4));
    CHECK(sunset.sun_direction.x == Approx(-1.0F));

    CHECK(day_sky(0.0F, params).sun_height < -0.8F);
}

TEST_CASE("directions have unit length at every hour", "[day]") {
    for (int quarter = 0; quarter < 96; ++quarter) {
        const DaySky sky = day_sky(static_cast<float>(quarter) * 0.25F, {});
        CHECK(length(sky.sun_direction) == Approx(1.0F).margin(1.0e-4));
        CHECK(length(sky.moon_direction) == Approx(1.0F).margin(1.0e-4));
    }
}

TEST_CASE("day is bright, night is moonlit, and one of the two always lights the ground", "[day]") {
    const DaySky noon = day_sky(12.0F, {});
    CHECK(noon.daylight == Approx(1.0F));
    CHECK(noon.night == Approx(0.0F));
    CHECK(noon.sun_energy > 1.0F);
    CHECK(noon.moon_energy == Approx(0.0F));
    CHECK(noon.twilight == Approx(0.0F));

    const DaySky midnight = day_sky(0.0F, {});
    CHECK(midnight.daylight == Approx(0.0F));
    CHECK(midnight.night == Approx(1.0F));
    CHECK(midnight.sun_energy == Approx(0.0F));
    CHECK(midnight.moon_energy > 0.2F);
    CHECK(midnight.moon_direction.y > 0.5F);
    CHECK(midnight.ambient_energy > noon.ambient_energy);

    // The night sky is darker than the day's, and blue rather than grey.
    CHECK(midnight.sky_top.b < noon.sky_top.b * 0.2F);
    CHECK(midnight.sky_top.b > midnight.sky_top.r);
}

TEST_CASE("dusk is red at the horizon", "[day]") {
    const DaySky dusk = day_sky(17.8F, {});
    CHECK(dusk.twilight > 0.8F);
    CHECK(dusk.sky_horizon.r > dusk.sky_horizon.b);
    CHECK(dusk.sun_color.r > dusk.sun_color.b * 2.0F);
    // By day the horizon is blue-white.
    CHECK(day_sky(12.0F, {}).sky_horizon.b > day_sky(12.0F, {}).sky_horizon.r);
}

TEST_CASE("nothing jumps from one minute to the next", "[day]") {
    DaySky before = day_sky(0.0F, {});
    for (int minute = 1; minute <= 24 * 60; ++minute) {
        const DaySky now = day_sky(static_cast<float>(minute) / 60.0F, {});
        REQUIRE(std::abs(now.sun_energy - before.sun_energy) < 0.05F);
        REQUIRE(std::abs(now.moon_energy - before.moon_energy) < 0.05F);
        REQUIRE(std::abs(now.sky_horizon.r - before.sky_horizon.r) < 0.05F);
        REQUIRE(std::abs(now.sky_top.b - before.sky_top.b) < 0.05F);
        REQUIRE(std::abs(now.ambient_energy - before.ambient_energy) < 0.6F);
        before = now;
    }
}

TEST_CASE("the dial runs from sunrise to sunset and again from sunset to sunrise", "[day]") {
    const DayParams params;
    CHECK_FALSE(is_night_hour(12.0F, params));
    CHECK(is_night_hour(23.0F, params));
    CHECK(is_night_hour(3.0F, params));
    CHECK_FALSE(is_night_hour(6.0F, params));
    CHECK(is_night_hour(18.0F, params));

    CHECK(half_progress(6.0F, params) == Approx(0.0F));
    CHECK(half_progress(12.0F, params) == Approx(0.5F));
    CHECK(half_progress(17.99F, params) == Approx(1.0F).margin(0.01));
    CHECK(half_progress(18.0F, params) == Approx(0.0F));
    CHECK(half_progress(0.0F, params) == Approx(0.5F));
    CHECK(half_progress(5.99F, params) == Approx(1.0F).margin(0.01));
}

TEST_CASE("the moon goes through its phases in eight days", "[day]") {
    CHECK(moon_phase(0) == Approx(0.42F));
    CHECK(moon_phase(8) == Approx(moon_phase(0)).margin(1.0e-4));
    for (int day = 0; day < 40; ++day) {
        CHECK(moon_phase(day) >= 0.0F);
        CHECK(moon_phase(day) < 1.0F);
    }
    // A full moon lights the night more than a new one, which still gives some.
    const float full = day_sky(0.0F, {}, 0.5F).moon_energy;
    const float dark = day_sky(0.0F, {}, 0.0F).moon_energy;
    CHECK(full > dark * 1.5F);
    CHECK(dark > 0.1F);
}

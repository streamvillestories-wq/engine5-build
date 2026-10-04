#include "e5/gameplay/patrol.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdint>
#include <numbers>
#include <set>

using namespace e5::gameplay;
using Catch::Approx;

namespace {

// NOLINTNEXTLINE(bugprone-exception-escape): a std::set in a test helper
struct Walk {
    float least_distance = 1.0e9F; // from the centre, over the whole walk
    float most_distance = 0.0F;
    float top_speed = 0.0F;
    float walked = 0.0F;
    float rested_seconds = 0.0F;
    int rests = 0;
    std::set<int> sectors; // which eighths of the ring it has stood in
};

Walk roam(const RoamParams& params, float x, float z, float seconds, std::uint32_t seed) {
    Walk walk;
    RoamState state{.random = seed};
    const float tick = 1.0F / 60.0F;
    const int ticks = static_cast<int>(seconds / tick);
    for (int index = 0; index < ticks; ++index) {
        const RoamStep step = step_roam(state, x, z, params, tick);
        if (step.state.resting && !state.resting) {
            ++walk.rests;
        }
        state = step.state;
        const float speed = std::hypot(step.velocity.x, step.velocity.z);
        walk.top_speed = std::max(walk.top_speed, speed);
        walk.walked += speed * tick;
        walk.rested_seconds += state.resting ? tick : 0.0F;
        x += step.velocity.x * tick;
        z += step.velocity.z * tick;
        const float distance = std::hypot(x - params.center_x, z - params.center_z);
        walk.least_distance = std::min(walk.least_distance, distance);
        walk.most_distance = std::max(walk.most_distance, distance);
        const float angle = std::atan2(z - params.center_z, x - params.center_x);
        const float pi = std::numbers::pi_v<float>;
        walk.sectors.insert(static_cast<int>(std::floor((angle + pi) / (2.0F * pi) * 8.0F)) % 8);
    }
    return walk;
}

} // namespace

TEST_CASE("it roams inside its ring: never into the middle, never beyond the edge", "[patrol]") {
    const RoamParams params{.center_x = 3.0F, .center_z = -4.0F, .inner_radius = 22.0F, .outer_radius = 29.0F};
    for (const std::uint32_t seed : {1U, 77U, 4242U, 987654U}) {
        const Walk walk = roam(params, 3.0F + 25.0F, -4.0F, 1800.0F, seed);
        CHECK(walk.least_distance >= 22.0F - 0.6F);
        CHECK(walk.most_distance <= 29.0F + 0.6F);
        CHECK(walk.top_speed <= params.speed + 1.0e-3F);
    }
}

TEST_CASE("it gets about: all round the ring, with rests between", "[patrol]") {
    const RoamParams params{.inner_radius = 22.0F, .outer_radius = 29.0F};
    const Walk walk = roam(params, 25.0F, 0.0F, 1800.0F, 12345U);
    CHECK(walk.sectors.size() == 8);
    CHECK(walk.rests > 20);
    CHECK(walk.walked > 1000.0F);
    // It stands between 3 and 9 seconds each time.
    CHECK(walk.rested_seconds / static_cast<float>(walk.rests) > 3.0F);
    CHECK(walk.rested_seconds / static_cast<float>(walk.rests) < 9.5F);
}

TEST_CASE("placed outside its ring, or in the middle, it finds its way into the ring", "[patrol]") {
    const RoamParams params{.inner_radius = 22.0F, .outer_radius = 29.0F};
    for (const float start : {60.0F, 5.0F}) {
        RoamState state;
        float x = start;
        float z = 0.0F;
        const float tick = 1.0F / 60.0F;
        for (int index = 0; index < 60 * 90; ++index) {
            const RoamStep step = step_roam(state, x, z, params, tick);
            state = step.state;
            x += step.velocity.x * tick;
            z += step.velocity.z * tick;
        }
        CHECK(std::hypot(x, z) >= 21.0F);
        CHECK(std::hypot(x, z) <= 30.0F);
    }
}

TEST_CASE("blocked on its way, it gives up and picks another spot", "[patrol]") {
    const RoamParams params{.inner_radius = 22.0F, .outer_radius = 29.0F, .give_up_seconds = 5.0F};
    const RoamState state;
    RoamStep step = step_roam(state, 25.0F, 0.0F, params, 0.1F);
    const float first_x = step.state.target_x;
    const float first_z = step.state.target_z;
    // It never moves (something holds it), so it never arrives.
    for (int index = 0; index < 60; ++index) {
        step = step_roam(step.state, 25.0F, 0.0F, params, 0.1F);
    }
    CHECK((step.state.target_x != Approx(first_x) || step.state.target_z != Approx(first_z)));
    CHECK_FALSE(step.state.resting);
}

TEST_CASE("the same seed gives the same walk", "[patrol]") {
    const RoamParams params{.inner_radius = 22.0F, .outer_radius = 29.0F};
    const Walk a = roam(params, 25.0F, 0.0F, 300.0F, 99U);
    const Walk b = roam(params, 25.0F, 0.0F, 300.0F, 99U);
    CHECK(a.walked == b.walked);
    CHECK(a.rests == b.rests);
}

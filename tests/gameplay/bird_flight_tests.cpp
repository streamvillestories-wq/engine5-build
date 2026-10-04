#include "e5/gameplay/bird_flight.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

using Catch::Approx;
using namespace e5::gameplay;

namespace {

constexpr float dt = 1.0F / 60.0F;

float distance(const Vec3& a, const Vec3& b) {
    return std::hypot(a.x - b.x, a.y - b.y, a.z - b.z);
}

struct FlightResult {
    int hits = 0;
    int steps = 0;
    float furthest_from_target = 0.0F;
    BirdState last;
};

FlightResult fly(BirdState bird, bool has_target, const Vec3& target, const Vec3& home, const BirdParams& params) {
    FlightResult result;
    for (; result.steps < 60 * 30; ++result.steps) {
        const BirdStep step = step_bird(bird, has_target, target, home, params, dt);
        bird = step.state;
        result.hits += step.hit ? 1 : 0;
        result.furthest_from_target = std::max(result.furthest_from_target, distance(bird.position, target));
        if (step.finished) {
            break;
        }
    }
    result.last = bird;
    return result;
}

} // namespace

TEST_CASE("a bird attacks its target again and again, then leaves", "[bird]") {
    const BirdParams params;
    const Vec3 target{.x = 0.5F, .y = 1.4F, .z = -7.0F};
    const Vec3 home{.x = 0.0F, .y = 0.0F, .z = 4.0F};
    for (const float side : {1.0F, -1.0F}) {
        const BirdState bird{.position = {.x = 0.3F, .y = 2.0F, .z = 4.0F},
                             .velocity = {.x = side, .y = 6.0F, .z = -3.0F},
                             .side = side};
        const FlightResult result = fly(bird, true, target, home, params);
        // 11 m to fly first, then roughly one dive per second and a half.
        CHECK(result.hits >= 4);
        CHECK(result.furthest_from_target < 40.0F);
        // Gone after its lifetime plus the time it takes to leave.
        CHECK(static_cast<float>(result.steps) * dt ==
              Approx(params.lifetime_seconds + params.leave_seconds).margin(0.1));
        CHECK(result.last.phase == BirdPhase::Leave);
    }
}

TEST_CASE("a bird flies at constant speed", "[bird]") {
    const BirdParams params;
    BirdState bird{.position = {}, .velocity = {.x = 0.0F, .y = 1.0F, .z = 0.0F}};
    const Vec3 target{.x = 5.0F, .y = 1.0F, .z = 5.0F};
    for (int i = 0; i < 400; ++i) {
        bird = step_bird(bird, true, target, {}, params, dt).state;
        const float speed = std::hypot(bird.velocity.x, bird.velocity.y, bird.velocity.z);
        REQUIRE(speed == Approx(params.speed).margin(0.01));
    }
}

TEST_CASE("without a target a bird circles above home and hits nothing", "[bird]") {
    const BirdParams params;
    const Vec3 home{.x = 3.0F, .y = 0.0F, .z = -2.0F};
    const BirdState bird{.position = {.x = 3.0F, .y = 1.5F, .z = -2.0F}, .velocity = {.x = 1.0F, .y = 3.0F, .z = 0.0F}};
    const FlightResult result = fly(bird, false, {}, home, params);
    CHECK(result.hits == 0);

    // Half-way through its life it is near the circle, not wandering off.
    BirdState mid = bird;
    for (int i = 0; i < 60 * 4; ++i) {
        mid = step_bird(mid, false, {}, home, params, dt).state;
    }
    const float from_axis = std::hypot(mid.position.x - home.x, mid.position.z - home.z);
    CHECK(from_axis < params.circle_radius * 2.0F);
    CHECK(mid.position.y == Approx(home.y + params.circle_height).margin(1.0));
}

TEST_CASE("the flight is deterministic", "[bird]") {
    const BirdParams params;
    const Vec3 target{.x = 2.0F, .y = 1.0F, .z = -9.0F};
    const BirdState bird{.position = {.x = 0.0F, .y = 2.0F, .z = 0.0F}, .velocity = {.x = 0.2F, .y = 1.0F, .z = -0.5F}};
    const FlightResult a = fly(bird, true, target, {}, params);
    const FlightResult b = fly(bird, true, target, {}, params);
    CHECK(a.hits == b.hits);
    CHECK(a.last.position.x == b.last.position.x);
    CHECK(a.last.position.z == b.last.position.z);
}
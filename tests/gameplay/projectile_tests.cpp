#include "e5/gameplay/projectile.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;
using namespace e5::gameplay;

TEST_CASE("a projectile without gravity flies straight", "[projectile]") {
    const Projectile start{.position = {.x = 1.0F, .y = 2.0F, .z = 3.0F},
                           .velocity = {.x = 10.0F, .y = 0.0F, .z = -20.0F}};
    const Projectile next = step_projectile(start, 0.0F, 0.1F);
    CHECK(next.position.x == Approx(2.0F));
    CHECK(next.position.y == Approx(2.0F));
    CHECK(next.position.z == Approx(1.0F));
    CHECK(next.velocity.z == Approx(-20.0F));
}

TEST_CASE("gravity bends the flight downward", "[projectile]") {
    Projectile arrow{.position = {}, .velocity = {.x = 0.0F, .y = 0.0F, .z = -50.0F}};
    for (int i = 0; i < 60; ++i) {
        arrow = step_projectile(arrow, 9.81F, 1.0F / 60.0F);
    }
    // After one second: 50 m downrange, and close to the analytic drop of g/2.
    CHECK(arrow.position.z == Approx(-50.0F));
    CHECK(arrow.position.y == Approx(-4.905F).epsilon(0.03));
    CHECK(arrow.velocity.y == Approx(-9.81F));
}

TEST_CASE("target rings score from the centre outward", "[target]") {
    constexpr float radius = 0.5F;
    CHECK(target_ring_score(0.0F, radius, 5) == 5);
    CHECK(target_ring_score(0.09F, radius, 5) == 5);
    CHECK(target_ring_score(0.11F, radius, 5) == 4);
    CHECK(target_ring_score(0.45F, radius, 5) == 1);
    CHECK(target_ring_score(0.5F, radius, 5) == 1);
    CHECK(target_ring_score(0.51F, radius, 5) == 0);
}

TEST_CASE("degenerate targets never score", "[target]") {
    CHECK(target_ring_score(0.1F, 0.0F, 5) == 0);
    CHECK(target_ring_score(0.1F, 0.5F, 0) == 0);
    CHECK(target_ring_score(-0.1F, 0.5F, 5) == 0);
}

#include "e5/gameplay/vitals.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;
using e5::gameplay::full_vitals;
using e5::gameplay::step_vitals;
using e5::gameplay::Vitals;
using e5::gameplay::VitalsParams;
using e5::gameplay::VitalsStep;

TEST_CASE("damage takes health away and is noticed", "[vitals]") {
    const VitalsParams params;
    const VitalsStep step = step_vitals(full_vitals(params), 30.0F, params, 0.016F);

    CHECK(step.state.health == Approx(70.0F));
    CHECK(step.hurt);
    CHECK_FALSE(step.died);
    CHECK_FALSE(step.state.dead);
}

TEST_CASE("health comes back only after a while without damage, and never above the maximum", "[vitals]") {
    const VitalsParams params;
    Vitals vitals = step_vitals(full_vitals(params), 40.0F, params, 0.1F).state;

    // Still within the delay: nothing comes back.
    for (int step = 0; step < 40; ++step) {
        vitals = step_vitals(vitals, 0.0F, params, 0.1F).state;
    }
    CHECK(vitals.health == Approx(60.0F));

    // A good while later: full again, and not more.
    for (int step = 0; step < 300; ++step) {
        vitals = step_vitals(vitals, 0.0F, params, 0.1F).state;
    }
    CHECK(vitals.health == Approx(params.max_health));
}

TEST_CASE("being hurt again restarts the wait for recovery", "[vitals]") {
    const VitalsParams params;
    Vitals vitals = step_vitals(full_vitals(params), 40.0F, params, 0.1F).state;
    for (int step = 0; step < 45; ++step) {
        vitals = step_vitals(vitals, 0.0F, params, 0.1F).state;
    }
    vitals = step_vitals(vitals, 5.0F, params, 0.1F).state;
    for (int step = 0; step < 30; ++step) {
        vitals = step_vitals(vitals, 0.0F, params, 0.1F).state;
    }
    CHECK(vitals.health == Approx(55.0F));
}

TEST_CASE("running out of health is death, once, and the dead take no more damage", "[vitals]") {
    const VitalsParams params;
    const VitalsStep fatal = step_vitals(full_vitals(params), 250.0F, params, 0.016F);
    CHECK(fatal.died);
    CHECK(fatal.state.dead);
    CHECK(fatal.state.health == Approx(0.0F));

    const VitalsStep after = step_vitals(fatal.state, 50.0F, params, 0.016F);
    CHECK_FALSE(after.died);
    CHECK_FALSE(after.hurt);
    CHECK(after.state.dead);
}

TEST_CASE("the dead come back at full health after the respawn time", "[vitals]") {
    const VitalsParams params;
    Vitals vitals = step_vitals(full_vitals(params), 250.0F, params, 0.016F).state;

    int respawns = 0;
    float seconds = 0.0F;
    while (vitals.dead && seconds < 20.0F) {
        const VitalsStep step = step_vitals(vitals, 0.0F, params, 0.1F);
        vitals = step.state;
        respawns += step.respawned ? 1 : 0;
        seconds += 0.1F;
    }
    CHECK(respawns == 1);
    CHECK(seconds == Approx(params.respawn_seconds).margin(0.11));
    CHECK(vitals.health == Approx(params.max_health));
    CHECK_FALSE(vitals.dead);
}

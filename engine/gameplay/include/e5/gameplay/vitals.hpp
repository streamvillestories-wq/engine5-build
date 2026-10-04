#pragma once

namespace e5::gameplay {

// A character's health over time: damage, slow recovery when left alone, death
// and coming back. Plain data and one step function, like the rest of the
// gameplay code, so a server can run the same rules.
struct Vitals {
    float health = 100.0F;
    bool dead = false;
    float seconds_since_hurt = 1000.0F; // time since the last damage; large = long ago
    float seconds_dead = 0.0F;          // meaningful while dead
};

struct VitalsParams {
    float max_health = 100.0F;
    float regen_delay_seconds = 5.0F; // unhurt for this long before health comes back
    float regen_per_second = 4.0F;
    float respawn_seconds = 4.0F; // dead for this long, then back at full health
};

struct VitalsStep {
    Vitals state;
    bool hurt = false;      // took damage in this step
    bool died = false;      // true on the step health ran out
    bool respawned = false; // true on the step it came back
};

[[nodiscard]] Vitals full_vitals(const VitalsParams& params) noexcept;

// `damage` is everything received since the last step. The dead take none.
[[nodiscard]] VitalsStep step_vitals(const Vitals& state, float damage, const VitalsParams& params,
                                     float delta_seconds) noexcept;

} // namespace e5::gameplay

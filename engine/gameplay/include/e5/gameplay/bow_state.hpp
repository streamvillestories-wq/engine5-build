#pragma once

namespace e5::gameplay {

// The archer's shot cycle. Engine-agnostic and deterministic, so the same
// rules can later run on a server to validate when a shot was allowed.
//
//   Lowered --aim held--> Drawing --(draw_seconds)--> Aiming --aim let go--> Releasing
//      ^                     |                           |                       |
//      +--aim let go/cancel--+----------cancel-----------+                       |
//      +<---------------------(release_seconds, aim not held)--------------------+
//                            ^---------(release_seconds, aim held)---------------+
//
// Holding aim draws the bow; letting go at full draw shoots. Letting go
// earlier, or cancelling at any time, lowers the bow without shooting. While
// Aiming, a skill that charges builds up power for as long as aim is held.
enum class BowPhase : unsigned char { Lowered, Drawing, Aiming, Releasing };

struct BowTimings {
    float draw_seconds = 1.0F;    // from raising the bow to full draw
    float release_seconds = 0.7F; // follow-through before the next draw may start
    float charge_seconds = 1.2F;  // from full draw to full power
    float min_charge = 0.35F;     // a shot released below this has no extra power
};

struct BowInput {
    bool aim_held = false;
    bool cancel_pressed = false; // true only on the step the button went down
    bool build_charge = false;   // the selected skill charges while aiming
};

struct BowState {
    BowPhase phase = BowPhase::Lowered;
    float phase_seconds = 0.0F; // time spent in the current phase
    float charge = 0.0F;        // 0..1, only non-zero while Aiming
};

struct BowStep {
    BowState state;
    float string_draw = 0.0F;    // 0 = string at rest, 1 = full draw
    bool arrow_released = false; // true on exactly the step the shot leaves
    float shot_power = 0.0F;     // with arrow_released: 0 = normal shot, up to 1 = fully charged
};

[[nodiscard]] BowStep step_bow(const BowState& state, const BowInput& input, const BowTimings& timings,
                               float delta_seconds) noexcept;

// Which way the character walks relative to where it faces, for choosing a
// strafing animation while aiming.
enum class StrafeDirection : unsigned char { Forward, Back, Left, Right };

[[nodiscard]] StrafeDirection select_strafe_direction(float forward_speed, float left_speed) noexcept;

} // namespace e5::gameplay
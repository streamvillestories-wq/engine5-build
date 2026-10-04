#pragma once

#include "e5/gameplay/character_motor.hpp"

namespace e5::gameplay {

// The time of day and what the sky looks like at it. Plain functions of the
// hour, so every player of a shared world sees the same sky from the same
// clock, and so the colours can be tested without a renderer.

struct Rgb {
    float r = 0.0F;
    float g = 0.0F;
    float b = 0.0F;
};

struct DayParams {
    float day_length_seconds = 1440.0F; // one full day, if the night ran at the day's pace
    float night_speed = 2.0F;           // the dark hours pass this much faster
    float sun_tilt_degrees = 35.0F;     // how far the sun's path leans away from straight overhead
    float sunrise_hour = 6.0F;
    float sunset_hour = 18.0F;
};

struct DaySky {
    Vec3 sun_direction;      // from the ground towards the sun; unit length
    Vec3 moon_direction;     // from the ground towards the moon; unit length
    float sun_height = 0.0F; // sine of the sun's elevation: 1 overhead, 0 on the horizon, negative below
    float daylight = 0.0F;   // 0 = night, 1 = full day
    float twilight = 0.0F;   // 1 while the sun is on the horizon, 0 well above or below
    float night = 0.0F;      // 0 by day, 1 in the dark; what stars and lamps follow
    float sun_energy = 0.0F;
    float moon_energy = 0.0F;
    float ambient_energy = 1.0F; // multiplier on the light the sky gives
    Rgb sun_color;
    Rgb moon_color;
    Rgb sky_top;
    Rgb sky_horizon;
    Rgb fog_color;
    // Added to the light the sky gives the ground, not to the sky that is seen: the eye
    // adapts to the dark, a screen does not, so nights get a blue fill and dusk a warm one.
    Rgb sky_fill;
};

// Hours since midnight, kept in [0, 24).
[[nodiscard]] float wrap_hour(float hour) noexcept;

// The hour after `delta_seconds` of play.
[[nodiscard]] float advance_hour(float hour, float delta_seconds, const DayParams& params) noexcept;

// True between sunset and sunrise.
[[nodiscard]] bool is_night_hour(float hour, const DayParams& params) noexcept;

// How far the current half of the day has come: 0 at sunrise (by day) or sunset
// (by night), 1 at the end of that half. What a sun-and-moon dial shows.
[[nodiscard]] float half_progress(float hour, const DayParams& params) noexcept;

// The moon's phase on a day counted from 0: 0 = new, 0.5 = full, back to new
// after eight days. The first night is nearly full.
[[nodiscard]] float moon_phase(int day) noexcept;

// `phase` as from moon_phase: a thin moon gives less light than a full one.
[[nodiscard]] DaySky day_sky(float hour, const DayParams& params, float phase = 0.5F) noexcept;

} // namespace e5::gameplay

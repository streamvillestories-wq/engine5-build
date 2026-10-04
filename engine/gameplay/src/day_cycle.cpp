#include "e5/gameplay/day_cycle.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace e5::gameplay {
namespace {

constexpr float hours_per_day = 24.0F;
constexpr float pi = std::numbers::pi_v<float>;

// Colours are linear, picked by eye in the island scene.
constexpr Rgb day_top{.r = 0.16F, .g = 0.36F, .b = 0.74F};
constexpr Rgb day_horizon{.r = 0.62F, .g = 0.74F, .b = 0.88F};
constexpr Rgb night_top{.r = 0.008F, .g = 0.013F, .b = 0.035F};
constexpr Rgb night_horizon{.r = 0.03F, .g = 0.045F, .b = 0.09F};
constexpr Rgb dusk_top{.r = 0.2F, .g = 0.2F, .b = 0.42F};
constexpr Rgb dusk_horizon{.r = 1.0F, .g = 0.48F, .b = 0.22F};
constexpr Rgb sun_noon{.r = 1.0F, .g = 0.96F, .b = 0.9F};
constexpr Rgb sun_low{.r = 1.0F, .g = 0.5F, .b = 0.24F};
constexpr Rgb moonlight{.r = 0.55F, .g = 0.68F, .b = 1.0F};

constexpr float noon_sun_energy = 1.25F;
constexpr float full_moon_energy = 0.3F;
// The night sky is dark; without this the ground would be black instead of blue.
constexpr float night_ambient_energy = 3.0F;
constexpr Rgb night_fill{.r = 0.035F, .g = 0.05F, .b = 0.095F};
constexpr Rgb twilight_fill{.r = 0.07F, .g = 0.055F, .b = 0.06F};
// At dusk the sun is too low to light the ground and the sky is already dim.
constexpr float twilight_ambient_energy = 1.2F;

[[nodiscard]] float smoothstep(float from, float to, float value) noexcept {
    const float t = std::clamp((value - from) / (to - from), 0.0F, 1.0F);
    return t * t * (3.0F - 2.0F * t);
}

[[nodiscard]] Rgb mix(const Rgb& a, const Rgb& b, float t) noexcept {
    return {.r = std::lerp(a.r, b.r, t), .g = std::lerp(a.g, b.g, t), .b = std::lerp(a.b, b.b, t)};
}

[[nodiscard]] bool is_night(float hour, const DayParams& params) noexcept {
    return hour < params.sunrise_hour || hour >= params.sunset_hour;
}

constexpr float first_night_phase = 0.42F;
constexpr float days_per_moon = 8.0F;
// Even a new moon leaves the night some light: the game must stay playable.
constexpr float new_moon_share = 0.45F;

} // namespace

bool is_night_hour(float hour, const DayParams& params) noexcept {
    return is_night(wrap_hour(hour), params);
}

float half_progress(float hour, const DayParams& params) noexcept {
    const float now = wrap_hour(hour);
    const float day_hours = std::max(params.sunset_hour - params.sunrise_hour, 0.1F);
    if (!is_night(now, params)) {
        return std::clamp((now - params.sunrise_hour) / day_hours, 0.0F, 1.0F);
    }
    const float since_sunset =
        now >= params.sunset_hour ? now - params.sunset_hour : now + hours_per_day - params.sunset_hour;
    return std::clamp(since_sunset / std::max(hours_per_day - day_hours, 0.1F), 0.0F, 1.0F);
}

float moon_phase(int day) noexcept {
    const float phase = first_night_phase + static_cast<float>(day) / days_per_moon;
    return phase - std::floor(phase);
}

float wrap_hour(float hour) noexcept {
    const float wrapped = std::fmod(hour, hours_per_day);
    return wrapped < 0.0F ? wrapped + hours_per_day : wrapped;
}

float advance_hour(float hour, float delta_seconds, const DayParams& params) noexcept {
    if (params.day_length_seconds <= 0.0F || delta_seconds <= 0.0F) {
        return wrap_hour(hour);
    }
    const float pace = is_night(wrap_hour(hour), params) ? std::max(params.night_speed, 0.0F) : 1.0F;
    return wrap_hour(hour + delta_seconds * pace * hours_per_day / params.day_length_seconds);
}

DaySky day_sky(float hour, const DayParams& params, float phase) noexcept {
    const float now = wrap_hour(hour);
    // The sun's angle along its path: 0 at sunrise, pi at sunset, on round the back by night.
    const float day_hours = std::max(params.sunset_hour - params.sunrise_hour, 0.1F);
    const float night_hours = std::max(hours_per_day - day_hours, 0.1F);
    float angle = 0.0F;
    if (!is_night(now, params)) {
        angle = (now - params.sunrise_hour) / day_hours * pi;
    } else {
        const float since_sunset =
            now >= params.sunset_hour ? now - params.sunset_hour : now + hours_per_day - params.sunset_hour;
        angle = pi + since_sunset / night_hours * pi;
    }
    // It rises in the east (+x), and its path leans to the south (+z).
    const float tilt = params.sun_tilt_degrees * pi / 180.0F;
    DaySky sky;
    sky.sun_direction = {
        .x = std::cos(angle), .y = std::sin(angle) * std::cos(tilt), .z = std::sin(angle) * std::sin(tilt)};
    // The moon is up when the sun is down, on a path of its own.
    const float moon_tilt = tilt * 0.6F;
    sky.moon_direction = {.x = -std::cos(angle),
                          .y = -std::sin(angle) * std::cos(moon_tilt),
                          .z = std::sin(angle) * std::sin(moon_tilt) + 0.25F};
    const float moon_length =
        std::sqrt(sky.moon_direction.x * sky.moon_direction.x + sky.moon_direction.y * sky.moon_direction.y +
                  sky.moon_direction.z * sky.moon_direction.z);
    sky.moon_direction = {.x = sky.moon_direction.x / moon_length,
                          .y = sky.moon_direction.y / moon_length,
                          .z = sky.moon_direction.z / moon_length};

    sky.sun_height = sky.sun_direction.y;
    sky.daylight = smoothstep(-0.08F, 0.3F, sky.sun_height);
    sky.twilight = 1.0F - smoothstep(0.0F, 0.32F, std::abs(sky.sun_height));
    sky.night = 1.0F - smoothstep(-0.2F, 0.02F, sky.sun_height);

    sky.sun_energy = noon_sun_energy * smoothstep(-0.03F, 0.18F, sky.sun_height);
    sky.sun_color = mix(sun_low, sun_noon, smoothstep(0.0F, 0.4F, sky.sun_height));
    const float moon_lit = 0.5F - 0.5F * std::cos(phase * 2.0F * pi);
    sky.moon_energy = full_moon_energy * std::lerp(new_moon_share, 1.0F, moon_lit) *
                      smoothstep(0.0F, 0.2F, sky.moon_direction.y) * sky.night;
    sky.moon_color = moonlight;
    sky.ambient_energy = std::lerp(1.0F, night_ambient_energy, sky.night) + twilight_ambient_energy * sky.twilight;

    // Red only where the sun is: the top of the sky turns violet, the horizon orange.
    const float glow = sky.twilight * smoothstep(-0.25F, 0.0F, sky.sun_height);
    sky.sky_top = mix(mix(night_top, day_top, sky.daylight), dusk_top, glow * 0.6F);
    // The sky shader paints the fire where the sun is; here the whole horizon only warms a little.
    sky.sky_horizon = mix(mix(night_horizon, day_horizon, sky.daylight), dusk_horizon, glow * 0.4F);
    // Fog takes the horizon's colour, toned down so distant hills do not glow at dusk.
    sky.fog_color = mix(sky.sky_horizon, mix(night_horizon, day_horizon, sky.daylight), 0.5F);
    sky.sky_fill = {.r = night_fill.r * sky.night + twilight_fill.r * sky.twilight,
                    .g = night_fill.g * sky.night + twilight_fill.g * sky.twilight,
                    .b = night_fill.b * sky.night + twilight_fill.b * sky.twilight};
    return sky;
}

} // namespace e5::gameplay

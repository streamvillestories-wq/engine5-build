#include "e5/gameplay/bird_flight.hpp"

#include <algorithm>
#include <cmath>

namespace e5::gameplay {
namespace {

Vec3 add(const Vec3& a, const Vec3& b) {
    return {.x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z};
}
Vec3 sub(const Vec3& a, const Vec3& b) {
    return {.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z};
}
Vec3 scale(const Vec3& v, float s) {
    return {.x = v.x * s, .y = v.y * s, .z = v.z * s};
}
float length(const Vec3& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}
// Unit vector, or `fallback` for a vector too short to have a direction.
Vec3 direction(const Vec3& v, const Vec3& fallback) {
    const float len = length(v);
    return len > 1e-5F ? scale(v, 1.0F / len) : fallback;
}

constexpr Vec3 up{.x = 0.0F, .y = 1.0F, .z = 0.0F};

// Turns the velocity toward `wanted_direction` by at most `acceleration * dt`, at constant speed.
Vec3 steer(const Vec3& velocity, const Vec3& wanted_direction, float speed, float acceleration, float dt) {
    const Vec3 change = sub(scale(wanted_direction, speed), velocity);
    const float change_length = length(change);
    const float allowed = acceleration * dt;
    const Vec3 next = change_length <= allowed ? scale(wanted_direction, speed)
                                               : add(velocity, scale(change, allowed / change_length));
    return scale(direction(next, wanted_direction), speed);
}

// Where a bird with nothing to attack wants to be: on a circle above home.
Vec3 circle_point(const BirdState& state, const Vec3& home, const BirdParams& params) {
    const float angle = state.side * (state.age_seconds * params.speed / std::max(params.circle_radius, 0.1F));
    return {.x = home.x + params.circle_radius * std::cos(angle),
            .y = home.y + params.circle_height,
            .z = home.z + params.circle_radius * std::sin(angle)};
}

} // namespace

BirdStep step_bird(const BirdState& state, bool has_target, const Vec3& target, const Vec3& home,
                   const BirdParams& params, float delta_seconds) noexcept {
    BirdStep step{.state = state};
    BirdState& next = step.state;
    next.age_seconds += delta_seconds;
    next.phase_seconds += delta_seconds;

    const auto enter = [&next](BirdPhase phase) {
        next.phase = phase;
        next.phase_seconds = 0.0F;
    };

    if (next.phase != BirdPhase::Leave && next.age_seconds >= params.lifetime_seconds) {
        enter(BirdPhase::Leave);
    }

    const Vec3 heading = direction(state.velocity, up);
    switch (next.phase) {
    case BirdPhase::Rise:
        next.velocity = scale(heading, params.speed);
        if (next.phase_seconds >= params.rise_seconds) {
            enter(BirdPhase::Attack);
        }
        break;
    case BirdPhase::Attack: {
        const Vec3 goal = has_target ? target : circle_point(state, home, params);
        // The longer a dive takes, the harder it turns: a bird that keeps
        // missing would otherwise circle its target for ever.
        const float acceleration = params.turn_acceleration * (1.0F + 4.0F * next.phase_seconds);
        next.velocity = steer(state.velocity, direction(sub(goal, state.position), heading), params.speed, acceleration,
                              delta_seconds);
        break;
    }
    case BirdPhase::Retreat:
        next.velocity = scale(heading, params.speed);
        if (next.phase_seconds >= params.retreat_seconds) {
            enter(BirdPhase::Attack);
        }
        break;
    case BirdPhase::Leave: {
        const Vec3 away = direction({.x = heading.x, .y = 1.2F, .z = heading.z}, up);
        next.velocity = steer(state.velocity, away, params.speed, params.turn_acceleration, delta_seconds);
        step.finished = next.phase_seconds >= params.leave_seconds;
        break;
    }
    }

    next.position = add(state.position, scale(next.velocity, delta_seconds));

    if (next.phase == BirdPhase::Attack && has_target &&
        length(sub(target, next.position)) <= params.hit_distance + params.speed * delta_seconds * 0.5F) {
        step.hit = true;
        // Peel off: back the way it came, upward, and to its own side.
        const Vec3 approach = direction(next.velocity, up);
        const Vec3 sideways = direction({.x = -approach.z, .y = 0.0F, .z = approach.x}, {.x = 1.0F});
        const Vec3 away =
            direction(add(add(scale(approach, -1.0F), scale(up, 0.7F)), scale(sideways, 0.6F * next.side)), up);
        next.velocity = scale(away, params.speed);
        enter(BirdPhase::Retreat);
    }
    return step;
}

} // namespace e5::gameplay
#include "e5/gameplay/projectile.hpp"

#include <algorithm>

namespace e5::gameplay {

Projectile step_projectile(const Projectile& projectile, float gravity, float delta_seconds) noexcept {
    Projectile next = projectile;
    next.velocity.y -= gravity * delta_seconds;
    next.position.x += next.velocity.x * delta_seconds;
    next.position.y += next.velocity.y * delta_seconds;
    next.position.z += next.velocity.z * delta_seconds;
    return next;
}

int target_ring_score(float distance_from_centre, float radius, int rings) noexcept {
    if (rings <= 0 || radius <= 0.0F || distance_from_centre < 0.0F || distance_from_centre > radius) {
        return 0;
    }
    const float ring_width = radius / static_cast<float>(rings);
    const int ring_from_centre = static_cast<int>(distance_from_centre / ring_width);
    // A hit exactly on the outer edge belongs to the outermost ring.
    return rings - std::min(ring_from_centre, rings - 1);
}

} // namespace e5::gameplay

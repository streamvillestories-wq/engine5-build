#include "e5/gameplay/bow_string.hpp"

#include <algorithm>
#include <cmath>

namespace e5::gameplay {

BowStringShape bow_string_shape(const Vec3& top, const Vec3& bottom, const Vec3& pull_direction,
                                float max_draw_distance, float draw) noexcept {
    Vec3 nock{.x = (top.x + bottom.x) * 0.5F, .y = (top.y + bottom.y) * 0.5F, .z = (top.z + bottom.z) * 0.5F};

    const float length = std::hypot(pull_direction.x, pull_direction.y, pull_direction.z);
    if (length > 0.0F) {
        const float distance = std::clamp(draw, 0.0F, 1.0F) * max_draw_distance / length;
        nock.x += pull_direction.x * distance;
        nock.y += pull_direction.y * distance;
        nock.z += pull_direction.z * distance;
    }
    return BowStringShape{.top = top, .nock = nock, .bottom = bottom};
}

} // namespace e5::gameplay

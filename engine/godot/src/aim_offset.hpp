#pragma once

#include <godot_cpp/classes/skeleton_modifier3d.hpp>

#include <array>
#include <cstdint>

namespace e5::bridge {

// Bends the character's spine up or down on top of whatever animation is
// playing, so the torso, arms and anything held follow the vertical aim. The
// pitch is spread over three spine bones to avoid a visible kink. Must be a
// child of the Skeleton3D it modifies.
class E5AimOffset : public godot::SkeletonModifier3D {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5AimOffset, godot::SkeletonModifier3D)

public:
    void _process_modification_with_delta(double delta) override;

    // Radians; positive aims up. Zero leaves the animation untouched.
    void set_pitch(float radians) { pitch_ = radians; }
    [[nodiscard]] float get_pitch() const { return pitch_; }

protected:
    static void _bind_methods() {}

private:
    static constexpr std::size_t bone_count = 3;

    float pitch_ = 0.0F;
    bool bones_resolved_ = false;
    std::array<std::int32_t, bone_count> bones_{-1, -1, -1};
};

} // namespace e5::bridge

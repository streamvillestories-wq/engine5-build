#pragma once

#include <godot_cpp/classes/skeleton_modifier3d.hpp>
#include <godot_cpp/variant/quaternion.hpp>

#include <cstdint>

namespace e5::bridge {

// Moves the bone long hair hangs from (`hair_root`, a child of the upper chest that sits at the
// head's joint; tools/blender/hair_rig.py --root-under makes it). The bone stays at the head and
// nods and tilts with it, but takes only a share of the head's turn to the side and follows
// with a short delay.
//
// Hair hung from the head bone itself copied all of the head: when she turned her head to aim,
// the whole length swung round to her shoulder and hid her from the player's camera, and every
// jolt of the head while running went into the hair at once, which read as jerky.
//
// Must be a child of the Skeleton3D, before the spring bones. E5ClothSimulator adds it.
class E5HairRoot : public godot::SkeletonModifier3D {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5HairRoot, godot::SkeletonModifier3D)

public:
    static constexpr const char* bone_name = "hair_root";

    void _process_modification_with_delta(double delta) override;

    // 0 = the hair keeps facing where the chest faces, 1 = it turns with the head.
    void set_turn_share(float share) { turn_share_ = share; }
    // Seconds the hair takes to catch up with a movement of the head. 0 = at once.
    void set_follow_seconds(float seconds) { follow_seconds_ = seconds; }

protected:
    static void _bind_methods() {}

private:
    float turn_share_ = 0.25F;
    float follow_seconds_ = 0.12F;
    bool resolved_ = false;
    bool started_ = false;
    std::int32_t bone_ = -1;
    std::int32_t chest_ = -1;
    std::int32_t head_ = -1;
    godot::Quaternion followed_; // the head's turn out of its rest, seen from the chest, as the hair has it
};

} // namespace e5::bridge

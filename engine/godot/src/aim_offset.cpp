#include "aim_offset.hpp"

#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cmath>

namespace e5::bridge {

void E5AimOffset::_process_modification_with_delta(double /*delta*/) {
    godot::Skeleton3D* const skeleton = get_skeleton();
    if (skeleton == nullptr || std::abs(pitch_) < 1e-4F) {
        return;
    }
    if (!bones_resolved_) {
        // Humanoid-profile names (the importer's retargeting produces them).
        constexpr std::array names{"Spine", "Chest", "UpperChest"};
        std::ranges::transform(names, bones_.begin(),
                               [skeleton](const char* name) { return skeleton->find_bone(name); });
        bones_resolved_ = true;
    }

    // The character faces +Z, so its left-right axis is X. Turning +Z toward
    // +Y (aiming up) is a negative rotation about X.
    const godot::Vector3 axis(1.0F, 0.0F, 0.0F);
    const float step = -pitch_ / static_cast<float>(bone_count);
    for (const std::int32_t bone : bones_) {
        if (bone < 0) {
            continue;
        }
        // Rotate the bone about its own origin; its children follow.
        godot::Transform3D pose = skeleton->get_bone_global_pose(bone);
        pose.basis = pose.basis.rotated(axis, step);
        skeleton->set_bone_global_pose(bone, pose);
    }
}

} // namespace e5::bridge

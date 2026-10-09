#include "hair_root.hpp"

#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cmath>

namespace e5::bridge {

void E5HairRoot::_process_modification_with_delta(double delta) {
    godot::Skeleton3D* const skeleton = get_skeleton();
    if (skeleton == nullptr) {
        return;
    }
    if (!resolved_) {
        bone_ = skeleton->find_bone(bone_name);
        chest_ = bone_ >= 0 ? skeleton->get_bone_parent(bone_) : -1;
        head_ = skeleton->find_bone("Head");
        resolved_ = true;
    }
    if (bone_ < 0 || chest_ < 0 || head_ < 0) {
        return;
    }

    const godot::Transform3D chest = skeleton->get_bone_global_pose(chest_);
    const godot::Transform3D head = skeleton->get_bone_global_pose(head_);
    const godot::Transform3D chest_rest = skeleton->get_bone_global_rest(chest_);
    const godot::Transform3D head_rest = skeleton->get_bone_global_rest(head_);

    // How the head is turned out of its rest, seen from the chest.
    const godot::Quaternion now =
        chest.basis.get_rotation_quaternion().inverse() * head.basis.get_rotation_quaternion();
    const godot::Quaternion at_rest =
        chest_rest.basis.get_rotation_quaternion().inverse() * head_rest.basis.get_rotation_quaternion();
    const godot::Quaternion turned = (now * at_rest.inverse()).normalized();

    // Split into the turn about the body's upright axis and the rest (nod, tilt): only a share
    // of the first is kept.
    const godot::Vector3 up = chest_rest.basis.get_rotation_quaternion().inverse().xform(godot::Vector3(0, 1, 0));
    const godot::Vector3 along = up * godot::Vector3(turned.x, turned.y, turned.z).dot(up);
    godot::Quaternion twist(along.x, along.y, along.z, turned.w);
    twist = twist.length_squared() > 1e-8F ? twist.normalized() : godot::Quaternion();
    const godot::Quaternion swing = turned * twist.inverse();
    const godot::Quaternion wanted = (swing * godot::Quaternion().slerp(twist, turn_share_)).normalized();

    if (!started_ || follow_seconds_ <= 0.0F) {
        followed_ = wanted;
        started_ = true;
    } else {
        const float share = 1.0F - std::exp(-static_cast<float>(delta) / follow_seconds_);
        followed_ = followed_.slerp(wanted, share).normalized();
    }

    skeleton->set_bone_pose_rotation(bone_, followed_ * skeleton->get_bone_rest(bone_).basis.get_rotation_quaternion());
    // At the head's joint, wherever the neck has carried it: the hair must not come off the scalp.
    skeleton->set_bone_pose_position(bone_, chest.affine_inverse().xform(head.origin));
}

} // namespace e5::bridge

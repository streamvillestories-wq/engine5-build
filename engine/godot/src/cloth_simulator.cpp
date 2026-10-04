#include "cloth_simulator.hpp"

#include "e5/gameplay/cloth_rig.hpp"
#include "godot_log.hpp"

#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/classes/skeleton_modifier3d.hpp>
#include <godot_cpp/classes/spring_bone_collision_capsule3d.hpp>
#include <godot_cpp/classes/spring_bone_simulator3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace e5::bridge {

void E5ClothSimulator::_bind_methods() {
    using godot::ClassDB;
    using godot::D_METHOD;
    using godot::PropertyInfo;

    ClassDB::bind_method(D_METHOD("set_hinged_parts", "parts"), &E5ClothSimulator::set_hinged_parts);
    ClassDB::bind_method(D_METHOD("get_hinged_parts"), &E5ClothSimulator::get_hinged_parts);
    ClassDB::bind_method(D_METHOD("set_stiffness", "stiffness"), &E5ClothSimulator::set_stiffness);
    ClassDB::bind_method(D_METHOD("get_stiffness"), &E5ClothSimulator::get_stiffness);
    ClassDB::bind_method(D_METHOD("set_drag", "drag"), &E5ClothSimulator::set_drag);
    ClassDB::bind_method(D_METHOD("get_drag"), &E5ClothSimulator::get_drag);
    ClassDB::bind_method(D_METHOD("set_gravity", "gravity"), &E5ClothSimulator::set_gravity);
    ClassDB::bind_method(D_METHOD("get_gravity"), &E5ClothSimulator::get_gravity);
    ClassDB::bind_method(D_METHOD("set_joint_radius", "radius"), &E5ClothSimulator::set_joint_radius);
    ClassDB::bind_method(D_METHOD("get_joint_radius"), &E5ClothSimulator::get_joint_radius);
    ClassDB::bind_method(D_METHOD("set_waist_radius", "radius"), &E5ClothSimulator::set_waist_radius);
    ClassDB::bind_method(D_METHOD("get_waist_radius"), &E5ClothSimulator::get_waist_radius);
    ClassDB::bind_method(D_METHOD("set_leg_radius", "radius"), &E5ClothSimulator::set_leg_radius);
    ClassDB::bind_method(D_METHOD("get_leg_radius"), &E5ClothSimulator::get_leg_radius);
    ClassDB::bind_method(D_METHOD("set_strand_radius", "radius"), &E5ClothSimulator::set_strand_radius);
    ClassDB::bind_method(D_METHOD("get_strand_radius"), &E5ClothSimulator::get_strand_radius);
    ClassDB::bind_method(D_METHOD("set_arm_radius", "radius"), &E5ClothSimulator::set_arm_radius);
    ClassDB::bind_method(D_METHOD("get_arm_radius"), &E5ClothSimulator::get_arm_radius);
    ClassDB::bind_method(D_METHOD("set_torso_radius", "radius"), &E5ClothSimulator::set_torso_radius);
    ClassDB::bind_method(D_METHOD("get_torso_radius"), &E5ClothSimulator::get_torso_radius);
    ClassDB::bind_method(D_METHOD("get_chain_count"), &E5ClothSimulator::get_chain_count);

    ADD_PROPERTY(PropertyInfo(godot::Variant::STRING, "hinged_parts"), "set_hinged_parts", "get_hinged_parts");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "stiffness", godot::PROPERTY_HINT_RANGE, "0,4,0.01"),
                 "set_stiffness", "get_stiffness");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "drag", godot::PROPERTY_HINT_RANGE, "0,1,0.01"), "set_drag",
                 "get_drag");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "gravity", godot::PROPERTY_HINT_RANGE, "0,4,0.01"), "set_gravity",
                 "get_gravity");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "joint_radius", godot::PROPERTY_HINT_RANGE, "0,0.3,0.005,suffix:m"),
        "set_joint_radius", "get_joint_radius");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "waist_radius", godot::PROPERTY_HINT_RANGE, "0,0.4,0.005,suffix:m"),
        "set_waist_radius", "get_waist_radius");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "leg_radius", godot::PROPERTY_HINT_RANGE, "0,0.4,0.005,suffix:m"),
                 "set_leg_radius", "get_leg_radius");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "strand_radius", godot::PROPERTY_HINT_RANGE, "0,0.3,0.005,suffix:m"),
        "set_strand_radius", "get_strand_radius");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "arm_radius", godot::PROPERTY_HINT_RANGE, "0,0.3,0.005,suffix:m"),
                 "set_arm_radius", "get_arm_radius");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "torso_radius", godot::PROPERTY_HINT_RANGE, "0,0.5,0.005,suffix:m"),
        "set_torso_radius", "get_torso_radius");
}

godot::Skeleton3D* E5ClothSimulator::find_skeleton() const {
    const godot::Node* const scope = get_parent();
    if (scope == nullptr) {
        return nullptr;
    }
    const godot::TypedArray<godot::Node> skeletons = scope->find_children("*", "Skeleton3D", true, false);
    for (const godot::Variant& node : skeletons) {
        if (auto* const skeleton = godot::Object::cast_to<godot::Skeleton3D>(node)) {
            return skeleton;
        }
    }
    return nullptr;
}

void E5ClothSimulator::_ready() {
    godot::Skeleton3D* const skeleton = find_skeleton();
    if (skeleton == nullptr) {
        logger().warn("E5ClothSimulator: no Skeleton3D found under the parent node; nothing to simulate");
        return;
    }

    std::vector<std::string> names;
    names.reserve(static_cast<std::size_t>(skeleton->get_bone_count()));
    for (std::int32_t bone = 0; bone < skeleton->get_bone_count(); ++bone) {
        names.emplace_back(skeleton->get_bone_name(bone).utf8().get_data());
    }
    const std::vector<std::string_view> views(names.begin(), names.end());
    const std::vector<gameplay::ClothChain> chains = gameplay::find_cloth_chains(views);
    if (chains.empty()) {
        logger().info("E5ClothSimulator: skeleton has no cloth_* bone chains; nothing to simulate");
        return;
    }

    // Skeleton modifiers must be children of the skeleton they modify.
    auto* const simulator = memnew(godot::SpringBoneSimulator3D);
    simulator->set_name("E5ClothSpringBones");
    skeleton->add_child(simulator);

    std::map<std::string, int> chains_per_part;
    for (const gameplay::ClothChain& chain : chains) {
        ++chains_per_part[chain.part];
    }

    const godot::PackedStringArray hinged = hinged_parts_.split(",", false);
    simulator->set_setting_count(static_cast<std::int32_t>(chains.size()));
    for (std::size_t index = 0; index < chains.size(); ++index) {
        const auto setting = static_cast<std::int32_t>(index);
        const std::vector<int>& bones = chains[index].bones;
        simulator->set_root_bone(setting, bones.front());
        simulator->set_end_bone(setting, bones.back());

        // The last bone has no child to aim at; give it a virtual tail as long
        // as the bone itself so the tip of the cloth swings too.
        const float tip_length =
            bones.size() >= 2 ? static_cast<float>(skeleton->get_bone_rest(bones.back()).origin.length()) : 0.1F;
        simulator->set_extend_end_bone(setting, true);
        simulator->set_end_bone_direction(setting, godot::SkeletonModifier3D::BONE_DIRECTION_FROM_PARENT);
        simulator->set_end_bone_length(setting, tip_length);

        simulator->set_stiffness(setting, stiffness_);
        simulator->set_drag(setting, drag_);
        simulator->set_gravity(setting, gravity_);
        // A part made of a single chain is a strand (ponytail, tail): one thick
        // rope. A part made of several chains is a sheet, each chain a thin slice.
        const bool is_strand = chains_per_part[chains[index].part] == 1;
        simulator->set_radius(setting, is_strand ? strand_radius_ : joint_radius_);
        simulator->set_enable_all_child_collisions(setting, true);
        if (hinged.has(godot::String(chains[index].part.c_str()))) {
            simulator->set_rotation_axis(setting, godot::SkeletonModifier3D::ROTATION_AXIS_X);
        }
    }

    add_body_colliders(skeleton, simulator);
    chain_count_ = static_cast<int>(chains.size());
    logger().info("E5ClothSimulator: simulating {} cloth chain(s)", chain_count_);
}

void E5ClothSimulator::add_body_colliders(godot::Skeleton3D* skeleton, godot::SpringBoneSimulator3D* simulator) const {
    struct Limb {
        const char* bone;
        const char* child; // the capsule spans from the bone to this child
        float radius;
    };
    // Humanoid-profile bone names (the importer's retargeting produces them).
    // The child is named explicitly: a bone's first child may be a cloth bone.
    const std::array limbs{
        Limb{.bone = "LeftUpperLeg", .child = "LeftLowerLeg", .radius = leg_radius_},
        Limb{.bone = "RightUpperLeg", .child = "RightLowerLeg", .radius = leg_radius_},
        Limb{.bone = "LeftLowerLeg", .child = "LeftFoot", .radius = leg_radius_},
        Limb{.bone = "RightLowerLeg", .child = "RightFoot", .radius = leg_radius_},
        Limb{.bone = "Hips", .child = "Spine", .radius = waist_radius_},
        Limb{.bone = "Spine", .child = "Chest", .radius = waist_radius_},
        Limb{.bone = "Chest", .child = "UpperChest", .radius = torso_radius_},
        Limb{.bone = "UpperChest", .child = "Neck", .radius = torso_radius_},
        Limb{.bone = "LeftShoulder", .child = "LeftUpperArm", .radius = arm_radius_ * 1.4F},
        Limb{.bone = "RightShoulder", .child = "RightUpperArm", .radius = arm_radius_ * 1.4F},
        Limb{.bone = "LeftUpperArm", .child = "LeftLowerArm", .radius = arm_radius_},
        Limb{.bone = "RightUpperArm", .child = "RightLowerArm", .radius = arm_radius_},
        Limb{.bone = "LeftLowerArm", .child = "LeftHand", .radius = arm_radius_},
        Limb{.bone = "RightLowerArm", .child = "RightHand", .radius = arm_radius_},
    };
    for (const Limb& limb : limbs) {
        const std::int32_t bone = skeleton->find_bone(limb.bone);
        const std::int32_t child = skeleton->find_bone(limb.child);
        if (limb.radius <= 0.0F || bone < 0 || child < 0 || skeleton->get_bone_parent(child) != bone) {
            continue;
        }
        const godot::Vector3 to_child = skeleton->get_bone_rest(child).origin;
        const auto length = static_cast<float>(to_child.length());
        if (length <= 0.0F) {
            continue;
        }

        auto* const capsule = memnew(godot::SpringBoneCollisionCapsule3D);
        capsule->set_name(godot::String("Collider_") + limb.bone);
        simulator->add_child(capsule);
        capsule->set_bone(bone);
        capsule->set_radius(limb.radius);
        capsule->set_height(length + 2.0F * limb.radius);
        // Capsules extend along their local Y; aim that at the child bone and
        // centre the capsule on the bone's midpoint.
        capsule->set_position_offset(to_child * 0.5F);
        capsule->set_rotation_offset(godot::Quaternion(godot::Vector3(0.0F, 1.0F, 0.0F), to_child / length));
    }
}

} // namespace e5::bridge
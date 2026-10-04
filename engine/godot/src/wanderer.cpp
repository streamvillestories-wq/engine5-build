#include "wanderer.hpp"

#include "e5/core/profiling.hpp"
#include "e5/gameplay/character_motor.hpp"
#include "enemy.hpp"
#include "godot_log.hpp"
#include "terrain.hpp"

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace e5::bridge {
namespace {

constexpr float gravity = 12.0F;     // m/s^2
constexpr float turn_speed = 1.6F;   // rad/s: something this heavy turns slowly
constexpr float moving_speed = 0.2F; // m/s: slower than this it is standing (blocked, or at rest)

} // namespace

void E5Wanderer::_bind_methods() {
    using godot::ClassDB;
    using godot::D_METHOD;
    using godot::PropertyInfo;
    ClassDB::bind_method(D_METHOD("set_animation_library", "library"), &E5Wanderer::set_animation_library);
    ClassDB::bind_method(D_METHOD("get_animation_library"), &E5Wanderer::get_animation_library);
    ClassDB::bind_method(D_METHOD("set_roam_center", "center"), &E5Wanderer::set_roam_center);
    ClassDB::bind_method(D_METHOD("get_roam_center"), &E5Wanderer::get_roam_center);
    ClassDB::bind_method(D_METHOD("set_roam_inner_radius", "metres"), &E5Wanderer::set_roam_inner_radius);
    ClassDB::bind_method(D_METHOD("get_roam_inner_radius"), &E5Wanderer::get_roam_inner_radius);
    ClassDB::bind_method(D_METHOD("set_roam_outer_radius", "metres"), &E5Wanderer::set_roam_outer_radius);
    ClassDB::bind_method(D_METHOD("get_roam_outer_radius"), &E5Wanderer::get_roam_outer_radius);
    ClassDB::bind_method(D_METHOD("set_move_speed", "speed"), &E5Wanderer::set_move_speed);
    ClassDB::bind_method(D_METHOD("get_move_speed"), &E5Wanderer::get_move_speed);
    ClassDB::bind_method(D_METHOD("set_rest_min_seconds", "seconds"), &E5Wanderer::set_rest_min_seconds);
    ClassDB::bind_method(D_METHOD("get_rest_min_seconds"), &E5Wanderer::get_rest_min_seconds);
    ClassDB::bind_method(D_METHOD("set_rest_max_seconds", "seconds"), &E5Wanderer::set_rest_max_seconds);
    ClassDB::bind_method(D_METHOD("get_rest_max_seconds"), &E5Wanderer::get_rest_max_seconds);
    ClassDB::bind_method(D_METHOD("set_walk_clip_speed", "speed"), &E5Wanderer::set_walk_clip_speed);
    ClassDB::bind_method(D_METHOD("get_walk_clip_speed"), &E5Wanderer::get_walk_clip_speed);
    ClassDB::bind_method(D_METHOD("get_distance_walked"), &E5Wanderer::get_distance_walked);

    ADD_PROPERTY(PropertyInfo(godot::Variant::OBJECT, "animation_library", godot::PROPERTY_HINT_RESOURCE_TYPE,
                              "AnimationLibrary"),
                 "set_animation_library", "get_animation_library");
    ADD_PROPERTY(PropertyInfo(godot::Variant::VECTOR3, "roam_center"), "set_roam_center", "get_roam_center");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "roam_inner_radius", godot::PROPERTY_HINT_RANGE, "0,500,0.5"),
                 "set_roam_inner_radius", "get_roam_inner_radius");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "roam_outer_radius", godot::PROPERTY_HINT_RANGE, "1,500,0.5"),
                 "set_roam_outer_radius", "get_roam_outer_radius");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "move_speed", godot::PROPERTY_HINT_RANGE, "0,20,0.1"),
                 "set_move_speed", "get_move_speed");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "rest_min_seconds", godot::PROPERTY_HINT_RANGE, "0,120,0.5"),
                 "set_rest_min_seconds", "get_rest_min_seconds");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "rest_max_seconds", godot::PROPERTY_HINT_RANGE, "0,120,0.5"),
                 "set_rest_max_seconds", "get_rest_max_seconds");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "walk_clip_speed", godot::PROPERTY_HINT_RANGE, "0.1,20,0.1"),
                 "set_walk_clip_speed", "get_walk_clip_speed");
}

void E5Wanderer::set_roam_center(const godot::Vector3& center) {
    params_.center_x = static_cast<float>(center.x);
    params_.center_z = static_cast<float>(center.z);
}

godot::Vector3 E5Wanderer::get_roam_center() const {
    return {params_.center_x, 0.0F, params_.center_z};
}

void E5Wanderer::_ready() {
    add_to_group(group_name);
    clip_idle_ = godot::StringName("idle");
    clip_walk_ = godot::StringName("walk");
    // On an island the scene only says where it stands, not how high the ground is there.
    if (const auto* const terrain =
            godot::Object::cast_to<E5Terrain>(get_tree()->get_first_node_in_group(E5Terrain::group_name))) {
        godot::Vector3 position = get_global_position();
        position.y = terrain->height_at(static_cast<float>(position.x), static_cast<float>(position.z));
        set_global_position(position);
    }
    // Each one wanders its own way.
    state_.random = static_cast<std::uint32_t>(godot::UtilityFunctions::randi()) | 1U;
    model_yaw_ = static_cast<float>(get_rotation().y);
    model_ = get_node<godot::Node3D>(godot::NodePath("Model"));
    const std::string name = godot::String(get_name()).utf8().get_data();
    if (model_ == nullptr) {
        logger().warn("E5Wanderer '{}' has no Node3D child named 'Model'; nothing will be shown", name);
    } else if (animation_library_.is_valid() && animator_.setup(this, model_, animation_library_)) {
        animator_.set_base(clip_idle_, 1.0F);
    } else {
        logger().warn("E5Wanderer '{}' has no usable animation library; it will glide", name);
    }
}

void E5Wanderer::_physics_process(double delta) {
    E5_PROFILE_SCOPE("E5Wanderer::_physics_process");
    const auto dt = static_cast<float>(delta);
    if (!ignoring_enemies_) {
        // Enemies do not stop it: it would stand for ever in front of one that never moves.
        // They still walk round it. Done here, not in _ready, so that all of them exist.
        ignoring_enemies_ = true;
        const godot::TypedArray<godot::Node> enemies = get_tree()->get_nodes_in_group(E5Enemy::group_name);
        for (const godot::Variant& node : enemies) {
            if (auto* const enemy = godot::Object::cast_to<E5Enemy>(node)) {
                add_collision_exception_with(enemy);
            }
        }
    }
    const godot::Vector3 before = get_global_position();
    const gameplay::RoamStep step =
        gameplay::step_roam(state_, static_cast<float>(before.x), static_cast<float>(before.z), params_, dt);
    state_ = step.state;

    godot::Vector3 velocity = get_velocity();
    velocity.x = step.velocity.x;
    velocity.z = step.velocity.z;
    velocity.y = is_on_floor() ? 0.0F : static_cast<float>(velocity.y) - gravity * dt;
    set_velocity(velocity);
    move_and_slide();

    // What it really covered: something in its way may have stopped it.
    godot::Vector3 moved = get_global_position() - before;
    moved.y = 0.0F;
    const float speed = dt > 0.0F ? static_cast<float>(moved.length()) / dt : 0.0F;
    distance_walked_ += static_cast<float>(moved.length());

    // It faces where it means to go.
    if (model_ != nullptr && (std::abs(step.velocity.x) > 0.01F || std::abs(step.velocity.z) > 0.01F)) {
        const float wanted = gameplay::facing_yaw(step.velocity.x, step.velocity.z);
        model_yaw_ = gameplay::turn_toward(model_yaw_, wanted, turn_speed * dt);
        model_->set_rotation(godot::Vector3(0.0F, model_yaw_, 0.0F));
    }
    if (animator_.is_ready()) {
        if (speed > moving_speed) {
            animator_.set_base(clip_walk_, speed / std::max(walk_clip_speed_, 0.1F));
        } else {
            animator_.set_base(clip_idle_, 1.0F);
        }
        animator_.update(dt);
    }
}

} // namespace e5::bridge

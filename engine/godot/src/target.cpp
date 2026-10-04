#include "target.hpp"

#include "e5/gameplay/projectile.hpp"
#include "e5/gameplay/skills.hpp"
#include "godot_log.hpp"

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/cylinder_shape3d.hpp>
#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <cmath>
#include <format>
#include <numbers>
#include <string>

namespace e5::bridge {
namespace {

constexpr float thickness = 0.08F; // metres
constexpr float quarter_turn = std::numbers::pi_v<float> * 0.5F;

// Colours of a standard archery face, counted from the outermost ring; they repeat for targets with more rings.
godot::Color ring_colour(int ring_from_outside) {
    switch (ring_from_outside % 5) {
    case 0:
        return {0.93F, 0.93F, 0.9F}; // white
    case 1:
        return {0.08F, 0.08F, 0.08F}; // black
    case 2:
        return {0.15F, 0.45F, 0.85F}; // blue
    case 3:
        return {0.85F, 0.15F, 0.12F}; // red
    default:
        return {0.97F, 0.83F, 0.15F}; // gold
    }
}

} // namespace

void E5Target::_bind_methods() {
    using godot::ClassDB;
    using godot::D_METHOD;
    using godot::PropertyInfo;

    ClassDB::bind_method(D_METHOD("set_radius", "radius"), &E5Target::set_radius);
    ClassDB::bind_method(D_METHOD("get_radius"), &E5Target::get_radius);
    ClassDB::bind_method(D_METHOD("set_rings", "rings"), &E5Target::set_rings);
    ClassDB::bind_method(D_METHOD("get_rings"), &E5Target::get_rings);
    ClassDB::bind_method(D_METHOD("get_hit_count"), &E5Target::get_hit_count);
    ClassDB::bind_method(D_METHOD("get_total_score"), &E5Target::get_total_score);
    ClassDB::bind_method(D_METHOD("get_last_score"), &E5Target::get_last_score);

    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "radius", godot::PROPERTY_HINT_RANGE, "0.1,5,0.05,suffix:m"),
                 "set_radius", "get_radius");
    ADD_PROPERTY(PropertyInfo(godot::Variant::INT, "rings", godot::PROPERTY_HINT_RANGE, "1,10,1"), "set_rings",
                 "get_rings");
}

void E5Target::_ready() {
    add_to_group(group_name);

    // Cylinders run along +Y; a quarter turn about X makes the face point along +Z.
    const godot::Vector3 face_forward(quarter_turn, 0.0F, 0.0F);

    for (int ring = 0; ring < rings_; ++ring) {
        godot::Ref<godot::StandardMaterial3D> material;
        material.instantiate();
        material->set_albedo(ring_colour(ring));
        material->set_roughness(0.9F);

        godot::Ref<godot::CylinderMesh> disc;
        disc.instantiate();
        const float ring_radius = radius_ * static_cast<float>(rings_ - ring) / static_cast<float>(rings_);
        disc->set_top_radius(ring_radius);
        disc->set_bottom_radius(ring_radius);
        // Each inner disc is a little thicker so it shows on both faces without z-fighting.
        disc->set_height(thickness + static_cast<float>(ring) * 0.004F);
        disc->set_radial_segments(48);
        disc->set_rings(0);
        disc->set_material(material);

        auto* const instance = memnew(godot::MeshInstance3D);
        instance->set_mesh(disc);
        instance->set_rotation(face_forward);
        add_child(instance);
    }

    godot::Ref<godot::CylinderShape3D> shape;
    shape.instantiate();
    shape->set_radius(radius_);
    shape->set_height(thickness);
    auto* const collision = memnew(godot::CollisionShape3D);
    collision->set_shape(shape);
    collision->set_rotation(face_forward);
    add_child(collision);

    label_ = memnew(godot::Label3D);
    label_->set_position(godot::Vector3(0.0F, radius_ + 0.25F, 0.0F));
    label_->set_billboard_mode(godot::BaseMaterial3D::BILLBOARD_ENABLED);
    label_->set_font_size(48);
    label_->set_pixel_size(0.004F);
    label_->set_outline_size(12);
    add_child(label_);
    update_label();
}

void E5Target::register_hit(const godot::Vector3& global_position, int score_multiplier) {
    // Distance from the centre, measured in the plane of the face.
    const godot::Vector3 local = to_local(global_position);
    const float distance = std::hypot(local.x, local.y);

    last_score_ = gameplay::target_ring_score(distance, radius_, rings_) * score_multiplier;
    if (score_multiplier > 1) {
        ++power_hit_count_;
    }
    total_score_ += last_score_;
    ++hit_count_;
    update_label();
    logger().info("target hit {}: {:.2f} m from the centre, {} point(s), total {}", hit_count_, distance, last_score_,
                  total_score_);
}

int E5Target::blast(godot::Node* context, const godot::Vector3& centre, float radius, int score_multiplier) {
    if (context == nullptr || !context->is_inside_tree()) {
        return 0;
    }
    int hit = 0;
    const godot::TypedArray<godot::Node> targets = context->get_tree()->get_nodes_in_group(group_name);
    for (const godot::Variant& node : targets) {
        auto* const target = godot::Object::cast_to<E5Target>(node);
        if (target == nullptr) {
            continue;
        }
        // In the target's own frame its face is a disc in the XY plane.
        const godot::Vector3 local = target->to_local(centre);
        const gameplay::Vec3 point{
            .x = static_cast<float>(local.x), .y = static_cast<float>(local.y), .z = static_cast<float>(local.z)};
        if (gameplay::distance_to_disc(point, target->radius_) > radius) {
            continue;
        }
        const float from_centre = std::hypot(point.x, point.y);
        const float pull_in = from_centre > target->radius_ ? target->radius_ / from_centre : 1.0F;
        target->register_hit(target->to_global(godot::Vector3(point.x * pull_in, point.y * pull_in, 0.0F)),
                             score_multiplier);
        ++hit;
    }
    return hit;
}

void E5Target::update_label() {
    if (label_ == nullptr) {
        return;
    }
    const std::string text = hit_count_ == 0
                                 ? std::string("no hits yet")
                                 : std::format("last {}   hits {}   total {}", last_score_, hit_count_, total_score_);
    label_->set_text(godot::String::utf8(text.c_str()));
}

} // namespace e5::bridge

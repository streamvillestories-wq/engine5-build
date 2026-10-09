#include "bow_string.hpp"

#include "e5/gameplay/bow_string.hpp"
#include "godot_log.hpp"

#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include <algorithm>

namespace e5::bridge {
namespace {

gameplay::Vec3 to_vec3(const godot::Vector3& v) {
    return {.x = static_cast<float>(v.x), .y = static_cast<float>(v.y), .z = static_cast<float>(v.z)};
}

godot::Vector3 to_godot(const gameplay::Vec3& v) {
    return {v.x, v.y, v.z};
}

// Places a unit cylinder (height 1 along +Y) so that it spans from -> to.
void span_segment(godot::MeshInstance3D* segment, const godot::Vector3& from, const godot::Vector3& to) {
    const godot::Vector3 delta = to - from;
    const auto length = static_cast<float>(delta.length());
    if (length <= 0.0F) {
        segment->set_visible(false);
        return;
    }
    segment->set_visible(true);
    const godot::Basis rotation(godot::Quaternion(godot::Vector3(0.0F, 1.0F, 0.0F), delta / length));
    segment->set_transform(
        godot::Transform3D(rotation.scaled_local(godot::Vector3(1.0F, length, 1.0F)), (from + to) * 0.5F));
}

} // namespace

void E5BowString::_bind_methods() {
    using godot::ClassDB;
    using godot::D_METHOD;
    using godot::PropertyInfo;

    ClassDB::bind_method(D_METHOD("set_draw", "draw"), &E5BowString::set_draw);
    ClassDB::bind_method(D_METHOD("get_draw"), &E5BowString::get_draw);
    ClassDB::bind_method(D_METHOD("set_max_draw_distance", "distance"), &E5BowString::set_max_draw_distance);
    ClassDB::bind_method(D_METHOD("get_max_draw_distance"), &E5BowString::get_max_draw_distance);
    ClassDB::bind_method(D_METHOD("set_pull_direction", "direction"), &E5BowString::set_pull_direction);
    ClassDB::bind_method(D_METHOD("get_pull_direction"), &E5BowString::get_pull_direction);
    ClassDB::bind_method(D_METHOD("set_thickness", "thickness"), &E5BowString::set_thickness);
    ClassDB::bind_method(D_METHOD("get_thickness"), &E5BowString::get_thickness);
    ClassDB::bind_method(D_METHOD("set_brace_height", "height"), &E5BowString::set_brace_height);
    ClassDB::bind_method(D_METHOD("get_brace_height"), &E5BowString::get_brace_height);
    ClassDB::bind_method(D_METHOD("get_nock_position"), &E5BowString::get_nock_position);
    ClassDB::bind_method(D_METHOD("set_colour", "colour"), &E5BowString::set_colour);
    ClassDB::bind_method(D_METHOD("get_colour"), &E5BowString::get_colour);
    ClassDB::bind_method(D_METHOD("set_glow", "glow"), &E5BowString::set_glow);
    ClassDB::bind_method(D_METHOD("get_glow"), &E5BowString::get_glow);
    ClassDB::bind_method(D_METHOD("refresh"), &E5BowString::refresh);

    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "draw", godot::PROPERTY_HINT_RANGE, "0,1,0.01"), "set_draw",
                 "get_draw");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "max_draw_distance", godot::PROPERTY_HINT_RANGE, "0,1.5,0.01,suffix:m"),
        "set_max_draw_distance", "get_max_draw_distance");
    ADD_PROPERTY(PropertyInfo(godot::Variant::VECTOR3, "pull_direction"), "set_pull_direction", "get_pull_direction");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "thickness", godot::PROPERTY_HINT_RANGE, "0.001,0.05,0.001,suffix:m"),
        "set_thickness", "get_thickness");
    ADD_PROPERTY(PropertyInfo(godot::Variant::COLOR, "colour"), "set_colour", "get_colour");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "glow", godot::PROPERTY_HINT_RANGE, "0,8,0.1"), "set_glow",
                 "get_glow");
}

void E5BowString::_ready() {
    refresh();
}

void E5BowString::refresh() {
    anchors_found_ = false;
    ornament_ = 0;
    const godot::Node* const bow = get_parent();
    const auto* const top =
        bow != nullptr ? godot::Object::cast_to<godot::Node3D>(bow->find_child("string_anchor_top", true, false))
                       : nullptr;
    const auto* const bottom =
        bow != nullptr ? godot::Object::cast_to<godot::Node3D>(bow->find_child("string_anchor_bottom", true, false))
                       : nullptr;
    if (top == nullptr || bottom == nullptr) {
        logger().error("E5BowString: parent has no 'string_anchor_top' / 'string_anchor_bottom' nodes; "
                       "prepare the bow with tools/blender/strip_bowstring.py");
        return;
    }
    // The anchors never move relative to the bow, so their positions are taken once.
    top_ = to_local(top->get_global_position());
    bottom_ = to_local(bottom->get_global_position());
    anchors_found_ = true;

    // What is modelled on the string goes where the nock goes.
    if (auto* const ornament =
            godot::Object::cast_to<godot::Node3D>(bow->find_child("string_ornament*", true, false))) {
        ornament_ = ornament->get_instance_id();
        ornament_offset_ = to_local(ornament->get_global_position()) - (top_ + bottom_) * 0.5F;
    }

    godot::Ref<godot::StandardMaterial3D> material;
    material.instantiate();
    material->set_albedo(colour_);
    material->set_roughness(0.9F);
    if (glow_ > 0.0F) {
        material->set_feature(godot::BaseMaterial3D::FEATURE_EMISSION, true);
        material->set_emission(colour_);
        material->set_emission_energy_multiplier(glow_);
    }

    // One unit-height mesh shared by both segments; each is stretched by its transform.
    godot::Ref<godot::CylinderMesh> mesh;
    mesh.instantiate();
    mesh->set_top_radius(thickness_ * 0.5F);
    mesh->set_bottom_radius(thickness_ * 0.5F);
    mesh->set_height(1.0F);
    mesh->set_radial_segments(6);
    mesh->set_rings(0);
    mesh->set_material(material);

    if (upper_segment_ == nullptr) {
        upper_segment_ = memnew(godot::MeshInstance3D);
        add_child(upper_segment_);
        lower_segment_ = memnew(godot::MeshInstance3D);
        add_child(lower_segment_);
    }
    upper_segment_->set_mesh(mesh);
    lower_segment_->set_mesh(mesh);

    update_segments();
}

void E5BowString::set_draw(float draw) {
    draw_ = draw;
    update_segments();
}

void E5BowString::set_max_draw_distance(float distance) {
    max_draw_distance_ = distance;
    update_segments();
}

void E5BowString::set_pull_direction(const godot::Vector3& direction) {
    pull_direction_ = direction;
    update_segments();
}

void E5BowString::set_nock_target(const godot::Vector3& local_point, float weight) {
    nock_target_ = local_point;
    nock_target_weight_ = std::clamp(weight, 0.0F, 1.0F);
    update_segments();
}

godot::Vector3 E5BowString::get_arrow_rest_position() const {
    const godot::Vector3 direction = pull_direction_.length() > 0.0F ? pull_direction_.normalized() : godot::Vector3();
    return (top_ + bottom_) * 0.5F - direction * brace_height_;
}

void E5BowString::update_segments() {
    if (!anchors_found_ || upper_segment_ == nullptr || lower_segment_ == nullptr) {
        return; // properties may be set before the node is ready
    }
    const gameplay::BowStringShape shape = gameplay::bow_string_shape(
        to_vec3(top_), to_vec3(bottom_), to_vec3(pull_direction_), max_draw_distance_, draw_);
    nock_ = to_godot(shape.nock).lerp(nock_target_, nock_target_weight_);
    span_segment(upper_segment_, top_, nock_);
    span_segment(lower_segment_, nock_, bottom_);
    if (ornament_ != 0) {
        if (auto* const ornament = godot::Object::cast_to<godot::Node3D>(godot::ObjectDB::get_instance(ornament_))) {
            ornament->set_global_position(to_global(nock_ + ornament_offset_));
        }
    }
}

} // namespace e5::bridge

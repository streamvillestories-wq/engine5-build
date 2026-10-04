#include "lightning_arc.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace e5::bridge {
namespace {

constexpr float lifetime = 0.55F;        // seconds
constexpr float refork_seconds = 0.045F; // a new shape this often
constexpr float points_per_metre = 2.5F;
constexpr float jitter = 0.16F; // metres a point strays from the straight line, per metre of root length
constexpr float max_stray = 0.7F;
constexpr float core_width = 0.09F;
constexpr float glow_width = 0.5F;

} // namespace

E5LightningArc* E5LightningArc::spawn(godot::Node* parent, const godot::Vector3& from, const godot::Vector3& to,
                                      const godot::Color& colour) {
    if (parent == nullptr) {
        return nullptr;
    }
    auto* const arc = memnew(E5LightningArc);
    arc->from_ = from;
    arc->to_ = to;
    arc->colour_ = colour;
    arc->random_.seed(static_cast<unsigned>(arc->get_instance_id()));
    parent->add_child(arc);
    return arc;
}

void E5LightningArc::_ready() {
    // The ribbon is built in world coordinates.
    set_as_top_level(true);
    set_global_transform(godot::Transform3D());

    mesh_.instantiate();
    godot::Ref<godot::StandardMaterial3D> material;
    material.instantiate();
    material->set_shading_mode(godot::BaseMaterial3D::SHADING_MODE_UNSHADED);
    material->set_transparency(godot::BaseMaterial3D::TRANSPARENCY_ALPHA);
    material->set_blend_mode(godot::BaseMaterial3D::BLEND_MODE_ADD);
    material->set_cull_mode(godot::BaseMaterial3D::CULL_DISABLED);
    material->set_flag(godot::BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
    auto* const instance = memnew(godot::MeshInstance3D);
    instance->set_mesh(mesh_);
    instance->set_material_override(material);
    instance->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
    add_child(instance);
    rebuild();
}

void E5LightningArc::_process(double delta) {
    age_ += static_cast<float>(delta);
    if (age_ >= lifetime) {
        queue_free();
        return;
    }
    until_refork_ -= static_cast<float>(delta);
    rebuild();
}

void E5LightningArc::rebuild() {
    const godot::Vector3 span = to_ - from_;
    const auto length = static_cast<float>(span.length());
    if (length < 0.01F) {
        return;
    }
    if (until_refork_ <= 0.0F || points_.empty()) {
        until_refork_ = refork_seconds;
        const int count = std::clamp(static_cast<int>(length * points_per_metre), 5, 40);
        // Two directions across the bolt to stray in.
        const godot::Vector3 along = span / length;
        const godot::Vector3 helper =
            std::abs(along.y) > 0.9F ? godot::Vector3(1.0F, 0.0F, 0.0F) : godot::Vector3(0.0F, 1.0F, 0.0F);
        const godot::Vector3 side_a = along.cross(helper).normalized();
        const godot::Vector3 side_b = along.cross(side_a);
        const float stray = std::min(jitter * std::sqrt(length), max_stray);
        std::uniform_real_distribution<float> spread(-1.0F, 1.0F);
        points_.clear();
        for (int index = 0; index <= count; ++index) {
            const float t = static_cast<float>(index) / static_cast<float>(count);
            // Both ends are fixed; the middle strays most.
            const float freedom = std::sin(t * std::numbers::pi_v<float>) * stray;
            points_.push_back(from_ + span * t + side_a * (spread(random_) * freedom) +
                              side_b * (spread(random_) * freedom));
        }
    }

    const godot::Viewport* const viewport = get_viewport();
    const godot::Camera3D* const camera = viewport != nullptr ? viewport->get_camera_3d() : nullptr;
    const godot::Vector3 eye = camera != nullptr ? camera->get_global_position() : from_ + godot::Vector3(0, 5, 5);
    // Bright at first, then fading; a flicker on top.
    const float fade = 1.0F - age_ / lifetime;
    std::uniform_real_distribution<float> flicker(0.65F, 1.0F);
    const float strength = fade * flicker(random_);

    mesh_->clear_surfaces();
    add_ribbon(eye, glow_width, godot::Color(colour_.r, colour_.g, colour_.b, 0.7F * strength));
    add_ribbon(eye, core_width, godot::Color(2.6F, 2.8F, 3.0F, strength));
}

void E5LightningArc::add_ribbon(const godot::Vector3& eye, float width, const godot::Color& colour) {
    mesh_->surface_begin(godot::Mesh::PRIMITIVE_TRIANGLE_STRIP);
    for (std::size_t index = 0; index < points_.size(); ++index) {
        const godot::Vector3& point = points_[index];
        const godot::Vector3 along =
            (points_[std::min(index + 1, points_.size() - 1)] - points_[index > 0 ? index - 1 : 0]).normalized();
        // Turned to face the eye, so the ribbon is never seen edge-on.
        godot::Vector3 side = along.cross(eye - point);
        side = side.length() > 0.0001F ? side.normalized() : godot::Vector3(0.0F, 1.0F, 0.0F);
        mesh_->surface_set_color(colour);
        mesh_->surface_add_vertex(point + side * (width * 0.5F));
        mesh_->surface_set_color(colour);
        mesh_->surface_add_vertex(point - side * (width * 0.5F));
    }
    mesh_->surface_end();
}

} // namespace e5::bridge

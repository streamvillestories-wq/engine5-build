#include "effect.hpp"

#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/gpu_particles3d.hpp>
#include <godot_cpp/classes/light3d.hpp>
#include <godot_cpp/classes/omni_light3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cstdint>

namespace e5::bridge {
namespace {

// The shockwave ring expands during the first part of the effect only.
constexpr float ring_fraction = 0.45F;
constexpr float ring_start_scale = 0.2F;

} // namespace

void E5Effect::_bind_methods() {
    using godot::ClassDB;
    using godot::D_METHOD;
    using godot::PropertyInfo;

    ClassDB::bind_method(D_METHOD("set_lifetime", "seconds"), &E5Effect::set_lifetime);
    ClassDB::bind_method(D_METHOD("get_lifetime"), &E5Effect::get_lifetime);
    ClassDB::bind_method(D_METHOD("set_ring_max_scale", "scale"), &E5Effect::set_ring_max_scale);
    ClassDB::bind_method(D_METHOD("get_ring_max_scale"), &E5Effect::get_ring_max_scale);

    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "lifetime", godot::PROPERTY_HINT_RANGE, "0.05,20,0.05,suffix:s"),
                 "set_lifetime", "get_lifetime");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "ring_max_scale", godot::PROPERTY_HINT_RANGE, "0.1,50,0.1"),
                 "set_ring_max_scale", "get_ring_max_scale");
}

void E5Effect::_ready() {
    const godot::TypedArray<godot::Node> particles = find_children("*", "GPUParticles3D", true, false);
    for (const godot::Variant& node : particles) {
        if (auto* const system = godot::Object::cast_to<godot::GPUParticles3D>(node)) {
            system->restart();
        }
    }
    const godot::TypedArray<godot::Node> lights = find_children("*", "OmniLight3D", true, false);
    for (const godot::Variant& node : lights) {
        if (auto* const light = godot::Object::cast_to<godot::OmniLight3D>(node)) {
            lights_.push_back({.light = light, .start_energy = light->get_param(godot::Light3D::PARAM_ENERGY)});
        }
    }
    ring_ = godot::Object::cast_to<godot::GeometryInstance3D>(find_child("Ring", true, false));
    if (ring_ != nullptr) {
        ring_->set_scale(godot::Vector3(ring_start_scale, ring_start_scale, ring_start_scale));
    }
}

void E5Effect::_process(double delta) {
    age_ += static_cast<float>(delta);
    if (age_ >= lifetime_) {
        queue_free();
        return;
    }
    const float t = age_ / lifetime_;

    // Lights flash and die away quickly rather than linearly.
    const float remaining = (1.0F - t) * (1.0F - t);
    for (const FadingLight& entry : lights_) {
        entry.light->set_param(godot::Light3D::PARAM_ENERGY, entry.start_energy * remaining);
    }

    if (ring_ != nullptr) {
        const float ring_t = std::min(t / ring_fraction, 1.0F);
        const float eased = 1.0F - (1.0F - ring_t) * (1.0F - ring_t); // fast at first, then settling
        const float scale = ring_start_scale + (ring_max_scale_ - ring_start_scale) * eased;
        ring_->set_scale(godot::Vector3(scale, scale, scale));
        ring_->set_transparency(ring_t);
    }
}

godot::Node3D* E5Effect::spawn(const godot::Ref<godot::PackedScene>& scene, godot::Node* parent,
                               const godot::Vector3& global_position) {
    if (scene.is_null() || parent == nullptr) {
        return nullptr;
    }
    auto* const instance = godot::Object::cast_to<godot::Node3D>(scene->instantiate());
    if (instance == nullptr) {
        return nullptr;
    }
    parent->add_child(instance);
    instance->set_global_position(global_position);
    return instance;
}

void E5Effect::set_active(godot::Node3D* effect, bool active) {
    if (effect == nullptr) {
        return;
    }
    for (std::int32_t index = 0; index < effect->get_child_count(); ++index) {
        godot::Node* const child = effect->get_child(index);
        if (auto* const particles = godot::Object::cast_to<godot::GPUParticles3D>(child)) {
            particles->set_emitting(active);
        } else if (auto* const visual = godot::Object::cast_to<godot::Node3D>(child)) {
            visual->set_visible(active);
        }
    }
}

} // namespace e5::bridge
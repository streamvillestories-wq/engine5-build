#include "arrow.hpp"

#include "combat.hpp"
#include "e5/core/profiling.hpp"
#include "effect.hpp"
#include "enemy.hpp"
#include "player_controller.hpp"

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/gpu_particles3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <cmath>
#include <cstdint>
#include <numbers>

namespace e5::bridge {
namespace {

constexpr float gravity = 9.81F;        // m/s^2
constexpr double stuck_lifetime = 20.0; // seconds a stuck arrow stays visible
constexpr float embed_depth = 0.06F;    // metres the tip sinks into what it hits
constexpr float shaft_radius = 0.006F;
constexpr int power_shot_score_multiplier = 2;
constexpr float tip_length = 0.05F;
constexpr float quarter_turn = std::numbers::pi_v<float> * 0.5F;

godot::Ref<godot::StandardMaterial3D> make_material(const godot::Color& colour, float roughness, float metallic) {
    godot::Ref<godot::StandardMaterial3D> material;
    material.instantiate();
    material->set_albedo(colour);
    material->set_roughness(roughness);
    material->set_metallic(metallic);
    return material;
}

// Cylinder meshes run along +Y; turning them -90 degrees about X lays them along -Z.
godot::MeshInstance3D* add_part(godot::Node3D* parent, const godot::Ref<godot::Mesh>& mesh, float z, float roll) {
    auto* const part = memnew(godot::MeshInstance3D);
    part->set_mesh(mesh);
    part->set_position(godot::Vector3(0.0F, 0.0F, z));
    part->set_rotation(godot::Vector3(-quarter_turn, 0.0F, roll));
    parent->add_child(part);
    return part;
}

// Basis whose -Z points along `direction`.
godot::Basis facing(const godot::Vector3& direction) {
    const godot::Vector3 up =
        std::abs(direction.y) > 0.99F ? godot::Vector3(1.0F, 0.0F, 0.0F) : godot::Vector3(0.0F, 1.0F, 0.0F);
    return godot::Basis::looking_at(direction, up);
}

godot::Vector3 to_godot(const gameplay::Vec3& v) {
    return {v.x, v.y, v.z};
}

} // namespace

void E5Arrow::_ready() {
    set_physics_process(flying_);

    godot::Ref<godot::CylinderMesh> shaft;
    shaft.instantiate();
    shaft->set_top_radius(shaft_radius);
    shaft->set_bottom_radius(shaft_radius);
    shaft->set_height(length);
    shaft->set_radial_segments(6);
    shaft->set_rings(0);
    shaft->set_material(make_material(godot::Color(0.55F, 0.4F, 0.24F), 0.8F, 0.0F));
    add_part(this, shaft, -length * 0.5F, 0.0F);

    godot::Ref<godot::CylinderMesh> tip;
    tip.instantiate();
    tip->set_top_radius(0.0F);
    tip->set_bottom_radius(shaft_radius * 2.2F);
    tip->set_height(tip_length);
    tip->set_radial_segments(6);
    tip->set_rings(0);
    tip->set_material(make_material(godot::Color(0.7F, 0.72F, 0.75F), 0.35F, 1.0F));
    add_part(this, tip, -(length + tip_length * 0.5F), 0.0F);

    godot::Ref<godot::BoxMesh> vane;
    vane.instantiate();
    vane->set_size(godot::Vector3(0.05F, 0.11F, 0.002F));
    vane->set_material(make_material(godot::Color(0.9F, 0.9F, 0.86F), 0.9F, 0.0F));
    add_part(this, vane, -0.09F, 0.0F);
    add_part(this, vane, -0.09F, quarter_turn);
}

void E5Arrow::set_power(float power, const godot::Ref<godot::PackedScene>& trail_effect,
                        const godot::Ref<godot::PackedScene>& impact_effect) {
    power_ = power;
    impact_effect_ = impact_effect;
    if (power_ > 0.0F) {
        set_trail_effect(trail_effect);
    }
}

void E5Arrow::set_trail_effect(const godot::Ref<godot::PackedScene>& effect) {
    if (effect.is_null() || trail_ != nullptr) {
        return;
    }
    trail_ = godot::Object::cast_to<godot::Node3D>(effect->instantiate());
    if (trail_ != nullptr) {
        add_child(trail_);
        trail_->set_position(godot::Vector3(0.0F, 0.0F, -length));
    }
}

void E5Arrow::launch(const godot::Vector3& velocity, const godot::RID& shooter) {
    const godot::Vector3 start = get_global_position();
    projectile_.position = {
        .x = static_cast<float>(start.x), .y = static_cast<float>(start.y), .z = static_cast<float>(start.z)};
    projectile_.velocity = {
        .x = static_cast<float>(velocity.x), .y = static_cast<float>(velocity.y), .z = static_cast<float>(velocity.z)};
    shooter_ = shooter;
    flying_ = true;
    age_seconds_ = 0.0;
    set_physics_process(true);
}

void E5Arrow::_physics_process(double delta) {
    age_seconds_ += delta;
    if (!flying_) {
        if (age_seconds_ > stuck_lifetime) {
            queue_free();
        }
        return;
    }
    if (age_seconds_ > flight_lifetime_) {
        queue_free();
        return;
    }
    E5_PROFILE_SCOPE("E5Arrow::_physics_process");

    const gameplay::Projectile next = gameplay::step_projectile(projectile_, gravity, static_cast<float>(delta));
    const godot::Vector3 from = to_godot(projectile_.position);
    const godot::Vector3 to = to_godot(next.position);
    const godot::Vector3 direction = to_godot(next.velocity).normalized();

    // Sweep the tip's path for this step: a fast arrow moves more than a
    // metre per step and would otherwise pass through thin targets.
    godot::TypedArray<godot::RID> excluded;
    excluded.push_back(shooter_);
    if (!shaft_swept_) {
        // Once, as it leaves the string: what stands where its own shaft is. The sweep below
        // starts at the tip, an arrow's length ahead of the string: whoever stood closer than
        // that was never touched, and a point-blank shot went through them (bug report 10).
        shaft_swept_ = true;
        const godot::Ref<godot::PhysicsRayQueryParameters3D> along_shaft =
            godot::PhysicsRayQueryParameters3D::create(from, from + direction * length, 0xFFFFFFFF, excluded);
        const godot::Dictionary close = get_world_3d()->get_direct_space_state()->intersect_ray(along_shaft);
        if (!close.is_empty()) {
            stick(close["position"], direction, close["collider"]);
            return;
        }
    }
    const godot::Ref<godot::PhysicsRayQueryParameters3D> query = godot::PhysicsRayQueryParameters3D::create(
        from + direction * length, to + direction * length, 0xFFFFFFFF, excluded);
    const godot::Dictionary hit = get_world_3d()->get_direct_space_state()->intersect_ray(query);
    if (!hit.is_empty()) {
        stick(hit["position"], direction, hit["collider"]);
        return;
    }

    projectile_ = next;
    set_global_transform(godot::Transform3D(facing(direction), to));
}

void E5Arrow::stick(const godot::Vector3& hit_position, const godot::Vector3& direction, godot::Object* collider) {
    flying_ = false;
    age_seconds_ = 0.0;
    // Leave the arrow where it struck, tip slightly buried.
    set_global_transform(godot::Transform3D(facing(direction), hit_position - direction * (length - embed_depth)));

    if (trail_ != nullptr) {
        // Let the sparks already in the air fade out; switch off the glow.
        E5Effect::set_active(trail_, false);
    }
    E5Effect::spawn(impact_effect_, get_parent(), hit_position);

    const int multiplier = power_ > 0.0F ? power_shot_score_multiplier : 1;
    if (blast_radius_ > 0.0F) {
        // An explosion hits everything in reach, including what the arrow struck.
        combat::blast(this, hit_position, blast_radius_, damage_, multiplier);
    } else {
        combat::hit(collider, hit_position, damage_, multiplier);
    }
    // Stuck in a body that moves, the arrow has to move with it.
    if (auto* const enemy = godot::Object::cast_to<E5Enemy>(collider)) {
        // The collision shape is wider than the body inside it: push the arrow in until it reaches the body.
        translate_object_local(godot::Vector3(0.0F, 0.0F, -enemy->get_body_radius() * 0.6F));
        enemy->attach(this);
    } else if (auto* const hero = godot::Object::cast_to<E5PlayerController>(collider)) {
        // Another player's hero: she walks on, and the arrow goes with her. (It stayed where
        // it struck, hanging in the air once she had moved: bug report 9.) Her body is a
        // capsule wider than she is: pushed in a little, the arrow reaches her.
        translate_object_local(godot::Vector3(0.0F, 0.0F, -0.22F));
        call_deferred("reparent", hero);
    }
}

} // namespace e5::bridge

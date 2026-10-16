#include "spell_bolt.hpp"

#include "combat.hpp"
#include "e5/core/profiling.hpp"
#include "effect.hpp"
#include "enemy.hpp"
#include "lingering.hpp"
#include "player_controller.hpp"

#include <godot_cpp/classes/scene_tree.hpp>

#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <cmath>
#include <cstdint>
#include <unordered_map>

namespace e5::bridge {
namespace {

constexpr double flight_lifetime = 4.0;    // seconds after which a spell that has hit nothing is gone
constexpr float homing_last_metres = 4.0F; // see _physics_process
constexpr double fade_seconds = 1.0;       // the trail's last sparks die out before the node is removed

// Which poison is on which enemy (ids: either may be gone), so that one struck again is poisoned
// anew and not twice over.
std::unordered_map<std::uint64_t, std::uint64_t>& poisons() {
    static std::unordered_map<std::uint64_t, std::uint64_t> on;
    return on;
}

} // namespace

void E5SpellBolt::set_trail_effect(const godot::Ref<godot::PackedScene>& effect, float scale) {
    if (effect.is_null() || trail_ != nullptr) {
        return;
    }
    trail_ = godot::Object::cast_to<godot::Node3D>(effect->instantiate());
    if (trail_ != nullptr) {
        add_child(trail_);
        trail_->set_scale(godot::Vector3(scale, scale, scale));
    }
}

void E5SpellBolt::set_homing(const godot::Vector3& target, float speed, float turn) {
    homing_target_ = target;
    homing_speed_ = speed;
    homing_turn_ = turn;
}

void E5SpellBolt::set_homing_on(const godot::Node3D* target) {
    homing_on_ = target != nullptr ? target->get_instance_id() : 0;
}

void E5SpellBolt::launch(const godot::Vector3& velocity, const godot::RID& caster) {
    velocity_ = velocity;
    caster_ = caster;
    flying_ = true;
    age_seconds_ = 0.0;
    set_physics_process(true);
}

void E5SpellBolt::_physics_process(double delta) {
    age_seconds_ += delta;
    if (!flying_) {
        if (age_seconds_ > fade_seconds) {
            queue_free();
        }
        return;
    }
    if (age_seconds_ > flight_lifetime) {
        queue_free();
        return;
    }
    E5_PROFILE_SCOPE("E5SpellBolt::_physics_process");

    if (launch_delay_ > 0.0F) {
        launch_delay_ -= static_cast<float>(delta);
        return;
    }
    const godot::Vector3 from = get_global_position();
    if (homing_on_ != 0) {
        const auto* const enemy = godot::Object::cast_to<E5Enemy>(godot::ObjectDB::get_instance(homing_on_));
        if (enemy != nullptr && enemy->is_alive()) {
            homing_target_ = enemy->get_aim_point();
        } else {
            homing_on_ = 0; // gone or dead: on to where it was last
        }
    }
    if (homing_speed_ > 0.0F) {
        // Steer the velocity towards the one that leads straight to the target.
        const godot::Vector3 wanted = (homing_target_ - from).normalized() * homing_speed_;
        velocity_ = velocity_.move_toward(wanted, homing_turn_ * static_cast<float>(delta));
        if (homing_on_ != 0 && from.distance_to(homing_target_) < homing_last_metres) {
            // Close to an enemy it follows, it goes straight in: turning as it does far off, it would
            // circle one that stands near (its turn is metres wide) and never strike.
            velocity_ = wanted;
        }
    }
    const godot::Vector3 to = from + velocity_ * static_cast<float>(delta);
    godot::TypedArray<godot::RID> excluded;
    excluded.push_back(caster_);
    if (hostile_) {
        // Its own kind does not stop it.
        const godot::TypedArray<godot::Node> enemies = get_tree()->get_nodes_in_group(E5Enemy::group_name);
        for (const godot::Variant& node : enemies) {
            if (const auto* const enemy = godot::Object::cast_to<E5Enemy>(node)) {
                excluded.push_back(enemy->get_rid());
            }
        }
    }
    const godot::Ref<godot::PhysicsRayQueryParameters3D> query =
        godot::PhysicsRayQueryParameters3D::create(from, to, 0xFFFFFFFF, excluded);
    const godot::Dictionary hit = get_world_3d()->get_direct_space_state()->intersect_ray(query);
    if (!hit.is_empty()) {
        burst(hit["position"], hit["collider"]);
        return;
    }
    // Its -Z points where it flies, so a trail effect can have a streak along the flight.
    if (velocity_.length_squared() > 0.0001F) {
        const godot::Vector3 direction = velocity_.normalized();
        const godot::Vector3 up =
            std::abs(direction.y) > 0.99F ? godot::Vector3(1.0F, 0.0F, 0.0F) : godot::Vector3(0.0F, 1.0F, 0.0F);
        set_global_transform(godot::Transform3D(godot::Basis::looking_at(direction, up), to));
    } else {
        set_global_position(to);
    }
}

void E5SpellBolt::burst(const godot::Vector3& position, godot::Object* collider) {
    flying_ = false;
    age_seconds_ = 0.0;
    set_global_position(position);
    if (trail_ != nullptr) {
        // Let the sparks already in the air fade out; switch off the glow.
        E5Effect::set_active(trail_, false);
    }
    godot::Node3D* const impact = E5Effect::spawn(impact_effect_, get_parent(), position);
    if (impact != nullptr && impact_scale_ != 1.0F) {
        impact->set_scale(godot::Vector3(impact_scale_, impact_scale_, impact_scale_));
    }
    if (hostile_) {
        if (auto* const player = godot::Object::cast_to<E5PlayerController>(collider)) {
            // From where it came: back along its flight.
            player->take_damage_from(damage_, position - velocity_.normalized() * 2.0F);
        }
        return;
    }
    if (blast_radius_ > 0.0F) {
        combat::blast(this, position, blast_radius_, damage_);
    } else {
        combat::hit(collider, position, damage_);
    }
    if (stun_seconds_ > 0.0F && damage_ > 0.0F) {
        const godot::TypedArray<godot::Node> enemies = get_tree()->get_nodes_in_group(E5Enemy::group_name);
        for (const godot::Variant& node : enemies) {
            auto* const caught = godot::Object::cast_to<E5Enemy>(node);
            if (caught == nullptr || !caught->is_alive() ||
                caught->get_aim_point().distance_to(position) > blast_radius_ + caught->get_body_radius()) {
                continue;
            }
            caught->stun(stun_seconds_);
            E5Effect::spawn(stun_visual_, caught,
                            caught->get_aim_point() + godot::Vector3(0.0F, caught->get_body_radius() + 0.7F, 0.0F));
        }
    }
    auto* const struck = godot::Object::cast_to<E5Enemy>(collider);
    if (poison_seconds_ > 0.0F && struck != nullptr && struck->is_alive()) {
        // (A small circle that goes with it: it hurts that one, and whoever stands right against it.)
        std::uint64_t& on = poisons()[struck->get_instance_id()];
        if (auto* const old = godot::Object::cast_to<godot::Node>(godot::ObjectDB::get_instance(on))) {
            old->queue_free();
        }
        const E5Lingering* const poison = E5Lingering::spawn(get_parent(), struck->get_aim_point(),
                                                             {.radius = 0.25F,
                                                              .seconds = poison_seconds_,
                                                              .tick_damage = poison_tick_damage_,
                                                              .tick_seconds = poison_tick_seconds_,
                                                              .root_seconds = 0.0F,
                                                              .visual = poison_visual_,
                                                              .cage = {}},
                                                             struck);
        on = poison != nullptr ? poison->get_instance_id() : 0;
    }
}

} // namespace e5::bridge

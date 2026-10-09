#include "lingering.hpp"

#include "combat.hpp"
#include "effect.hpp"
#include "enemy.hpp"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <algorithm>

namespace e5::bridge {
namespace {

constexpr float fade_seconds = 1.4F; // its last wisps, after it has stopped working
constexpr float cage_grow_seconds = 0.22F;

} // namespace

E5Lingering* E5Lingering::spawn(godot::Node* parent, const godot::Vector3& position, const Setup& setup,
                                E5Enemy* follows) {
    if (parent == nullptr || !parent->is_inside_tree()) {
        return nullptr;
    }
    auto* const zone = memnew(E5Lingering);
    zone->setup_ = setup;
    zone->follows_ = follows != nullptr ? follows->get_instance_id() : 0;
    parent->add_child(zone);
    zone->set_global_position(position);
    if (setup.visual.is_valid()) {
        zone->visual_ = godot::Object::cast_to<godot::Node3D>(setup.visual->instantiate());
        if (zone->visual_ != nullptr) {
            zone->add_child(zone->visual_);
        }
    }
    if (setup.root_seconds > 0.0F) {
        const godot::TypedArray<godot::Node> enemies = parent->get_tree()->get_nodes_in_group(E5Enemy::group_name);
        for (const godot::Variant& node : enemies) {
            auto* const enemy = godot::Object::cast_to<E5Enemy>(node);
            if (enemy == nullptr || !enemy->is_alive() ||
                enemy->get_aim_point().distance_to(position) > setup.radius + enemy->get_body_radius()) {
                continue;
            }
            enemy->root(setup.root_seconds);
            if (setup.cage.is_null()) {
                continue;
            }
            if (auto* const cage = godot::Object::cast_to<godot::Node3D>(setup.cage->instantiate())) {
                zone->add_child(cage);
                cage->set_global_position(enemy->get_global_position());
                cage->set_scale(godot::Vector3(1.0F, 0.05F, 1.0F));
                zone->cages_.push_back(cage);
            }
        }
    }
    return zone;
}

void E5Lingering::_physics_process(double delta) {
    const auto dt = static_cast<float>(delta);
    age_ += dt;

    if (follows_ != 0) {
        auto* const enemy = godot::Object::cast_to<E5Enemy>(godot::ObjectDB::get_instance(follows_));
        if (enemy != nullptr && enemy->is_inside_tree() && enemy->is_alive()) {
            set_global_position(enemy->get_aim_point());
        } else {
            follows_ = 0; // it stays where that one fell
        }
    }

    // The brambles shoot up out of the ground, and sink back at the end.
    const float held_for = std::max(setup_.root_seconds, 0.01F);
    const float up = std::clamp(std::min(age_ / cage_grow_seconds, (held_for - age_) / cage_grow_seconds), 0.05F, 1.0F);
    for (godot::Node3D* const cage : cages_) {
        cage->set_scale(godot::Vector3(1.0F, up, 1.0F));
    }

    if (age_ < setup_.seconds) {
        until_tick_ -= dt;
        if (setup_.tick_damage > 0.0F && until_tick_ <= 0.0F) {
            until_tick_ = setup_.tick_seconds;
            combat::blast(this, get_global_position(), setup_.radius, setup_.tick_damage);
        }
        return;
    }
    if (!ended_) {
        ended_ = true;
        if (visual_ != nullptr) {
            E5Effect::set_active(visual_, false);
        }
        for (godot::Node3D* const cage : cages_) {
            cage->set_visible(false);
        }
    }
    if (age_ >= setup_.seconds + fade_seconds) {
        queue_free();
    }
}

} // namespace e5::bridge

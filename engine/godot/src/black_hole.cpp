#include "black_hole.hpp"

#include "effect.hpp"
#include "enemy.hpp"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace e5::bridge {
namespace {

// Seconds of its life.
constexpr float open_seconds = 0.6F;     // it grows to full size
constexpr float catch_until = 2.4F;      // enemies that come in reach until then are swallowed too
constexpr float pull_seconds = 1.1F;     // from where an enemy stood into the orbit
constexpr float hold_seconds = 10.0F;    // from the first catch to the spit
constexpr float collapse_seconds = 0.5F; // after the spit it shrinks away

constexpr float reach = 8.0F;          // metres: everything closer is swallowed
constexpr float orbit_radius = 2.0F;   // metres from the core: in the bright disk, outside the black sphere
constexpr float orbit_speed = 2.4F;    // radians per second
constexpr float captive_scale = 0.45F; // how small an enemy becomes inside
constexpr float spit_speed = 8.0F;     // m/s outwards
constexpr float spit_lift = 7.0F;      // m/s upwards
constexpr float spit_distance = 2.2F;  // metres from the core where they reappear
constexpr float disk_spin = 3.0F;      // radians per second

float smooth(float t) {
    const float x = std::clamp(t, 0.0F, 1.0F);
    return x * x * (3.0F - 2.0F * x);
}

} // namespace

void E5BlackHole::configure(const godot::Ref<godot::PackedScene>& look, const godot::Ref<godot::PackedScene>& burst,
                            float damage_per_second, float spit_damage) {
    look_scene_ = look;
    burst_scene_ = burst;
    damage_per_second_ = damage_per_second;
    spit_damage_ = spit_damage;
}

void E5BlackHole::_ready() {
    add_to_group(group_name);
    if (look_scene_.is_valid()) {
        look_ = godot::Object::cast_to<godot::Node3D>(look_scene_->instantiate());
        if (look_ != nullptr) {
            add_child(look_);
            look_->set_scale(godot::Vector3(0.01F, 0.01F, 0.01F));
            disk_ = godot::Object::cast_to<godot::Node3D>(look_->find_child("Disk", true, false));
        }
    }
}

void E5BlackHole::_physics_process(double delta) {
    const auto dt = static_cast<float>(delta);
    age_ += dt;

    // Its size: opening, full, collapsing.
    const float spit_at = (captives_.empty() ? catch_until : captives_.front().caught_at) + hold_seconds;
    float size = smooth(age_ / open_seconds);
    if (spat_) {
        size = 1.0F - smooth((age_ - spit_at) / collapse_seconds);
        if (age_ >= spit_at + collapse_seconds) {
            queue_free();
            return;
        }
    }
    if (look_ != nullptr) {
        const float pulse = 1.0F + 0.04F * std::sin(age_ * 7.0F);
        const float scale = std::max(size * pulse, 0.01F);
        look_->set_scale(godot::Vector3(scale, scale, scale));
    }
    if (disk_ != nullptr) {
        disk_->rotate_object_local(godot::Vector3(0.0F, 1.0F, 0.0F), disk_spin * dt);
    }

    if (spat_) {
        return;
    }
    if (age_ >= open_seconds * 0.5F && age_ <= catch_until) {
        catch_enemies();
    }
    hold(dt);
    // Nobody caught at all: it closes after the time it would have held them.
    if (age_ >= spit_at) {
        spit_out();
    }
}

void E5BlackHole::catch_enemies() {
    const godot::Vector3 centre = get_global_position();
    const godot::TypedArray<godot::Node> enemies = get_tree()->get_nodes_in_group(E5Enemy::group_name);
    for (const godot::Variant& node : enemies) {
        auto* const enemy = godot::Object::cast_to<E5Enemy>(node);
        if (enemy == nullptr || !enemy->is_alive() || enemy->is_held()) {
            continue;
        }
        const godot::Vector3 position = enemy->get_global_position();
        if (enemy->get_aim_point().distance_to(centre) > reach) {
            continue;
        }
        enemy->set_held(true);
        const godot::Vector3 offset = position - centre;
        captives_.push_back({.enemy_id = enemy->get_instance_id(),
                             .start = position,
                             .angle = std::atan2(static_cast<float>(offset.z), static_cast<float>(offset.x)),
                             .height = 0.25F * static_cast<float>(captives_.size() % 3) - 0.25F,
                             .caught_at = age_});
    }
}

void E5BlackHole::hold(float delta) {
    const godot::Vector3 centre = get_global_position();
    damage_due_ += damage_per_second_ * delta;
    const float tick = std::floor(damage_due_);
    damage_due_ -= tick;
    for (Captive& captive : captives_) {
        auto* const enemy = godot::Object::cast_to<E5Enemy>(godot::ObjectDB::get_instance(captive.enemy_id));
        if (enemy == nullptr) {
            continue;
        }
        captive.angle += orbit_speed * delta;
        const godot::Vector3 orbit =
            centre + godot::Vector3(std::cos(captive.angle) * orbit_radius, captive.height - 0.15F,
                                    std::sin(captive.angle) * orbit_radius);
        // Drawn in along a curve, shrinking on the way.
        const float pulled = smooth((age_ - captive.caught_at) / pull_seconds);
        enemy->set_global_position(captive.start.lerp(orbit, pulled));
        enemy->set_shrink(1.0F - (1.0F - captive_scale) * pulled);
        enemy->spin(orbit_speed * 2.0F * delta);
        if (tick > 0.0F && pulled >= 1.0F) {
            enemy->take_damage(tick, enemy->get_aim_point());
        }
    }
}

void E5BlackHole::spit_out() {
    spat_ = true;
    const godot::Vector3 centre = get_global_position();
    E5Effect::spawn(burst_scene_, get_parent(), centre);
    for (const Captive& captive : captives_) {
        auto* const enemy = godot::Object::cast_to<E5Enemy>(godot::ObjectDB::get_instance(captive.enemy_id));
        if (enemy == nullptr) {
            continue;
        }
        // Out in the direction it was circling, flung up and away.
        const godot::Vector3 outward(std::cos(captive.angle), 0.0F, std::sin(captive.angle));
        enemy->set_global_position(centre + outward * spit_distance - godot::Vector3(0.0F, 0.6F, 0.0F));
        enemy->set_shrink(1.0F);
        enemy->set_held(false);
        enemy->fling(outward * spit_speed + godot::Vector3(0.0F, spit_lift, 0.0F));
        enemy->take_damage(spit_damage_, enemy->get_aim_point());
    }
    captives_.clear();
}

} // namespace e5::bridge

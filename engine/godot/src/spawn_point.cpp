#include "spawn_point.hpp"

#include "enemy.hpp"
#include "godot_log.hpp"
#include "terrain.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cmath>
#include <cstdint>
#include <numbers>

namespace e5::bridge {
namespace {

constexpr float lowest_ground = 0.8F;   // metres above the sea: nothing is made in the water
constexpr float steepest_ground = 0.75F; // rise over run an enemy can still stand on
constexpr int tries_for_a_spot = 16;

// A number from 0 to 1 that depends only on what goes in: the same spots on every machine.
float chance(std::uint32_t seed, std::uint32_t index) {
    std::uint32_t value = seed * 747796405U + index * 2891336453U + 1U;
    value ^= value >> 16U;
    value *= 2246822519U;
    value ^= value >> 13U;
    value *= 3266489917U;
    value ^= value >> 16U;
    return static_cast<float>(value >> 8U) / 16777216.0F;
}

} // namespace

void E5SpawnPoint::_bind_methods() {
    using godot::ClassDB;
    using godot::D_METHOD;
    using godot::PropertyInfo;

    ClassDB::bind_method(D_METHOD("set_enemy_scene", "scene"), &E5SpawnPoint::set_enemy_scene);
    ClassDB::bind_method(D_METHOD("get_enemy_scene"), &E5SpawnPoint::get_enemy_scene);
    ClassDB::bind_method(D_METHOD("set_count", "count"), &E5SpawnPoint::set_count);
    ClassDB::bind_method(D_METHOD("get_count"), &E5SpawnPoint::get_count);
    ClassDB::bind_method(D_METHOD("set_radius", "metres"), &E5SpawnPoint::set_radius);
    ClassDB::bind_method(D_METHOD("get_radius"), &E5SpawnPoint::get_radius);
    ClassDB::bind_method(D_METHOD("set_respawn_seconds", "seconds"), &E5SpawnPoint::set_respawn_seconds);
    ClassDB::bind_method(D_METHOD("get_respawn_seconds"), &E5SpawnPoint::get_respawn_seconds);
    ClassDB::bind_method(D_METHOD("set_seed", "seed"), &E5SpawnPoint::set_seed);
    ClassDB::bind_method(D_METHOD("get_seed"), &E5SpawnPoint::get_seed);
    ClassDB::bind_method(D_METHOD("set_label", "label"), &E5SpawnPoint::set_label);
    ClassDB::bind_method(D_METHOD("get_label"), &E5SpawnPoint::get_label);
    ClassDB::bind_method(D_METHOD("set_kind", "kind"), &E5SpawnPoint::set_kind);
    ClassDB::bind_method(D_METHOD("get_kind"), &E5SpawnPoint::get_kind);
    ClassDB::bind_method(D_METHOD("get_alive_count"), &E5SpawnPoint::get_alive_count);
    ClassDB::bind_method(D_METHOD("spawn"), &E5SpawnPoint::spawn);

    ADD_PROPERTY(PropertyInfo(godot::Variant::OBJECT, "enemy_scene", godot::PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"),
                 "set_enemy_scene", "get_enemy_scene");
    ADD_PROPERTY(PropertyInfo(godot::Variant::INT, "count", godot::PROPERTY_HINT_RANGE, "1,40,1"), "set_count",
                 "get_count");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "radius", godot::PROPERTY_HINT_RANGE, "0,60,0.5,suffix:m"),
                 "set_radius", "get_radius");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "respawn_seconds", godot::PROPERTY_HINT_RANGE, "0,600,1,suffix:s"),
        "set_respawn_seconds", "get_respawn_seconds");
    ADD_PROPERTY(PropertyInfo(godot::Variant::INT, "seed"), "set_seed", "get_seed");
    ADD_PROPERTY(PropertyInfo(godot::Variant::STRING, "label"), "set_label", "get_label");
    ADD_PROPERTY(PropertyInfo(godot::Variant::STRING, "kind"), "set_kind", "get_kind");
}

void E5SpawnPoint::_ready() {
    add_to_group(group_name);
    // Once the whole scene is ready: the ground must be built before anything is stood on it.
    call_deferred("spawn");
}

void E5SpawnPoint::spawn() {
    if (enemy_scene_.is_null() || count_ <= 0) {
        logger().warn("spawn point '{}' has no enemy scene", godot::String(get_name()).utf8().get_data());
        return;
    }
    const auto* const terrain =
        godot::Object::cast_to<E5Terrain>(get_tree()->get_first_node_in_group(E5Terrain::group_name));
    const godot::Vector3 centre = get_global_position();
    const auto place_seed = static_cast<std::uint32_t>(seed_) * 7919U +
                            static_cast<std::uint32_t>(static_cast<std::int32_t>(std::lround(centre.x * 3.0))) * 31U +
                            static_cast<std::uint32_t>(static_cast<std::int32_t>(std::lround(centre.z * 3.0)));
    int made = 0;
    for (int index = 0; index < count_; ++index) {
        // A spot within the radius, evenly over its area; another if this one will not do.
        godot::Vector3 spot = centre;
        bool found = terrain == nullptr;
        for (int attempt = 0; attempt < tries_for_a_spot; ++attempt) {
            const auto draw = static_cast<std::uint32_t>(index * tries_for_a_spot + attempt) * 3U;
            const float away = radius_ * std::sqrt(chance(place_seed, draw));
            const float around = chance(place_seed, draw + 1U) * 2.0F * std::numbers::pi_v<float>;
            spot = centre + godot::Vector3(std::cos(around) * away, 0.0F, std::sin(around) * away);
            if (terrain == nullptr) {
                break;
            }
            const auto x = static_cast<float>(spot.x);
            const auto z = static_cast<float>(spot.z);
            if (terrain->height_at(x, z) >= lowest_ground && terrain->slope_at(x, z) <= steepest_ground) {
                found = true;
                break;
            }
        }
        if (!found) {
            continue;
        }
        auto* const enemy = godot::Object::cast_to<godot::Node3D>(enemy_scene_->instantiate());
        if (enemy == nullptr) {
            continue;
        }
        if (auto* const as_enemy = godot::Object::cast_to<E5Enemy>(enemy)) {
            as_enemy->set_respawn_seconds(respawn_seconds_);
        }
        // Where it stands and which way it looks are set before it is ready: its home is
        // where it finds itself then, and it puts itself on the ground.
        enemy->set_position(spot - centre);
        enemy->set_rotation(godot::Vector3(
            0.0F, chance(place_seed, static_cast<std::uint32_t>(index) * 3U + 2U) * 2.0F * std::numbers::pi_v<float>,
            0.0F));
        add_child(enemy);
        ++made;
    }
    if (made < count_) {
        logger().warn("spawn point '{}': room for only {} of {}", godot::String(get_name()).utf8().get_data(), made,
                      count_);
    }
}

int E5SpawnPoint::get_alive_count() const {
    int alive = 0;
    const godot::TypedArray<godot::Node> children = get_children();
    for (const godot::Variant& child : children) {
        if (const auto* const enemy = godot::Object::cast_to<E5Enemy>(child)) {
            alive += enemy->is_alive() ? 1 : 0;
        }
    }
    return alive;
}

} // namespace e5::bridge

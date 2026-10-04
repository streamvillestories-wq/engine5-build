#include "pickup.hpp"

#include "e5/gameplay/inventory.hpp"
#include "inventory.hpp"
#include "player_controller.hpp"

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/gradient.hpp>
#include <godot_cpp/classes/gradient_texture2d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/omni_light3d.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/sprite3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace e5::bridge {
namespace {

constexpr float hop_seconds = 0.5F;
constexpr float hop_height = 1.1F;    // metres above the straight line, at the top of the hop
constexpr float hover_height = 0.45F; // metres above the ground, where the icon floats
constexpr float bob_height = 0.06F;
constexpr float icon_size = 0.34F;         // metres
constexpr float magnet_range = 2.8F;       // metres: from here it flies to the player
constexpr float leave_range = 4.0F;        // metres: a thrown-away item wakes up beyond this
constexpr float collect_range = 0.55F;     // metres from the player's chest
constexpr float chest_height = 1.0F;       // metres above the player's feet
constexpr float home_acceleration = 30.0F; // m/s^2
constexpr float lifetime_seconds = 180.0F;
constexpr float fade_seconds = 3.0F;
constexpr float full_bag_retry_seconds = 2.0F;
constexpr float ray_up = 1.5F; // metres above the target the ground is looked for from
constexpr float ray_down = 6.0F;

[[nodiscard]] godot::Color gold_color() {
    return {1.0F, 0.82F, 0.35F};
}

} // namespace

void E5Pickup::_bind_methods() {
    using godot::ClassDB;
    using godot::D_METHOD;
    using godot::PropertyInfo;
    ClassDB::bind_method(D_METHOD("set_item", "item"), &E5Pickup::set_item);
    ClassDB::bind_method(D_METHOD("get_item"), &E5Pickup::get_item);
    ClassDB::bind_method(D_METHOD("set_count", "count"), &E5Pickup::set_count);
    ClassDB::bind_method(D_METHOD("get_count"), &E5Pickup::get_count);
    ClassDB::bind_method(D_METHOD("set_gold", "gold"), &E5Pickup::set_gold);
    ClassDB::bind_method(D_METHOD("get_gold"), &E5Pickup::get_gold);
    ADD_PROPERTY(PropertyInfo(godot::Variant::INT, "item"), "set_item", "get_item");
    ADD_PROPERTY(PropertyInfo(godot::Variant::INT, "count"), "set_count", "get_count");
    ADD_PROPERTY(PropertyInfo(godot::Variant::INT, "gold"), "set_gold", "get_gold");
}

E5Pickup* E5Pickup::spawn(godot::Node* parent, const godot::Vector3& from, const godot::Vector3& to, int item,
                          int count, int gold) {
    auto* const pickup = memnew(E5Pickup);
    pickup->item_ = item;
    pickup->count_ = count;
    pickup->gold_ = gold;
    pickup->phase_ = Phase::Hop;
    pickup->hop_from_ = from;
    pickup->hop_to_ = to;
    parent->add_child(pickup);
    pickup->set_global_position(from);

    // The spot it lands on is on the ground, wherever that is under the target.
    const godot::Ref<godot::World3D> world = pickup->get_world_3d();
    if (world.is_valid() && world->get_direct_space_state() != nullptr) {
        const godot::Ref<godot::PhysicsRayQueryParameters3D> query = godot::PhysicsRayQueryParameters3D::create(
            to + godot::Vector3(0.0F, ray_up, 0.0F), to - godot::Vector3(0.0F, ray_down, 0.0F), 1U);
        const godot::Dictionary hit = world->get_direct_space_state()->intersect_ray(query);
        if (hit.has("position")) {
            pickup->hop_to_ = hit["position"];
        }
    }
    pickup->hop_to_.y += hover_height;
    return pickup;
}

void E5Pickup::_ready() {
    add_to_group(group_name);
    build();
}

void E5Pickup::build() {
    const bool is_gold = gold_ > 0;
    const gameplay::ItemInfo& info = gameplay::item_info(static_cast<gameplay::ItemId>(std::max(item_, 0)));
    const godot::Color colour = is_gold ? gold_color() : rarity_color(info.rarity);
    const godot::String key =
        is_gold ? godot::String("gold") : godot::String::utf8(info.key.data(), static_cast<int64_t>(info.key.size()));

    icon_ = memnew(godot::Sprite3D);
    const godot::String path = godot::String("res://ui/icons/") + key + godot::String(".svg");
    if (godot::ResourceLoader::get_singleton()->exists(path)) {
        const godot::Ref<godot::Texture2D> texture = godot::ResourceLoader::get_singleton()->load(path);
        if (texture.is_valid()) {
            icon_->set_texture(texture);
            icon_->set_pixel_size(icon_size / static_cast<float>(std::max(texture->get_width(), 1)));
        }
    }
    icon_->set_billboard_mode(godot::BaseMaterial3D::BILLBOARD_ENABLED);
    // Brighter than white, so the glow of the picture picks it up.
    icon_->set_modulate(colour.lerp(godot::Color(1.0F, 1.0F, 1.0F), 0.35F) * 1.6F);
    add_child(icon_);

    auto* const light = memnew(godot::OmniLight3D);
    light->set_color(colour);
    light->set_param(godot::Light3D::PARAM_ENERGY, 0.9F);
    light->set_param(godot::Light3D::PARAM_RANGE, 2.2F);
    add_child(light);

    // A beam of light for what is worth a detour: the rarer, the taller.
    const float beam_height = is_gold ? 0.0F : static_cast<float>(info.rarity) * 1.1F;
    if (beam_height > 0.0F) {
        godot::Ref<godot::Gradient> gradient;
        gradient.instantiate();
        godot::PackedColorArray colours;
        colours.push_back(godot::Color(colour.r, colour.g, colour.b, 0.0F));
        colours.push_back(godot::Color(colour.r, colour.g, colour.b, 0.55F));
        godot::PackedFloat32Array offsets;
        offsets.push_back(0.0F);
        offsets.push_back(1.0F);
        gradient->set_colors(colours);
        gradient->set_offsets(offsets);
        godot::Ref<godot::GradientTexture2D> texture;
        texture.instantiate();
        texture->set_gradient(gradient);
        texture->set_width(4);
        texture->set_height(64);
        // Top of the picture (the beam's upper end) clear, bottom bright.
        texture->set_fill_from(godot::Vector2(0.0F, 0.0F));
        texture->set_fill_to(godot::Vector2(0.0F, 1.0F));

        godot::Ref<godot::StandardMaterial3D> material;
        material.instantiate();
        material->set_shading_mode(godot::BaseMaterial3D::SHADING_MODE_UNSHADED);
        material->set_transparency(godot::BaseMaterial3D::TRANSPARENCY_ALPHA);
        material->set_blend_mode(godot::BaseMaterial3D::BLEND_MODE_ADD);
        material->set_billboard_mode(godot::BaseMaterial3D::BILLBOARD_FIXED_Y);
        material->set_cull_mode(godot::BaseMaterial3D::CULL_DISABLED);
        material->set_texture(godot::BaseMaterial3D::TEXTURE_ALBEDO, texture);

        godot::Ref<godot::QuadMesh> quad;
        quad.instantiate();
        quad->set_size(godot::Vector2(0.14F, beam_height));
        quad->set_material(material);
        auto* const beam = memnew(godot::MeshInstance3D);
        beam->set_mesh(quad);
        beam->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
        beam->set_position(godot::Vector3(0.0F, beam_height * 0.5F - 0.2F, 0.0F));
        add_child(beam);
    }
}

void E5Pickup::_process(double delta) {
    const auto dt = static_cast<float>(delta);
    age_seconds_ += dt;
    phase_seconds_ += dt;
    retry_seconds_ = std::max(retry_seconds_ - dt, 0.0F);

    if (phase_ == Phase::Hop) {
        const float t = std::min(phase_seconds_ / hop_seconds, 1.0F);
        godot::Vector3 position = hop_from_.lerp(hop_to_, t);
        position.y += std::sin(t * std::numbers::pi_v<float>) * hop_height;
        set_global_position(position);
        if (t >= 1.0F) {
            phase_ = Phase::Rest;
            phase_seconds_ = 0.0F;
        }
        return;
    }

    auto* const player =
        godot::Object::cast_to<E5PlayerController>(get_tree()->get_first_node_in_group(E5PlayerController::group_name));
    const bool reachable = player != nullptr && !player->is_dead();
    const godot::Vector3 chest =
        reachable ? player->get_global_position() + godot::Vector3(0.0F, chest_height, 0.0F) : godot::Vector3();
    const float distance = reachable ? static_cast<float>(chest.distance_to(get_global_position())) : 1.0e9F;

    if (phase_ == Phase::Home) {
        if (!reachable) {
            phase_ = Phase::Rest;
            hop_to_ = get_global_position();
            return;
        }
        home_speed_ += home_acceleration * dt;
        const float step = home_speed_ * dt;
        if (distance <= std::max(collect_range, step)) {
            collect(*player);
            return;
        }
        set_global_position(get_global_position().move_toward(chest, step));
        return;
    }

    // Resting: it bobs where it landed and waits.
    godot::Vector3 position = hop_to_;
    position.y += std::sin(age_seconds_ * 2.2F) * bob_height;
    set_global_position(position);

    if (wait_for_leave_ && distance > leave_range) {
        wait_for_leave_ = false;
    }
    if (age_seconds_ > lifetime_seconds) {
        const float left = 1.0F - (age_seconds_ - lifetime_seconds) / fade_seconds;
        if (left <= 0.0F) {
            queue_free();
            return;
        }
        set_scale(godot::Vector3(left, left, left));
    }
    if (!reachable || wait_for_leave_ || retry_seconds_ > 0.0F || distance > magnet_range) {
        return;
    }
    const E5Inventory* const inventory = player->get_inventory();
    if (gold_ <= 0 && (inventory == nullptr || !inventory->has_room_for(item_))) {
        // Come back when there is room; asking every frame would be wasted work.
        retry_seconds_ = full_bag_retry_seconds;
        return;
    }
    phase_ = Phase::Home;
    home_speed_ = 2.0F;
}

void E5Pickup::collect(godot::Node3D& player) {
    const auto* const controller = godot::Object::cast_to<E5PlayerController>(&player);
    E5Inventory* const inventory = controller != nullptr ? controller->get_inventory() : nullptr;
    if (inventory == nullptr) {
        return;
    }
    if (gold_ > 0) {
        inventory->add_gold(gold_);
        queue_free();
        return;
    }
    count_ = inventory->add(item_, count_);
    if (count_ <= 0) {
        queue_free();
        return;
    }
    // The bag filled up on the way: the rest stays here.
    phase_ = Phase::Rest;
    hop_to_ = get_global_position();
    retry_seconds_ = full_bag_retry_seconds;
}

} // namespace e5::bridge

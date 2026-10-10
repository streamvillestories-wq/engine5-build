#include "inventory.hpp"

#include "pickup.hpp"
#include "player_controller.hpp"

#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>
#include <tuple>

namespace e5::bridge {
namespace {

constexpr std::uint64_t potion_pause_msec = 1200;
constexpr float drop_distance = 1.6F; // metres in front of the player
constexpr float chest_height = 1.0F;

[[nodiscard]] gameplay::ItemId item_from(int item) {
    return item > 0 && item < static_cast<int>(gameplay::ItemId::Count) ? static_cast<gameplay::ItemId>(item)
                                                                        : gameplay::ItemId::None;
}

[[nodiscard]] godot::String text(std::string_view view) {
    return godot::String::utf8(view.data(), static_cast<int64_t>(view.size()));
}

} // namespace

godot::Color rarity_color(gameplay::Rarity rarity) {
    switch (rarity) {
    case gameplay::Rarity::Uncommon:
        return {0.42F, 0.85F, 0.45F};
    case gameplay::Rarity::Rare:
        return {0.36F, 0.66F, 1.0F};
    case gameplay::Rarity::Epic:
        return {0.76F, 0.48F, 1.0F};
    case gameplay::Rarity::Legendary:
        return {1.0F, 0.6F, 0.18F};
    case gameplay::Rarity::Common:
        break;
    }
    return {0.82F, 0.82F, 0.8F};
}

namespace {
godot::String weapon_class_name(gameplay::WeaponClass weapon) {
    switch (weapon) {
    case gameplay::WeaponClass::Bow:
        return "Bow";
    case gameplay::WeaponClass::Sword:
        return "Sword";
    case gameplay::WeaponClass::None:
        break;
    }
    return {};
}
} // namespace

void E5Inventory::_bind_methods() {
    using godot::ClassDB;
    using godot::D_METHOD;
    using godot::PropertyInfo;
    ClassDB::bind_method(D_METHOD("get_slot_count"), &E5Inventory::get_slot_count);
    ClassDB::bind_method(D_METHOD("get_bag_slot_count"), &E5Inventory::get_bag_slot_count);
    ClassDB::bind_method(D_METHOD("get_weapon_slot"), &E5Inventory::get_weapon_slot);
    ClassDB::bind_method(D_METHOD("get_weapon_key"), &E5Inventory::get_weapon_key);
    ClassDB::bind_method(D_METHOD("get_weapon_class"), &E5Inventory::get_weapon_class);
    ClassDB::bind_method(D_METHOD("get_slot", "slot"), &E5Inventory::get_slot);
    ClassDB::bind_method(D_METHOD("describe", "item"), &E5Inventory::describe);
    ClassDB::bind_method(D_METHOD("add", "item", "count"), &E5Inventory::add);
    ClassDB::bind_method(D_METHOD("add_gold", "amount"), &E5Inventory::add_gold);
    ClassDB::bind_method(D_METHOD("get_gold"), &E5Inventory::get_gold);
    ClassDB::bind_method(D_METHOD("has_room_for", "item"), &E5Inventory::has_room_for);
    ClassDB::bind_method(D_METHOD("move", "from", "to"), &E5Inventory::move);
    ClassDB::bind_method(D_METHOD("use", "slot"), &E5Inventory::use);
    ClassDB::bind_method(D_METHOD("use_potion"), &E5Inventory::use_potion);
    ClassDB::bind_method(D_METHOD("drop", "slot"), &E5Inventory::drop);
    ClassDB::bind_method(D_METHOD("sort"), &E5Inventory::sort);
    ClassDB::bind_method(D_METHOD("count_of", "item"), &E5Inventory::count_of);
    ClassDB::bind_method(D_METHOD("get_potion_count"), &E5Inventory::get_potion_count);
    ClassDB::bind_method(D_METHOD("get_bonuses"), &E5Inventory::get_bonuses);
    ClassDB::bind_method(D_METHOD("get_potion_cooldown"), &E5Inventory::get_potion_cooldown);

    ADD_SIGNAL(godot::MethodInfo("changed"));
    ADD_SIGNAL(godot::MethodInfo("picked_up", PropertyInfo(godot::Variant::INT, "item"),
                                 PropertyInfo(godot::Variant::INT, "count")));
    ADD_SIGNAL(godot::MethodInfo("gold_gained", PropertyInfo(godot::Variant::INT, "amount")));
    ADD_SIGNAL(godot::MethodInfo("bag_full"));
    ADD_SIGNAL(godot::MethodInfo("used", PropertyInfo(godot::Variant::INT, "item")));
}

godot::Dictionary E5Inventory::get_slot(int slot) const {
    const gameplay::ItemStack stack = gameplay::stack_at(inventory_, slot);
    godot::Dictionary result;
    result["item"] = stack.empty() ? 0 : static_cast<int>(stack.item);
    result["count"] = stack.empty() ? 0 : stack.count;
    return result;
}

// Bound as a method of the node: scripts call it on the instance.
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
godot::Dictionary E5Inventory::describe(int item) const {
    const gameplay::ItemInfo& info = gameplay::item_info(item_from(item));
    static constexpr std::array<const char*, 4> kinds{"Consumable", "Material", "Charm", "Weapon"};
    static constexpr std::array<const char*, 5> rarities{"Common", "Uncommon", "Rare", "Epic", "Legendary"};
    godot::Dictionary result;
    result["key"] = text(info.key);
    result["name"] = text(info.name);
    result["description"] = text(info.description);
    result["kind"] = kinds.at(static_cast<std::size_t>(info.kind));
    result["rarity"] = rarities.at(static_cast<std::size_t>(info.rarity));
    result["color"] = rarity_color(info.rarity);
    result["max_stack"] = info.max_stack;
    result["value"] = info.value;
    result["heal"] = info.heal;
    result["bonus_health"] = info.bonus_health;
    result["bonus_regen"] = info.bonus_regen;
    result["bonus_speed"] = info.bonus_speed;
    result["weapon"] = weapon_class_name(info.weapon);
    result["level"] = info.level;
    result["damage"] = info.damage;
    result["draw_speed"] = info.draw_speed;
    result["crit_chance"] = info.crit_chance;
    result["crit_damage"] = info.crit_damage;
    return result;
}

int E5Inventory::add(int item, int count) {
    const int left = gameplay::add_item(inventory_, item_from(item), count);
    const int taken = std::max(count, 0) - left;
    if (taken > 0) {
        items_picked_ += taken;
        emit_signal("picked_up", item, taken);
        emit_signal("changed");
    }
    if (left > 0) {
        emit_signal("bag_full");
    }
    return left;
}

void E5Inventory::give(gameplay::ItemId item, int count) {
    std::ignore = gameplay::add_item(inventory_, item, count);
}

void E5Inventory::arm(gameplay::ItemId weapon) {
    if (gameplay::item_info(weapon).kind == gameplay::ItemKind::Weapon) {
        inventory_.slots.at(static_cast<std::size_t>(gameplay::weapon_slot)) = {.item = weapon, .count = 1};
        emit_signal("changed");
    }
}

gameplay::WeaponClass E5Inventory::weapon_class() const {
    return gameplay::item_info(gameplay::stack_at(inventory_, gameplay::weapon_slot).item).weapon;
}

godot::String E5Inventory::get_weapon_class() const {
    return weapon_class_name(weapon_class());
}

godot::String E5Inventory::get_weapon_key() const {
    const gameplay::ItemStack held = gameplay::stack_at(inventory_, gameplay::weapon_slot);
    return held.empty() ? godot::String() : text(gameplay::item_info(held.item).key);
}

void E5Inventory::add_gold(int amount) {
    if (amount <= 0) {
        return;
    }
    inventory_.gold += amount;
    emit_signal("gold_gained", amount);
    emit_signal("changed");
}

bool E5Inventory::has_room_for(int item) const {
    const gameplay::ItemId id = item_from(item);
    if (id == gameplay::ItemId::None) {
        return false;
    }
    // A copy takes one: cheaper to read than a second set of rules that could drift.
    gameplay::Inventory copy = inventory_;
    return gameplay::add_item(copy, id, 1) == 0;
}

bool E5Inventory::move(int from, int to) {
    if (!gameplay::move_stack(inventory_, from, to)) {
        return false;
    }
    emit_signal("changed");
    return true;
}

bool E5Inventory::use(int slot) {
    const gameplay::ItemStack stack = gameplay::stack_at(inventory_, slot);
    if (stack.empty()) {
        return false;
    }
    const gameplay::ItemInfo& info = gameplay::item_info(stack.item);
    if (info.kind == gameplay::ItemKind::Charm || info.kind == gameplay::ItemKind::Weapon) {
        return move(slot, gameplay::equip_destination(inventory_, slot));
    }
    if (info.kind != gameplay::ItemKind::Consumable) {
        return false;
    }
    auto* const player = godot::Object::cast_to<E5PlayerController>(get_parent());
    const std::uint64_t now = godot::Time::get_singleton()->get_ticks_msec();
    // Not wasted on the healthy or the dead, and not gulped down two at a time.
    if (player == nullptr || now < potion_ready_at_msec_ || !player->heal(info.heal)) {
        return false;
    }
    potion_ready_at_msec_ = now + potion_pause_msec;
    gameplay::take_from(inventory_, slot, 1);
    emit_signal("used", static_cast<int>(stack.item));
    emit_signal("changed");
    return true;
}

bool E5Inventory::use_potion() {
    for (const gameplay::ItemId potion : {gameplay::ItemId::HealthPotion, gameplay::ItemId::GreaterHealthPotion}) {
        const int slot = gameplay::find_item(inventory_, potion);
        if (slot >= 0) {
            return use(slot);
        }
    }
    return false;
}

bool E5Inventory::drop(int slot) {
    const auto* const player = godot::Object::cast_to<E5PlayerController>(get_parent());
    const gameplay::ItemStack stack = gameplay::stack_at(inventory_, slot);
    // (The weapon in hand is not thrown away: she would stand unarmed.)
    if (player == nullptr || stack.empty() || player->get_parent() == nullptr || gameplay::is_weapon_slot(slot)) {
        return false;
    }
    gameplay::take_from(inventory_, slot, stack.count);
    const float yaw = player->get_model_yaw();
    const godot::Vector3 feet = player->get_global_position();
    const godot::Vector3 ahead(std::sin(yaw) * drop_distance, 0.0F, std::cos(yaw) * drop_distance);
    E5Pickup* const pickup = E5Pickup::spawn(player->get_parent(), feet + godot::Vector3(0.0F, chest_height, 0.0F),
                                             feet + ahead, static_cast<int>(stack.item), stack.count, 0);
    // It was just thrown away: it does not fly straight back.
    pickup->wait_until_player_left();
    emit_signal("changed");
    return true;
}

void E5Inventory::sort() {
    gameplay::sort_bag(inventory_);
    emit_signal("changed");
}

int E5Inventory::count_of(int item) const {
    return gameplay::count_of(inventory_, item_from(item));
}

int E5Inventory::get_potion_count() const {
    return gameplay::count_of(inventory_, gameplay::ItemId::HealthPotion) +
           gameplay::count_of(inventory_, gameplay::ItemId::GreaterHealthPotion);
}

godot::Dictionary E5Inventory::get_bonuses() const {
    const gameplay::Bonuses worn = bonuses();
    godot::Dictionary result;
    result["health"] = worn.health;
    result["regen"] = worn.regen;
    result["speed"] = worn.speed;
    result["damage"] = worn.damage;
    result["draw_speed"] = worn.draw_speed;
    result["crit_chance"] = gameplay::base_crit_chance + worn.crit_chance;
    result["crit_damage"] = gameplay::base_crit_damage + worn.crit_damage;
    return result;
}

float E5Inventory::get_potion_cooldown() const {
    const std::uint64_t now = godot::Time::get_singleton()->get_ticks_msec();
    if (now >= potion_ready_at_msec_) {
        return 0.0F;
    }
    return static_cast<float>(potion_ready_at_msec_ - now) / static_cast<float>(potion_pause_msec);
}

} // namespace e5::bridge

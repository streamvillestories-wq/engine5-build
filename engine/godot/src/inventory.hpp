#pragma once

#include "e5/gameplay/inventory.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <cstdint>

namespace e5::bridge {

// The colour a rarity is shown in, everywhere: slot frames, names, loot on the ground.
[[nodiscard]] godot::Color rarity_color(gameplay::Rarity rarity);

// What the player carries, as a node under the player (named `Inventory`).
// The rules are e5::gameplay's (inventory.hpp); this class holds the data,
// carries out what needs the game world (healing, dropping on the ground) and
// tells the interface when something changed. The interface is a script
// (game/ui/game_interface.gd) and only talks to the methods bound here.
class E5Inventory : public godot::Node {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5Inventory, godot::Node)

public:
    static constexpr const char* node_name = "Inventory";

    // Bound as methods of the node: scripts call them on the instance.
    // NOLINTBEGIN(readability-convert-member-functions-to-static)
    [[nodiscard]] int get_slot_count() const { return gameplay::slot_count; }
    [[nodiscard]] int get_bag_slot_count() const { return gameplay::bag_slots; }
    [[nodiscard]] int get_weapon_slot() const { return gameplay::weapon_slot; }
    // NOLINTEND(readability-convert-member-functions-to-static)
    // { item, count }; item 0 = empty.
    [[nodiscard]] godot::Dictionary get_slot(int slot) const;
    // Everything the interface shows about an item: name, description, kind, rarity, colour, effects.
    [[nodiscard]] godot::Dictionary describe(int item) const;

    // Puts items into the bag and returns how many did not fit.
    int add(int item, int count);
    // What a character starts with: no notice, and not counted as picked up.
    void give(gameplay::ItemId item, int count);
    // The weapon a character starts with, in her hand.
    void arm(gameplay::ItemId weapon);
    // The key of the weapon in hand ("bow_hunter"), or empty: the game shows that model.
    [[nodiscard]] godot::String get_weapon_key() const;
    void add_gold(int amount);
    [[nodiscard]] int get_gold() const { return inventory_.gold; }
    [[nodiscard]] bool has_room_for(int item) const;

    bool move(int from, int to);
    // A potion is drunk, a charm is put on or taken off, a weapon taken in hand. False when nothing happened.
    bool use(int slot);
    // The quick key: the smallest potion that is there.
    bool use_potion();
    // Puts the whole stack on the ground in front of the player.
    bool drop(int slot);
    void sort();

    [[nodiscard]] int count_of(int item) const;
    [[nodiscard]] int get_potion_count() const;
    // { health, regen, speed, damage, draw_speed, crit_chance, crit_damage } from what is worn and held.
    [[nodiscard]] godot::Dictionary get_bonuses() const;
    [[nodiscard]] gameplay::Bonuses bonuses() const { return gameplay::worn_bonuses(inventory_); }
    // 0..1: how much of the pause between two potions is still to wait.
    [[nodiscard]] float get_potion_cooldown() const;

    [[nodiscard]] int get_items_picked() const { return items_picked_; }

protected:
    static void _bind_methods();

private:
    gameplay::Inventory inventory_;
    int items_picked_ = 0;
    std::uint64_t potion_ready_at_msec_ = 0;
};

} // namespace e5::bridge

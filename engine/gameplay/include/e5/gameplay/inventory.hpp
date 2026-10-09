#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace e5::gameplay {

// What a character carries: a bag of stacks, a few worn charms, a weapon and a purse.
// Plain data and free functions, like the rest of the gameplay code, so a
// server can own the same rules. Nothing here knows about pictures or nodes;
// `ItemInfo::key` is what the game uses to find an item's icon.

enum class ItemId : std::uint8_t {
    None,
    HealthPotion,
    GreaterHealthPotion,
    ShroomCap,
    GlowSpore,
    CrystalShard,
    CharmVigor,
    CharmRenewal,
    CharmSwiftness,
    // The archer's bows, from plain to precious (docs/ASSET_PIPELINE.md, "Weapons: levels, rarity
    // and what they give"). New items go at the end: the numbers are sent between machines.
    BowWarden,
    BowHunter,
    BowIronbound,
    BowLeafwood,
    BowMoonglass,
    BowBriarbloom,
    BowStormfeather,
    BowNightthorn,
    BowDragonfire,
    Count,
};

enum class ItemKind : std::uint8_t {
    Consumable, // used up: a potion
    Material,   // does nothing yet; for crafting and trade later
    Charm,      // worn in a charm slot, changes the wearer while worn
    Weapon,     // held: goes into the weapon slot, and what it gives counts while it is there
};

enum class Rarity : std::uint8_t { Common, Uncommon, Rare, Epic, Legendary };

// What every archer has without any bow's help: one arrow in twenty strikes true, for half as
// much again.
inline constexpr float base_crit_chance = 0.05F;
inline constexpr float base_crit_damage = 0.5F;

struct ItemInfo {
    std::string_view key; // file-name friendly: "health_potion"
    std::string_view name;
    std::string_view description;
    ItemKind kind = ItemKind::Material;
    Rarity rarity = Rarity::Common;
    int max_stack = 1;
    int value = 0;             // gold
    float heal = 0.0F;         // health given back when used
    float bonus_health = 0.0F; // added to maximum health while worn
    float bonus_regen = 0.0F;  // added to health per second while worn
    float bonus_speed = 0.0F;  // share added to walking and running speed while worn
    // A weapon's own. All are shares: 0.18 = 18 %.
    int level = 0;            // item level: what the others are measured by
    float damage = 0.0F;      // added to the damage of everything shot with it
    float draw_speed = 0.0F;  // the bow is drawn that much faster
    float crit_chance = 0.0F; // added to the chance of a critical hit
    float crit_damage = 0.0F; // added to what a critical hit does on top
};

// `ItemId::None` and anything out of range give an empty record with `max_stack` 0.
[[nodiscard]] const ItemInfo& item_info(ItemId item) noexcept;

struct ItemStack {
    ItemId item = ItemId::None;
    int count = 0;

    [[nodiscard]] bool empty() const noexcept { return item == ItemId::None || count <= 0; }
    friend bool operator==(const ItemStack&, const ItemStack&) = default;
};

inline constexpr int bag_slots = 24;
inline constexpr int charm_slots = 3;
// Slots are numbered through: the bag first, then the charm slots, then the weapon.
inline constexpr int weapon_slot = bag_slots + charm_slots;
inline constexpr int slot_count = weapon_slot + 1;

[[nodiscard]] constexpr bool is_charm_slot(int slot) noexcept {
    return slot >= bag_slots && slot < weapon_slot;
}
[[nodiscard]] constexpr bool is_weapon_slot(int slot) noexcept {
    return slot == weapon_slot;
}

struct Inventory {
    std::array<ItemStack, slot_count> slots{};
    int gold = 0;
};

// A charm slot holds at most one charm; reading any other index gives an empty stack.
[[nodiscard]] ItemStack stack_at(const Inventory& inventory, int slot) noexcept;

// Puts items into the bag: onto stacks of the same item first, then into empty
// slots, front to back. Returns how many did not fit.
[[nodiscard]] int add_item(Inventory& inventory, ItemId item, int count) noexcept;

// Takes up to `count` out of a slot and returns what was taken.
ItemStack take_from(Inventory& inventory, int slot, int count) noexcept;

// Moves a stack onto another slot. Onto the same item it tops that stack up and
// leaves the rest behind; onto anything else the two change places. Only charms
// go into charm slots, one each, and only a weapon into the weapon slot. A weapon in
// hand is only ever changed for another, never put away: the hero would stand unarmed.
// Returns false when nothing changed.
bool move_stack(Inventory& inventory, int from, int to) noexcept;

// The slot `use` would send this stack to: a charm in the bag goes to the first free
// charm slot (the first one if all are taken), a worn charm to the first free bag
// slot. -1 when there is nowhere, or the stack is not a charm.
[[nodiscard]] int charm_destination(const Inventory& inventory, int slot) noexcept;
// The same for anything that is worn or held: a weapon in the bag goes to the weapon slot
// (the one there comes back in its place); the weapon in hand goes nowhere (-1).
[[nodiscard]] int equip_destination(const Inventory& inventory, int slot) noexcept;

// Tidies the bag: stacks of the same item are joined, then ordered by kind,
// rarity (best first) and name. Worn charms stay where they are.
void sort_bag(Inventory& inventory) noexcept;

[[nodiscard]] int count_of(const Inventory& inventory, ItemId item) noexcept;
// First bag slot holding the item, or -1.
[[nodiscard]] int find_item(const Inventory& inventory, ItemId item) noexcept;
[[nodiscard]] int free_bag_slots(const Inventory& inventory) noexcept;

struct Bonuses {
    float health = 0.0F;
    float regen = 0.0F;
    float speed = 0.0F;
    float damage = 0.0F;
    float draw_speed = 0.0F;
    float crit_chance = 0.0F;
    float crit_damage = 0.0F;
};

// What the worn charms and the weapon in hand add up to.
[[nodiscard]] Bonuses worn_bonuses(const Inventory& inventory) noexcept;

// What a hit does with those bonuses: `damage` raised by the weapon, and, if `roll` (0..1, a
// random number of the caller's) falls under the chance, by a critical hit on top.
[[nodiscard]] float weapon_hit(float damage, const Bonuses& bonuses, float roll) noexcept;
[[nodiscard]] bool is_critical(const Bonuses& bonuses, float roll) noexcept;

// What an enemy leaves behind.
struct Loot {
    int gold = 0;
    std::array<ItemStack, 3> items{}; // unused entries are empty
};

// The same seed always gives the same loot. Every kill leaves some gold; the
// rest is chance: materials often, a potion sometimes, a charm rarely.
[[nodiscard]] Loot roll_loot(std::uint32_t seed) noexcept;

} // namespace e5::gameplay

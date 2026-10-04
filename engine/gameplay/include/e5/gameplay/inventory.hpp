#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace e5::gameplay {

// What a character carries: a bag of stacks, a few worn charms and a purse.
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
    Count,
};

enum class ItemKind : std::uint8_t {
    Consumable, // used up: a potion
    Material,   // does nothing yet; for crafting and trade later
    Charm,      // worn in a charm slot, changes the wearer while worn
};

enum class Rarity : std::uint8_t { Common, Uncommon, Rare, Epic };

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
// Slots are numbered through: the bag first, then the charm slots.
inline constexpr int slot_count = bag_slots + charm_slots;

[[nodiscard]] constexpr bool is_charm_slot(int slot) noexcept {
    return slot >= bag_slots && slot < slot_count;
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
// go into charm slots, one each. Returns false when nothing changed.
bool move_stack(Inventory& inventory, int from, int to) noexcept;

// The slot `use` would send this stack to: a charm in the bag goes to the first free
// charm slot (the first one if all are taken), a worn charm to the first free bag
// slot. -1 when there is nowhere, or the stack is not a charm.
[[nodiscard]] int charm_destination(const Inventory& inventory, int slot) noexcept;

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
};

// What the worn charms add up to.
[[nodiscard]] Bonuses worn_bonuses(const Inventory& inventory) noexcept;

// What an enemy leaves behind.
struct Loot {
    int gold = 0;
    std::array<ItemStack, 3> items{}; // unused entries are empty
};

// The same seed always gives the same loot. Every kill leaves some gold; the
// rest is chance: materials often, a potion sometimes, a charm rarely.
[[nodiscard]] Loot roll_loot(std::uint32_t seed) noexcept;

} // namespace e5::gameplay

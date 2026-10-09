#include "e5/gameplay/inventory.hpp"

#include <algorithm>
#include <cstddef>
#include <tuple>

namespace e5::gameplay {
namespace {

using enum ItemId;

constexpr ItemInfo no_item{.max_stack = 0};

constexpr std::array<ItemInfo, static_cast<std::size_t>(Count)> items{{
    {.max_stack = 0},
    {.key = "health_potion",
     .name = "Health Potion",
     .description = "Bitter, red and warm. Closes wounds in a breath.",
     .kind = ItemKind::Consumable,
     .rarity = Rarity::Common,
     .max_stack = 10,
     .value = 12,
     .heal = 40.0F},
    {.key = "greater_health_potion",
     .name = "Greater Health Potion",
     .description = "Brewed from glow spores. Brings back the nearly dead.",
     .kind = ItemKind::Consumable,
     .rarity = Rarity::Uncommon,
     .max_stack = 5,
     .value = 40,
     .heal = 100.0F},
    {.key = "shroom_cap",
     .name = "Shroom Cap",
     .description = "The cap of a Shroomling. Tough, and it smells of rain.",
     .kind = ItemKind::Material,
     .rarity = Rarity::Common,
     .max_stack = 20,
     .value = 3},
    {.key = "glow_spore",
     .name = "Glow Spore",
     .description = "It still glows faintly. Alchemists pay well for these.",
     .kind = ItemKind::Material,
     .rarity = Rarity::Uncommon,
     .max_stack = 20,
     .value = 9},
    {.key = "crystal_shard",
     .name = "Crystal Shard",
     .description = "A splinter of the great tree's crystal. It hums when held.",
     .kind = ItemKind::Material,
     .rarity = Rarity::Rare,
     .max_stack = 10,
     .value = 35},
    {.key = "charm_vigor",
     .name = "Charm of Vigor",
     .description = "A heart of glass that beats with its wearer's.",
     .kind = ItemKind::Charm,
     .rarity = Rarity::Rare,
     .max_stack = 1,
     .value = 120,
     .bonus_health = 30.0F},
    {.key = "charm_renewal",
     .name = "Charm of Renewal",
     .description = "One drop that never dries.",
     .kind = ItemKind::Charm,
     .rarity = Rarity::Rare,
     .max_stack = 1,
     .value = 120,
     .bonus_regen = 3.0F},
    {.key = "charm_swiftness",
     .name = "Charm of Swiftness",
     .description = "Feather-light. The ground seems shorter with it.",
     .kind = ItemKind::Charm,
     .rarity = Rarity::Epic,
     .max_stack = 1,
     .value = 200,
     .bonus_speed = 0.12F},
    // --- the archer's bows -------------------------------------------------------------------
    // A bow's worth is its level; its rarity says how many things it gives beside damage
    // (common: none, uncommon: one, rare: two, epic: three, legendary: four). Damage is about
    // one percent a level; the rest is spent from what a rarity allows.
    {.key = "bow_warden",
     .name = "Warden's Bow",
     .description = "The bow every Warden is given with her oath. It has never failed one.",
     .kind = ItemKind::Weapon,
     .rarity = Rarity::Common,
     .max_stack = 1,
     .value = 20,
     .level = 1},
    {.key = "bow_hunter",
     .name = "Hunter's Bow",
     .description = "Yew, twine and a grip of worn leather. Honest work.",
     .kind = ItemKind::Weapon,
     .rarity = Rarity::Common,
     .max_stack = 1,
     .value = 45,
     .level = 6,
     .damage = 0.06F},
    {.key = "bow_ironbound",
     .name = "Ironbound Bow",
     .description = "Bound in iron where lesser bows crack. It hits like a door slamming.",
     .kind = ItemKind::Weapon,
     .rarity = Rarity::Uncommon,
     .max_stack = 1,
     .value = 110,
     .level = 12,
     .damage = 0.12F,
     .crit_damage = 0.15F},
    {.key = "bow_leafwood",
     .name = "Leafwood Bow",
     .description = "Cut green and never dried. It bends as if it wanted to.",
     .kind = ItemKind::Weapon,
     .rarity = Rarity::Uncommon,
     .max_stack = 1,
     .value = 120,
     .level = 13,
     .damage = 0.11F,
     .draw_speed = 0.1F},
    {.key = "bow_moonglass",
     .name = "Moonglass Bow",
     .description = "Its string is a thread of moonlight. Arrows leave it without a sound.",
     .kind = ItemKind::Weapon,
     .rarity = Rarity::Rare,
     .max_stack = 1,
     .value = 320,
     .level = 20,
     .damage = 0.2F,
     .draw_speed = 0.1F,
     .crit_chance = 0.06F},
    {.key = "bow_briarbloom",
     .name = "Briarbloom Bow",
     .description = "Still in flower. Whoever carries it heals as a hedge does.",
     .kind = ItemKind::Weapon,
     .rarity = Rarity::Rare,
     .max_stack = 1,
     .value = 340,
     .bonus_health = 20.0F,
     .bonus_regen = 1.5F,
     .level = 22,
     .damage = 0.2F},
    {.key = "bow_stormfeather",
     .name = "Stormfeather Bow",
     .description = "Fletched from a thunderbird's wing. It is lighter than it has any right to be.",
     .kind = ItemKind::Weapon,
     .rarity = Rarity::Epic,
     .max_stack = 1,
     .value = 800,
     .bonus_speed = 0.06F,
     .level = 30,
     .damage = 0.3F,
     .draw_speed = 0.18F,
     .crit_chance = 0.08F},
    {.key = "bow_nightthorn",
     .name = "Nightthorn Bow",
     .description = "It has no string that a hand can find. What it looses is not quite an arrow.",
     .kind = ItemKind::Weapon,
     .rarity = Rarity::Epic,
     .max_stack = 1,
     .value = 850,
     .level = 32,
     .damage = 0.32F,
     .crit_chance = 0.12F,
     .crit_damage = 0.4F},
    {.key = "bow_dragonfire",
     .name = "Dragonfire Bow",
     .description = "Two dragons, forever about to bite. The grip is warm, and gets warmer.",
     .kind = ItemKind::Weapon,
     .rarity = Rarity::Legendary,
     .max_stack = 1,
     .value = 2400,
     .bonus_health = 25.0F,
     .level = 40,
     .damage = 0.42F,
     .draw_speed = 0.1F,
     .crit_chance = 0.1F,
     .crit_damage = 0.5F},
}};

[[nodiscard]] constexpr bool valid_slot(int slot) noexcept {
    return slot >= 0 && slot < slot_count;
}

[[nodiscard]] ItemStack& at(Inventory& inventory, int slot) noexcept {
    return inventory.slots.at(static_cast<std::size_t>(slot));
}

[[nodiscard]] bool fits(const ItemStack& stack, int slot) noexcept {
    if (stack.empty()) {
        return true;
    }
    const ItemKind kind = item_info(stack.item).kind;
    if (is_charm_slot(slot)) {
        return kind == ItemKind::Charm;
    }
    return !is_weapon_slot(slot) || kind == ItemKind::Weapon;
}

// A small, well-known generator (xorshift32): loot must be the same on every machine.
[[nodiscard]] std::uint32_t next(std::uint32_t& state) noexcept {
    state ^= state << 13U;
    state ^= state >> 17U;
    state ^= state << 5U;
    return state;
}

// A number in [0, 1).
[[nodiscard]] float chance(std::uint32_t& state) noexcept {
    return static_cast<float>(next(state) >> 8U) / 16777216.0F;
}

} // namespace

const ItemInfo& item_info(ItemId item) noexcept {
    const auto index = static_cast<std::size_t>(item);
    return index < items.size() ? items.at(index) : no_item;
}

ItemStack stack_at(const Inventory& inventory, int slot) noexcept {
    return valid_slot(slot) ? inventory.slots.at(static_cast<std::size_t>(slot)) : ItemStack{};
}

int add_item(Inventory& inventory, ItemId item, int count) noexcept {
    const int max_stack = item_info(item).max_stack;
    if (max_stack <= 0 || count <= 0) {
        return std::max(count, 0);
    }
    for (int slot = 0; slot < bag_slots && count > 0; ++slot) {
        ItemStack& stack = at(inventory, slot);
        if (stack.item == item && stack.count < max_stack) {
            const int added = std::min(count, max_stack - stack.count);
            stack.count += added;
            count -= added;
        }
    }
    for (int slot = 0; slot < bag_slots && count > 0; ++slot) {
        ItemStack& stack = at(inventory, slot);
        if (stack.empty()) {
            const int added = std::min(count, max_stack);
            stack = {.item = item, .count = added};
            count -= added;
        }
    }
    return count;
}

ItemStack take_from(Inventory& inventory, int slot, int count) noexcept {
    if (!valid_slot(slot) || count <= 0) {
        return {};
    }
    ItemStack& stack = at(inventory, slot);
    if (stack.empty()) {
        return {};
    }
    const ItemStack taken{.item = stack.item, .count = std::min(count, stack.count)};
    stack.count -= taken.count;
    if (stack.count <= 0) {
        stack = {};
    }
    return taken;
}

bool move_stack(Inventory& inventory, int from, int to) noexcept {
    if (!valid_slot(from) || !valid_slot(to) || from == to) {
        return false;
    }
    ItemStack& source = at(inventory, from);
    ItemStack& target = at(inventory, to);
    if (source.empty() || !fits(source, to) || !fits(target, from)) {
        return false;
    }
    // The weapon in hand is changed for another, never put away.
    if (is_weapon_slot(from) && target.empty()) {
        return false;
    }
    if (target.item == source.item && !is_charm_slot(to) && !is_weapon_slot(to) && !is_weapon_slot(from)) {
        const int moved = std::min(source.count, item_info(source.item).max_stack - target.count);
        if (moved <= 0) {
            return false;
        }
        target.count += moved;
        source.count -= moved;
        if (source.count <= 0) {
            source = {};
        }
        return true;
    }
    std::swap(source, target);
    return true;
}

int charm_destination(const Inventory& inventory, int slot) noexcept {
    const ItemStack stack = stack_at(inventory, slot);
    if (stack.empty() || item_info(stack.item).kind != ItemKind::Charm) {
        return -1;
    }
    if (is_charm_slot(slot)) {
        for (int bag = 0; bag < bag_slots; ++bag) {
            if (stack_at(inventory, bag).empty()) {
                return bag;
            }
        }
        return -1;
    }
    for (int charm = bag_slots; charm < slot_count; ++charm) {
        if (stack_at(inventory, charm).empty()) {
            return charm;
        }
    }
    return bag_slots;
}

int equip_destination(const Inventory& inventory, int slot) noexcept {
    const ItemStack stack = stack_at(inventory, slot);
    if (stack.empty() || item_info(stack.item).kind != ItemKind::Weapon) {
        return charm_destination(inventory, slot);
    }
    return is_weapon_slot(slot) ? -1 : weapon_slot;
}

void sort_bag(Inventory& inventory) noexcept {
    std::array<ItemStack, bag_slots> old{};
    for (int slot = 0; slot < bag_slots; ++slot) {
        old.at(static_cast<std::size_t>(slot)) = at(inventory, slot);
        at(inventory, slot) = {};
    }
    std::ranges::stable_sort(old, [](const ItemStack& a, const ItemStack& b) {
        if (a.empty() || b.empty()) {
            return !a.empty() && b.empty();
        }
        const ItemInfo& ia = item_info(a.item);
        const ItemInfo& ib = item_info(b.item);
        // Rarity best first, so it is compared the other way round.
        return std::tuple(ia.kind, ib.rarity, ia.name) < std::tuple(ib.kind, ia.rarity, ib.name);
    });
    // Adding them back in order joins the stacks.
    for (const ItemStack& stack : old) {
        if (!stack.empty()) {
            std::ignore = add_item(inventory, stack.item, stack.count);
        }
    }
}

int count_of(const Inventory& inventory, ItemId item) noexcept {
    int count = 0;
    for (int slot = 0; slot < bag_slots; ++slot) {
        const ItemStack stack = stack_at(inventory, slot);
        if (stack.item == item) {
            count += stack.count;
        }
    }
    return count;
}

int find_item(const Inventory& inventory, ItemId item) noexcept {
    for (int slot = 0; slot < bag_slots; ++slot) {
        const ItemStack stack = stack_at(inventory, slot);
        if (stack.item == item && !stack.empty()) {
            return slot;
        }
    }
    return -1;
}

int free_bag_slots(const Inventory& inventory) noexcept {
    int count = 0;
    for (int slot = 0; slot < bag_slots; ++slot) {
        if (stack_at(inventory, slot).empty()) {
            ++count;
        }
    }
    return count;
}

Bonuses worn_bonuses(const Inventory& inventory) noexcept {
    Bonuses total;
    for (int slot = bag_slots; slot < slot_count; ++slot) {
        const ItemStack stack = stack_at(inventory, slot);
        if (!stack.empty()) {
            const ItemInfo& info = item_info(stack.item);
            total.health += info.bonus_health;
            total.regen += info.bonus_regen;
            total.speed += info.bonus_speed;
            total.damage += info.damage;
            total.draw_speed += info.draw_speed;
            total.crit_chance += info.crit_chance;
            total.crit_damage += info.crit_damage;
        }
    }
    return total;
}

bool is_critical(const Bonuses& bonuses, float roll) noexcept {
    return roll < base_crit_chance + bonuses.crit_chance;
}

float weapon_hit(float damage, const Bonuses& bonuses, float roll) noexcept {
    const float raised = damage * (1.0F + bonuses.damage);
    return is_critical(bonuses, roll) ? raised * (1.0F + base_crit_damage + bonuses.crit_damage) : raised;
}

Loot roll_loot(std::uint32_t seed) noexcept {
    // xorshift stays at zero forever, so zero is replaced.
    std::uint32_t state = seed == 0 ? 0x9E3779B9U : seed;
    // The first values of nearby seeds are alike; a few rounds part them.
    for (int round = 0; round < 4; ++round) {
        std::ignore = next(state);
    }

    Loot loot;
    loot.gold = 3 + static_cast<int>(chance(state) * 6.0F);
    std::size_t used = 0;
    const auto drop = [&loot, &used](ItemId item, int count) {
        if (used < loot.items.size()) {
            loot.items.at(used++) = {.item = item, .count = count};
        }
    };
    if (chance(state) < 0.7F) {
        drop(ShroomCap, 1 + static_cast<int>(chance(state) * 2.0F));
    }
    if (chance(state) < 0.3F) {
        drop(GlowSpore, 1);
    }
    const float potion = chance(state);
    if (potion < 0.06F) {
        drop(GreaterHealthPotion, 1);
    } else if (potion < 0.3F) {
        drop(HealthPotion, 1);
    }
    const float rare = chance(state);
    if (rare < 0.05F) {
        const std::array charms{CharmVigor, CharmRenewal, CharmSwiftness};
        drop(charms.at(next(state) % charms.size()), 1);
    } else if (rare < 0.15F) {
        drop(CrystalShard, 1);
    }
    return loot;
}

} // namespace e5::gameplay

#include "e5/gameplay/inventory.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

using Catch::Approx;
using namespace e5::gameplay;

TEST_CASE("every item is described", "[inventory]") {
    for (int id = 1; id < static_cast<int>(ItemId::Count); ++id) {
        const ItemInfo& info = item_info(static_cast<ItemId>(id));
        CHECK_FALSE(info.key.empty());
        CHECK_FALSE(info.name.empty());
        CHECK_FALSE(info.description.empty());
        CHECK(info.max_stack >= 1);
    }
    CHECK(item_info(ItemId::None).max_stack == 0);
    CHECK(item_info(ItemId::Count).max_stack == 0);
}

TEST_CASE("items stack up before they take a new slot", "[inventory]") {
    Inventory inventory;
    CHECK(add_item(inventory, ItemId::HealthPotion, 4) == 0);
    CHECK(add_item(inventory, ItemId::BowHunter, 1) == 0);
    CHECK(add_item(inventory, ItemId::HealthPotion, 8) == 0);

    // Ten to a stack: the first slot is full, the rest starts a new one behind the bow.
    CHECK(stack_at(inventory, 0) == ItemStack{.item = ItemId::HealthPotion, .count = 10});
    CHECK(stack_at(inventory, 1) == ItemStack{.item = ItemId::BowHunter, .count = 1});
    CHECK(stack_at(inventory, 2) == ItemStack{.item = ItemId::HealthPotion, .count = 2});
    CHECK(count_of(inventory, ItemId::HealthPotion) == 12);
    CHECK(free_bag_slots(inventory) == bag_slots - 3);
}

TEST_CASE("a full bag gives back what does not fit", "[inventory]") {
    Inventory inventory;
    // Weapons do not stack: one per slot.
    CHECK(add_item(inventory, ItemId::BowHunter, bag_slots + 2) == 2);
    CHECK(free_bag_slots(inventory) == 0);
    CHECK(add_item(inventory, ItemId::HealthPotion, 3) == 3);
    // Nothing went into the weapon slot.
    CHECK(stack_at(inventory, weapon_slot).empty());

    CHECK(add_item(inventory, ItemId::None, 5) == 5);
    CHECK(add_item(inventory, ItemId::HealthPotion, -1) == 0);
}

TEST_CASE("taking from a stack", "[inventory]") {
    Inventory inventory;
    REQUIRE(add_item(inventory, ItemId::HealthPotion, 5) == 0);
    CHECK(take_from(inventory, 0, 2) == ItemStack{.item = ItemId::HealthPotion, .count = 2});
    CHECK(stack_at(inventory, 0).count == 3);
    CHECK(take_from(inventory, 0, 99) == ItemStack{.item = ItemId::HealthPotion, .count = 3});
    CHECK(stack_at(inventory, 0).empty());
    CHECK(take_from(inventory, 0, 1).empty());
    CHECK(take_from(inventory, -1, 1).empty());
    CHECK(take_from(inventory, slot_count, 1).empty());
}

TEST_CASE("moving stacks swaps or tops up", "[inventory]") {
    Inventory inventory;
    REQUIRE(add_item(inventory, ItemId::HealthPotion, 3) == 0);
    REQUIRE(add_item(inventory, ItemId::BowHunter, 1) == 0);

    SECTION("onto an empty slot") {
        CHECK(move_stack(inventory, 0, 7));
        CHECK(stack_at(inventory, 0).empty());
        CHECK(stack_at(inventory, 7) == ItemStack{.item = ItemId::HealthPotion, .count = 3});
    }
    SECTION("onto another item: they change places") {
        CHECK(move_stack(inventory, 0, 1));
        CHECK(stack_at(inventory, 0).item == ItemId::BowHunter);
        CHECK(stack_at(inventory, 1).item == ItemId::HealthPotion);
    }
    SECTION("onto the same item: the stack is topped up and the rest stays") {
        inventory.slots.at(5) = {.item = ItemId::HealthPotion, .count = 9};
        CHECK(move_stack(inventory, 0, 5));
        CHECK(stack_at(inventory, 5).count == 10);
        CHECK(stack_at(inventory, 0).count == 2);
        // Full now: nothing more goes in.
        CHECK_FALSE(move_stack(inventory, 0, 5));
    }
    SECTION("nothing to move") {
        CHECK_FALSE(move_stack(inventory, 9, 3));
        CHECK_FALSE(move_stack(inventory, 0, 0));
        CHECK_FALSE(move_stack(inventory, 0, slot_count));
    }
}

TEST_CASE("sorting joins stacks and orders the bag", "[inventory]") {
    Inventory inventory;
    inventory.slots.at(3) = {.item = ItemId::HealthPotion, .count = 4};
    inventory.slots.at(8) = {.item = ItemId::BowHunter, .count = 1};
    inventory.slots.at(15) = {.item = ItemId::HealthPotion, .count = 5};
    inventory.slots.at(20) = {.item = ItemId::BowDragonfire, .count = 1};

    sort_bag(inventory);

    // What is used up first, then the weapons, the best first.
    CHECK(stack_at(inventory, 0) == ItemStack{.item = ItemId::HealthPotion, .count = 9});
    CHECK(stack_at(inventory, 1) == ItemStack{.item = ItemId::BowDragonfire, .count = 1});
    CHECK(stack_at(inventory, 2) == ItemStack{.item = ItemId::BowHunter, .count = 1});
    CHECK(free_bag_slots(inventory) == bag_slots - 3);
}

TEST_CASE("finding items", "[inventory]") {
    Inventory inventory;
    inventory.slots.at(6) = {.item = ItemId::HealthPotion, .count = 2};
    CHECK(find_item(inventory, ItemId::HealthPotion) == 6);
    CHECK(find_item(inventory, ItemId::BowHunter) == -1);
}

TEST_CASE("loot is repeatable, always has gold, and a potion turns up one time in four", "[inventory]") {
    CHECK(roll_loot(1234).gold == roll_loot(1234).gold);
    CHECK(roll_loot(1234).items == roll_loot(1234).items);

    int potions = 0;
    const int kills = 4000;
    for (std::uint32_t seed = 0; seed < static_cast<std::uint32_t>(kills); ++seed) {
        const Loot loot = roll_loot(seed);
        REQUIRE(loot.gold >= 3);
        REQUIRE(loot.gold <= 8);
        for (const ItemStack& stack : loot.items) {
            if (stack.empty()) {
                continue;
            }
            REQUIRE(stack.item == ItemId::HealthPotion);
            REQUIRE(stack.count == 1);
            ++potions;
        }
    }
    // The chance is 25 %; the bounds leave room for the generator's noise.
    CHECK(potions > kills * 19 / 100);
    CHECK(potions < kills * 31 / 100);
}

TEST_CASE("a weapon goes into the weapon slot and is only ever changed for another", "[inventory][weapon]") {
    Inventory inventory;
    inventory.slots.at(static_cast<std::size_t>(weapon_slot)) = {.item = ItemId::BowWarden, .count = 1};
    REQUIRE(add_item(inventory, ItemId::BowDragonfire, 1) == 0);
    REQUIRE(add_item(inventory, ItemId::HealthPotion, 2) == 0);

    // From the bag into her hand: the two change places.
    CHECK(equip_destination(inventory, 0) == weapon_slot);
    CHECK(move_stack(inventory, 0, weapon_slot));
    CHECK(stack_at(inventory, weapon_slot).item == ItemId::BowDragonfire);
    CHECK(stack_at(inventory, 0).item == ItemId::BowWarden);

    // Nothing but a weapon goes there, and the one in hand is not put away.
    CHECK_FALSE(move_stack(inventory, 1, weapon_slot));
    CHECK(equip_destination(inventory, weapon_slot) == -1);
    CHECK_FALSE(move_stack(inventory, weapon_slot, 5));
    CHECK_FALSE(move_stack(inventory, weapon_slot, 1));
    CHECK(stack_at(inventory, weapon_slot).item == ItemId::BowDragonfire);
}

TEST_CASE("the bow in hand raises what an arrow does", "[inventory][weapon]") {
    Inventory inventory;
    CHECK(worn_bonuses(inventory).damage == Approx(0.0F));
    CHECK(weapon_hit(100.0F, worn_bonuses(inventory), 0.5F) == Approx(100.0F));
    // One arrow in twenty strikes true without any bow's help, for half as much again.
    CHECK(weapon_hit(100.0F, worn_bonuses(inventory), 0.04F) == Approx(150.0F));

    inventory.slots.at(static_cast<std::size_t>(weapon_slot)) = {.item = ItemId::BowDragonfire, .count = 1};
    const Bonuses held = worn_bonuses(inventory);
    CHECK(held.damage == Approx(0.42F));
    CHECK(held.health == Approx(25.0F));
    CHECK(weapon_hit(100.0F, held, 0.5F) == Approx(142.0F));
    CHECK(is_critical(held, 0.14F));
    CHECK_FALSE(is_critical(held, 0.16F));
    CHECK(weapon_hit(100.0F, held, 0.1F) == Approx(142.0F * 2.0F));
    // A bow in the bag gives nothing.
    Inventory carried;
    REQUIRE(add_item(carried, ItemId::BowDragonfire, 1) == 0);
    CHECK(worn_bonuses(carried).damage == Approx(0.0F));
}

TEST_CASE("the bows get better with their level, and rarer", "[inventory][weapon]") {
    const std::array bows{ItemId::BowWarden,       ItemId::BowHunter,     ItemId::BowIronbound,
                          ItemId::BowLeafwood,     ItemId::BowMoonglass,  ItemId::BowBriarbloom,
                          ItemId::BowStormfeather, ItemId::BowNightthorn, ItemId::BowDragonfire};
    int level = 0;
    Rarity rarity = Rarity::Common;
    for (const ItemId bow : bows) {
        const ItemInfo& info = item_info(bow);
        CHECK(info.kind == ItemKind::Weapon);
        CHECK(info.max_stack == 1);
        CHECK(info.level > level);
        CHECK(info.rarity >= rarity);
        CHECK_FALSE(info.key.empty());
        level = info.level;
        rarity = info.rarity;
    }
    CHECK(item_info(ItemId::BowDragonfire).rarity == Rarity::Legendary);
}

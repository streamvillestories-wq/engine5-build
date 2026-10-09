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
    CHECK(add_item(inventory, ItemId::ShroomCap, 1) == 0);
    CHECK(add_item(inventory, ItemId::HealthPotion, 8) == 0);

    // Ten to a stack: the first slot is full, the rest starts a new one behind the caps.
    CHECK(stack_at(inventory, 0) == ItemStack{.item = ItemId::HealthPotion, .count = 10});
    CHECK(stack_at(inventory, 1) == ItemStack{.item = ItemId::ShroomCap, .count = 1});
    CHECK(stack_at(inventory, 2) == ItemStack{.item = ItemId::HealthPotion, .count = 2});
    CHECK(count_of(inventory, ItemId::HealthPotion) == 12);
    CHECK(free_bag_slots(inventory) == bag_slots - 3);
}

TEST_CASE("a full bag gives back what does not fit", "[inventory]") {
    Inventory inventory;
    // Charms do not stack: one per slot.
    CHECK(add_item(inventory, ItemId::CharmVigor, bag_slots + 2) == 2);
    CHECK(free_bag_slots(inventory) == 0);
    CHECK(add_item(inventory, ItemId::ShroomCap, 3) == 3);
    // Nothing went into the charm slots.
    CHECK(stack_at(inventory, bag_slots).empty());

    CHECK(add_item(inventory, ItemId::None, 5) == 5);
    CHECK(add_item(inventory, ItemId::ShroomCap, -1) == 0);
}

TEST_CASE("taking from a stack", "[inventory]") {
    Inventory inventory;
    REQUIRE(add_item(inventory, ItemId::ShroomCap, 5) == 0);
    CHECK(take_from(inventory, 0, 2) == ItemStack{.item = ItemId::ShroomCap, .count = 2});
    CHECK(stack_at(inventory, 0).count == 3);
    CHECK(take_from(inventory, 0, 99) == ItemStack{.item = ItemId::ShroomCap, .count = 3});
    CHECK(stack_at(inventory, 0).empty());
    CHECK(take_from(inventory, 0, 1).empty());
    CHECK(take_from(inventory, -1, 1).empty());
    CHECK(take_from(inventory, slot_count, 1).empty());
}

TEST_CASE("moving stacks swaps or tops up", "[inventory]") {
    Inventory inventory;
    REQUIRE(add_item(inventory, ItemId::HealthPotion, 3) == 0);
    REQUIRE(add_item(inventory, ItemId::ShroomCap, 2) == 0);

    SECTION("onto an empty slot") {
        CHECK(move_stack(inventory, 0, 7));
        CHECK(stack_at(inventory, 0).empty());
        CHECK(stack_at(inventory, 7) == ItemStack{.item = ItemId::HealthPotion, .count = 3});
    }
    SECTION("onto another item: they change places") {
        CHECK(move_stack(inventory, 0, 1));
        CHECK(stack_at(inventory, 0).item == ItemId::ShroomCap);
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

TEST_CASE("only charms are worn", "[inventory]") {
    Inventory inventory;
    REQUIRE(add_item(inventory, ItemId::HealthPotion, 1) == 0);
    REQUIRE(add_item(inventory, ItemId::CharmVigor, 1) == 0);
    REQUIRE(add_item(inventory, ItemId::CharmSwiftness, 1) == 0);
    const int first_charm = bag_slots;

    CHECK_FALSE(move_stack(inventory, 0, first_charm));
    CHECK(charm_destination(inventory, 0) == -1);

    CHECK(charm_destination(inventory, 1) == first_charm);
    CHECK(move_stack(inventory, 1, first_charm));
    CHECK(worn_bonuses(inventory).health == 30.0F);
    CHECK(worn_bonuses(inventory).speed == 0.0F);

    // A potion cannot take a worn charm's place by swapping either.
    CHECK_FALSE(move_stack(inventory, first_charm, 0));
    // Another charm can.
    CHECK(move_stack(inventory, 2, first_charm));
    CHECK(stack_at(inventory, 2).item == ItemId::CharmVigor);
    CHECK(worn_bonuses(inventory).health == 0.0F);
    CHECK(worn_bonuses(inventory).speed > 0.1F);

    // Taking it off: to the first free bag slot.
    CHECK(charm_destination(inventory, first_charm) == 1);
    // Putting the next one on: the next free charm slot.
    CHECK(charm_destination(inventory, 2) == first_charm + 1);
}

TEST_CASE("a worn charm stays on when the bag is full", "[inventory]") {
    Inventory inventory;
    inventory.slots.at(bag_slots) = {.item = ItemId::CharmRenewal, .count = 1};
    REQUIRE(add_item(inventory, ItemId::CharmVigor, bag_slots) == 0);
    CHECK(charm_destination(inventory, bag_slots) == -1);
    CHECK(worn_bonuses(inventory).regen == 3.0F);
}

TEST_CASE("sorting joins stacks and orders the bag", "[inventory]") {
    Inventory inventory;
    inventory.slots.at(3) = {.item = ItemId::ShroomCap, .count = 4};
    inventory.slots.at(8) = {.item = ItemId::CharmVigor, .count = 1};
    inventory.slots.at(11) = {.item = ItemId::HealthPotion, .count = 2};
    inventory.slots.at(15) = {.item = ItemId::ShroomCap, .count = 5};
    inventory.slots.at(20) = {.item = ItemId::CrystalShard, .count = 1};
    inventory.slots.at(bag_slots + 1) = {.item = ItemId::CharmSwiftness, .count = 1};

    sort_bag(inventory);

    // Consumables, then materials (the rare one first), then charms.
    CHECK(stack_at(inventory, 0) == ItemStack{.item = ItemId::HealthPotion, .count = 2});
    CHECK(stack_at(inventory, 1) == ItemStack{.item = ItemId::CrystalShard, .count = 1});
    CHECK(stack_at(inventory, 2) == ItemStack{.item = ItemId::ShroomCap, .count = 9});
    CHECK(stack_at(inventory, 3) == ItemStack{.item = ItemId::CharmVigor, .count = 1});
    CHECK(free_bag_slots(inventory) == bag_slots - 4);
    CHECK(stack_at(inventory, bag_slots + 1).item == ItemId::CharmSwiftness);
}

TEST_CASE("finding items", "[inventory]") {
    Inventory inventory;
    inventory.slots.at(6) = {.item = ItemId::HealthPotion, .count = 2};
    CHECK(find_item(inventory, ItemId::HealthPotion) == 6);
    CHECK(find_item(inventory, ItemId::GlowSpore) == -1);
}

TEST_CASE("loot is repeatable, always has gold, and everything turns up", "[inventory]") {
    CHECK(roll_loot(1234).gold == roll_loot(1234).gold);
    CHECK(roll_loot(1234).items == roll_loot(1234).items);

    int charms = 0;
    int potions = 0;
    int caps = 0;
    const int kills = 4000;
    for (std::uint32_t seed = 0; seed < static_cast<std::uint32_t>(kills); ++seed) {
        const Loot loot = roll_loot(seed);
        REQUIRE(loot.gold >= 3);
        REQUIRE(loot.gold <= 8);
        for (const ItemStack& stack : loot.items) {
            if (stack.empty()) {
                continue;
            }
            REQUIRE(stack.count <= item_info(stack.item).max_stack);
            const ItemInfo& info = item_info(stack.item);
            charms += info.kind == ItemKind::Charm ? 1 : 0;
            potions += info.kind == ItemKind::Consumable ? 1 : 0;
            caps += stack.item == ItemId::ShroomCap ? 1 : 0;
        }
    }
    // Chances are 5 %, 30 % and 70 %; the bounds leave room for the generator's noise.
    CHECK(charms > kills * 2 / 100);
    CHECK(charms < kills * 9 / 100);
    CHECK(potions > kills * 22 / 100);
    CHECK(potions < kills * 38 / 100);
    CHECK(caps > kills * 62 / 100);
    CHECK(caps < kills * 78 / 100);
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
    // A weapon is not a charm.
    CHECK_FALSE(move_stack(inventory, 0, bag_slots));
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

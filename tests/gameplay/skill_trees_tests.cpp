// The heroes' skill trees (2026-10-21): the tables hold together, and the growth's rules.
#include "e5/gameplay/skill_trees.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <set>

using e5::gameplay::Growth;
using e5::gameplay::NodeEffect;
using e5::gameplay::SkillId;
using e5::gameplay::SkillSet;
using e5::gameplay::TreeNode;

TEST_CASE("every hero with a tree has three branches of ten nodes in four rows") {
    for (const SkillSet set : {SkillSet::Shadow, SkillSet::Bone, SkillSet::Blade, SkillSet::Dwarf, SkillSet::Mage,
                               SkillSet::Archer}) {
        CHECK(e5::gameplay::set_has_tree(set));
        for (int branch = 0; branch < 3; ++branch) {
            REQUIRE(e5::gameplay::tree_branch(set, branch) != nullptr);
            int count = 0;
            int capstones = 0;
            int twists = 0;
            for (const TreeNode& node : e5::gameplay::all_tree_nodes()) {
                if (node.set != set || node.branch != branch) {
                    continue;
                }
                ++count;
                capstones += node.row == 3 ? 1 : 0;
                twists += node.effect == NodeEffect::Twist ? 1 : 0;
                CHECK(node.row >= 0);
                CHECK(node.row <= 3);
                if (node.row == 0) {
                    CHECK(node.needs == 0);
                } else {
                    const TreeNode* const before = e5::gameplay::tree_node(node.needs);
                    REQUIRE(before != nullptr);
                    CHECK(before->set == set);
                    CHECK(before->branch == branch);
                    CHECK(before->row == node.row - 1);
                }
                if (node.effect == NodeEffect::Twist) {
                    CHECK(node.twist != e5::gameplay::TwistId::None);
                }
            }
            CHECK(count == 10);
            CHECK(capstones == 1);
            CHECK(twists == 1);
        }
    }
    CHECK_FALSE(e5::gameplay::set_has_tree(SkillSet::Wizard));
}

TEST_CASE("node ids are unique and run from one") {
    std::set<int> ids;
    int expected = 1;
    for (const TreeNode& node : e5::gameplay::all_tree_nodes()) {
        CHECK(node.id == expected++);
        CHECK(ids.insert(node.id).second);
        CHECK(e5::gameplay::tree_node(node.id) == &node);
    }
    CHECK(e5::gameplay::tree_node(0) == nullptr);
    CHECK(e5::gameplay::tree_node(expected) == nullptr);
    CHECK(static_cast<int>(ids.size()) < e5::gameplay::tree_node_limit);
}

TEST_CASE("levels come from experience, a point each, and stop at the top") {
    Growth growth;
    CHECK(e5::gameplay::xp_for_level(1) == 0);
    CHECK(e5::gameplay::xp_for_level(2) == 60);
    CHECK(e5::gameplay::xp_for_level(3) > e5::gameplay::xp_for_level(2));
    CHECK(e5::gameplay::xp_for_kill(40.0F) == 18);
    CHECK(e5::gameplay::xp_for_kill(1.0F) == 10);
    CHECK(e5::gameplay::xp_for_kill(10000.0F) == 120);
    CHECK(e5::gameplay::gain_xp(growth, 59) == 0);
    CHECK(growth.level == 1);
    CHECK(e5::gameplay::gain_xp(growth, 1) == 1);
    CHECK(growth.level == 2);
    CHECK(growth.points == 1);
    CHECK(growth.xp == 0);
    CHECK(e5::gameplay::gain_xp(growth, 100000) > 0);
    CHECK(growth.level == e5::gameplay::max_hero_level);
    CHECK(growth.points == e5::gameplay::max_hero_level - 1);
    CHECK(e5::gameplay::gain_xp(growth, 1000) == 0);
}

TEST_CASE("a node needs a point and the node above it, and does what it says") {
    Growth growth;
    const TreeNode& first = e5::gameplay::all_tree_nodes().front(); // Shadow, row 0
    CHECK_FALSE(e5::gameplay::can_choose(growth, SkillSet::Shadow, first.id));
    growth.points = 2;
    CHECK_FALSE(e5::gameplay::can_choose(growth, SkillSet::Bone, first.id)); // another hero's node
    const TreeNode* second = nullptr;
    for (const TreeNode& node : e5::gameplay::all_tree_nodes()) {
        if (node.needs == first.id) {
            second = &node;
            break;
        }
    }
    REQUIRE(second != nullptr);
    CHECK_FALSE(e5::gameplay::can_choose(growth, SkillSet::Shadow, second->id));
    CHECK(e5::gameplay::choose(growth, SkillSet::Shadow, first.id));
    CHECK(growth.points == 1);
    CHECK_FALSE(e5::gameplay::choose(growth, SkillSet::Shadow, first.id)); // not twice
    CHECK(e5::gameplay::choose(growth, SkillSet::Shadow, second->id));
    CHECK(growth.points == 0);
    const e5::gameplay::SkillMods mods = e5::gameplay::mods_of(growth, SkillSet::Shadow);
    // Keen Edge (Dagger Combo +12 %) and, under it, Deadly Assassinate (Assassinate +12 %).
    CHECK(mods.damage.at(static_cast<std::size_t>(SkillId::DaggerCombo)) == Catch::Approx(1.12F));
    CHECK(mods.damage.at(static_cast<std::size_t>(SkillId::Assassinate)) == Catch::Approx(1.12F));
    CHECK(mods.damage.at(static_cast<std::size_t>(SkillId::ShadowStep)) == Catch::Approx(1.0F));
    CHECK(mods.all_damage == Catch::Approx(1.0F));
    CHECK_FALSE(mods.has(e5::gameplay::TwistId::SpinalTap));
    // Another hero's growth reads nothing from this one's tree.
    CHECK(e5::gameplay::mods_of(growth, SkillSet::Bone).damage.at(static_cast<std::size_t>(SkillId::DaggerCombo)) ==
          Catch::Approx(1.0F));
}

TEST_CASE("a whole tree chosen gives every kind of effect") {
    Growth growth;
    growth.points = 100;
    for (const TreeNode& node : e5::gameplay::all_tree_nodes()) {
        if (node.set == SkillSet::Shadow) {
            CHECK(e5::gameplay::choose(growth, SkillSet::Shadow, node.id));
        }
    }
    const e5::gameplay::SkillMods mods = e5::gameplay::mods_of(growth, SkillSet::Shadow);
    CHECK(mods.count.at(static_cast<std::size_t>(SkillId::FanOfKnives)) == 2);
    CHECK(mods.cooldown.at(static_cast<std::size_t>(SkillId::ShadowStep)) < 1.0F);
    CHECK(mods.duration.at(static_cast<std::size_t>(SkillId::VeilOfShadows)) > 1.0F);
    CHECK(mods.move > 1.0F);
    CHECK(mods.max_health > 0.0F);
    CHECK(mods.damage_taken < 1.0F);
    CHECK(mods.lifesteal > 0.0F);
    CHECK(mods.reach.at(static_cast<std::size_t>(SkillId::Caltrops)) > 1.0F); // the five more are in the tree
    CHECK(mods.has(e5::gameplay::TwistId::SpinalTap));
    CHECK(mods.has(e5::gameplay::TwistId::VenomFan));
    CHECK(mods.has(e5::gameplay::TwistId::DeeperShadows));
}

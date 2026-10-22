// The six heroes' skill trees (2026-10-21, the keeper: "give each hero a set of new skills, each one
// upgradable, think about a skill tree as a base"). Written by the generator in the session's notes;
// edit here: this file is the truth, the window reads it through E5PlayerController::get_skill_tree.
// Each hero: three branches of ten nodes in four rows (3, 3, 3, 1): a node of row 2 needs the one
// above it, row 3 the one above that, the capstone the middle of row 3.
#include "e5/gameplay/skill_trees.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace e5::gameplay {

namespace {

using enum NodeEffect;

// NOLINTBEGIN(readability-magic-numbers)
constexpr std::array<TreeNode, 180> nodes{{
    TreeNode{1, SkillSet::Shadow, 0, 0, "Keen Edge", "Dagger Combo does 12 % more damage.", 0, Damage, SkillId::DaggerCombo, 0.12F},
    TreeNode{2, SkillSet::Shadow, 0, 0, "Quick Step", "Shadow Step is ready 12 % sooner.", 0, Cooldown, SkillId::ShadowStep, 0.12F},
    TreeNode{3, SkillSet::Shadow, 0, 0, "Wide Caltrops", "Caltrops reaches a quarter further.", 0, Reach, SkillId::Caltrops, 0.25F},
    TreeNode{4, SkillSet::Shadow, 0, 1, "Deadly Assassinate", "Assassinate does 12 % more damage.", 1, Damage, SkillId::Assassinate, 0.12F},
    TreeNode{5, SkillSet::Shadow, 0, 1, "Deep Stab", "Shadow Step does 12 % more damage.", 2, Damage, SkillId::ShadowStep, 0.12F},
    TreeNode{6, SkillSet::Shadow, 0, 1, "Marked for Death", "Death Mark lasts 30 % longer.", 3, Duration, SkillId::DeathMark, 0.30F},
    TreeNode{7, SkillSet::Shadow, 0, 2, "Wider Dance", "Blade Dance reaches a quarter further.", 4, Reach, SkillId::BladeDance, 0.25F},
    TreeNode{8, SkillSet::Shadow, 0, 2, "Spinal Tap", "Shadow Step’s stab in the back leaves its victim stunned for a moment and a half.", 5, Twist, SkillId::ShadowStep, 0.0F, TwistId::SpinalTap},
    TreeNode{9, SkillSet::Shadow, 0, 2, "Cruel Mark", "Death Mark is ready 12 % sooner.", 6, Cooldown, SkillId::DeathMark, 0.12F},
    TreeNode{10, SkillSet::Shadow, 0, 3, "Master Assassin", "Everything does 12 % more damage.", 8, AllDamage, SkillId::None, 0.12F},
    TreeNode{11, SkillSet::Shadow, 1, 0, "Bitter Blades", "Venom Edge does 12 % more damage.", 0, Damage, SkillId::VenomEdge, 0.12F},
    TreeNode{12, SkillSet::Shadow, 1, 0, "More Knives", "Fan of Knives has two more.", 0, Count, SkillId::FanOfKnives, 2.0F},
    TreeNode{13, SkillSet::Shadow, 1, 0, "Sharp Caltrops", "Caltrops does 12 % more damage.", 0, Damage, SkillId::Caltrops, 0.12F},
    TreeNode{14, SkillSet::Shadow, 1, 1, "Tight Garrote", "Garrote does 12 % more damage.", 11, Damage, SkillId::Garrote, 0.12F},
    TreeNode{15, SkillSet::Shadow, 1, 1, "Sharp Knives", "Fan of Knives does 12 % more damage.", 12, Damage, SkillId::FanOfKnives, 0.12F},
    TreeNode{16, SkillSet::Shadow, 1, 1, "Thick Smoke", "Smoke Bomb reaches a quarter further.", 13, Reach, SkillId::SmokeBomb, 0.25F},
    TreeNode{17, SkillSet::Shadow, 1, 2, "Swift Venom", "Venom Edge is ready 12 % sooner.", 14, Cooldown, SkillId::VenomEdge, 0.12F},
    TreeNode{18, SkillSet::Shadow, 1, 2, "Venom Fan", "Every knife of the fan is poisoned, Venom Edge or not.", 15, Twist, SkillId::FanOfKnives, 0.0F, TwistId::VenomFan},
    TreeNode{19, SkillSet::Shadow, 1, 2, "Lingering Smoke", "Smoke Bomb lasts 30 % longer.", 16, Duration, SkillId::SmokeBomb, 0.30F},
    TreeNode{20, SkillSet::Shadow, 1, 3, "Plague Bearer", "8 % of the damage done heals.", 18, Lifesteal, SkillId::None, 0.08F},
    TreeNode{21, SkillSet::Shadow, 2, 0, "Longer Veil", "Veil of Shadows lasts 30 % longer.", 0, Duration, SkillId::VeilOfShadows, 0.30F},
    TreeNode{22, SkillSet::Shadow, 2, 0, "Flurry Edge", "Shadow Flurry does 12 % more damage.", 0, Damage, SkillId::ShadowFlurry, 0.12F},
    TreeNode{23, SkillSet::Shadow, 2, 0, "Quick Dash", "Shadow Dash is ready 12 % sooner.", 0, Cooldown, SkillId::ShadowDash, 0.12F},
    TreeNode{24, SkillSet::Shadow, 2, 1, "Quick Veil", "Veil of Shadows is ready 12 % sooner.", 21, Cooldown, SkillId::VeilOfShadows, 0.12F},
    TreeNode{25, SkillSet::Shadow, 2, 1, "Lasting Cripple", "Crippling Strike lasts 30 % longer.", 22, Duration, SkillId::CripplingStrike, 0.30F},
    TreeNode{26, SkillSet::Shadow, 2, 1, "Shadow Pace", "6 % faster on foot.", 23, MoveSpeed, SkillId::None, 0.06F},
    TreeNode{27, SkillSet::Shadow, 2, 2, "Smoke Dance", "Smoke Bomb is ready 12 % sooner.", 24, Cooldown, SkillId::SmokeBomb, 0.12F},
    TreeNode{28, SkillSet::Shadow, 2, 2, "Deeper Shadows", "The first blow out of the veil does three times its damage, not twice.", 25, Twist, SkillId::VeilOfShadows, 0.0F, TwistId::DeeperShadows},
    TreeNode{29, SkillSet::Shadow, 2, 2, "Vital Shadow", "15 more health.", 26, MaxHealth, SkillId::None, 15.0F},
    TreeNode{30, SkillSet::Shadow, 2, 3, "One with the Dark", "Blows do 16 % less harm.", 28, DamageTaken, SkillId::None, 0.16F},
    TreeNode{31, SkillSet::Bone, 0, 0, "Hungry Souls", "Soul Bolt does 12 % more damage.", 0, Damage, SkillId::SoulBolt, 0.12F},
    TreeNode{32, SkillSet::Bone, 0, 0, "Deep Drain", "Soul Drain does 12 % more damage.", 0, Damage, SkillId::SoulDrain, 0.12F},
    TreeNode{33, SkillSet::Bone, 0, 0, "Heavy Coil", "Death Coil does 12 % more damage.", 0, Damage, SkillId::DeathCoil, 0.12F},
    TreeNode{34, SkillSet::Bone, 0, 1, "Quick Souls", "Soul Bolt is ready 12 % sooner.", 31, Cooldown, SkillId::SoulBolt, 0.12F},
    TreeNode{35, SkillSet::Bone, 0, 1, "Quick Coil", "Death Coil is ready 12 % sooner.", 32, Cooldown, SkillId::DeathCoil, 0.12F},
    TreeNode{36, SkillSet::Bone, 0, 1, "Grave Lore", "15 % more experience from kills.", 33, Experience, SkillId::None, 0.15F},
    TreeNode{37, SkillSet::Bone, 0, 2, "Wailing Souls", "Soul Bolt does 12 % more damage.", 34, Damage, SkillId::SoulBolt, 0.12F},
    TreeNode{38, SkillSet::Bone, 0, 2, "Soul Harvest", "Soul Drain heals him for all of its damage, not half.", 35, Twist, SkillId::SoulDrain, 0.0F, TwistId::SoulHarvest},
    TreeNode{39, SkillSet::Bone, 0, 2, "Wide Requiem", "Requiem reaches a quarter further.", 36, Reach, SkillId::Requiem, 0.25F},
    TreeNode{40, SkillSet::Bone, 0, 3, "Lord of Souls", "Everything does 12 % more damage.", 38, AllDamage, SkillId::None, 0.12F},
    TreeNode{41, SkillSet::Bone, 1, 0, "Long Spikes", "Bone Spikes reaches a quarter further.", 0, Reach, SkillId::BoneSpikes, 0.25F},
    TreeNode{42, SkillSet::Bone, 1, 0, "Far Sentinel", "Bone Sentinel reaches a quarter further.", 0, Reach, SkillId::BoneSentinel, 0.25F},
    TreeNode{43, SkillSet::Bone, 1, 0, "Firm Grasp", "Grave Grasp lasts 30 % longer.", 0, Duration, SkillId::GraveGrasp, 0.30F},
    TreeNode{44, SkillSet::Bone, 1, 1, "Sharp Spikes", "Bone Spikes does 12 % more damage.", 41, Damage, SkillId::BoneSpikes, 0.12F},
    TreeNode{45, SkillSet::Bone, 1, 1, "Lasting Sentinel", "Bone Sentinel lasts 30 % longer.", 42, Duration, SkillId::BoneSentinel, 0.30F},
    TreeNode{46, SkillSet::Bone, 1, 1, "Wide Grasp", "Grave Grasp reaches a quarter further.", 43, Reach, SkillId::GraveGrasp, 0.25F},
    TreeNode{47, SkillSet::Bone, 1, 2, "Quick Spikes", "Bone Spikes is ready 12 % sooner.", 44, Cooldown, SkillId::BoneSpikes, 0.12F},
    TreeNode{48, SkillSet::Bone, 1, 2, "Grasping Hands", "The hands drag their victims into the middle of the grasp.", 45, Twist, SkillId::GraveGrasp, 0.0F, TwistId::GraspingHands},
    TreeNode{49, SkillSet::Bone, 1, 2, "Crushing Grasp", "Grave Grasp does 12 % more damage.", 46, Damage, SkillId::GraveGrasp, 0.12F},
    TreeNode{50, SkillSet::Bone, 1, 3, "Unbreakable", "Blows do 16 % less harm.", 48, DamageTaken, SkillId::None, 0.16F},
    TreeNode{51, SkillSet::Bone, 2, 0, "Wide Curse", "Curse of Frailty reaches a quarter further.", 0, Reach, SkillId::CurseOfFrailty, 0.25F},
    TreeNode{52, SkillSet::Bone, 2, 0, "Louder Wail", "Banshee Wail does 12 % more damage.", 0, Damage, SkillId::BansheeWail, 0.12F},
    TreeNode{53, SkillSet::Bone, 2, 0, "Wide Chill", "Grave Chill reaches a quarter further.", 0, Reach, SkillId::GraveChill, 0.25F},
    TreeNode{54, SkillSet::Bone, 2, 1, "Long Curse", "Curse of Frailty lasts 30 % longer.", 51, Duration, SkillId::CurseOfFrailty, 0.30F},
    TreeNode{55, SkillSet::Bone, 2, 1, "Far Wail", "Banshee Wail reaches a quarter further.", 52, Reach, SkillId::BansheeWail, 0.25F},
    TreeNode{56, SkillSet::Bone, 2, 1, "Spreading Plague", "Plague reaches a quarter further.", 53, Reach, SkillId::Plague, 0.25F},
    TreeNode{57, SkillSet::Bone, 2, 2, "Quick Curse", "Curse of Frailty is ready 12 % sooner.", 54, Cooldown, SkillId::CurseOfFrailty, 0.12F},
    TreeNode{58, SkillSet::Bone, 2, 2, "Terrifying Wail", "The wail throws its enemies back off their feet.", 55, Twist, SkillId::BansheeWail, 0.0F, TwistId::TerrifyingWail},
    TreeNode{59, SkillSet::Bone, 2, 2, "Fleeting Form", "Spectral Form is ready 12 % sooner.", 56, Cooldown, SkillId::SpectralForm, 0.12F},
    TreeNode{60, SkillSet::Bone, 2, 3, "Dread Wraith", "12 % faster on foot.", 58, MoveSpeed, SkillId::None, 0.12F},
    TreeNode{61, SkillSet::Blade, 0, 0, "Sharp Slash", "Sword Combo does 12 % more damage.", 0, Damage, SkillId::Slash, 0.12F},
    TreeNode{62, SkillSet::Blade, 0, 0, "Quick Riposte", "Counter Attack is ready 12 % sooner.", 0, Cooldown, SkillId::CounterAttack, 0.12F},
    TreeNode{63, SkillSet::Blade, 0, 0, "Hard Bash", "Shield Bash does 12 % more damage.", 0, Damage, SkillId::ShieldBash, 0.12F},
    TreeNode{64, SkillSet::Blade, 0, 1, "Quick Bash", "Shield Bash is ready 12 % sooner.", 61, Cooldown, SkillId::ShieldBash, 0.12F},
    TreeNode{65, SkillSet::Blade, 0, 1, "Long Riposte", "Counter Attack lasts 30 % longer.", 62, Duration, SkillId::CounterAttack, 0.30F},
    TreeNode{66, SkillSet::Blade, 0, 1, "Sure Execute", "Execute does 12 % more damage.", 63, Damage, SkillId::Execute, 0.12F},
    TreeNode{67, SkillSet::Blade, 0, 2, "Heavy Throw", "Sword Throw does 12 % more damage.", 64, Damage, SkillId::SwordThrow, 0.12F},
    TreeNode{68, SkillSet::Blade, 0, 2, "Mortal Strike", "What Execute does not kill stands stunned.", 65, Twist, SkillId::Execute, 0.0F, TwistId::MortalStrike},
    TreeNode{69, SkillSet::Blade, 0, 2, "Quick Execute", "Execute is ready 12 % sooner.", 66, Cooldown, SkillId::Execute, 0.12F},
    TreeNode{70, SkillSet::Blade, 0, 3, "Blademaster", "Everything does 12 % more damage.", 68, AllDamage, SkillId::None, 0.12F},
    TreeNode{71, SkillSet::Blade, 1, 0, "Longer Rage", "Enrage lasts 30 % longer.", 0, Duration, SkillId::Enrage, 0.30F},
    TreeNode{72, SkillSet::Blade, 1, 0, "Heavy Leap", "Jump Attack does 12 % more damage.", 0, Damage, SkillId::JumpAttack, 0.12F},
    TreeNode{73, SkillSet::Blade, 1, 0, "Wide Cleave", "Cleave reaches a quarter further.", 0, Reach, SkillId::Cleave, 0.25F},
    TreeNode{74, SkillSet::Blade, 1, 1, "Quick Rage", "Enrage is ready 12 % sooner.", 71, Cooldown, SkillId::Enrage, 0.12F},
    TreeNode{75, SkillSet::Blade, 1, 1, "Long Shout", "Battle Shout lasts 30 % longer.", 72, Duration, SkillId::BattleShout, 0.30F},
    TreeNode{76, SkillSet::Blade, 1, 1, "Long Stampede", "Sprintsz lasts 30 % longer.", 73, Duration, SkillId::Stampede, 0.30F},
    TreeNode{77, SkillSet::Blade, 1, 2, "Blood Rage", "4 % of the damage done heals.", 74, Lifesteal, SkillId::None, 0.04F},
    TreeNode{78, SkillSet::Blade, 1, 2, "Bloodbath", "While she rages, a tenth of the damage she does heals her.", 75, Twist, SkillId::Enrage, 0.0F, TwistId::Bloodbath},
    TreeNode{79, SkillSet::Blade, 1, 2, "Whirling Blade", "Whirlwind does 12 % more damage.", 76, Damage, SkillId::BladeWhirl, 0.12F},
    TreeNode{80, SkillSet::Blade, 1, 3, "Unstoppable", "Blows do 16 % less harm.", 78, DamageTaken, SkillId::None, 0.16F},
    TreeNode{81, SkillSet::Blade, 2, 0, "Tall Banner", "War Banner reaches a quarter further.", 0, Reach, SkillId::WarBanner, 0.25F},
    TreeNode{82, SkillSet::Blade, 2, 0, "Iron Will", "Never Give Up lasts 30 % longer.", 0, Duration, SkillId::NeverGiveUp, 0.30F},
    TreeNode{83, SkillSet::Blade, 2, 0, "Deep Hamstring", "Hamstring does 12 % more damage.", 0, Damage, SkillId::Hamstring, 0.12F},
    TreeNode{84, SkillSet::Blade, 2, 1, "Long Banner", "War Banner lasts 30 % longer.", 81, Duration, SkillId::WarBanner, 0.30F},
    TreeNode{85, SkillSet::Blade, 2, 1, "Quick Heroic", "Heroic Strike is ready 12 % sooner.", 82, Cooldown, SkillId::HeroicStrike, 0.12F},
    TreeNode{86, SkillSet::Blade, 2, 1, "Field Dressing", "Wounds close 0.4 health a second faster.", 83, Regen, SkillId::None, 0.40F},
    TreeNode{87, SkillSet::Blade, 2, 2, "Sharp Rain", "Rain of Blades does 12 % more damage.", 84, Damage, SkillId::RainOfBlades, 0.12F},
    TreeNode{88, SkillSet::Blade, 2, 2, "Stand Fast", "Never Give Up also closes a fifth of her wounds at once.", 85, Twist, SkillId::NeverGiveUp, 0.0F, TwistId::StandFast},
    TreeNode{89, SkillSet::Blade, 2, 2, "Seismic Force", "Seismic Slash does 12 % more damage.", 86, Damage, SkillId::SeismicSlash, 0.12F},
    TreeNode{90, SkillSet::Blade, 2, 3, "Warlord", "30 more health.", 88, MaxHealth, SkillId::None, 30.0F},
    TreeNode{91, SkillSet::Dwarf, 0, 0, "Heavy Axe", "Axe Combo does 12 % more damage.", 0, Damage, SkillId::AxeCombo, 0.12F},
    TreeNode{92, SkillSet::Dwarf, 0, 0, "Sharp Whirl", "Whirlwind does 12 % more damage.", 0, Damage, SkillId::Whirlwind, 0.12F},
    TreeNode{93, SkillSet::Dwarf, 0, 0, "Heavy Hammer", "Forge Hammer does 12 % more damage.", 0, Damage, SkillId::ForgeHammer, 0.12F},
    TreeNode{94, SkillSet::Dwarf, 0, 1, "Heavier Axe", "Axe Combo does 12 % more damage.", 91, Damage, SkillId::AxeCombo, 0.12F},
    TreeNode{95, SkillSet::Dwarf, 0, 1, "Quick Hammer", "Forge Hammer is ready 12 % sooner.", 92, Cooldown, SkillId::ForgeHammer, 0.12F},
    TreeNode{96, SkillSet::Dwarf, 0, 1, "Far Leap", "Leap Strike reaches a quarter further.", 93, Reach, SkillId::LeapStrike, 0.25F},
    TreeNode{97, SkillSet::Dwarf, 0, 2, "Heavy Throw", "Throwing Axe does 12 % more damage.", 94, Damage, SkillId::ThrowingAxe, 0.12F},
    TreeNode{98, SkillSet::Dwarf, 0, 2, "Thunder Leap", "Everything the Leap Strike catches stands stunned.", 95, Twist, SkillId::LeapStrike, 0.0F, TwistId::ThunderLeap},
    TreeNode{99, SkillSet::Dwarf, 0, 2, "Hard Leap", "Leap Strike does 12 % more damage.", 96, Damage, SkillId::LeapStrike, 0.12F},
    TreeNode{100, SkillSet::Dwarf, 0, 3, "Ironbreaker", "Everything does 12 % more damage.", 98, AllDamage, SkillId::None, 0.12F},
    TreeNode{101, SkillSet::Dwarf, 1, 0, "Lasting Runes", "Runic Bulwark lasts 30 % longer.", 0, Duration, SkillId::RunicBulwark, 0.30F},
    TreeNode{102, SkillSet::Dwarf, 1, 0, "Heavy Anvil", "Ancestral Anvil does 12 % more damage.", 0, Damage, SkillId::AncestralAnvil, 0.12F},
    TreeNode{103, SkillSet::Dwarf, 1, 0, "Long Horn", "Rally Horn lasts 30 % longer.", 0, Duration, SkillId::RallyHorn, 0.30F},
    TreeNode{104, SkillSet::Dwarf, 1, 1, "Quick Runes", "Runic Bulwark is ready 12 % sooner.", 101, Cooldown, SkillId::RunicBulwark, 0.12F},
    TreeNode{105, SkillSet::Dwarf, 1, 1, "Wide Grip", "Earthen Grip reaches a quarter further.", 102, Reach, SkillId::EarthenGrip, 0.25F},
    TreeNode{106, SkillSet::Dwarf, 1, 1, "Long Brew", "Stone Brew lasts 30 % longer.", 103, Duration, SkillId::StoneBrew, 0.30F},
    TreeNode{107, SkillSet::Dwarf, 1, 2, "Fierce Cry", "Battle Cry does 12 % more damage.", 104, Damage, SkillId::Battlecry, 0.12F},
    TreeNode{108, SkillSet::Dwarf, 1, 2, "Living Runes", "The Runic Bulwark closes his wounds while it stands, a fiftieth a second.", 105, Twist, SkillId::RunicBulwark, 0.0F, TwistId::LivingRunes},
    TreeNode{109, SkillSet::Dwarf, 1, 2, "Stone Skin", "Blows do 8 % less harm.", 106, DamageTaken, SkillId::None, 0.08F},
    TreeNode{110, SkillSet::Dwarf, 1, 3, "Runelord", "Blows do 16 % less harm.", 108, DamageTaken, SkillId::None, 0.16F},
    TreeNode{111, SkillSet::Dwarf, 2, 0, "Loaded Cannon", "Dwarven Cannon does 12 % more damage.", 0, Damage, SkillId::DwarvenCannon, 0.12F},
    TreeNode{112, SkillSet::Dwarf, 2, 0, "Big Keg", "Powder Keg reaches a quarter further.", 0, Reach, SkillId::PowderKeg, 0.25F},
    TreeNode{113, SkillSet::Dwarf, 2, 0, "Quick Trap", "Rune Trap is ready 12 % sooner.", 0, Cooldown, SkillId::RuneTrap, 0.12F},
    TreeNode{114, SkillSet::Dwarf, 2, 1, "Long Cannon", "Dwarven Cannon lasts 30 % longer.", 111, Duration, SkillId::DwarvenCannon, 0.30F},
    TreeNode{115, SkillSet::Dwarf, 2, 1, "Heavy Avalanche", "Avalanche does 12 % more damage.", 112, Damage, SkillId::Avalanche, 0.12F},
    TreeNode{116, SkillSet::Dwarf, 2, 1, "Big Boulder", "Stonebreaker does 12 % more damage.", 113, Damage, SkillId::Stonebreaker, 0.12F},
    TreeNode{117, SkillSet::Dwarf, 2, 2, "Quick Cannon", "Dwarven Cannon is ready 12 % sooner.", 114, Cooldown, SkillId::DwarvenCannon, 0.12F},
    TreeNode{118, SkillSet::Dwarf, 2, 2, "Shrapnel Keg", "The keg’s blast also slows what it catches by half for three seconds.", 115, Twist, SkillId::PowderKeg, 0.0F, TwistId::ShrapnelKeg},
    TreeNode{119, SkillSet::Dwarf, 2, 2, "Deep Tremor", "Tremor does 12 % more damage.", 116, Damage, SkillId::Tremor, 0.12F},
    TreeNode{120, SkillSet::Dwarf, 2, 3, "Master Engineer", "Every skill is ready 24 % sooner.", 118, Cooldown, SkillId::None, 0.24F},
    TreeNode{121, SkillSet::Mage, 0, 0, "Hot Firebolt", "Firebolt does 12 % more damage.", 0, Damage, SkillId::Firebolt, 0.12F},
    TreeNode{122, SkillSet::Mage, 0, 0, "Long Chain", "Chain Lightning reaches a quarter further.", 0, Reach, SkillId::ChainBolt, 0.25F},
    TreeNode{123, SkillSet::Mage, 0, 0, "Long Wall", "Flame Wall reaches a quarter further.", 0, Reach, SkillId::FlameWall, 0.25F},
    TreeNode{124, SkillSet::Mage, 0, 1, "Quick Firebolt", "Firebolt is ready 12 % sooner.", 121, Cooldown, SkillId::Firebolt, 0.12F},
    TreeNode{125, SkillSet::Mage, 0, 1, "Strong Chain", "Chain Lightning does 12 % more damage.", 122, Damage, SkillId::ChainBolt, 0.12F},
    TreeNode{126, SkillSet::Mage, 0, 1, "Lasting Surge", "Arcane Surge lasts 30 % longer.", 123, Duration, SkillId::ArcaneSurge, 0.30F},
    TreeNode{127, SkillSet::Mage, 0, 2, "More Missiles", "Arcane Missiles has two more.", 124, Count, SkillId::ArcaneMissiles, 2.0F},
    TreeNode{128, SkillSet::Mage, 0, 2, "Overload", "The Firebolt’s burst also stuns for a second.", 125, Twist, SkillId::Firebolt, 0.0F, TwistId::Overload},
    TreeNode{129, SkillSet::Mage, 0, 2, "Keen Bolt", "Arcane Bolt does 12 % more damage.", 126, Damage, SkillId::ArcaneBolt, 0.12F},
    TreeNode{130, SkillSet::Mage, 0, 3, "Stormlord", "Everything does 12 % more damage.", 128, AllDamage, SkillId::None, 0.12F},
    TreeNode{131, SkillSet::Mage, 1, 0, "Thick Frost", "Frost Armor lasts 30 % longer.", 0, Duration, SkillId::FrostArmor, 0.30F},
    TreeNode{132, SkillSet::Mage, 1, 0, "Long Blizzard", "Blizzard lasts 30 % longer.", 0, Duration, SkillId::Blizzard, 0.30F},
    TreeNode{133, SkillSet::Mage, 1, 0, "Wide Warp", "Time Warp reaches a quarter further.", 0, Reach, SkillId::TimeWarp, 0.25F},
    TreeNode{134, SkillSet::Mage, 1, 1, "Quick Frost", "Frost Armor is ready 12 % sooner.", 131, Cooldown, SkillId::FrostArmor, 0.12F},
    TreeNode{135, SkillSet::Mage, 1, 1, "Wide Blizzard", "Blizzard reaches a quarter further.", 132, Reach, SkillId::Blizzard, 0.25F},
    TreeNode{136, SkillSet::Mage, 1, 1, "Long Warp", "Time Warp lasts 30 % longer.", 133, Duration, SkillId::TimeWarp, 0.30F},
    TreeNode{137, SkillSet::Mage, 1, 2, "Quick Dome", "Dome of Protection is ready 12 % sooner.", 134, Cooldown, SkillId::ProtectionDome, 0.12F},
    TreeNode{138, SkillSet::Mage, 1, 2, "Shatter", "The cold of the Ice Prison slows everything that stands near it.", 135, Twist, SkillId::IcePrison, 0.0F, TwistId::Shatter},
    TreeNode{139, SkillSet::Mage, 1, 2, "Quick Prison", "Ice Prison is ready 12 % sooner.", 136, Cooldown, SkillId::IcePrison, 0.12F},
    TreeNode{140, SkillSet::Mage, 1, 3, "Winter's Heart", "Blows do 16 % less harm.", 138, DamageTaken, SkillId::None, 0.16F},
    TreeNode{141, SkillSet::Mage, 2, 0, "Heavy Orb", "Arcane Orb does 12 % more damage.", 0, Damage, SkillId::ArcaneOrb, 0.12F},
    TreeNode{142, SkillSet::Mage, 2, 0, "Deep Well", "Gravity Well reaches a quarter further.", 0, Reach, SkillId::GravityWell, 0.25F},
    TreeNode{143, SkillSet::Mage, 2, 0, "Wide Static", "Static Trap reaches a quarter further.", 0, Reach, SkillId::StaticTrap, 0.25F},
    TreeNode{144, SkillSet::Mage, 2, 1, "Quick Orb", "Arcane Orb is ready 12 % sooner.", 141, Cooldown, SkillId::ArcaneOrb, 0.12F},
    TreeNode{145, SkillSet::Mage, 2, 1, "Heavy Meteors", "Meteor Shower does 12 % more damage.", 142, Damage, SkillId::MeteorShower, 0.12F},
    TreeNode{146, SkillSet::Mage, 2, 1, "Blink Step", "6 % faster on foot.", 143, MoveSpeed, SkillId::None, 0.06F},
    TreeNode{147, SkillSet::Mage, 2, 2, "Hungry Hole", "Black Whole does 12 % more damage.", 144, Damage, SkillId::RovingHole, 0.12F},
    TreeNode{148, SkillSet::Mage, 2, 2, "Event Horizon", "The Gravity Well’s implosion stuns twice as long.", 145, Twist, SkillId::GravityWell, 0.0F, TwistId::EventHorizon},
    TreeNode{149, SkillSet::Mage, 2, 2, "Quick Teleport", "Teleportation is ready 12 % sooner.", 146, Cooldown, SkillId::Teleport, 0.12F},
    TreeNode{150, SkillSet::Mage, 2, 3, "Voidlord", "Every skill is ready 24 % sooner.", 148, Cooldown, SkillId::None, 0.24F},
    TreeNode{151, SkillSet::Archer, 0, 0, "Keen Arrows", "Shot does 12 % more damage.", 0, Damage, SkillId::Shot, 0.12F},
    TreeNode{152, SkillSet::Archer, 0, 0, "Quick Ricochet", "Ricochet Arrow is ready 12 % sooner.", 0, Cooldown, SkillId::RicochetArrow, 0.12F},
    TreeNode{153, SkillSet::Archer, 0, 0, "Heavy Volley", "Piercing Volley does 12 % more damage.", 0, Damage, SkillId::PiercingVolley, 0.12F},
    TreeNode{154, SkillSet::Archer, 0, 1, "Wide Volley", "Piercing Volley has two more.", 151, Count, SkillId::PiercingVolley, 2.0F},
    TreeNode{155, SkillSet::Archer, 0, 1, "Heavy Ricochet", "Ricochet Arrow does 12 % more damage.", 152, Damage, SkillId::RicochetArrow, 0.12F},
    TreeNode{156, SkillSet::Archer, 0, 1, "Sharp Mark", "Hunter's Mark does 12 % more damage.", 153, Damage, SkillId::HuntersMark, 0.12F},
    TreeNode{157, SkillSet::Archer, 0, 2, "Fast Gatling", "Gatling Arrow Gun does 12 % more damage.", 154, Damage, SkillId::GatlingGun, 0.12F},
    TreeNode{158, SkillSet::Archer, 0, 2, "Double Nock", "Her standard shot looses two arrows at once.", 155, Twist, SkillId::Shot, 0.0F, TwistId::DoubleNock},
    TreeNode{159, SkillSet::Archer, 0, 2, "Quick Mark", "Hunter's Mark is ready 12 % sooner.", 156, Cooldown, SkillId::HuntersMark, 0.12F},
    TreeNode{160, SkillSet::Archer, 0, 3, "Deadeye", "Everything does 12 % more damage.", 158, AllDamage, SkillId::None, 0.12F},
    TreeNode{161, SkillSet::Archer, 1, 0, "Thorn Tower", "Vine Tower is ready 12 % sooner.", 0, Cooldown, SkillId::VineTower, 0.12F},
    TreeNode{162, SkillSet::Archer, 1, 0, "Cruel Trap", "Bear Trap does 12 % more damage.", 0, Damage, SkillId::BearTrap, 0.12F},
    TreeNode{163, SkillSet::Archer, 1, 0, "Wide Smoke", "Smoke Arrow reaches a quarter further.", 0, Reach, SkillId::SmokeArrow, 0.25F},
    TreeNode{164, SkillSet::Archer, 1, 1, "Rooted", "Blows do 8 % less harm.", 161, DamageTaken, SkillId::None, 0.08F},
    TreeNode{165, SkillSet::Archer, 1, 1, "Quick Trap", "Bear Trap is ready 12 % sooner.", 162, Cooldown, SkillId::BearTrap, 0.12F},
    TreeNode{166, SkillSet::Archer, 1, 1, "Quick Smoke", "Smoke Arrow is ready 12 % sooner.", 163, Cooldown, SkillId::SmokeArrow, 0.12F},
    TreeNode{167, SkillSet::Archer, 1, 2, "Long Watch", "Owl Sentinel lasts 30 % longer.", 164, Duration, SkillId::OwlSentinel, 0.30F},
    TreeNode{168, SkillSet::Archer, 1, 2, "Mangling Trap", "The Bear Trap also leaves what it catches stunned for two seconds.", 165, Twist, SkillId::BearTrap, 0.0F, TwistId::ManglingTrap},
    TreeNode{169, SkillSet::Archer, 1, 2, "Wary", "Blows do 8 % less harm.", 166, DamageTaken, SkillId::None, 0.08F},
    TreeNode{170, SkillSet::Archer, 1, 3, "Master Trapper", "Every skill is ready 24 % sooner.", 168, Cooldown, SkillId::None, 0.24F},
    TreeNode{171, SkillSet::Archer, 2, 0, "Fierce Wolf", "Spirit Wolf does 12 % more damage.", 0, Damage, SkillId::SpiritWolf, 0.12F},
    TreeNode{172, SkillSet::Archer, 2, 0, "Light Step", "6 % faster on foot.", 0, MoveSpeed, SkillId::None, 0.06F},
    TreeNode{173, SkillSet::Archer, 2, 0, "Long Hawk", "Hawk Eye lasts 30 % longer.", 0, Duration, SkillId::HawkEye, 0.30F},
    TreeNode{174, SkillSet::Archer, 2, 1, "Far Wolf", "Spirit Wolf reaches a quarter further.", 171, Reach, SkillId::SpiritWolf, 0.25F},
    TreeNode{175, SkillSet::Archer, 2, 1, "Quick Evade", "Evasive Shot is ready 12 % sooner.", 172, Cooldown, SkillId::EvasiveShot, 0.12F},
    TreeNode{176, SkillSet::Archer, 2, 1, "Burning Phoenix", "Phoenix Arrow does 12 % more damage.", 173, Damage, SkillId::PhoenixArrow, 0.12F},
    TreeNode{177, SkillSet::Archer, 2, 2, "Quick Wolf", "Spirit Wolf is ready 12 % sooner.", 174, Cooldown, SkillId::SpiritWolf, 0.12F},
    TreeNode{178, SkillSet::Archer, 2, 2, "Alpha Wolf", "The Spirit Wolf’s bite holds its prey still for a second.", 175, Twist, SkillId::SpiritWolf, 0.0F, TwistId::AlphaWolf},
    TreeNode{179, SkillSet::Archer, 2, 2, "Blood of the Wild", "4 % of the damage done heals.", 176, Lifesteal, SkillId::None, 0.04F},
    TreeNode{180, SkillSet::Archer, 2, 3, "Wildheart", "12 % faster on foot.", 178, MoveSpeed, SkillId::None, 0.12F},
}};

constexpr std::array<TreeBranch, 18> branches{{
    TreeBranch{SkillSet::Shadow, 0, "Assassin", "#c9a0ff"},
    TreeBranch{SkillSet::Shadow, 1, "Poisoner", "#7fe07f"},
    TreeBranch{SkillSet::Shadow, 2, "Shadowmancer", "#8aa4ff"},
    TreeBranch{SkillSet::Bone, 0, "Necromancer", "#7fe0a0"},
    TreeBranch{SkillSet::Bone, 1, "Bonelord", "#e0d8b0"},
    TreeBranch{SkillSet::Bone, 2, "Wraith", "#b0a0ff"},
    TreeBranch{SkillSet::Blade, 0, "Duelist", "#ffd27f"},
    TreeBranch{SkillSet::Blade, 1, "Berserker", "#ff8a7a"},
    TreeBranch{SkillSet::Blade, 2, "Warlord", "#9fd1ff"},
    TreeBranch{SkillSet::Dwarf, 0, "Ironbreaker", "#ffb070"},
    TreeBranch{SkillSet::Dwarf, 1, "Runesmith", "#9fd8ff"},
    TreeBranch{SkillSet::Dwarf, 2, "Engineer", "#ffd98a"},
    TreeBranch{SkillSet::Mage, 0, "Stormcaller", "#ffe08a"},
    TreeBranch{SkillSet::Mage, 1, "Frostweaver", "#9fe0ff"},
    TreeBranch{SkillSet::Mage, 2, "Voidwalker", "#c9a0ff"},
    TreeBranch{SkillSet::Archer, 0, "Marksman", "#ffd27f"},
    TreeBranch{SkillSet::Archer, 1, "Trapper", "#9fe09f"},
    TreeBranch{SkillSet::Archer, 2, "Wildheart", "#b0e0a0"},
}};
// NOLINTEND(readability-magic-numbers)

} // namespace

std::span<const TreeNode> all_tree_nodes() noexcept {
    return nodes;
}

std::span<const TreeBranch> all_tree_branches() noexcept {
    return branches;
}

const TreeNode* tree_node(int id) noexcept {
    if (id < 1 || id > static_cast<int>(nodes.size())) {
        return nullptr;
    }
    return &nodes.at(static_cast<std::size_t>(id - 1)); // ids run 1..n in order
}

const TreeBranch* tree_branch(SkillSet set, int branch) noexcept {
    for (const TreeBranch& found : branches) {
        if (found.set == set && found.branch == branch) {
            return &found;
        }
    }
    return nullptr;
}

bool set_has_tree(SkillSet set) noexcept {
    return tree_branch(set, 0) != nullptr;
}

int xp_for_level(int level) noexcept {
    // 60 for level 2, each level 30 more than the last: level 10 takes 1,110 in all, level 20 4,410.
    if (level <= 1) {
        return 0;
    }
    return 60 + 30 * (level - 2);
}

int xp_for_kill(float enemy_max_health) noexcept {
    // A Shroomling (about 40 health) gives 18; a troll (400) gives 90; nothing gives more than 120.
    return std::clamp(10 + static_cast<int>(enemy_max_health / 5.0F), 10, 120);
}

int gain_xp(Growth& growth, int amount) noexcept {
    int gained = 0;
    growth.xp += std::max(amount, 0);
    while (growth.level < max_hero_level && growth.xp >= xp_for_level(growth.level + 1)) {
        growth.xp -= xp_for_level(growth.level + 1);
        ++growth.level;
        ++growth.points;
        ++gained;
    }
    if (growth.level >= max_hero_level) {
        growth.xp = 0;
    }
    return gained;
}

bool can_choose(const Growth& growth, SkillSet set, int id) noexcept {
    const TreeNode* const node = tree_node(id);
    if (node == nullptr || node->set != set || growth.points <= 0 || growth.chosen.test(static_cast<std::size_t>(id))) {
        return false;
    }
    return node->needs == 0 || growth.chosen.test(static_cast<std::size_t>(node->needs));
}

bool choose(Growth& growth, SkillSet set, int id) noexcept {
    if (!can_choose(growth, set, id)) {
        return false;
    }
    growth.chosen.set(static_cast<std::size_t>(id));
    --growth.points;
    return true;
}

SkillMods mods_of(const Growth& growth, SkillSet set) noexcept {
    SkillMods mods;
    mods.damage.fill(1.0F);
    mods.cooldown.fill(1.0F);
    mods.reach.fill(1.0F);
    mods.duration.fill(1.0F);
    mods.count.fill(0);
    for (const TreeNode& node : nodes) {
        if (node.set != set || !growth.chosen.test(static_cast<std::size_t>(node.id))) {
            continue;
        }
        const auto slot = static_cast<std::size_t>(node.skill);
        switch (node.effect) {
        case Damage:
            mods.damage.at(slot) *= 1.0F + node.amount;
            break;
        case Cooldown:
            if (node.skill == SkillId::None) {
                for (float& each : mods.cooldown) {
                    each *= 1.0F - node.amount;
                }
            } else {
                mods.cooldown.at(slot) *= 1.0F - node.amount;
            }
            break;
        case Reach:
            mods.reach.at(slot) *= 1.0F + node.amount;
            break;
        case Count:
            mods.count.at(slot) += static_cast<int>(node.amount);
            break;
        case Duration:
            mods.duration.at(slot) *= 1.0F + node.amount;
            break;
        case MaxHealth:
            mods.max_health += node.amount;
            break;
        case Regen:
            mods.regen += node.amount;
            break;
        case MoveSpeed:
            mods.move *= 1.0F + node.amount;
            break;
        case AllDamage:
            mods.all_damage *= 1.0F + node.amount;
            break;
        case DamageTaken:
            mods.damage_taken *= 1.0F - node.amount;
            break;
        case Lifesteal:
            mods.lifesteal += node.amount;
            break;
        case Experience:
            mods.experience *= 1.0F + node.amount;
            break;
        case Twist:
            mods.twists.set(static_cast<std::size_t>(node.twist));
            break;
        }
    }
    return mods;
}

} // namespace e5::gameplay

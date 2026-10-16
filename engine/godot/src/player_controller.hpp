#pragma once

#include "character_animator.hpp"
#include "e5/gameplay/bow_state.hpp"
#include "e5/gameplay/character_motor.hpp"
#include "e5/gameplay/skills.hpp"
#include "e5/gameplay/vitals.hpp"

#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace godot {
class BoneAttachment3D;
class Camera3D;
class GPUParticles3D;
class OmniLight3D;
class CanvasLayer;
class Node3D;
class SpringArm3D;
} // namespace godot

namespace e5::bridge {

struct SpellTiming; // player_controller.cpp
class E5Enemy;

class E5AimOffset;
class E5Arrow;
class E5Bird;
class E5BowString;
class E5HealthHud;
class E5Inventory;
class E5SkillBarHud;

// Third-person character. Movement and animation-state rules live in
// e5::gameplay (engine-agnostic, unit-tested); this class feeds them Godot
// input, hands the velocity to Godot's physics (Jolt) for collision
// resolution, and plays the matching animation.
//
// Expected children:
//   CameraPivot (Node3D)    orbit point at shoulder height; holds the camera rig
//                           (typically SpringArm3D -> Camera3D)
//   Model (Node3D)          the visual, front along +Z (glTF convention), with
//                           a humanoid skeleton if `animation_library` is set
//
// `animation_library` must contain clips named idle, walk, run and jump.
//
// Archery is enabled when the library also has bow_draw, bow_aim, bow_recoil and
// bow_walk_forward/back/left/right, and an E5BowString exists somewhere below
// this node. `e5_skill_1` .. `e5_skill_10` pick a skill; holding `e5_aim` draws the
// bow and letting it go at full draw uses the skill; `e5_cancel` lowers the bow.
class E5PlayerController : public godot::CharacterBody3D {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5PlayerController, godot::CharacterBody3D)

public:
    static constexpr const char* group_name = "e5_player";
    // The heroes of the other players in a shared game are in this group instead: they are
    // shown and animated here, but played on another machine. See set_remote.
    static constexpr const char* remote_group_name = "e5_remote_player";

    void _ready() override;
    void _physics_process(double delta) override;
    void _unhandled_input(const godot::Ref<godot::InputEvent>& event) override;

    void set_walk_speed(float speed) { params_.walk_speed = speed; }
    [[nodiscard]] float get_walk_speed() const { return params_.walk_speed; }
    void set_sprint_speed(float speed) { params_.sprint_speed = speed; }
    [[nodiscard]] float get_sprint_speed() const { return params_.sprint_speed; }
    void set_jump_velocity(float velocity) { params_.jump_velocity = velocity; }
    [[nodiscard]] float get_jump_velocity() const { return params_.jump_velocity; }
    void set_fall_acceleration(float acceleration) { params_.gravity = acceleration; }
    [[nodiscard]] float get_fall_acceleration() const { return params_.gravity; }
    void set_turn_speed(float radians_per_second) { turn_speed_ = radians_per_second; }
    [[nodiscard]] float get_turn_speed() const { return turn_speed_; }
    void set_mouse_sensitivity(float radians_per_pixel) { mouse_sensitivity_ = radians_per_pixel; }
    [[nodiscard]] float get_mouse_sensitivity() const { return mouse_sensitivity_; }
    // The dodge (the block key, for a hero who has the clip `dodge` and no shield): a dive and
    // roll the way she is steered, or the way she faces. `dodge_distance` is how far it carries
    // her; nothing hurts her while it lasts.
    void set_dodge_distance(float metres) { dodge_distance_ = metres; }
    [[nodiscard]] float get_dodge_distance() const { return dodge_distance_; }
    [[nodiscard]] bool is_dodging() const { return dodge_left_ > 0.0F; }
    // The dance (a key of its own, for a hero whose library has the clip `emote`): she dances it
    // through unless she is steered, attacks or is made to stop. Only for show; what is heard
    // with it is game/characters/emote.gd. True also for another player's hero seen dancing.
    // The name of the instant skill she is in the middle of (a kick, a blow of a combo), or empty:
    // for what is shown and heard with it (game/characters/dagger_carry.gd).
    [[nodiscard]] godot::String get_action_skill_name() const;
    // The slots after the standard attack that are this hero's to fill, shown also while empty.
    [[nodiscard]] int get_open_slot_count() const;
    [[nodiscard]] gameplay::SkillSet skill_bar_set() const;
    // A hero whose blows are her sword's (the first warrior and the new one).
    [[nodiscard]] bool fights_with_sword() const;
    /// The dwarf: every blow of his is his axe's.
    [[nodiscard]] bool fights_with_axe() const;
    /// The Shadowblade: every blow of his is his daggers'.
    [[nodiscard]] bool fights_with_dagger() const;
    [[nodiscard]] bool is_emoting() const { return remote_ ? net_clip_ == clip_emote_ : emote_left_ > 0.0F; }
    // On a horse (game/characters/mount.gd decides when: the E key at a horse, and shows it under
    // her). Only a hero whose library has the clip `ride` can. Another player's hero rides when
    // that is the clip she plays. `try_set_mounted` is the player's wish: refused while she is
    // busy, in the air or dead; it returns whether she now is as asked.
    [[nodiscard]] bool is_mounted() const { return remote_ ? net_clip_ == clip_ride_ : mounted_; }
    void set_mounted(bool mounted);
    bool try_set_mounted(bool mounted);
    void set_mount_speed(float speed) { mount_speed_ = speed; }
    [[nodiscard]] float get_mount_speed() const { return mount_speed_; }
    void set_mount_sprint_speed(float speed) { mount_sprint_speed_ = speed; }
    [[nodiscard]] float get_mount_sprint_speed() const { return mount_sprint_speed_; }
    void set_mount_walk_speed(float speed) { mount_walk_speed_ = speed; }
    [[nodiscard]] float get_mount_walk_speed() const { return mount_walk_speed_; }
    // Quick cast (a setting): a number key chooses its skill and uses it at once, the key
    // doing what the right mouse button does: held it draws the bow, let go it shoots.
    void set_quick_cast(bool enabled) { quick_cast_ = enabled; }
    [[nodiscard]] bool get_quick_cast() const { return quick_cast_; }
    void set_aim_move_speed(float speed) { aim_move_speed_ = speed; }
    [[nodiscard]] float get_aim_move_speed() const { return aim_move_speed_; }
    void set_arrow_speed(float speed) { arrow_speed_ = speed; }
    [[nodiscard]] float get_arrow_speed() const { return arrow_speed_; }
    void set_capture_mouse_on_ready(bool capture) { capture_mouse_on_ready_ = capture; }
    [[nodiscard]] bool get_capture_mouse_on_ready() const { return capture_mouse_on_ready_; }
    void set_animation_library(const godot::Ref<godot::AnimationLibrary>& library) { animation_library_ = library; }
    [[nodiscard]] godot::Ref<godot::AnimationLibrary> get_animation_library() const { return animation_library_; }

    // Effect scenes of the power shot and the arrow rain. All optional.
    void set_charge_effect(const godot::Ref<godot::PackedScene>& scene) { charge_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_charge_effect() const { return charge_effect_; }
    void set_charge_full_effect(const godot::Ref<godot::PackedScene>& scene) { charge_full_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_charge_full_effect() const { return charge_full_effect_; }
    void set_rain_marker_effect(const godot::Ref<godot::PackedScene>& scene) { rain_marker_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_rain_marker_effect() const { return rain_marker_effect_; }
    void set_rain_impact_effect(const godot::Ref<godot::PackedScene>& scene) { rain_impact_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_rain_impact_effect() const { return rain_impact_effect_; }
    void set_trail_effect(const godot::Ref<godot::PackedScene>& scene) { trail_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_trail_effect() const { return trail_effect_; }
    void set_impact_effect(const godot::Ref<godot::PackedScene>& scene) { impact_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_impact_effect() const { return impact_effect_; }

    // Frost Fan and Fire Arrow: carried by the arrow, and played where it lands.
    void set_frost_trail_effect(const godot::Ref<godot::PackedScene>& scene) { frost_trail_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_frost_trail_effect() const { return frost_trail_effect_; }
    void set_frost_impact_effect(const godot::Ref<godot::PackedScene>& scene) { frost_impact_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_frost_impact_effect() const { return frost_impact_effect_; }
    void set_fire_trail_effect(const godot::Ref<godot::PackedScene>& scene) { fire_trail_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_fire_trail_effect() const { return fire_trail_effect_; }
    void set_fire_impact_effect(const godot::Ref<godot::PackedScene>& scene) { fire_impact_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_fire_impact_effect() const { return fire_impact_effect_; }
    // Thunder Kick: played in front of her when the kick lands; emits along its local +Z.
    void set_kick_effect(const godot::Ref<godot::PackedScene>& scene) { kick_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_kick_effect() const { return kick_effect_; }

    // Kingfishers: the bird scene (root E5Bird), the glow in her raised hand, the burst when they are released.
    void set_bird_scene(const godot::Ref<godot::PackedScene>& scene) { bird_scene_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_bird_scene() const { return bird_scene_; }
    void set_summon_cast_effect(const godot::Ref<godot::PackedScene>& scene) { summon_cast_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_summon_cast_effect() const { return summon_cast_effect_; }
    void set_summon_burst_effect(const godot::Ref<godot::PackedScene>& scene) { summon_burst_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_summon_burst_effect() const { return summon_burst_effect_; }

    // Which skills are on the bar: 0 = the archer's, 1 = the wizard's, 2 = the warrior's, 3 = the dwarf's.
    void set_skill_set(int set) { skill_set_ = set; }
    [[nodiscard]] int get_skill_set() const { return skill_set_; }
    // The name of the skill used last and how many were used: for the tests' report.
    [[nodiscard]] godot::String get_last_skill() const;
    [[nodiscard]] int get_skills_used() const { return skills_used_; }
    // The wizard's spells: the glow in his hand while he casts, what an arcane bolt carries and
    // leaves where it bursts, and the ring of a frost nova. The fireball uses the fire effects.
    void set_cast_effect(const godot::Ref<godot::PackedScene>& scene) { cast_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_cast_effect() const { return cast_effect_; }
    void set_bolt_trail_effect(const godot::Ref<godot::PackedScene>& scene) { bolt_trail_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_bolt_trail_effect() const { return bolt_trail_effect_; }
    void set_bolt_impact_effect(const godot::Ref<godot::PackedScene>& scene) { bolt_impact_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_bolt_impact_effect() const { return bolt_impact_effect_; }
    void set_nova_effect(const godot::Ref<godot::PackedScene>& scene) { nova_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_nova_effect() const { return nova_effect_; }
    // Where lightning strikes; the mark on the ground a meteor falls on and its explosion; what a star carries and
    // leaves.
    void set_lightning_effect(const godot::Ref<godot::PackedScene>& scene) { lightning_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_lightning_effect() const { return lightning_effect_; }
    void set_meteor_marker_effect(const godot::Ref<godot::PackedScene>& scene) { meteor_marker_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_meteor_marker_effect() const { return meteor_marker_effect_; }
    void set_meteor_impact_effect(const godot::Ref<godot::PackedScene>& scene) { meteor_impact_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_meteor_impact_effect() const { return meteor_impact_effect_; }
    void set_star_trail_effect(const godot::Ref<godot::PackedScene>& scene) { star_trail_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_star_trail_effect() const { return star_trail_effect_; }
    void set_black_hole_effect(const godot::Ref<godot::PackedScene>& scene) { black_hole_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_black_hole_effect() const { return black_hole_effect_; }
    void set_black_hole_burst_effect(const godot::Ref<godot::PackedScene>& scene) { black_hole_burst_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_black_hole_burst_effect() const {
        return black_hole_burst_effect_;
    }
    void set_star_impact_effect(const godot::Ref<godot::PackedScene>& scene) { star_impact_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_star_impact_effect() const { return star_impact_effect_; }

    // What plays on the weapon during each of a melee character's special blows (in the order
    // they are on the bar): effect scenes made for a weapon that points along +Y from its grip.
    // The dwarf's fourth is his battle cry and plays at his feet instead.
    void set_weapon_effect_1(const godot::Ref<godot::PackedScene>& scene) { weapon_effect_1_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_weapon_effect_1() const { return weapon_effect_1_; }
    void set_weapon_effect_2(const godot::Ref<godot::PackedScene>& scene) { weapon_effect_2_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_weapon_effect_2() const { return weapon_effect_2_; }
    void set_weapon_effect_3(const godot::Ref<godot::PackedScene>& scene) { weapon_effect_3_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_weapon_effect_3() const { return weapon_effect_3_; }
    void set_weapon_effect_4(const godot::Ref<godot::PackedScene>& scene) { weapon_effect_4_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_weapon_effect_4() const { return weapon_effect_4_; }

    // Health. Damage is collected and applied in the next physics step. At zero she dies, lies
    // for a few seconds, and comes back where she started.
    void take_damage(float amount);
    // Cooldowns and the charge (e5/gameplay/skills.hpp). The skill bar reads these.
    [[nodiscard]] float get_charge() const { return charge_always_full_ ? 1.0F : charge_; }
    // For a place to try things out (the test arena): the skill that takes a full charge is
    // always ready, and using it costs nothing.
    void set_charge_always_full(bool full) { charge_always_full_ = full; }
    [[nodiscard]] bool get_charge_always_full() const { return charge_always_full_; }
    [[nodiscard]] float get_skill_cooldown(int slot) const;         // seconds left
    [[nodiscard]] float get_skill_cooldown_seconds(int slot) const; // of how many
    [[nodiscard]] bool skill_takes_charge(int slot) const;
    [[nodiscard]] bool is_skill_ready(int slot) const;
    // The draw or cast under way, for the bar over the skills. Progress: 0..1 up to the moment
    // a drawn bow may be let go or a spell strikes; -1 when there is nothing to wait for (a
    // weapon's blows are too quick for a bar). Seconds: how long that wait is in all. Power:
    // 0..1, what a skill that charges has built up at full draw.
    [[nodiscard]] float get_cast_progress() const;
    [[nodiscard]] float get_cast_seconds() const;
    [[nodiscard]] float get_cast_power() const;
    // What she did to an enemy: fills the charge. Called by whatever does damage for her.
    void credit_damage(float damage, bool killed);
    // Tests that use one skill again and again switch the waiting off.
    void set_cooldowns_enabled(bool enabled) { cooldowns_enabled_ = enabled; }
    // Damage that comes from somewhere: a raised shield stops it if it is not behind her.
    void take_damage_from(float amount, const godot::Vector3& from);
    // The shield (the warrior's): whether it is up, how long until it can be raised again,
    // and how many hits it has stopped (for tests).
    [[nodiscard]] bool can_block() const { return block_enabled_; }
    [[nodiscard]] bool is_blocking() const { return block_.raised; }
    [[nodiscard]] float get_block_cooldown() const { return block_.cooldown_left; }
    [[nodiscard]] float get_block_cooldown_seconds() const { return block_params_.cooldown_seconds; }
    [[nodiscard]] float get_block_time_left() const {
        return block_.raised ? block_params_.max_hold_seconds - block_.held_seconds : 0.0F;
    }
    [[nodiscard]] int get_hits_blocked() const { return hits_blocked_; }
    // A stance (the new warrior's Counter Attack, e5/gameplay/skills.hpp): the seconds she still
    // stands ready, also for another player's hero (game/characters/stance_glow.gd shows it), and
    // how many blows she has answered (for tests).
    [[nodiscard]] float get_stance_seconds() const { return counter_left_; }
    [[nodiscard]] int get_counters_struck() const { return counters_struck_; }
    // Enrage (the new warrior's): the seconds her frenzy still lasts, also for another player's
    // hero (stance_glow.gd shows it), and how many blows she has struck in it (for tests).
    [[nodiscard]] float get_enrage_seconds() const { return enrage_left_; }
    [[nodiscard]] int get_enrage_blows() const { return enrage_blows_; }
    // Never Give Up (the new warrior's): the seconds it still lasts, also for another player's
    // hero (stance_glow.gd shows it), and what it has kept from her and given back (for tests).
    [[nodiscard]] float get_resolve_seconds() const { return resolve_left_; }
    // Runic Bulwark (the new dwarf's): the seconds it still lasts, also for another player's hero
    // (stance_glow.gd shows it), and what it has kept from him (for tests).
    [[nodiscard]] float get_bulwark_seconds() const { return bulwark_left_; }
    // The Shadowblade's two stances, for his scene's scripts (shadow_look.gd, stance_glow.gd) and enemies.
    [[nodiscard]] bool is_veiled() const { return veil_left_ > 0.0F; }
    [[nodiscard]] float get_veil_seconds() const { return veil_left_; }
    [[nodiscard]] float get_venom_seconds() const { return venom_left_; }
    [[nodiscard]] int get_shadow_steps() const { return shadow_steps_; }
    [[nodiscard]] int get_knives_thrown() const { return knives_thrown_; }
    [[nodiscard]] int get_ambushes() const { return ambushes_; }
    [[nodiscard]] int get_poisonings() const { return poisonings_; }
    [[nodiscard]] float get_healed_by_skills() const { return healed_by_skills_; }
    [[nodiscard]] int get_cursed_count() const { return cursed_count_; }
    // The Archer's five of 2026-10-16: her spirit wolf's bites, her owl's arrows, her springs (for tests).
    [[nodiscard]] int get_wolf_bites() const { return wolf_bites_; }
    [[nodiscard]] int get_owl_shots() const { return owl_shots_; }
    [[nodiscard]] int get_springs() const { return springs_; }
    // The new Wizard's five of 2026-10-16 (for tests): the enemies his orb passed through, those his
    // gravity well caught as it imploded, his pylon's strikes, the enemies he froze, the missiles sent.
    [[nodiscard]] int get_orb_touches() const { return orb_touches_; }
    [[nodiscard]] int get_well_caught() const { return well_caught_; }
    [[nodiscard]] int get_pylon_zaps() const { return pylon_zaps_; }
    [[nodiscard]] int get_prisons() const { return prisons_; }
    [[nodiscard]] int get_missiles_sent() const { return missiles_sent_; }
    // The new Warrior's five of 2026-10-16 (for tests): the enemies her charge struck, the strikes of
    // her falling swords, her blows that executed, the health her banner gave her, what her blows are
    // worth now (1.25 beside her banner), the strikes of her thrown sword.
    [[nodiscard]] int get_charge_struck() const { return charge_struck_; }
    [[nodiscard]] int get_blade_rain_hits() const { return blade_rain_hits_; }
    [[nodiscard]] int get_executions() const { return executions_; }
    [[nodiscard]] float get_banner_healed() const { return banner_healed_; }
    [[nodiscard]] float get_blow_factor() const { return banner_blow_factor(); }
    [[nodiscard]] int get_throw_hits() const { return throw_hits_; }
    // The new Dwarf's five of 2026-10-16: the enemies his tremor's rings struck, his cannon's shots, the
    // seconds his Stone Brew still lasts (stance_glow.gd shows it, also for another player's hero), what
    // it kept from him and gave him back, the enemies his keg caught (for tests). His thrown axe's
    // strikes are counted with her sword's (get_throw_hits).
    [[nodiscard]] int get_tremor_hits() const { return tremor_hits_; }
    [[nodiscard]] int get_cannon_shots() const { return cannon_shots_; }
    [[nodiscard]] float get_stone_brew_seconds() const { return stone_brew_left_; }
    [[nodiscard]] float get_stone_brew_spared() const { return stone_brew_spared_; }
    [[nodiscard]] float get_stone_brew_healed() const { return stone_brew_healed_; }
    [[nodiscard]] int get_keg_caught() const { return keg_caught_; }
    [[nodiscard]] float get_bulwark_spared() const { return bulwark_spared_; }
    // Ancestral Anvil: how often it has pulsed, and the health it gave this hero (for tests).
    [[nodiscard]] int get_anvil_pulses() const { return anvil_pulses_; }
    [[nodiscard]] float get_anvil_healed() const { return anvil_healed_; }
    // Sprintsz (the new warrior's): the seconds it still lasts, and how often it has pounded (for tests).
    [[nodiscard]] float get_stampede_seconds() const { return stampede_.seconds_left; }
    [[nodiscard]] int get_stampede_ticks() const { return stampede_ticks_; }
    // Cut in Pieces (the new warrior's): how many of its blows have landed (for tests).
    [[nodiscard]] int get_pieces_blows() const { return pieces_blows_; }
    [[nodiscard]] float get_pieces_spared() const { return pieces_spared_; }
    [[nodiscard]] int get_firebolts_thrown() const { return firebolts_thrown_; }
    [[nodiscard]] int get_chain_strikes() const { return chain_strikes_; }
    [[nodiscard]] int get_hydras() const { return hydras_; }
    [[nodiscard]] int get_teleports() const { return teleports_; }
    [[nodiscard]] int get_hole_ticks() const { return hole_ticks_; }
    [[nodiscard]] int get_blizzard_ticks() const { return blizzard_ticks_; }
    [[nodiscard]] int get_traps_set() const { return traps_set_; }
    // The new wizard's Whose Ur Mommy?: whether this hero is such a copy (his scene's CloneGlow
    // asks), and for the one who called copies: how many, how often they cast, what struck them.
    [[nodiscard]] bool is_clone() const { return clone_; }
    [[nodiscard]] int get_clones() const { return clones_; }
    [[nodiscard]] int get_clone_casts() const { return clone_casts_; }
    [[nodiscard]] float get_clone_struck() const { return clone_struck_; }
    [[nodiscard]] int get_traps_sprung() const { return traps_sprung_; }
    [[nodiscard]] int get_traps_lying() const { return static_cast<int>(traps_.size()); }
    [[nodiscard]] int get_blizzard_held() const { return blizzard_held_; }
    [[nodiscard]] int get_hole_pulls() const { return hole_pulls_; }
    [[nodiscard]] float get_teleported_metres() const { return teleported_metres_; }
    [[nodiscard]] int get_hydra_spits() const { return hydra_spits_; }
    // The new wizard's Frost Armor: how long his body still shows it (while the barrier holds),
    // what the barrier can still take, and what it has taken in all.
    [[nodiscard]] float get_armor_seconds() const { return armor_barrier_ > 0.0F ? armor_left_ : 0.0F; }
    [[nodiscard]] float get_armor_barrier() const { return armor_left_ > 0.0F ? armor_barrier_ : 0.0F; }
    [[nodiscard]] float get_armor_absorbed() const { return armor_absorbed_; }
    [[nodiscard]] int get_armor_slowed() const { return armor_slowed_; }
    [[nodiscard]] float get_firebolt_power() const { return firebolt_power_; }
    // The Archer's Vine Tower: how many have grown under her (for tests).
    [[nodiscard]] int get_towers_grown() const { return towers_grown_; }
    // The Archer's Gatling Arrow Gun: the turret she built or is building, if it is still there;
    // whether she mans it; how many arrows have left it (for tests). `try_set_manning` is the
    // player's wish (the E key: game/characters/gatling.gd): taking her place at it is refused
    // while she is busy, in the air, riding or dead, and without a turret that stands.
    [[nodiscard]] godot::Node3D* get_turret() const;
    [[nodiscard]] bool is_manning() const { return manning_; }
    bool try_set_manning(bool manning);
    [[nodiscard]] int get_gatling_arrows() const { return gatling_arrows_; }
    // The Archer's Disguise: whether she is a bush now (enemies ask: E5Enemy::nearest_player), how
    // long still, and what the bush she took looks like, for game/characters/disguise.gd, which
    // shows it in her place. `end_disguise` is also the E key's. `find_bush`: where the nearest
    // bush within so many metres stands, or nothing (for tests).
    [[nodiscard]] bool is_disguised() const { return disguised_; }
    [[nodiscard]] float get_disguise_seconds() const { return disguise_left_; }
    [[nodiscard]] godot::Ref<godot::Mesh> get_disguise_mesh() const { return disguise_mesh_; }
    [[nodiscard]] godot::Ref<godot::Material> get_disguise_material() const { return disguise_material_; }
    [[nodiscard]] godot::Basis get_disguise_basis() const { return disguise_basis_; }
    [[nodiscard]] int get_disguises() const { return disguises_; }
    void end_disguise();
    /// Another player's hero, shown here: she is a bush or she is not, as her own machine says.
    void set_shown_disguised(bool disguised);
    /// ... and stands at her arrow gun or does not.
    void set_shown_manning(bool manning);
    [[nodiscard]] godot::Variant find_bush(float radius, const godot::String& kind = godot::String()) const;
    [[nodiscard]] float get_resolve_spared() const { return resolve_spared_; }
    /// Damage a Dome of Protection has kept from her, and whether she stands in one now.
    [[nodiscard]] float get_dome_spared() const { return dome_spared_; }
    [[nodiscard]] bool in_dome() const;
    [[nodiscard]] float get_resolve_healed() const { return resolve_healed_; }

    // A remote hero is another player's, shown here: it takes no input, has no camera and no
    // interface, collides with nothing, and does what `apply_net_state` tells it. To be set
    // before the node enters the tree.
    void set_remote(bool remote) { remote_ = remote; }
    [[nodiscard]] bool is_remote() const { return remote_; }
    // What the other machines need to show this hero:
    // [position, facing, clip, clip speed, look yaw, look pitch, share of health left].
    [[nodiscard]] godot::Array get_net_state() const;
    void apply_net_state(const godot::Vector3& position, float facing, const godot::String& clip, float speed,
                         float look_yaw, float look_pitch);
    // The skills used since the last call, for the other machines to replay, each as
    // [kind (0 = a skill with its own animation starts, 1 = the bow is loosed), slot, power,
    //  combo step, look yaw, look pitch].
    godot::Array take_net_events();
    // A remote hero replays one: the same effects and missiles, which hurt nobody. What the
    // real ones hit is decided on the machine that plays her.
    // `skill` is the skill's number (SkillId), not its slot: bars differ from player to player.
    void apply_net_event(int kind, int skill, float power, int combo_step, float look_yaw, float look_pitch);
    // Damage a hit on this remote hero would do, collected for her own machine.
    void take_remote_damage(float amount, const godot::Vector3& position);
    float take_outgoing_damage();
    [[nodiscard]] float get_health() const { return vitals_.health; }
    [[nodiscard]] bool is_dead() const { return vitals_.dead; }
    [[nodiscard]] int get_death_count() const { return death_count_; }
    [[nodiscard]] float get_damage_taken() const { return damage_taken_; }
    // Gives health back; false when there was nothing to heal (full health, or dead).
    bool heal(float amount);
    // Maximum health with what the worn charms add.
    [[nodiscard]] float get_effective_max_health() const;
    [[nodiscard]] E5Inventory* get_inventory() const { return inventory_; }
    [[nodiscard]] float get_model_yaw() const { return model_yaw_; }
    // While a menu is open: she takes no movement, aiming or camera input.
    void set_input_blocked(bool blocked) { input_blocked_ = blocked; }
    [[nodiscard]] bool is_input_blocked() const { return input_blocked_; }
    void set_max_health(float health) { vitals_params_.max_health = health; }
    [[nodiscard]] float get_max_health() const { return vitals_params_.max_health; }

    // Orbit angle of the camera about the character, in radians.
    void set_camera_yaw(float radians);
    [[nodiscard]] float get_camera_yaw() const { return look_.yaw; }
    // Selects a skill slot (0-based). Ignored for empty slots and while the bow is raised.
    void select_skill(int slot);
    // The slot the player selected (the right button's), also while the left button has the bar on its first.
    [[nodiscard]] int get_selected_skill() const {
        return static_cast<int>(use_button_ == UseButton::Standard ? selected_slot_ : skills_.selected_index());
    }
    // For the interface script, which draws the skill bar: what a slot holds (empty for none), whether
    // the standard attack is in use, and a switch for the plain bar this node draws itself.
    [[nodiscard]] godot::String get_skill_name(int slot) const;
    // The skill book (F5, game/ui/skill_book.gd): every skill this hero has, on the bar or not,
    // with what the book shows of each (name, description, damage, damage at full power,
    // cooldown, needs_charge, kind, from_wish, standard, slot: where it is on the bar, or -1).
    [[nodiscard]] godot::Array get_skill_catalogue() const;
    // Puts one of her skills, by its name, into a slot of the bar (not the first: that is her
    // standard attack). If it was in another slot, the two exchange what they hold. Refused
    // while she is busy with a skill. `skill_bar_changed` is emitted when the bar changed.
    bool set_skill_slot(int slot, const godot::String& skill_name);
    bool clear_skill_slot(int slot);
    // The name of the skill the player selected (see get_selected_skill); empty for none.
    [[nodiscard]] godot::String get_selected_skill_name() const { return get_skill_name(get_selected_skill()); }
    [[nodiscard]] bool is_standard_attack_in_use() const { return use_button_ == UseButton::Standard && is_busy(); }
    void set_skill_bar_visible(bool visible);

    // Camera tilt in radians; positive looks up.
    void set_camera_pitch(float radians);

    // Name of the clip currently playing; empty without an animation library.
    [[nodiscard]] godot::String get_current_animation() const;

protected:
    static void _bind_methods();

private:
    void setup_animation();
    void apply_look_to_nodes();
    void update_facing(const gameplay::Vec3& velocity, float delta);
    // The bright arc of the combo's blow that is being struck.
    void show_slash_arc();
    void update_animation(const gameplay::Vec3& velocity, float delta);
    void update_bow_string(float string_draw);
    void setup_archery();
    void setup_tip_glows();
    void setup_aim_rig();
    // What every character with skills needs: the camera to aim with, the crosshair, the
    // skill bar, and the hands to cast or draw from.
    void setup_skill_ui();
    // Spells need no bow: a character whose library has their clips can cast them.
    void setup_spells();
    void cast_spell(gameplay::SkillId spell);
    void cast_lightning();
    void cast_meteor();
    void cast_star_barrage();
    void cast_black_hole();
    // The warrior's blows: a hit on everything in an area in front of her.
    [[nodiscard]] godot::Ref<godot::PackedScene> blade_effect(gameplay::SkillId skill) const;
    void start_blade_effect(gameplay::SkillId skill);
    // What a special blow leaves where it lands: the weapon effect's variant named
    // <weapon effect>_impact.tscn, if there is one.
    [[nodiscard]] godot::Ref<godot::PackedScene> impact_effect(gameplay::SkillId skill) const;
    // Shakes the camera for a moment; a heavier blow shakes harder.
    void update_shake(float delta);
    // The sword combo: counts the time since the last blow, and picks the next one. Returns
    // the timing of that blow, or nullptr for any other skill (which ends the combo).
    void tick_combo(float delta);
    const SpellTiming* advance_combo(gameplay::SkillId skill);
    void strike_melee(gameplay::SkillId skill);
    // The Counter Attack: the stance begins; a blow that came from `from` is answered; the
    // stance runs out and the wounds bleed.
    void start_stance(gameplay::SkillId skill);
    void answer_blow(const godot::Vector3& from);
    void update_counter(float delta);
    // The Whirlwind, a channel: it begins; and while it lasts it strikes round her and draws
    // enemies in. (Her turning is in update_facing, her pose in update_animation.)
    void start_channel(gameplay::SkillId skill);
    void update_whirl(float delta);
    [[nodiscard]] bool is_whirling() const { return whirl_.seconds_left > 0.0F; }
    // The Jump Attack: she leaves the ground for the place aimed at; flies, lands and strikes,
    // and gathers herself. While it lasts nothing else moves her.
    void start_leap(gameplay::SkillId skill);
    // While its button is held a mark on the ground shows where she would land; let go, she leaps
    // (bug report 32: there was no telling where a leap would end).
    void update_leap_aim(bool held);
    void update_leap(float delta);
    void land_leap();
    [[nodiscard]] bool is_leaping() const { return leap_.flying || leap_.recover_left > 0.0F; }
    // Enrage: the frenzy begins; while it lasts and an enemy is near, it is what moves her and
    // strikes (update_enrage then returns true, and nothing else is asked that step).
    void start_enrage(gameplay::SkillId skill);
    bool update_enrage(float delta);
    void strike_enrage(const godot::Vector3& forward);
    [[nodiscard]] bool is_enraged() const { return enrage_left_ > 0.0F; }
    // The Vine Tower: she kneels and charges while the button is held; let go, the tower grows
    // under her (or, let go too soon, nothing does).
    void start_tower_charge(gameplay::SkillId skill);
    void update_tower_charge(bool held, float delta);
    void start_firebolt_charge();
    void update_firebolt_charge(bool held, float delta);
    void end_firebolt_glow();
    void throw_firebolt();
    void grow_tower(float power);
    [[nodiscard]] bool is_charging_tower() const { return tower_charging_; }
    // The Gatling Arrow Gun is built the way the tower is charged (the same kneeling, the same
    // count), and stands when the count is full: `place_turret` sets it down in front of her,
    // `raise_gatling` is the moment it is done, `update_gatling` is all she does while she mans it.
    godot::Node3D* place_turret(bool built);
    void raise_gatling();
    void update_gatling(float delta);
    void leave_turret();
    // Disguise: the bush nearest to her, in a forest or standing by itself in the scene.
    struct Bush {
        std::uint64_t forest = 0; // its forest, or
        std::uint64_t node = 0;   // the bush itself: ids
        int index = -1;
        godot::Transform3D transform;
        godot::Ref<godot::Mesh> mesh; // empty: no bush
        godot::Ref<godot::Material> material;
    };
    [[nodiscard]] Bush nearest_bush(float radius, const godot::String& kind = godot::String()) const;
    void start_disguise(float reach);
    void update_disguise(float delta);
    // Cut in Pieces: it begins if an enemy is near; while it lasts it is what moves her (from one
    // enemy to the next) and strikes, and update_pieces returns true: nothing else is asked.
    [[nodiscard]] E5Enemy* pieces_victim() const;
    void start_pieces(gameplay::SkillId skill);
    bool update_pieces(float delta);
    [[nodiscard]] bool is_cutting() const { return pieces_.seconds_left > 0.0F; }
    // The Seismic Slash: her blow lands; the ground breaks open ahead of her, row after row.
    void strike_seismic();
    void strike_fissure();
    // The Shadowblade's (skills.hpp, "The Shadowblade's five").
    [[nodiscard]] E5Enemy* shadow_step_target() const;
    void shadow_step();
    void strike_shadow_step();
    void throw_knives();
    float ambush();
    void envenom(E5Enemy* enemy);
    void envenom_near(const godot::Vector3& centre, float radius);
    // The Bonecaller's (skills.hpp, "The Bonecaller's five").
    void strike_bone_spikes();
    void drain_soul();
    void curse_area();
    void sing_requiem();
    void heal_self(float amount);
    // The Archer's five (skills.hpp, "The Archer's five"): the two arrows, the wolf that runs and
    // bites, the owl that stands and shoots, the spring back and its three arrows.
    void fire_hunter_arrow(gameplay::SkillId skill);
    void send_wolf();
    void update_wolf(float delta);
    void place_owl();
    void update_owl(float delta);
    void spring_back();
    void update_spring(float delta);
    // The new Wizard's five (skills.hpp, "The new Wizard's five"): the orb that flies through, the
    // well that draws in and implodes, the pylon that stands and strikes, the prison of ice, the
    // missiles that leave one after another.
    void cast_orb();
    void update_orb(float delta);
    void cast_well();
    void update_well(float delta);
    void place_pylon();
    void update_pylon(float delta);
    void cast_prison();
    void cast_missiles();
    void update_missiles(float delta);
    // The new Warrior's five (skills.hpp, "The new Warrior's five"): the dash that flings aside, the
    // swords that fall, the blow that ends what is low, the banner that strengthens her alone, the
    // sword that flies out and back.
    void start_dash();
    void update_dash(float delta); // after she has moved: strikes, and ends it at a wall
    void end_dash();
    void cast_blade_rain();
    void update_blade_rain(float delta);
    void strike_execute();
    void plant_banner();
    void update_banner(float delta);
    // Her blows' factor: banner_blow_factor beside her OWN banner, else 1 (nobody else's helps her,
    // hers helps nobody else).
    [[nodiscard]] float banner_blow_factor() const;
    [[nodiscard]] bool near_own_banner() const;
    void throw_sword(gameplay::SkillId skill); // her Sword Throw, or the new Dwarf's Throwing Axe
    void update_throw(float delta);
    void show_sword_in_hand(bool shown);
    [[nodiscard]] godot::Node* weapons_node() const; // swords.gd's node: "Swords" or "Axes"
    // The new Dwarf's five (skills.hpp, "The new Dwarf's five"): the rings of shock, the cannon that
    // stands and fires, the keg that blows up (his Throwing Axe is throw_sword, his Stone Brew a stance).
    void strike_tremor();
    void update_tremor(float delta);
    void place_cannon();
    void update_cannon(float delta);
    void set_keg();
    void update_keg(float delta);
    void blow_keg();
    // The living enemy nearest to `place`, within `slack` of it (its body's width allowed for).
    [[nodiscard]] E5Enemy* enemy_near(const godot::Vector3& place, float slack) const;
    // Where the way from `from` to `to` first meets something that is neither a hero nor an enemy.
    [[nodiscard]] std::optional<godot::Vector3> wall_between(const godot::Vector3& from,
                                                             const godot::Vector3& to) const;
    void raise_anvil();
    void hurl_stone();
    void update_anvil(float delta);
    void update_eruptions(float delta);
    void cast_chain();
    void summon_hydra();
    void cast_hole();
    void cast_blizzard();
    // Whose Ur Mommy?: true if the copy appeared (there was ground for it to stand on).
    bool start_clone();
    void update_clone(float delta);
    void end_clone();
    // The archer's Bear Trap: one put on the ground before her (shut or open), the one she has
    // pulled open laid armed, and the watch for what steps into those that lie.
    godot::Node3D* place_trap(bool open);
    void lay_trap();
    void update_traps();
    void update_blizzard(float delta);
    void update_hole(float delta);
    // The new wizard's Teleportation: true if he went (there was ground to stand on).
    bool start_teleport();
    // The ground under `point`, if there is any within reach below it.
    [[nodiscard]] std::optional<godot::Vector3> ground_at(const godot::Vector3& point) const;
    void update_hydra(float delta);
    // The ground under `point` (what is neither a hero nor an enemy), or `point` itself.
    [[nodiscard]] godot::Vector3 ground_under(const godot::Vector3& point) const;
    void update_chain(float delta);
    // The living enemy nearest to `place`, within `reach` of it (its body's width allowed for),
    // that the chain under way has not struck yet.
    [[nodiscard]] E5Enemy* chain_next(const godot::Vector3& place, float reach) const;
    // The glow in his hand for a spell: the cast effect's variant for the spell's element
    // (<cast effect>_fire.tscn, _frost, _lightning, _star, _void), or the cast effect itself.
    [[nodiscard]] godot::Ref<godot::PackedScene> cast_glow(gameplay::SkillId spell) const;
    // Where a spell leaves him: the palm of the right hand, or the middle between both.
    [[nodiscard]] godot::Vector3 casting_point(bool both_hands) const;
    void finish_prewarm(float delta);
    // Applies the damage of this step; returns true while she is dead (and nothing else happens).
    bool update_vitals(float delta);
    // Health rules with the worn charms counted in.
    [[nodiscard]] gameplay::VitalsParams effective_vitals() const;
    // How she moves right now: worn charms, and the slow walk while aiming.
    [[nodiscard]] gameplay::MotorParams motor_params() const;
    // The aim button, unless a menu has the input.
    [[nodiscard]] bool aim_held() const;
    [[nodiscard]] bool attack_held() const;
    // Which mouse button the skill in use belongs to.
    enum class UseButton : std::uint8_t { None, Standard, Selected };
    void set_use_button(UseButton next);
    void update_nocked_arrow(float string_draw);
    void update_aim_camera(float delta);
    E5Arrow* spawn_arrow(const godot::Vector3& position, const godot::Vector3& direction);
    void fire_arrow(float power);
    void fire_rain();
    void fire_fan();
    void fire_blast_arrow();
    void fire_special_arrow(gameplay::SkillId skill);
    void strike_kick();
    void release_birds();
    void update_summon(float delta);
    void launch_bird(E5Bird* bird, int index);
    // The clip an instant skill plays; nullptr if the skill has none or the library lacks it.
    [[nodiscard]] const godot::StringName* instant_clip(gameplay::SkillId skill) const;
    [[nodiscard]] bool can_start_instant_skill(gameplay::SkillId skill) const;
    void start_instant_skill(gameplay::SkillId skill);
    [[nodiscard]] godot::Vector3 raised_hand_position() const;
    void use_skill(float power);
    void update_rain_marker();
    struct AimPoint {
        godot::Vector3 position;
        bool hit = false; // false: nothing under the crosshair, position is far along the view
    };
    [[nodiscard]] AimPoint find_aim_point(const godot::Vector3& fallback_origin) const;
    void update_charge_effect(float charge, float delta);
    void prewarm_effects();
    void setup_charge_effect();
    [[nodiscard]] bool is_aiming() const { return bow_.phase != gameplay::BowPhase::Lowered; }
    // 0 lowered, 1 drawing, 2 held at full draw, 3 just let go: for what is heard (game/characters/bow_sound.gd).
    [[nodiscard]] int get_bow_phase() const { return static_cast<int>(bow_.phase); }
    // In the middle of using a skill: the selection must not change now.
    [[nodiscard]] bool is_busy() const {
        return is_aiming() || action_.active || block_.raised || dodge_left_ > 0.0F || is_whirling() || is_leaping() ||
               enrage_driving_ || tower_charging_ || firebolt_charging_ || is_cutting() || manning_ ||
               dash_.left > 0.0F;
    }

    gameplay::MotorParams params_;
    gameplay::LookAngles look_;
    float model_yaw_ = 0.0F;
    float turn_speed_ = 12.0F; // rad/s: a half turn in about a quarter second
    float mouse_sensitivity_ = 0.0022F;
    bool quick_cast_ = false;
    int quick_key_slot_ = -1; // the number key that is held as the right button, or -1
    bool capture_mouse_on_ready_ = true;
    bool remote_ = false;
    bool has_net_state_ = false;
    godot::Vector3 net_position_;
    float net_facing_ = 0.0F;
    godot::StringName net_clip_;
    float net_speed_ = 1.0F;
    godot::Array net_events_;
    bool pending_start_ = false;
    int pending_combo_step_ = -1;
    float outgoing_damage_ = 0.0F;
    // What a skill of this hero does: nothing if she is remote (see apply_net_event).
    // What is shot from the bow is raised by the bow in hand, and now and then strikes true.
    [[nodiscard]] float dealt(gameplay::SkillId skill, float power = 0.0F) const;
    // The bow's timings with the bow in hand: a quicker bow is drawn sooner.
    [[nodiscard]] gameplay::BowTimings bow_timings() const;
    void note_net_event(int kind, float power);
    void update_remote(float delta);
    godot::Ref<godot::AnimationLibrary> animation_library_;
    godot::Ref<godot::PackedScene> charge_effect_;
    godot::Ref<godot::PackedScene> charge_full_effect_;
    godot::Ref<godot::PackedScene> trail_effect_;
    godot::Ref<godot::PackedScene> impact_effect_;
    godot::Ref<godot::PackedScene> rain_marker_effect_;
    godot::Ref<godot::PackedScene> rain_impact_effect_;
    godot::Ref<godot::PackedScene> frost_trail_effect_;
    godot::Ref<godot::PackedScene> frost_impact_effect_;
    godot::Ref<godot::PackedScene> fire_trail_effect_;
    godot::Ref<godot::PackedScene> fire_impact_effect_;
    godot::Ref<godot::PackedScene> kick_effect_;
    godot::Ref<godot::PackedScene> bird_scene_;
    godot::Ref<godot::PackedScene> summon_cast_effect_;
    godot::Ref<godot::PackedScene> summon_burst_effect_;
    godot::Ref<godot::PackedScene> cast_effect_;
    godot::Ref<godot::PackedScene> bolt_trail_effect_;
    godot::Ref<godot::PackedScene> bolt_impact_effect_;
    godot::Ref<godot::PackedScene> nova_effect_;
    godot::Ref<godot::PackedScene> lightning_effect_;
    godot::Ref<godot::PackedScene> meteor_marker_effect_;
    godot::Ref<godot::PackedScene> meteor_impact_effect_;
    godot::Ref<godot::PackedScene> star_trail_effect_;
    godot::Ref<godot::PackedScene> star_impact_effect_;
    godot::Ref<godot::PackedScene> black_hole_effect_;
    godot::Ref<godot::PackedScene> black_hole_burst_effect_;
    godot::Ref<godot::PackedScene> weapon_effect_1_;
    godot::Ref<godot::PackedScene> weapon_effect_2_;
    godot::Ref<godot::PackedScene> weapon_effect_3_;
    godot::Ref<godot::PackedScene> weapon_effect_4_;
    // The sword combo: the blow last struck (-1 = none) and the time since it ended.
    int combo_step_ = -1;
    float combo_idle_seconds_ = 0.0F;
    godot::Vector3 combo_push_; // the velocity the combo's step added in the last frame
    // The shield block: only a hero whose library has the block clip can.
    bool block_enabled_ = false;
    bool block_key_was_down_ = false;
    godot::StringName clip_dodge_;
    godot::StringName clip_emote_;
    float emote_left_ = 0.0F; // seconds of the dance still to come
    bool emote_key_was_down_ = false;
    godot::StringName clip_ride_;
    bool mounted_ = false;
    float mount_speed_ = 7.5F;         // m/s on the horse
    float mount_sprint_speed_ = 11.0F; // with the sprint key
    float mount_walk_speed_ = 1.3F;    // with the walk key: the pace the horse's walk is made for
    godot::StringName clip_dodge_alt_; // a second dodge to compare with the first, on its own key
    godot::StringName dodge_clip_;     // the one under way
    bool dodge_enabled_ = false;
    bool dodge_key_was_down_ = false;
    float dodge_distance_ = 3.4F;
    float dodge_seconds_ = 1.0F;       // the clip's length
    float dodge_left_ = 0.0F;          // seconds of the dodge under way
    float dodge_cooldown_left_ = 0.0F; // seconds until the next one
    godot::Vector3 dodge_direction_;
    gameplay::BlockParams block_params_;
    gameplay::BlockState block_;
    int hits_blocked_ = 0;
    float counter_left_ = 0.0F;       // seconds she still stands ready to answer a blow
    float counter_swing_left_ = 0.0F; // seconds of the answering blow her arm still shows
    int counters_struck_ = 0;
    struct Bleeding {
        std::uint64_t enemy = 0; // an id: it may be gone
        gameplay::BleedState wound;
    };
    std::vector<Bleeding> bleeding_;
    gameplay::BleedState whirl_; // the Whirlwind under way: the time it still has and until its next tick
    godot::StringName clip_whirl_;
    struct Leap {
        bool flying = false;
        bool striking = false;     // the clip of the blow has begun: she is coming down
        float elapsed = 0.0F;      // seconds in the air
        float rise = 0.0F;         // m/s upwards
        float recover_left = 0.0F; // seconds on the ground after the blow
        gameplay::LeapArc arc;
        godot::Vector3 direction; // level, towards where she lands
    };
    Leap leap_;
    float remote_pieces_left_ = 0.0F;      // another player's hero: she leaps from enemy to enemy for so long yet
    float remote_leap_left_ = 0.0F;        // another player's hero: seconds until her leap lands here
    bool leap_aiming_ = false;             // the Jump Attack's button is held: its mark shows
    godot::Node3D* leap_marker_ = nullptr; // non-owning child, made when first needed
    bool tower_charging_ = false;          // the Vine Tower: she kneels and charges
    int towers_grown_ = 0;
    float tower_charge_ = 0.0F;             // seconds charged
    std::uint64_t tower_charge_effect_ = 0; // what shows round her meanwhile: an id, it may be gone
    godot::StringName clip_kneel_;
    std::uint64_t turret_id_ = 0;     // her Gatling Arrow Gun, built or being built: an id, it may be gone
    bool manning_ = false;            // she stands at it
    float gatling_until_next_ = 0.0F; // seconds until its next arrow is due
    int gatling_arrows_ = 0;
    bool disguised_ = false;     // her Disguise: she is a bush
    float disguise_left_ = 0.0F; // seconds
    Bush disguise_bush_;         // the one she took: put back when it ends
    godot::Ref<godot::Mesh> disguise_mesh_;
    godot::Ref<godot::Material> disguise_material_;
    godot::Basis disguise_basis_;
    int disguises_ = 0;
    gameplay::BleedState stampede_; // her Sprintsz under way: the time it still has and until its next pounding
    int stampede_ticks_ = 0;
    gameplay::BleedState pieces_; // her Cut in Pieces under way: the time it still has and until its next blow
    int pieces_blows_ = 0;
    float pieces_spared_ = 0.0F;      // damage that did not reach her while she cut
    bool firebolt_charging_ = false;  // the new wizard's Firebolt: fire gathers at his wand while the button is held
    bool firebolt_release_ = false;   // let go with enough gathered: the throw begins in this step
    float firebolt_charge_ = 0.0F;    // seconds gathered
    float firebolt_power_ = 0.0F;     // 0..1: how far the fireball he is throwing was charged
    std::uint64_t firebolt_glow_ = 0; // the fire at the wand's tip meanwhile: an id, it may be gone
    int firebolts_thrown_ = 0;
    // The new wizard's Chain Lightning under way: where it struck last, how many leaps it has
    // made, when the next is due, and whom it has struck (ids: they may be gone).
    struct Chain {
        godot::Vector3 at;
        int leaps = 0;
        float until_next = 0.0F;
        bool active = false;
        std::vector<std::uint64_t> struck;
    };
    Chain chain_;
    std::uint64_t hydra_id_ = 0;    // the new wizard's Poison Hydra that stands: an id, it may be gone
    float hydra_left_ = 0.0F;       // seconds it still stands
    float hydra_until_spit_ = 0.0F; // until it spits again
    int hydras_ = 0;
    // The Archer's Spirit Wolf while it runs: its look (an id: it may be gone), where it is, which way
    // it runs, how long more, and whom it has bitten (each once).
    struct WolfRun {
        std::uint64_t look = 0;
        godot::Vector3 at;
        godot::Vector3 way;
        float left = 0.0F;
        std::vector<std::uint64_t> bitten;
    };
    WolfRun wolf_;
    int wolf_bites_ = 0;
    // Her Owl Sentinel that stands (an id: it may be gone), how long more, until it shoots again.
    std::uint64_t owl_id_ = 0;
    float owl_left_ = 0.0F;
    float owl_until_shot_ = 0.0F;
    int owl_shots_ = 0;
    // Her Evasive Shot: how long she still springs and which way, the arrows loosed and until the
    // next, and the hop off the ground still to be given.
    struct Spring {
        float left = 0.0F;
        godot::Vector3 way;
        int arrows = gameplay::evasive_arrows; // (all loosed: nothing under way)
        float until_arrow = 0.0F;
        bool lift = false;
        bool moving = false; // it carried her in the last step
    };
    Spring spring_;
    int springs_ = 0;
    // The new Wizard's Arcane Orb in flight: its look (an id: it may be gone), where it is, which way
    // it flies, how long more, and whom it has passed through (each is hurt once).
    struct Orb {
        std::uint64_t look = 0;
        godot::Vector3 at;
        godot::Vector3 way;
        float left = 0.0F;
        std::vector<std::uint64_t> touched;
    };
    Orb orb_;
    int orb_touches_ = 0;
    // His Gravity Well while it draws: its look, where its heart is, how long more, how long it has been.
    struct Well {
        std::uint64_t look = 0;
        godot::Vector3 at;
        float left = 0.0F;
        float age = 0.0F;
    };
    Well well_;
    int well_caught_ = 0;
    // His Lightning Pylon that stands (an id: it may be gone), how long more, until it strikes again.
    std::uint64_t pylon_id_ = 0;
    float pylon_left_ = 0.0F;
    float pylon_until_zap_ = 0.0F;
    int pylon_zaps_ = 0;
    int prisons_ = 0;
    // His Arcane Missiles still to leave the wand: how many, until the next, onto whom (an id: it may
    // be gone), and the place aimed at if nobody was there.
    struct Missiles {
        int left = 0;
        float until_next = 0.0F;
        std::uint64_t target = 0;
        godot::Vector3 aimed;
        int sent = 0;
    };
    Missiles missiles_;
    int missiles_sent_ = 0;
    // The new Warrior's Charge while it carries her: how long more, which way, whom it struck (ids:
    // they may be gone; they pass through her until it ends), whether it carried her in the last step.
    struct Dash {
        float left = 0.0F;
        godot::Vector3 way;
        std::vector<std::uint64_t> struck;
        bool moving = false;
    };
    Dash dash_;
    int charge_struck_ = 0;
    // Her Rain of Blades: the swords still to fall, each with where it lands, until it appears and until
    // it lands, and which of her swords it shows.
    struct FallingBlade {
        godot::Vector3 at;
        float until_shown = 0.0F;
        float until_landed = 0.0F;
        int model = 0;
        bool shown = false;
    };
    std::vector<FallingBlade> blade_rain_;
    int blade_rain_hits_ = 0;
    int executions_ = 0;
    // Her War Banner that stands (an id: it may be gone) and how long more.
    std::uint64_t banner_id_ = 0;
    float banner_left_ = 0.0F;
    float banner_healed_ = 0.0F;
    // Her thrown sword: its look (an id), where it is, which way it flew out, how long more out, whether
    // it is coming back, how long it has flown, whom it struck going out and coming back.
    struct ThrownSword {
        std::uint64_t look = 0;
        gameplay::SkillId skill = gameplay::SkillId::SwordThrow; // or the new Dwarf's ThrowingAxe
        godot::Vector3 at;
        godot::Vector3 way;
        float out_left = 0.0F;
        bool back = false;
        float age = 0.0F;
        std::vector<std::uint64_t> struck_out;
        std::vector<std::uint64_t> struck_back;
    };
    ThrownSword thrown_;
    int throw_hits_ = 0;
    // The new Dwarf's Tremor: its rings still to run out, each with how long until it does, where from
    // and which it is (0 the first, the narrowest).
    struct TremorRing {
        float in_seconds = 0.0F;
        godot::Vector3 centre;
        int ring = 0;
    };
    std::vector<TremorRing> tremor_;
    int tremor_hits_ = 0;
    // His Dwarven Cannon that stands (an id: it may be gone), how long more, until it fires again.
    std::uint64_t cannon_id_ = 0;
    float cannon_left_ = 0.0F;
    float cannon_until_shot_ = 0.0F;
    int cannon_shots_ = 0;
    // His Stone Brew: how long more, what it kept from him, what it gave back.
    float stone_brew_left_ = 0.0F;
    float stone_brew_spared_ = 0.0F;
    float stone_brew_healed_ = 0.0F;
    // His Powder Keg that lies (an id: it may be gone) and until its fuse has burnt down.
    std::uint64_t keg_id_ = 0;
    float keg_left_ = 0.0F;
    int keg_caught_ = 0;
    int teleports_ = 0;
    // The new wizard's Black Whole that drifts: what is seen of it (an id: it may be gone), where
    // it is and which way it goes, how long it still lasts and until it hurts again.
    struct Hole {
        std::uint64_t look = 0;
        godot::Vector3 at;
        godot::Vector3 way;
        float left = 0.0F;
        float until_tick = 0.0F;
    };
    Hole hole_;
    // The new wizard's Blizzard that rages: where, how long yet, until it hurts again, who stands
    // in it and for how long, and whom it has held already (ids: they may be gone).
    struct Blizzard {
        struct Inside {
            std::uint64_t enemy = 0;
            float seconds = 0.0F;
        };
        godot::Vector3 at;
        float left = 0.0F;
        float until_tick = 0.0F;
        std::vector<Inside> inside;
        std::vector<std::uint64_t> held;
    };
    Blizzard blizzard_;
    // Whose Ur Mommy?. For the copy itself: that it is one, what its spells do of his, and the
    // life it has left. For him: his copy (an id: it may be gone), where it stands, how long it
    // still lasts, until it casts again, and how many spells it has cast.
    bool clone_ = false;
    float clone_share_ = 0.0F;
    std::uint64_t clone_id_ = 0;
    godot::Vector3 clone_place_;
    float clone_facing_ = 0.0F;
    float clone_yaw_ = 0.0F;
    float clone_pitch_ = 0.0F;
    float clone_left_ = 0.0F;
    float clone_until_cast_ = 0.0F;
    int clone_cast_ = 0;
    int clones_ = 0;
    int clone_casts_ = 0;
    float clone_struck_ = 0.0F;        // what struck his copies, as it counted against them
    std::uint64_t trap_setting_ = 0;   // the bear trap she is pulling open: an id, it may be gone
    std::vector<std::uint64_t> traps_; // those that lie armed, the oldest first (ids)
    int traps_set_ = 0;
    int traps_sprung_ = 0;
    int blizzard_ticks_ = 0;
    int blizzard_held_ = 0;
    int hole_ticks_ = 0;
    int hole_pulls_ = 0;             // steps in which it drew an enemy along
    float teleported_metres_ = 0.0F; // over the ground, all of them together
    float remote_blink_left_ = 0.0F; // another player's hero: until the light where he arrives is shown
    int hydra_spits_ = 0;
    int chain_strikes_ = 0;
    float armor_left_ = 0.0F;       // seconds the new wizard's Frost Armor still lasts
    float armor_barrier_ = 0.0F;    // what its barrier can still take
    float armor_tick_ = 0.0F;       // until those near him are slowed anew
    float armor_absorbed_ = 0.0F;   // damage it has taken in his place
    int armor_slowed_ = 0;          // how often it slowed an enemy
    float bulwark_left_ = 0.0F;     // seconds his Runic Bulwark still lasts
    float veil_left_ = 0.0F;        // seconds the Shadowblade's Veil of Shadows still lasts
    std::uint64_t veil_effect_ = 0; // its wisps, taken away when a blow ends it early
    float venom_left_ = 0.0F;       // seconds his Venom Edge still lasts
    std::uint64_t step_target_ = 0; // whom his Shadow Step stabs
    float remote_step_left_ = 0.0F; // another player's Shadowblade: his step's jump arrives within this
    gameplay::SkillId pieces_skill_ = gameplay::SkillId::CutInPieces; // or his Shadow Flurry
    std::vector<std::pair<std::uint64_t, std::uint64_t>> venom_on_;   // an enemy, and the poison his blades put on it
    int shadow_steps_ = 0;
    int knives_thrown_ = 0;
    int ambushes_ = 0;
    int poisonings_ = 0;
    bool armor_slows_ = true;       // Frost Armor slows what is near; Bone Armor, the same barrier, does not
    float healed_by_skills_ = 0.0F; // the Bonecaller's drain and requiem: for the report
    int cursed_count_ = 0;
    float bulwark_spared_ = 0.0F; // damage it has kept from him
    // His Ancestral Anvil: where it stands and how long more; the pulses are struck from here, the
    // healing is asked of every anvil in the world by each machine for its own hero.
    godot::Vector3 anvil_at_;
    float anvil_left_ = 0.0F;
    float anvil_until_pulse_ = 0.0F;
    int anvil_pulses_ = 0;
    float anvil_heal_tick_ = 0.0F; // until this hero is healed by the anvils near her again
    float anvil_healed_ = 0.0F;
    float resolve_left_ = 0.0F;   // seconds her Never Give Up still lasts
    float resolve_spared_ = 0.0F; // damage it has kept from her
    float dome_spared_ = 0.0F;    // damage a Dome of Protection has kept from her
    float resolve_healed_ = 0.0F; // health it has given back
    float enrage_left_ = 0.0F;    // seconds her frenzy still lasts
    bool enrage_driving_ = false; // it moved her in this step: the player did not
    int enrage_blow_ = -1;        // the blow of the combo under way in it, or -1: she runs
    int enrage_next_blow_ = 0;    // the one that comes after
    float enrage_blow_elapsed_ = 0.0F;
    bool enrage_struck_ = false; // the blow under way has landed
    bool enrage_arc_shown_ = false;
    int enrage_blows_ = 0;
    // Bursts of the Seismic Slash still to come: the ground breaks open further and further out.
    struct Eruption {
        bool molten = false; // the Molten Fissure's: lava, not stone
        bool bone = false;   // the Bonecaller's Bone Spikes: bone, not stone
        float in_seconds = 0.0F;
        godot::Vector3 place;
        float size = 1.0F;
    };
    std::vector<Eruption> eruptions_;
    godot::StringName clip_block_;
    godot::Ref<godot::PackedScene> block_spark_;
    // Seconds until each skill can be used again, by its number; and the charge, 0 to 1.
    std::array<float, gameplay::skill_id_limit> cooldown_left_{};
    float charge_ = 0.0F;
    bool charge_always_full_ = false;
    bool cooldowns_enabled_ = true;
    [[nodiscard]] bool skill_ready(gameplay::SkillId skill) const;
    void spend(gameplay::SkillId skill);
    bool combo_queued_ = false;
    bool combo_arc_shown_ = false; // this blow's arc is in the air already
    godot::Ref<godot::PackedScene> slash_arc_;
    godot::Ref<godot::PackedScene> landing_dust_; // the button was pressed during a blow: the next one follows it
    // The node the sword hangs in ("WeaponHolder"), if the character has one. Non-owning.
    godot::Node3D* weapon_holder_ = nullptr;
    // The weapon effects' impact variants, in the order of weapon_effect_1..4; unset where there is none.
    std::array<godot::Ref<godot::PackedScene>, 4> weapon_impacts_;
    gameplay::Vitals vitals_;
    gameplay::VitalsParams vitals_params_;
    float pending_damage_ = 0.0F;
    float damage_taken_ = 0.0F;
    int death_count_ = 0;
    godot::Transform3D spawn_transform_;
    E5HealthHud* health_hud_ = nullptr; // non-owning child
    E5Inventory* inventory_ = nullptr;  // non-owning child
    bool input_blocked_ = false;
    godot::Node* interface_ = nullptr; // non-owning child
    godot::StringName action_use_potion_;
    godot::StringName clip_death_;
    float shake_ = 0.0F; // metres the camera is thrown about; fades out
    float shake_time_ = 0.0F;
    // The cast glow's variants, in the order of cast_glow_names (player_controller.cpp); unset where there is none.
    std::array<godot::Ref<godot::PackedScene>, 5> cast_glows_;
    int skill_set_ = 0;
    bool spells_enabled_ = false;
    // How fast the clip of the instant skill in progress is played.
    float action_playback_scale_ = 1.0F;
    float aim_move_speed_ = 1.6F;     // m/s while the bow is raised
    float arrow_speed_ = 70.0F;       // m/s at release
    float aim_camera_blend_ = 0.0F;   // 0 = normal camera, 1 = over the shoulder
    float mount_camera_blend_ = 0.0F; // 0 = on foot, 1 = the camera where it stands for a rider
    float camera_rest_distance_ = 0.0F;
    bool archery_enabled_ = false;
    gameplay::BowState bow_;
    gameplay::BowTimings bow_timings_;
    gameplay::SkillBar skills_;
    // After a cancel the aim button must be let go before it draws again.
    bool aim_blocked_ = false;
    bool aim_was_pressed_ = false;
    // Left mouse uses the first slot, right mouse the selected one. While the left one is in
    // use the bar is switched to the first slot underneath (the HUD keeps showing the
    // player's selection) and switched back afterwards.
    UseButton use_button_ = UseButton::None;
    gameplay::SkillId last_skill_ = gameplay::SkillId::None;
    int skills_used_ = 0;
    std::size_t selected_slot_ = 0;
    bool other_button_was_pressed_ = false;
    // The instant skill in progress (the kick), if any.
    gameplay::ActionState action_;
    gameplay::ActionTimings action_timings_;
    gameplay::SkillId action_skill_ = gameplay::SkillId::None;
    // The kingfisher sitting on her raised hand during the summon (an id: it may be gone).
    std::uint64_t perched_bird_id_ = 0;
    // Birds of the flock that have not come out of her hand yet, and the time until the next one.
    int birds_to_release_ = 0;
    float next_bird_seconds_ = 0.0F;
    godot::Vector3 rain_target_;
    bool rain_target_valid_ = false;

    // Non-owning: these are child nodes owned by the scene tree and outlive
    // every callback that uses the pointers.
    godot::Node3D* camera_pivot_ = nullptr;
    godot::Node3D* model_ = nullptr;
    CharacterAnimator animator_;
    E5AimOffset* aim_offset_ = nullptr;
    godot::BoneAttachment3D* right_hand_ = nullptr;
    godot::BoneAttachment3D* left_hand_ = nullptr;
    E5BowString* bow_string_ = nullptr;
    E5Arrow* nocked_arrow_ = nullptr;
    E5SkillBarHud* skill_hud_ = nullptr;
    // After the bar was rearranged: the plain bar's names, and word of it to the interface.
    void skill_bar_changed();
    godot::Node3D* rain_marker_ = nullptr;
    // Glow on the tip of the arrow on the string, shown while its skill is selected.
    struct TipGlow {
        gameplay::SkillId skill;
        godot::Node3D* node;
    };
    std::vector<TipGlow> tip_glows_;
    godot::Node3D* charge_core_ = nullptr;
    godot::Node3D* charge_ground_ring_ = nullptr;
    std::vector<godot::Node3D*> charge_visuals_; // meshes and lights: shown only while charging
    float charge_time_ = 0.0F;
    bool charge_was_full_ = false;
    godot::OmniLight3D* charge_light_ = nullptr;
    float charge_light_energy_ = 0.0F;
    std::vector<godot::GPUParticles3D*> charge_particles_;
    // Effects shown once at start, then removed. Kept as ids, not pointers: one of
    // them frees itself and must not be touched afterwards.
    std::vector<std::uint64_t> prewarm_ids_;
    float prewarm_seconds_left_ = 0.0F;
    godot::SpringArm3D* camera_arm_ = nullptr;
    godot::Camera3D* camera_ = nullptr;
    godot::CanvasLayer* crosshair_ = nullptr;

    // Cached so per-frame queries do not rebuild StringNames.
    godot::StringName action_forward_;
    godot::StringName action_back_;
    godot::StringName action_left_;
    godot::StringName action_right_;
    godot::StringName action_jump_;
    godot::StringName action_sprint_;
    godot::StringName action_aim_;
    godot::StringName action_attack_;
    std::array<godot::StringName, 12> skill_actions_; // actions::skill_slot_count
    godot::StringName clip_idle_;
    godot::StringName clip_walk_;
    godot::StringName clip_run_;
    godot::StringName clip_jump_;
    godot::StringName clip_bow_draw_;
    godot::StringName clip_bow_aim_;
    godot::StringName clip_bow_recoil_;
    godot::StringName clip_bow_walk_forward_;
    godot::StringName clip_bow_walk_back_;
    godot::StringName clip_bow_walk_left_;
    godot::StringName clip_bow_walk_right_;
    godot::StringName clip_kick_;
    godot::StringName clip_summon_;
    godot::StringName clip_bolt_;
    godot::StringName clip_fireball_;
    godot::StringName clip_nova_;
    godot::StringName clip_lightning_;
    godot::StringName clip_meteor_;
    godot::StringName clip_barrage_;
    godot::StringName clip_black_hole_;
    std::array<godot::StringName, 3> clip_combo_;
    std::array<godot::StringName, 3> clip_axe_combo_;
    std::array<godot::StringName, 3> clip_dagger_combo_;
    // The dagger is used from its key and the bar goes back to what was chosen before.
    int dagger_return_slot_ = -1;
    bool dagger_return_pending_ = false;
    godot::StringName clip_whirlwind_;
    godot::StringName clip_earthbreaker_;
    godot::StringName clip_leap_;
    godot::StringName clip_battlecry_;
    godot::StringName clip_fissure_;
    godot::StringName clip_anvil_;
    godot::StringName clip_stone_;
    godot::StringName clip_keg_; // the new Dwarf's kick: his Powder Keg goes off his foot
    godot::StringName clip_flame_;
    godot::StringName clip_frost_;
    godot::StringName clip_thunder_;
    godot::StringName clip_star_;
};

} // namespace e5::bridge

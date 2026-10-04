#pragma once

#include "character_animator.hpp"
#include "e5/gameplay/bow_state.hpp"
#include "e5/gameplay/character_motor.hpp"
#include "e5/gameplay/skills.hpp"
#include "e5/gameplay/vitals.hpp"

#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <array>
#include <cstdint>
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
    void apply_net_event(int kind, int slot, float power, int combo_step, float look_yaw, float look_pitch);
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
    // In the middle of using a skill: the selection must not change now.
    [[nodiscard]] bool is_busy() const { return is_aiming() || action_.active || block_.raised; }

    gameplay::MotorParams params_;
    gameplay::LookAngles look_;
    float model_yaw_ = 0.0F;
    float turn_speed_ = 12.0F; // rad/s: a half turn in about a quarter second
    float mouse_sensitivity_ = 0.0022F;
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
    [[nodiscard]] float dealt(gameplay::SkillId skill, float power = 0.0F) const {
        return remote_ ? 0.0F : gameplay::skill_damage(skill, power);
    }
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
    gameplay::BlockParams block_params_;
    gameplay::BlockState block_;
    int hits_blocked_ = 0;
    godot::StringName clip_block_;
    godot::Ref<godot::PackedScene> block_spark_;
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
    float aim_move_speed_ = 1.6F;   // m/s while the bow is raised
    float arrow_speed_ = 70.0F;     // m/s at release
    float aim_camera_blend_ = 0.0F; // 0 = normal camera, 1 = over the shoulder
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
    std::array<godot::StringName, 10> skill_actions_;
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
    godot::StringName clip_whirlwind_;
    godot::StringName clip_earthbreaker_;
    godot::StringName clip_leap_;
    godot::StringName clip_battlecry_;
    godot::StringName clip_flame_;
    godot::StringName clip_frost_;
    godot::StringName clip_thunder_;
    godot::StringName clip_star_;
};

} // namespace e5::bridge

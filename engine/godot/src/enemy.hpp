#pragma once

#include "character_animator.hpp"
#include "e5/gameplay/enemy.hpp"

#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace godot {
class Label3D;
class MeshInstance3D;
class Node3D;
} // namespace godot

namespace e5::bridge {

// A simple melee enemy: the root of an enemy scene (see game/creatures). Its
// behaviour is the state machine in e5::gameplay (enemy.hpp); this class moves
// the body, plays the matching clip, shows the health bar and damage numbers,
// and takes damage from every skill through combat.hpp.
//
// Expected children:
//   Model (Node3D)   the visual, front along +Z, with a humanoid skeleton
//   a CollisionShape3D
//
// `animation_library` needs clips named idle, walk, attack, hit and death.
// When it dies it lies for a moment and, if `respawn_seconds` is above zero,
// comes back at the place where it started.
class E5Enemy : public godot::CharacterBody3D {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5Enemy, godot::CharacterBody3D)

public:
    // Every enemy is in this group.
    static constexpr const char* group_name = "e5_enemy";

    void _ready() override;
    void _process(double delta) override;
    void _physics_process(double delta) override;

    // `position` is where the blow landed (for the damage number and the effect).
    void take_damage(float amount, const godot::Vector3& position);

    // Held by something else (a black hole): it stops thinking and moving on its own, nothing
    // collides with it, and the holder places it. Letting go restores all that.
    void set_held(bool held);
    [[nodiscard]] bool is_held() const { return held_; }
    // How large the visible body is (1 = as made); for being drawn into something.
    void set_shrink(float scale);
    // Turns the visible body about the vertical axis.
    void spin(float radians);
    // Holds it where it stands for so many seconds (brambles): it cannot walk or fly on, but
    // it still strikes and casts at what is in its reach, and can be hit.
    void root(float seconds) {
        rooted_left_ = std::max(rooted_left_, seconds);
        note_effect(effect_root, seconds, 0.0F);
    }
    [[nodiscard]] bool is_rooted() const { return rooted_left_ > 0.0F; }
    // Drawn along by something (a whirlwind) for the next step, on top of where it goes by
    // itself: to be said again every step. Not while it is rooted; only those that walk.
    void drag(const godot::Vector3& velocity) {
        dragged_ = velocity;
        if (remote_) {
            outgoing_drag_ = velocity; // the latest: told to the deciding machine a few times a second
            has_outgoing_drag_ = true;
        }
    }
    // Walks it about while it has noticed nobody (a giant on his rounds; a script says where to,
    // every step: game/creatures/warden_attacks.gd): it goes that way at that pace, looks where it
    // goes and plays its walk. One that strolls never sleeps: its rounds go on with nobody near.
    void stroll(const godot::Vector3& velocity) {
        strolling_ = velocity;
        strolls_ = true;
    }
    // Takes `share` of its pace away for so many seconds (the blow of a jump attack). A new
    // slowing replaces the old one if it is at least as strong.
    void slow(float share, float seconds);
    // Stuns it for so many seconds: it stands, neither walks nor strikes nor casts, and sees
    // nobody. It can be hit meanwhile, and goes for the hero again when it is over.
    void stun(float seconds) {
        stunned_left_ = std::max(stunned_left_, seconds);
        note_effect(effect_stun, seconds, 0.0F);
    }
    [[nodiscard]] bool is_stunned() const { return stunned_left_ > 0.0F; }
    // Cursed (the Bonecaller's Curse of Frailty): every blow struck on THIS machine hurts it `factor`
    // times as much for so many seconds, which is his alone: a shown hero's blows hurt nothing, and
    // other players' are counted on their own machines. Not told to the machine that decides the
    // enemy (no skill helps another player). A new curse replaces the old one if it is as strong.
    void curse(float factor, float seconds) {
        if (factor >= curse_factor_ || cursed_left_ <= 0.0F) {
            curse_factor_ = factor;
        }
        cursed_left_ = std::max(cursed_left_, seconds);
    }
    [[nodiscard]] bool is_cursed() const { return cursed_left_ > 0.0F; }
    // Throws it: it flies under gravity until it lands, then carries on as before.
    void fling(const godot::Vector3& velocity);

    // What skills did to this enemy besides hurting it, on a machine that only shows it: kept
    // until the network script takes it and tells the machine that decides the enemy
    // (game/net/net.gd). Each is [what, a, b]; the numbers are those of `apply_effect`.
    static constexpr int effect_stun = 1;     // a: seconds
    static constexpr int effect_root = 2;     // a: seconds
    static constexpr int effect_slow = 3;     // a: share of its pace, b: seconds
    static constexpr int effect_drag = 4;     // a, b: metres a second along x and z
    static constexpr int effect_fling = 5;    // a, b: metres a second along x and z ...
    static constexpr int effect_fling_up = 6; // ... a: and upwards (follows a fling at once)
    [[nodiscard]] godot::Array take_outgoing_effects();
    // Its loot is rolled where it is decided; the number it was rolled from is kept until the
    // network script takes it (0: nothing dropped since) and tells the others, who drop the
    // same there (`drop_loot_seeded`). Every player picks up their own copy.
    [[nodiscard]] int take_dropped_seed();
    void drop_loot_seeded(int seed);
    // The same, arriving where the enemy is decided.
    void apply_effect(int what, float a, float b);

    // A boss's great attacks (EnemyPhase::Special): with a `special_range` it does one of
    // `special_count` in turn, plays the clip `special_<n>` for as long as that clip is, and
    // says so with the signal `special_started(index)` (0-based), on every machine. What the
    // attack does is a script's business (game/creatures/boss_attacks.gd).
    void set_special_range(float metres) { params_.special_range = metres; }
    [[nodiscard]] float get_special_range() const { return params_.special_range; }
    void set_special_cooldown(float seconds) { params_.special_cooldown_seconds = seconds; }
    [[nodiscard]] float get_special_cooldown() const { return params_.special_cooldown_seconds; }
    void set_special_count(int count) { special_count_ = std::max(count, 1); }
    [[nodiscard]] int get_special_count() const { return special_count_; }
    [[nodiscard]] int get_phase() const { return static_cast<int>(state_.phase); }
    [[nodiscard]] float get_phase_seconds() const { return state_.phase_seconds; }
    [[nodiscard]] float get_facing() const { return model_yaw_; }

    // Kept in reserve (a boss's helpers, which he throws): `starts_parked` makes it begin
    // out of the world, unseen and untouchable, like one that died long ago and does not come
    // back. `revive` puts it at a place, alive and after the heroes; when it dies it lies a
    // moment and is out of the world again. It exists on every machine from the start, so it
    // is the same enemy everywhere like any other.
    void set_starts_parked(bool parked) { starts_parked_ = parked; }
    [[nodiscard]] bool get_starts_parked() const { return starts_parked_; }
    void park();
    void revive(const godot::Vector3& position);
    [[nodiscard]] bool is_parked() const { return parked_; }
    // Whether it is set onto the island's ground where it stands when the game starts (the scene
    // then only says where, not how high). Off for one that stands somewhere else: in a cave
    // under the island, where the ground above its head is not its floor.
    void set_on_terrain(bool on) { on_terrain_ = on; }
    [[nodiscard]] bool get_on_terrain() const { return on_terrain_; }
    // How many times as long as a man's its stride is (a giant's legs): its walk clip is played
    // that much slower for the same pace, or his feet would race.
    void set_walk_stride(float times) { walk_stride_ = std::max(times, 0.1F); }
    [[nodiscard]] float get_walk_stride() const { return walk_stride_; }
    // Whose soul it leaves when it falls: its family ("shroomling", "troll" ...: the item
    // "soul_<family>"); empty for one that leaves none.
    void set_soul(const godot::String& family) { soul_ = family; }
    [[nodiscard]] godot::String get_soul() const { return soul_; }
    void set_drops_loot(bool drops) { drops_loot_ = drops; }
    [[nodiscard]] bool get_drops_loot() const { return drops_loot_; }

    // Makes something that stuck in the body (an arrow) move and turn with it.
    void attach(godot::Node3D* stuck);

    // In a shared game one machine decides what the enemies do; on the others they are remote:
    // they do not think or move by themselves but follow `apply_net_state`, and the damage they
    // take there is collected for that one machine (`take_outgoing_damage`) instead of applied.
    // What an enemy does to a player is always decided on that player's own machine: a blow
    // or a bolt hurts only the hero who is played there.
    void set_remote(bool remote);
    [[nodiscard]] bool is_remote() const { return remote_; }
    // [x, y, z, facing, phase, health, target x, y, z, bolts thrown so far]. During a great
    // attack the phase's number has which one added to it (Special, Special + 1, ...).
    [[nodiscard]] godot::PackedFloat32Array get_net_state() const;
    void apply_net_state(const godot::PackedFloat32Array& state);
    // The damage dealt to it here since the last call, to be sent to the machine that decides.
    float take_outgoing_damage();
    // Damage another player dealt on their machine, arriving at the one that decides.
    void take_damage_from_peer(float amount);

    [[nodiscard]] bool is_alive() const { return state_.phase != gameplay::EnemyPhase::Dead; }
    // Far from every hero and with nothing to do, an enemy sleeps: it is not moved, not
    // animated and asks nothing, until a hero comes near or something hurts it.
    [[nodiscard]] bool is_asleep() const { return asleep_; }
    [[nodiscard]] bool is_aggro() const { return state_.aggro; }
    // Whether the model, as it is drawn, looks towards that place (within a quarter turn).
    // For tests: an enemy that has noticed the hero must face her.
    [[nodiscard]] bool looks_towards(const godot::Vector3& place) const;
    [[nodiscard]] float get_health() const { return state_.health; }
    // The middle of the body in world space: what skills aim at.
    [[nodiscard]] godot::Vector3 get_aim_point() const;
    // Totals since the scene started, for tests and statistics.
    [[nodiscard]] float get_damage_taken() const { return damage_taken_; }
    [[nodiscard]] int get_death_count() const { return death_count_; }

    void set_animation_library(const godot::Ref<godot::AnimationLibrary>& library) { animation_library_ = library; }
    [[nodiscard]] godot::Ref<godot::AnimationLibrary> get_animation_library() const { return animation_library_; }
    void set_health_bar_shader(const godot::Ref<godot::Shader>& shader) { health_bar_shader_ = shader; }
    [[nodiscard]] godot::Ref<godot::Shader> get_health_bar_shader() const { return health_bar_shader_; }
    void set_hit_effect(const godot::Ref<godot::PackedScene>& scene) { hit_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_hit_effect() const { return hit_effect_; }
    void set_max_health(float health) { params_.max_health = health; }
    [[nodiscard]] float get_max_health() const { return params_.max_health; }
    void set_move_speed(float speed) { move_speed_ = speed; }
    [[nodiscard]] float get_move_speed() const { return move_speed_; }
    void set_aggro_range(float metres) { params_.aggro_range = metres; }
    [[nodiscard]] float get_aggro_range() const { return params_.aggro_range; }
    void set_attack_range(float metres) { params_.attack_range = metres; }
    [[nodiscard]] float get_attack_range() const { return params_.attack_range; }
    void set_respawn_seconds(float seconds) { respawn_seconds_ = seconds; }
    [[nodiscard]] float get_respawn_seconds() const { return respawn_seconds_; }
    // Magic: with a cast range above zero it throws a spell at a player further away than a few metres.
    void set_cast_range(float metres) { params_.cast_range = metres; }
    [[nodiscard]] float get_cast_range() const { return params_.cast_range; }
    void set_cast_damage(float damage) { cast_damage_ = damage; }
    [[nodiscard]] float get_cast_damage() const { return cast_damage_; }
    void set_cast_cooldown(float seconds) { params_.cast_cooldown_seconds = seconds; }
    [[nodiscard]] float get_cast_cooldown() const { return params_.cast_cooldown_seconds; }
    void set_cast_effect(const godot::Ref<godot::PackedScene>& effect) { cast_effect_ = effect; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_cast_effect() const { return cast_effect_; }
    void set_bolt_trail(const godot::Ref<godot::PackedScene>& effect) { bolt_trail_ = effect; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_bolt_trail() const { return bolt_trail_; }
    void set_bolt_impact(const godot::Ref<godot::PackedScene>& effect) { bolt_impact_ = effect; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_bolt_impact() const { return bolt_impact_; }
    [[nodiscard]] int get_cast_count() const { return cast_count_; }
    // Flying: with a hover height above zero it floats that far above the ground instead of
    // walking, bobs, drifts sideways around the player while it keeps its distance, and
    // drops to the ground when it dies.
    void set_hover_height(float metres) { hover_height_ = metres; }
    [[nodiscard]] float get_hover_height() const { return hover_height_; }
    void set_keep_distance(float metres) { params_.keep_distance = metres; }
    [[nodiscard]] float get_keep_distance() const { return params_.keep_distance; }
    void set_cast_min_range(float metres) { params_.cast_min_range = metres; }
    [[nodiscard]] float get_cast_min_range() const { return params_.cast_min_range; }
    // A continuous effect it carries (a glow, a trail of sparks); switched off while it is dead.
    void set_trail_effect(const godot::Ref<godot::PackedScene>& effect) { trail_effect_ = effect; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_trail_effect() const { return trail_effect_; }
    // Shown on the ground in front of it at the moment its blow comes down (a giant's fists).
    void set_blow_effect(const godot::Ref<godot::PackedScene>& effect) { blow_effect_ = effect; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_blow_effect() const { return blow_effect_; }
    // A single blow of at least this much staggers it; set high for something heavy.
    void set_stagger_damage(float damage) { params_.stagger_damage = damage; }
    [[nodiscard]] float get_stagger_damage() const { return params_.stagger_damage; }
    // What it throws: how fast it flies, how long the cast takes and when in it the shot leaves
    // (a share of the cast: set both to fit the cast clip).
    void set_bolt_speed(float speed) { bolt_speed_ = speed; }
    [[nodiscard]] float get_bolt_speed() const { return bolt_speed_; }
    void set_cast_seconds(float seconds) { params_.cast_seconds = seconds; }
    [[nodiscard]] float get_cast_seconds() const { return params_.cast_seconds; }
    void set_cast_release_share(float share) { params_.cast_release_share = share; }
    [[nodiscard]] float get_cast_release_share() const { return params_.cast_release_share; }
    // How long one blow takes; set it to the length of the attack clip.
    void set_attack_seconds(float seconds) { params_.attack_seconds = seconds; }
    [[nodiscard]] float get_attack_seconds() const { return params_.attack_seconds; }
    void set_attack_damage(float damage) { attack_damage_ = damage; }
    [[nodiscard]] float get_attack_damage() const { return attack_damage_; }
    void set_body_height(float metres) { body_height_ = metres; }
    [[nodiscard]] float get_body_height() const { return body_height_; }
    void set_body_radius(float metres) { body_radius_ = metres; }
    [[nodiscard]] float get_body_radius() const { return body_radius_; }

protected:
    static void _bind_methods();

private:
    struct DamageNumber {
        std::uint64_t label_id = 0; // an id: the label is freed with the scene
        float age = 0.0F;
    };

    void build_health_bar();
    void update_health_bar();
    void show_damage_number(float amount, const godot::Vector3& position);
    void die();
    void drop_loot();
    void update_dead(float dt);
    void walk(bool moving, const godot::Vector3& direction, float dt);
    // Movement of a flying enemy for one step; `step` says what it wants.
    void fly(const gameplay::EnemyStep& step, const godot::Vector3& to_player, float dt);
    // Height of the ground under it.
    [[nodiscard]] float ground_height() const;
    // Where its spells come from: in front of its chest.
    [[nodiscard]] godot::Vector3 cast_origin() const;
    void throw_bolt(const godot::Vector3& player_position);
    void update_blow(bool attack_started, float dt, float distance_to_player);
    void update_remote(float dt);
    // The hero nearest to it, of all players: the one it goes for. Null if there is none.
    [[nodiscard]] godot::Node3D* nearest_player() const;
    // Metres to the hero played on this machine; very far if there is none.
    [[nodiscard]] float distance_to_local_player() const;
    void respawn();
    void play_phase_animation(bool moving);

    godot::Ref<godot::AnimationLibrary> animation_library_;
    godot::Ref<godot::Shader> health_bar_shader_;
    godot::Ref<godot::PackedScene> hit_effect_;
    gameplay::EnemyParams params_;
    gameplay::EnemyState state_;
    float move_speed_ = 1.5F;      // m/s
    float turn_speed_ = 6.0F;      // rad/s
    float respawn_seconds_ = 6.0F; // after dying; 0 = stays dead
    float body_height_ = 1.0F;     // metres; the bar floats above it
    float body_radius_ = 0.35F;    // metres; allowance for area effects
    float gravity_ = 12.0F;        // m/s^2
    float attack_damage_ = 8.0F;   // what one blow takes from the player
    float cast_damage_ = 12.0F;    // what one spell takes from the player
    int cast_count_ = 0;
    float bolt_speed_ = 13.0F;  // m/s: slow enough to step out of from a distance
    float hover_height_ = 0.0F; // metres above the ground; 0 = it walks
    float hover_seconds_ = 0.0F;
    float circle_side_ = 1.0F; // +1 or -1: which way round the player it drifts
    godot::Ref<godot::PackedScene> trail_effect_;
    godot::Ref<godot::PackedScene> blow_effect_;
    godot::Node3D* trail_ = nullptr; // non-owning child
    godot::Ref<godot::PackedScene> cast_effect_;
    godot::Ref<godot::PackedScene> bolt_trail_;
    godot::Ref<godot::PackedScene> bolt_impact_;
    godot::StringName clip_cast_;
    // Seconds until the blow of the attack in progress lands; negative = none pending.
    float blow_in_seconds_ = -1.0F;
    int special_count_ = 1;
    int special_index_ = 0; // which great attack is, or was last, in progress
    bool starts_parked_ = false;
    bool parked_ = false;
    bool drops_loot_ = true;
    godot::String soul_;
    float walk_stride_ = 1.0F;
    godot::Vector3 strolling_; // where it is walked to in the next step while idle (see stroll)
    bool strolls_ = false;
    bool on_terrain_ = true;
    [[nodiscard]] godot::StringName special_clip() const;
    void begin_special(int index);
    bool held_ = false;
    bool flung_ = false;
    float rooted_left_ = 0.0F;  // seconds it still cannot move
    godot::Vector3 dragged_;    // what draws it along in the next step (see drag)
    float slowed_share_ = 0.0F; // of its pace, taken away while slowed_left_ runs
    float slowed_left_ = 0.0F;
    float stunned_left_ = 0.0F; // seconds it still stands stunned
    float cursed_left_ = 0.0F;  // seconds it is still cursed ...
    float curse_factor_ = 1.0F; // ... and how many times a blow hurts it meanwhile
    // Its pace now: move_speed_, less what slows it.
    [[nodiscard]] float pace() const { return move_speed_ * (slowed_left_ > 0.0F ? 1.0F - slowed_share_ : 1.0F); }
    bool remote_ = false;
    bool has_net_state_ = false;
    godot::Vector3 net_position_;
    float net_facing_ = 0.0F;
    float outgoing_damage_ = 0.0F;
    godot::Vector3 target_position_; // where the hero it goes for stands
    int bolts_thrown_ = 0;
    int bolts_shown_ = 0;

    // Damage that arrived since the last physics step.
    float pending_damage_ = 0.0F;
    godot::Array outgoing_effects_;
    int dropped_seed_ = 0;
    godot::Vector3 outgoing_drag_;
    bool has_outgoing_drag_ = false;
    godot::Vector3 arriving_fling_;
    void note_effect(int what, float a, float b);
    bool asleep_ = false;
    float sleep_check_seconds_ = 0.0F; // until it looks again whether to sleep or wake
    // Whether it should sleep now; looked at a few times a second, not every frame.
    [[nodiscard]] bool should_sleep(float dt);
    void set_asleep(bool asleep);
    float heaviest_pending_blow_ = 0.0F;
    float damage_taken_ = 0.0F;
    int death_count_ = 0;
    float model_yaw_ = 0.0F; // which way the model looks, in the world
    float spawn_yaw_ = 0.0F; // ... and which way it looked when it was placed
    godot::Transform3D spawn_transform_;
    std::uint32_t collision_layer_ = 1;
    std::vector<DamageNumber> damage_numbers_;

    // Non-owning: child nodes owned by the scene tree.
    godot::Node3D* model_ = nullptr;
    godot::MeshInstance3D* health_bar_ = nullptr;
    CharacterAnimator animator_;
    godot::StringName clip_idle_;
    godot::StringName clip_walk_;
    godot::StringName clip_attack_;
    godot::StringName clip_hit_;
    godot::StringName clip_death_;
};

} // namespace e5::bridge
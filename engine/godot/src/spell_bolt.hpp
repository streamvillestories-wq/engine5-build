#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>

namespace e5::bridge {

// A spell in flight: a glowing effect that flies in a straight line (no
// gravity, unlike an arrow), sweeps a ray along each step so it cannot pass
// through thin things, and bursts where it hits. With a blast radius it hurts
// everything near the burst, otherwise only what it struck. It has no body of
// its own: what is seen is the trail effect it carries.
class E5SpellBolt : public godot::Node3D {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5SpellBolt, godot::Node3D)

public:
    void _physics_process(double delta) override;

    // All before launch. Effects may be unset.
    void set_trail_effect(const godot::Ref<godot::PackedScene>& effect, float scale = 1.0F);
    void set_impact_effect(const godot::Ref<godot::PackedScene>& effect) { impact_effect_ = effect; }
    // The enemy it strikes is poisoned: `tick_damage` every `tick_seconds` for `seconds`, with
    // `visual` on it meanwhile. One poison to an enemy: struck again, it begins anew.
    void set_poison(float tick_damage, float seconds, float tick_seconds,
                    const godot::Ref<godot::PackedScene>& visual) {
        poison_tick_damage_ = tick_damage;
        poison_seconds_ = seconds;
        poison_tick_seconds_ = tick_seconds;
        poison_visual_ = visual;
    }
    // How large the impact effect is shown, of the size it was made at.
    void set_impact_scale(float scale) { impact_scale_ = scale; }
    // What its burst catches is stunned for `seconds`, with `visual` over its head meanwhile (the
    // dwarf's Stonebreaker). Only where it does damage: a shown hero's stuns nobody.
    void set_stun(float seconds, const godot::Ref<godot::PackedScene>& visual) {
        stun_seconds_ = seconds;
        stun_visual_ = visual;
    }
    void set_damage(float damage) { damage_ = damage; }
    // This share of the damage it does heals the hero played here (the Bonecaller's Death Coil).
    void set_heal_share(float share) { heal_share_ = share; }
    void set_blast_radius(float radius) { blast_radius_ = radius; }
    // Makes the spell steer towards a place in the world: it may start in any direction and
    // bends its path there, speeding up to `speed`. `turn` is how hard it steers (m/s^2).
    void set_homing(const godot::Vector3& target, float speed, float turn);
    // With set_homing: it steers towards this enemy's aim point instead (an E5Enemy; anything else is
    // not followed), for as long as it is there and alive; then towards where it was last.
    void set_homing_on(const godot::Node3D* target);
    // Thrown by an enemy: it hurts the player and nothing else, and flies through other enemies.
    void set_hostile(bool hostile) { hostile_ = hostile; }
    // Thrown by a player's unit (E5Enemy::team, 2026-10-22): it hurts what that side fights, heroes
    // of other sides and the wild and other sides' units, and flies through its own side.
    void set_team(int team) { team_ = team; }
    // Seconds the spell hangs where it is before it starts to fly.
    void set_launch_delay(float seconds) { launch_delay_ = seconds; }

    // `caster` is excluded from collision so the spell cannot hit who cast it.
    void launch(const godot::Vector3& velocity, const godot::RID& caster);

protected:
    static void _bind_methods() {}

private:
    void burst(const godot::Vector3& position, godot::Object* collider);

    godot::Vector3 velocity_;
    godot::RID caster_;
    godot::Ref<godot::PackedScene> impact_effect_;
    godot::Node3D* trail_ = nullptr; // non-owning child
    float damage_ = 0.0F;
    float heal_share_ = 0.0F;
    float blast_radius_ = 0.0F;
    float impact_scale_ = 1.0F;
    float poison_tick_damage_ = 0.0F;
    float poison_seconds_ = 0.0F; // 0: it poisons nobody
    float poison_tick_seconds_ = 0.5F;
    godot::Ref<godot::PackedScene> poison_visual_;
    float stun_seconds_ = 0.0F; // 0: it stuns nobody
    godot::Ref<godot::PackedScene> stun_visual_;
    godot::Vector3 homing_target_;
    float homing_speed_ = 0.0F; // 0 = flies straight
    float homing_turn_ = 0.0F;
    std::uint64_t homing_on_ = 0; // an instance id: it may be gone
    float launch_delay_ = 0.0F;
    double age_seconds_ = 0.0;
    bool flying_ = false;
    bool hostile_ = false;
    int team_ = 0;
};

} // namespace e5::bridge

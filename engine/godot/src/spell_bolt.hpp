#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector3.hpp>

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
    void set_damage(float damage) { damage_ = damage; }
    void set_blast_radius(float radius) { blast_radius_ = radius; }
    // Makes the spell steer towards a place in the world: it may start in any direction and
    // bends its path there, speeding up to `speed`. `turn` is how hard it steers (m/s^2).
    void set_homing(const godot::Vector3& target, float speed, float turn);
    // Thrown by an enemy: it hurts the player and nothing else, and flies through other enemies.
    void set_hostile(bool hostile) { hostile_ = hostile; }
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
    float blast_radius_ = 0.0F;
    godot::Vector3 homing_target_;
    float homing_speed_ = 0.0F; // 0 = flies straight
    float homing_turn_ = 0.0F;
    float launch_delay_ = 0.0F;
    double age_seconds_ = 0.0;
    bool flying_ = false;
    bool hostile_ = false;
};

} // namespace e5::bridge

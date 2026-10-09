#pragma once

#include "e5/gameplay/projectile.hpp"

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace e5::bridge {

// An arrow. Until `launch` is called it is only a visual (used for the arrow
// resting on the bowstring). After launch it flies under gravity, sweeps a ray
// along each step so it cannot tunnel through thin objects, sticks where it
// hits, and reports the hit to an E5Target.
//
// The node's origin is the nock (tail); the arrow points along its local -Z.
// The visual is built in code for now; a modelled arrow can replace it later.
class E5Arrow : public godot::Node3D {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5Arrow, godot::Node3D)

public:
    static constexpr float length = 0.75F; // metres, nock to tip

    void _ready() override;
    void _physics_process(double delta) override;

    // Makes this a power shot (0 < power <= 1): it carries the trail effect,
    // plays the impact effect where it lands and scores double on targets.
    // Call before launch.
    void set_power(float power, const godot::Ref<godot::PackedScene>& trail_effect,
                   const godot::Ref<godot::PackedScene>& impact_effect);

    // Effect carried at the tip while flying (also set by set_power). Call before launch.
    void set_trail_effect(const godot::Ref<godot::PackedScene>& effect);
    // Makes the arrow explode where it lands: every target within `radius` is hit,
    // not only the one it struck. 0 (the default) is an ordinary arrow.
    void set_blast_radius(float radius) { blast_radius_ = radius; }
    // Damage to an enemy it hits (or to every enemy in the blast). Defaults to a plain shot.
    void set_damage(float damage) { damage_ = damage; }
    // Effect played where the arrow lands (also set by set_power).
    void set_impact_effect(const godot::Ref<godot::PackedScene>& effect) { impact_effect_ = effect; }
    // What else an arrow does where it strikes (the archer's newer skills).
    enum class Special : unsigned char {
        None,
        Venom,   // leaves a cloud that hurts what stands in it, and stays on the enemy it struck
        Gale,    // goes through enemies, throwing each back, and only sticks in the world
        Storm,   // lightning leaps on to the enemies nearby
        Bramble, // brambles hold every enemy around the place
    };
    // `damage`: of the cloud's ticks, or of each leap of the lightning. `first`: the cloud, the
    // strike at each enemy the lightning reaches, the burst of leaves. `second`: the cage of
    // brambles round each enemy held. Call before launch.
    void set_special(Special special, float damage, const godot::Ref<godot::PackedScene>& first,
                     const godot::Ref<godot::PackedScene>& second = {});
    // Seconds after which an arrow that has hit nothing is removed.
    void set_flight_lifetime(double seconds) { flight_lifetime_ = seconds; }

    // `shooter` is excluded from collision so the arrow cannot hit its archer.
    void launch(const godot::Vector3& velocity, const godot::RID& shooter);

protected:
    static void _bind_methods() {}

private:
    void stick(const godot::Vector3& hit_position, const godot::Vector3& direction, godot::Object* collider);
    // A gale arrow meeting an enemy: hurts it, throws it back and flies on. False if it does not go through.
    bool pierce(const godot::Vector3& hit_position, const godot::Vector3& direction, godot::Object* collider);
    void leave_behind(const godot::Vector3& hit_position, godot::Object* collider);

    gameplay::Projectile projectile_;
    godot::RID shooter_;
    bool flying_ = false;
    bool shaft_swept_ = false; // the stretch from the string to the tip has been looked at
    float power_ = 0.0F;
    float blast_radius_ = 0.0F;
    float damage_ = 20.0F;
    godot::Ref<godot::PackedScene> impact_effect_;
    Special special_ = Special::None;
    float special_damage_ = 0.0F;
    godot::Ref<godot::PackedScene> special_first_;
    godot::Ref<godot::PackedScene> special_second_;
    godot::TypedArray<godot::RID> passed_; // the enemies a gale arrow has gone through
    godot::Node3D* trail_ = nullptr;       // non-owning child
    double age_seconds_ = 0.0;
    double flight_lifetime_ = 6.0;
};

} // namespace e5::bridge

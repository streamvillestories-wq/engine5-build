#pragma once

#include "e5/gameplay/bird_flight.hpp"

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <vector>

namespace godot {
class Skeleton3D;
} // namespace godot

namespace e5::bridge {

// A summoned attack bird: the root of a bird scene (see game/creatures). Once
// launched it flies by the rules in e5::gameplay (bird_flight.hpp): it dives at
// an E5Target near its summoner again and again, scoring a hit each time, and
// leaves when its time is up. With no target in range it circles its summoner.
//
// The model needs no animation clips: the wings are flapped in code by turning
// the bones named in `left_wing_bones` / `right_wing_bones` (root of the wing
// first) about the model's front axis. The model's front is +Z.
class E5Bird : public godot::Node3D {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5Bird, godot::Node3D)

public:
    // Every bird is in this group while it lives.
    static constexpr const char* group_name = "e5_bird";

    void _ready() override;
    void _process(double delta) override;
    void _physics_process(double delta) override;

    // Sits still (on a hand, a branch) with folded wings until launched. Whoever
    // perches the bird places it; `set_wing_spread` opens the wings (0 folded, 1 spread).
    void perch();
    // A harmless bird flies and pecks for show: the copy of another player's, whose machine
    // decides what the real one does.
    void set_harmless(bool harmless) { harmless_ = harmless; }
    void set_wing_spread(float spread) { wing_spread_ = spread; }

    // Starts the flight. `side` is +1 or -1 (which way it peels off), `index`
    // spreads several birds over the available targets, `summoner` is the node
    // it stays near.
    void launch(const godot::Vector3& position, const godot::Vector3& velocity, float side, int index,
                const godot::Node3D* summoner);

    void set_left_wing_bones(const godot::PackedStringArray& bones) { left_wing_bones_ = bones; }
    [[nodiscard]] godot::PackedStringArray get_left_wing_bones() const { return left_wing_bones_; }
    void set_right_wing_bones(const godot::PackedStringArray& bones) { right_wing_bones_ = bones; }
    [[nodiscard]] godot::PackedStringArray get_right_wing_bones() const { return right_wing_bones_; }
    void set_flap_rate(float beats_per_second) { flap_rate_ = beats_per_second; }
    [[nodiscard]] float get_flap_rate() const { return flap_rate_; }
    void set_flap_angle(float radians) { flap_angle_ = radians; }
    [[nodiscard]] float get_flap_angle() const { return flap_angle_; }
    void set_attack_range(float metres) { attack_range_ = metres; }
    [[nodiscard]] float get_attack_range() const { return attack_range_; }
    void set_hit_effect(const godot::Ref<godot::PackedScene>& scene) { hit_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_hit_effect() const { return hit_effect_; }
    void set_vanish_effect(const godot::Ref<godot::PackedScene>& scene) { vanish_effect_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_vanish_effect() const { return vanish_effect_; }

protected:
    static void _bind_methods();

private:
    struct WingBone {
        std::int32_t bone = -1;
        godot::Quaternion rest;        // the bone's own rest rotation
        godot::Quaternion parent_rest; // its parent's rest orientation in skeleton space
        float direction = 1.0F;        // +1 left wing, -1 right wing
        float lag = 0.0F;              // radians the segment trails behind the wing root
        float share = 1.0F;            // fraction of the flap angle this segment adds
    };

    void collect_wing(const godot::PackedStringArray& names, float direction);
    void flap(float delta);
    // Returns false if there is nothing to attack.
    [[nodiscard]] bool find_target(const godot::Vector3& home, godot::Vector3& target_position);

    godot::PackedStringArray left_wing_bones_;
    godot::PackedStringArray right_wing_bones_;
    float flap_rate_ = 7.0F;     // wing beats per second
    float flap_angle_ = 0.7F;    // radians up and down at the wing root
    float attack_range_ = 40.0F; // metres from the summoner
    godot::Ref<godot::PackedScene> hit_effect_;
    godot::Ref<godot::PackedScene> vanish_effect_;

    godot::Skeleton3D* skeleton_ = nullptr; // non-owning child
    std::vector<WingBone> wing_bones_;
    float flap_phase_ = 0.0F;

    gameplay::BirdState state_;
    gameplay::BirdParams params_;
    bool launched_ = false;
    bool perched_ = false;
    bool harmless_ = false;
    float wing_spread_ = 1.0F;
    int index_ = 0;
    // Ids, not pointers: the summoner and the targets may be removed while the bird lives.
    std::uint64_t summoner_id_ = 0;
    std::uint64_t target_id_ = 0;
    godot::Vector3 home_;
};

} // namespace e5::bridge
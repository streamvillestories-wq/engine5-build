#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <vector>

namespace e5::bridge {

// The wizard's black hole. It opens where it is placed, swallows every living
// enemy within reach, holds them shrunken and circling around its core for a
// while, then collapses and spits them out in all directions. Its look is a
// scene (game/effects/black_hole.tscn) that it scales and turns; the burst
// when it lets go is another.
class E5BlackHole : public godot::Node3D {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5BlackHole, godot::Node3D)

public:
    // Every black hole is in this group: only one at a time.
    static constexpr const char* group_name = "e5_black_hole";

    // All before it enters the tree.
    void configure(const godot::Ref<godot::PackedScene>& look, const godot::Ref<godot::PackedScene>& burst,
                   float damage_per_second, float spit_damage);

    void _ready() override;
    void _physics_process(double delta) override;

protected:
    static void _bind_methods() {}

private:
    struct Captive {
        std::uint64_t enemy_id = 0; // an id: the enemy may be freed meanwhile
        godot::Vector3 start;       // where it was taken from
        float angle = 0.0F;         // its place on the orbit
        float height = 0.0F;
        float caught_at = 0.0F; // seconds into the hole's life
    };

    void catch_enemies();
    void hold(float delta);
    void spit_out();

    godot::Ref<godot::PackedScene> look_scene_;
    godot::Ref<godot::PackedScene> burst_scene_;
    godot::Node3D* look_ = nullptr; // non-owning child
    godot::Node3D* disk_ = nullptr; // non-owning, inside the look
    std::vector<Captive> captives_;
    float damage_per_second_ = 3.0F;
    float spit_damage_ = 30.0F;
    float age_ = 0.0F;
    float damage_due_ = 0.0F;
    bool spat_ = false;
};

} // namespace e5::bridge

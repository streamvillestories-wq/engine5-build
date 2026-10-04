#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>

#include <vector>

namespace godot {
class GeometryInstance3D;
class OmniLight3D;
} // namespace godot

namespace e5::bridge {

// A one-shot visual effect: the root node of an effect scene (game/effects).
// On ready it restarts every particle system below it, then over `lifetime`
// seconds fades any lights out, grows and fades a child named "Ring" (a
// shockwave), and finally frees itself. Purely cosmetic; never networked.
class E5Effect : public godot::Node3D {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5Effect, godot::Node3D)

public:
    void _ready() override;
    void _process(double delta) override;

    void set_lifetime(float seconds) { lifetime_ = seconds; }
    [[nodiscard]] float get_lifetime() const { return lifetime_; }
    void set_ring_max_scale(float scale) { ring_max_scale_ = scale; }
    [[nodiscard]] float get_ring_max_scale() const { return ring_max_scale_; }

    // Instances `scene` under `parent` at a world position. Returns nullptr
    // (and does nothing) if the scene is not set.
    static godot::Node3D* spawn(const godot::Ref<godot::PackedScene>& scene, godot::Node* parent,
                                const godot::Vector3& global_position);

    // Switches a continuous effect (a trail, a glow) on or off: its particle
    // systems start or stop emitting, everything else is shown or hidden.
    static void set_active(godot::Node3D* effect, bool active);

protected:
    static void _bind_methods();

private:
    struct FadingLight {
        godot::OmniLight3D* light; // non-owning child
        float start_energy;
    };

    float lifetime_ = 1.0F;       // seconds until the effect removes itself
    float ring_max_scale_ = 4.0F; // final size of the "Ring" child relative to its authored size
    float age_ = 0.0F;
    std::vector<FadingLight> lights_;
    godot::GeometryInstance3D* ring_ = nullptr; // non-owning child
};

} // namespace e5::bridge
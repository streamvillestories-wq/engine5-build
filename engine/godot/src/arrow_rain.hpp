#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace e5::bridge {

// A volley falling on an area. Placed at the centre of the target area, it
// waits `delay` seconds (the arrows are "in the air"), then drops `count`
// arrows over `duration` seconds in the pattern from e5::gameplay, shows an
// optional marker for as long as it lasts, and removes itself.
class E5ArrowRain : public godot::Node3D {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5ArrowRain, godot::Node3D)

public:
    void _ready() override;
    void _physics_process(double delta) override;

    // `shooter` is excluded from collision: the archer may stand in her own rain.
    void configure(const godot::RID& shooter, const godot::Ref<godot::PackedScene>& marker,
                   const godot::Ref<godot::PackedScene>& impact_effect);

    void set_radius(float radius) { radius_ = radius; }
    [[nodiscard]] float get_radius() const { return radius_; }

protected:
    static void _bind_methods() {}

private:
    void drop_arrow(int index);

    godot::RID shooter_;
    godot::Ref<godot::PackedScene> marker_;
    godot::Ref<godot::PackedScene> impact_effect_;
    int count_ = 24;
    float radius_ = 3.0F;   // metres
    float delay_ = 0.7F;    // seconds before the first arrow lands
    float duration_ = 1.2F; // seconds over which the arrows fall
    float height_ = 14.0F;  // metres above the area where arrows appear
    float speed_ = 45.0F;   // m/s
    float elapsed_ = 0.0F;
    int dropped_ = 0;
};

} // namespace e5::bridge
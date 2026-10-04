#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
class MeshInstance3D;
}

namespace e5::bridge {

// Draws a thin bowstring and lets it be pulled back. Add it as a child of a
// bow model that contains two nodes named `string_anchor_top` and
// `string_anchor_bottom` (created by tools/blender/strip_bowstring.py).
//
// `draw` (0..1) pulls the nock point away from the bow along `pull_direction`
// (in this node's space) by up to `max_draw_distance`.
class E5BowString : public godot::Node3D {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5BowString, godot::Node3D)

public:
    void _ready() override;

    void set_draw(float draw);
    [[nodiscard]] float get_draw() const { return draw_; }
    void set_max_draw_distance(float distance);
    [[nodiscard]] float get_max_draw_distance() const { return max_draw_distance_; }
    void set_pull_direction(const godot::Vector3& direction);
    [[nodiscard]] godot::Vector3 get_pull_direction() const { return pull_direction_; }
    void set_thickness(float thickness) { thickness_ = thickness; }
    [[nodiscard]] float get_thickness() const { return thickness_; }

    // Lets the nock point follow something else (the drawing hand): weight 0
    // keeps the computed position, 1 puts the nock exactly on local_point.
    void set_nock_target(const godot::Vector3& local_point, float weight);

    void set_brace_height(float height) { brace_height_ = height; }
    [[nodiscard]] float get_brace_height() const { return brace_height_; }
    // Where an arrow crosses the bow, in this node's space: on the far side of
    // the string's rest line by the brace height.
    [[nodiscard]] godot::Vector3 get_arrow_rest_position() const;

    // Nock point in this node's space; where an arrow and the drawing hand belong.
    [[nodiscard]] godot::Vector3 get_nock_position() const { return nock_; }

protected:
    static void _bind_methods();

private:
    void update_segments();

    float draw_ = 0.0F;
    float max_draw_distance_ = 0.45F; // metres
    float thickness_ = 0.004F;        // metres, diameter
    float brace_height_ = 0.09F;      // metres from the string at rest to the grip
    godot::Vector3 nock_target_;
    float nock_target_weight_ = 0.0F;
    // The modelled string sits on the bow's +X side, toward the archer.
    godot::Vector3 pull_direction_{1.0F, 0.0F, 0.0F};

    bool anchors_found_ = false;
    godot::Vector3 top_;
    godot::Vector3 bottom_;
    godot::Vector3 nock_;

    // Non-owning: child nodes owned by the scene tree.
    godot::MeshInstance3D* upper_segment_ = nullptr;
    godot::MeshInstance3D* lower_segment_ = nullptr;
};

} // namespace e5::bridge

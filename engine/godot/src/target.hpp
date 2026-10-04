#pragma once

#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
class Label3D;
}

namespace e5::bridge {

// A round archery target that faces along its local +Z. It builds its own
// rings, collision shape and score label, counts hits and scores them by ring
// (centre = `rings` points, outermost ring = 1).
class E5Target : public godot::StaticBody3D {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5Target, godot::StaticBody3D)

public:
    // Every target is in this group, so area effects can find them.
    static constexpr const char* group_name = "e5_target";

    void _ready() override;

    // Scores a hit on every target within `radius` of `centre` (an explosion, a
    // shockwave), at the point of each target nearest to it. Returns how many were hit.
    static int blast(godot::Node* context, const godot::Vector3& centre, float radius, int score_multiplier = 1);
    // Called by an arrow that struck this target; `global_position` is the point of impact.
    // score_multiplier is 1 for a normal arrow and higher for a power shot.
    void register_hit(const godot::Vector3& global_position, int score_multiplier = 1);

    void set_radius(float radius) { radius_ = radius; }
    [[nodiscard]] float get_radius() const { return radius_; }
    void set_rings(int rings) { rings_ = rings; }
    [[nodiscard]] int get_rings() const { return rings_; }
    [[nodiscard]] int get_hit_count() const { return hit_count_; }
    [[nodiscard]] int get_power_hit_count() const { return power_hit_count_; }
    [[nodiscard]] int get_total_score() const { return total_score_; }
    [[nodiscard]] int get_last_score() const { return last_score_; }

protected:
    static void _bind_methods();

private:
    void update_label();

    float radius_ = 0.6F; // metres
    int rings_ = 5;
    int hit_count_ = 0;
    int power_hit_count_ = 0;
    int total_score_ = 0;
    int last_score_ = 0;
    godot::Label3D* label_ = nullptr; // non-owning child, owned by the scene tree
};

} // namespace e5::bridge

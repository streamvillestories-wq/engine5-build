#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
class Skeleton3D;
class SpringBoneSimulator3D;
} // namespace godot

namespace e5::bridge {

// Makes skirts, capes, hair and similar loose parts swing. Place it next to a
// character model (as a sibling or parent of the model's skeleton): on ready it
// finds the skeleton, collects the bone chains named cloth_<part>_<chain>_<segment>
// (created by tools/blender/cloth_rig.py) and drives them with Godot's
// SpringBoneSimulator3D, with capsule colliders on the legs, chest, shoulders
// and arms so cloth and hair stay outside the body.
//
// The effect is cosmetic and runs locally; it is never part of networked state.
class E5ClothSimulator : public godot::Node {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5ClothSimulator, godot::Node)

public:
    void _ready() override;

    void set_stiffness(float stiffness) { stiffness_ = stiffness; }
    [[nodiscard]] float get_stiffness() const { return stiffness_; }
    void set_drag(float drag) { drag_ = drag; }
    [[nodiscard]] float get_drag() const { return drag_; }
    void set_gravity(float gravity) { gravity_ = gravity; }
    [[nodiscard]] float get_gravity() const { return gravity_; }
    void set_joint_radius(float radius) { joint_radius_ = radius; }
    [[nodiscard]] float get_joint_radius() const { return joint_radius_; }
    void set_hinged_parts(const godot::String& parts) { hinged_parts_ = parts; }
    [[nodiscard]] godot::String get_hinged_parts() const { return hinged_parts_; }
    void set_strand_radius(float radius) { strand_radius_ = radius; }
    [[nodiscard]] float get_strand_radius() const { return strand_radius_; }
    void set_arm_radius(float radius) { arm_radius_ = radius; }
    [[nodiscard]] float get_arm_radius() const { return arm_radius_; }
    void set_torso_radius(float radius) { torso_radius_ = radius; }
    [[nodiscard]] float get_torso_radius() const { return torso_radius_; }
    void set_waist_radius(float radius) { waist_radius_ = radius; }
    [[nodiscard]] float get_waist_radius() const { return waist_radius_; }
    void set_leg_radius(float radius) { leg_radius_ = radius; }
    [[nodiscard]] float get_leg_radius() const { return leg_radius_; }

    // Number of chains being simulated; 0 if the skeleton has no cloth bones.
    [[nodiscard]] int get_chain_count() const { return chain_count_; }

protected:
    static void _bind_methods();

private:
    [[nodiscard]] godot::Skeleton3D* find_skeleton() const;
    void add_body_colliders(godot::Skeleton3D* skeleton, godot::SpringBoneSimulator3D* simulator) const;

    // Defaults tuned on a knee-length skirt and a ponytail at running speed
    // (4.5 m/s). The simulation works in world space, so drag acts like air
    // resistance: with the engine's own defaults (stiffness 1, drag 0.4) the
    // cloth streamed out horizontally behind a running character.
    float stiffness_ = 2.5F;      // pull back toward the authored shape
    float drag_ = 0.25F;          // air resistance; higher trails further behind
    float gravity_ = 1.0F;        // downward pull
    float joint_radius_ = 0.03F;  // metres; thickness of a chain for collision
    float leg_radius_ = 0.085F;   // metres; thickness of the leg colliders
    float strand_radius_ = 0.05F; // metres; thickness of a single-chain part such as a ponytail
    // Names of parts, separated by commas, whose chains only swing towards and away from the
    // body (about each bone's own X axis) and never sideways. For a cape: chains that are free
    // to swing sideways slide off a bent back and hang beside him, and the cloth between them
    // and their neighbours then cuts through the body. cloth_rig.py's curtain layout turns the
    // bones so that X lies across the cloth.
    godot::String hinged_parts_;
    float arm_radius_ = 0.05F;   // metres; thickness of the arm colliders
    float torso_radius_ = 0.13F; // metres; thickness of the chest colliders
    // Metres; thickness of colliders on the hips and belly. 0 = none: a skirt hangs from there
    // and would be pushed off the body. A cape down the back needs them, or the back comes through.
    float waist_radius_ = 0.0F;
    int chain_count_ = 0;
};

} // namespace e5::bridge

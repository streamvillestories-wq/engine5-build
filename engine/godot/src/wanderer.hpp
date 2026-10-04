#pragma once

#include "character_animator.hpp"
#include "e5/gameplay/patrol.hpp"

#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
class Node3D;
} // namespace godot

namespace e5::bridge {

// A peaceful creature that roams about a place: freely within a ring around it, from spot
// to spot with rests between. It fights nobody and cannot be hurt; it is solid, so the
// player walks round it, and it waits or slides past when the player or a rock stands in
// its way (enemies it walks through). Where it goes is decided in e5::gameplay
// (patrol.hpp). Expects a child `Model` and an animation library with `idle` and `walk`.
class E5Wanderer : public godot::CharacterBody3D {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5Wanderer, godot::CharacterBody3D)

public:
    static constexpr const char* group_name = "e5_wanderer";

    void _ready() override;
    void _physics_process(double delta) override;

    void set_animation_library(const godot::Ref<godot::AnimationLibrary>& library) { animation_library_ = library; }
    [[nodiscard]] godot::Ref<godot::AnimationLibrary> get_animation_library() const { return animation_library_; }
    // The middle of the place it keeps to; only x and z count.
    void set_roam_center(const godot::Vector3& center);
    [[nodiscard]] godot::Vector3 get_roam_center() const;
    // It stays between these two distances from the middle.
    void set_roam_inner_radius(float metres) { params_.inner_radius = metres; }
    [[nodiscard]] float get_roam_inner_radius() const { return params_.inner_radius; }
    void set_roam_outer_radius(float metres) { params_.outer_radius = metres; }
    [[nodiscard]] float get_roam_outer_radius() const { return params_.outer_radius; }
    void set_move_speed(float speed) { params_.speed = speed; }
    [[nodiscard]] float get_move_speed() const { return params_.speed; }
    // How long it stands at each spot: somewhere between the two.
    void set_rest_min_seconds(float seconds) { params_.rest_min_seconds = seconds; }
    [[nodiscard]] float get_rest_min_seconds() const { return params_.rest_min_seconds; }
    void set_rest_max_seconds(float seconds) { params_.rest_max_seconds = seconds; }
    [[nodiscard]] float get_rest_max_seconds() const { return params_.rest_max_seconds; }
    // The speed the walk clip's steps cover on this body: the clip's own (made for a
    // human) times how much larger the creature is. Sets how fast the clip plays.
    void set_walk_clip_speed(float speed) { walk_clip_speed_ = speed; }
    [[nodiscard]] float get_walk_clip_speed() const { return walk_clip_speed_; }

    // Metres walked since the scene started (for unattended checks).
    [[nodiscard]] float get_distance_walked() const { return distance_walked_; }

protected:
    static void _bind_methods();

private:
    gameplay::RoamParams params_;
    gameplay::RoamState state_;
    godot::Ref<godot::AnimationLibrary> animation_library_;
    CharacterAnimator animator_;
    godot::Node3D* model_ = nullptr; // non-owning child
    godot::StringName clip_idle_;
    godot::StringName clip_walk_;
    float walk_clip_speed_ = 1.6F;
    float model_yaw_ = 0.0F;
    float distance_walked_ = 0.0F;
    bool ignoring_enemies_ = false;
};

} // namespace e5::bridge

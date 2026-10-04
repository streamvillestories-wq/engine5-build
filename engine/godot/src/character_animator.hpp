#pragma once

#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {
class AnimationTree;
class Node;
class Node3D;
class Skeleton3D;
} // namespace godot

namespace e5::bridge {

// Plays a character's clips in two layers:
//   base   one clip for the whole body (idle, run, aim stance, aim-walk ...)
//   upper  optionally, a second clip that replaces only the upper body
//          (spine, arms, head), so e.g. drawing a bow can play while the
//          legs keep walking
//
// Built on Godot's AnimationTree: two cross-fading selectors feeding a blend
// node that is filtered to the upper-body bones. Not a Godot class itself; it
// is owned by the node that controls the character.
class CharacterAnimator {
public:
    // Creates the animation tree under `owner`, animating the skeleton found
    // inside `model`. Returns false (and plays nothing) if that is not possible.
    [[nodiscard]] bool setup(godot::Node* owner, godot::Node3D* model,
                             const godot::Ref<godot::AnimationLibrary>& library);

    [[nodiscard]] bool is_ready() const { return tree_ != nullptr; }
    [[nodiscard]] bool has_clip(const godot::StringName& clip) const;
    [[nodiscard]] float clip_length(const godot::StringName& clip) const;

    // Cross-fades the whole body to `clip`. Asking for the clip that is already
    // playing only updates the speed; it does not restart it.
    void set_base(const godot::StringName& clip, float playback_scale);

    // Cross-fades the upper body to `clip`; an empty name removes the overlay.
    // Every change restarts the clip, so a one-shot can be played again.
    void set_upper(const godot::StringName& clip);

    // Advances the overlay's fade. Call once per physics step.
    void update(float delta_seconds);

    [[nodiscard]] const godot::StringName& base_clip() const { return base_clip_; }
    [[nodiscard]] godot::Skeleton3D* skeleton() const { return skeleton_; }

private:
    godot::Ref<godot::AnimationLibrary> library_;
    // Non-owning: child nodes owned by the scene tree.
    godot::AnimationTree* tree_ = nullptr;
    godot::Skeleton3D* skeleton_ = nullptr;
    godot::StringName base_clip_;
    godot::StringName upper_clip_;
    float upper_blend_ = 0.0F;
};

} // namespace e5::bridge

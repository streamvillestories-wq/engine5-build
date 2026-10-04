#include "character_animator.hpp"

#include "godot_log.hpp"

#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_node_animation.hpp>
#include <godot_cpp/classes/animation_node_blend2.hpp>
#include <godot_cpp/classes/animation_node_blend_tree.hpp>
#include <godot_cpp/classes/animation_node_time_scale.hpp>
#include <godot_cpp/classes/animation_node_transition.hpp>
#include <godot_cpp/classes/animation_tree.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <algorithm>
#include <cstdint>

namespace e5::bridge {
namespace {

constexpr double crossfade_seconds = 0.2;
constexpr float upper_fade_rate = 8.0F; // 1/s; the overlay fades in or out in 1/8 s
constexpr float min_playback_scale = 0.6F;
constexpr float max_playback_scale = 1.6F;

// Bones that carry the body rather than act: these stay with the base clip.
bool is_lower_body(const godot::String& bone) {
    return bone == "Hips" || bone == "Root" || bone.contains("Leg") || bone.contains("Foot") || bone.contains("Toe") ||
           bone.begins_with("cloth_");
}

// A selector node with one input per clip, cross-fading between them.
godot::Ref<godot::AnimationNodeTransition> add_selector(godot::AnimationNodeBlendTree* tree, const godot::String& name,
                                                        const godot::PackedStringArray& clips,
                                                        const godot::Ref<godot::AnimationLibrary>& library,
                                                        bool one_shot_layer) {
    godot::Ref<godot::AnimationNodeTransition> selector;
    selector.instantiate();
    selector->set_input_count(static_cast<std::int32_t>(clips.size()));
    selector->set_xfade_time(crossfade_seconds);
    selector->set_allow_transition_to_self(one_shot_layer);
    tree->add_node(name, selector);

    for (std::int32_t index = 0; index < clips.size(); ++index) {
        selector->set_input_name(index, clips[index]);
        // Looping clips keep running in the background and are faded back in where
        // they are; restarting them on every switch made each stop and start replay
        // the clip's opening motion. One-shots (jump, draw, release) start over.
        const bool loops = library->get_animation(clips[index])->get_loop_mode() != godot::Animation::LOOP_NONE;
        selector->set_input_reset(index, one_shot_layer || !loops);
        godot::Ref<godot::AnimationNodeAnimation> clip;
        clip.instantiate();
        clip->set_animation(clips[index]);
        const godot::String node_name = name + godot::String("_") + clips[index];
        tree->add_node(node_name, clip);
        tree->connect_node(name, index, node_name);
    }
    return selector;
}

} // namespace

bool CharacterAnimator::setup(godot::Node* owner, godot::Node3D* model,
                              const godot::Ref<godot::AnimationLibrary>& library) {
    if (owner == nullptr || model == nullptr || library.is_null()) {
        return false;
    }
    const godot::TypedArray<godot::Node> skeletons = model->find_children("*", "Skeleton3D", true, false);
    for (const godot::Variant& node : skeletons) {
        skeleton_ = godot::Object::cast_to<godot::Skeleton3D>(node);
        if (skeleton_ != nullptr) {
            break;
        }
    }
    if (skeleton_ == nullptr) {
        logger().error("CharacterAnimator: the model has no Skeleton3D; animation is disabled");
        return false;
    }
    library_ = library;

    godot::PackedStringArray clips;
    const godot::TypedArray<godot::StringName> names = library->get_animation_list();
    for (const godot::Variant& name : names) {
        clips.push_back(godot::String(name));
    }
    clips.sort(); // deterministic input order

    godot::Ref<godot::AnimationNodeBlendTree> root;
    root.instantiate();
    add_selector(root.ptr(), "base", clips, library, false);
    add_selector(root.ptr(), "upper", clips, library, true);

    godot::Ref<godot::AnimationNodeTimeScale> speed;
    speed.instantiate();
    root->add_node("base_speed", speed);
    root->connect_node("base_speed", 0, "base");

    godot::Ref<godot::AnimationNodeBlend2> mix;
    mix.instantiate();
    mix->set_filter_enabled(true);
    // Track paths are relative to the model scene; the importer names the skeleton uniquely.
    const godot::String skeleton_path = godot::String("%") + godot::String(skeleton_->get_name()) + godot::String(":");
    for (std::int32_t bone = 0; bone < skeleton_->get_bone_count(); ++bone) {
        const godot::String bone_name = skeleton_->get_bone_name(bone);
        if (!is_lower_body(bone_name)) {
            mix->set_filter_path(godot::NodePath(skeleton_path + bone_name), true);
        }
    }
    root->add_node("mix", mix);
    root->connect_node("mix", 0, "base_speed");
    root->connect_node("mix", 1, "upper");
    root->connect_node("output", 0, "mix");

    tree_ = memnew(godot::AnimationTree);
    owner->add_child(tree_);
    tree_->set_root_node(tree_->get_path_to(model));
    tree_->add_animation_library(godot::StringName(), library);
    tree_->set_tree_root(root);
    tree_->set("parameters/mix/blend_amount", 0.0F);
    tree_->set_active(true);
    return true;
}

bool CharacterAnimator::has_clip(const godot::StringName& clip) const {
    return library_.is_valid() && library_->has_animation(clip);
}

float CharacterAnimator::clip_length(const godot::StringName& clip) const {
    return has_clip(clip) ? static_cast<float>(library_->get_animation(clip)->get_length()) : 0.0F;
}

void CharacterAnimator::set_base(const godot::StringName& clip, float playback_scale) {
    if (tree_ == nullptr) {
        return;
    }
    if (clip != base_clip_) {
        tree_->set("parameters/base/transition_request", godot::String(clip));
        base_clip_ = clip;
    }
    tree_->set("parameters/base_speed/scale", std::clamp(playback_scale, min_playback_scale, max_playback_scale));
}

void CharacterAnimator::set_upper(const godot::StringName& clip) {
    if (tree_ == nullptr || clip == upper_clip_) {
        return;
    }
    upper_clip_ = clip;
    if (!clip.is_empty()) {
        tree_->set("parameters/upper/transition_request", godot::String(clip));
    }
}

void CharacterAnimator::update(float delta_seconds) {
    if (tree_ == nullptr) {
        return;
    }
    const float target = upper_clip_.is_empty() ? 0.0F : 1.0F;
    const float step = upper_fade_rate * delta_seconds;
    upper_blend_ += std::clamp(target - upper_blend_, -step, step);
    tree_->set("parameters/mix/blend_amount", upper_blend_);
}

} // namespace e5::bridge

#include "bird.hpp"

#include "combat.hpp"
#include "e5/core/profiling.hpp"
#include "e5/gameplay/skills.hpp"
#include "effect.hpp"
#include "enemy.hpp"
#include "godot_log.hpp"
#include "target.hpp"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

namespace e5::bridge {
namespace {

constexpr float full_turn = std::numbers::pi_v<float> * 2.0F;
// Each wing segment further out swings a little later and a little less: the wing whips instead of hinging.
constexpr float segment_lag = 0.9F; // radians
constexpr float segment_share = 0.55F;
constexpr float folded_wing_angle = 1.25F;  // radians the wing root hangs down when folded
constexpr float perched_flap_rate = 2.2F;   // beats per second while sitting
constexpr float perched_flap_angle = 0.28F; // radians // of the previous segment's angle

gameplay::Vec3 to_plain(const godot::Vector3& v) {
    return {.x = static_cast<float>(v.x), .y = static_cast<float>(v.y), .z = static_cast<float>(v.z)};
}

godot::Vector3 to_godot(const gameplay::Vec3& v) {
    return {v.x, v.y, v.z};
}

} // namespace

void E5Bird::_bind_methods() {
    using godot::ClassDB;
    using godot::D_METHOD;
    using godot::PropertyInfo;

    ClassDB::bind_method(D_METHOD("set_left_wing_bones", "bones"), &E5Bird::set_left_wing_bones);
    ClassDB::bind_method(D_METHOD("get_left_wing_bones"), &E5Bird::get_left_wing_bones);
    ClassDB::bind_method(D_METHOD("set_right_wing_bones", "bones"), &E5Bird::set_right_wing_bones);
    ClassDB::bind_method(D_METHOD("get_right_wing_bones"), &E5Bird::get_right_wing_bones);
    ClassDB::bind_method(D_METHOD("set_flap_rate", "beats_per_second"), &E5Bird::set_flap_rate);
    ClassDB::bind_method(D_METHOD("get_flap_rate"), &E5Bird::get_flap_rate);
    ClassDB::bind_method(D_METHOD("set_flap_angle", "radians"), &E5Bird::set_flap_angle);
    ClassDB::bind_method(D_METHOD("get_flap_angle"), &E5Bird::get_flap_angle);
    ClassDB::bind_method(D_METHOD("set_attack_range", "metres"), &E5Bird::set_attack_range);
    ClassDB::bind_method(D_METHOD("get_attack_range"), &E5Bird::get_attack_range);
    ClassDB::bind_method(D_METHOD("set_hit_effect", "scene"), &E5Bird::set_hit_effect);
    ClassDB::bind_method(D_METHOD("get_hit_effect"), &E5Bird::get_hit_effect);
    ClassDB::bind_method(D_METHOD("set_vanish_effect", "scene"), &E5Bird::set_vanish_effect);
    ClassDB::bind_method(D_METHOD("get_vanish_effect"), &E5Bird::get_vanish_effect);

    ADD_PROPERTY(PropertyInfo(godot::Variant::PACKED_STRING_ARRAY, "left_wing_bones"), "set_left_wing_bones",
                 "get_left_wing_bones");
    ADD_PROPERTY(PropertyInfo(godot::Variant::PACKED_STRING_ARRAY, "right_wing_bones"), "set_right_wing_bones",
                 "get_right_wing_bones");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "flap_rate", godot::PROPERTY_HINT_RANGE, "0.5,30,0.1,suffix:Hz"),
                 "set_flap_rate", "get_flap_rate");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "flap_angle", godot::PROPERTY_HINT_RANGE, "0,1.5,0.01,radians"),
                 "set_flap_angle", "get_flap_angle");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "attack_range", godot::PROPERTY_HINT_RANGE, "1,200,1,suffix:m"),
                 "set_attack_range", "get_attack_range");
    ADD_PROPERTY(PropertyInfo(godot::Variant::OBJECT, "hit_effect", godot::PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"),
                 "set_hit_effect", "get_hit_effect");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::OBJECT, "vanish_effect", godot::PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"),
        "set_vanish_effect", "get_vanish_effect");
}

void E5Bird::_ready() {
    add_to_group(group_name);
    const godot::TypedArray<godot::Node> skeletons = find_children("*", "Skeleton3D", true, false);
    for (const godot::Variant& node : skeletons) {
        skeleton_ = godot::Object::cast_to<godot::Skeleton3D>(node);
        if (skeleton_ != nullptr) {
            break;
        }
    }
    if (skeleton_ != nullptr) {
        collect_wing(left_wing_bones_, 1.0F);
        collect_wing(right_wing_bones_, -1.0F);
    }
    if (wing_bones_.empty()) {
        logger().warn("E5Bird '{}': no wing bones found; it will fly without flapping",
                      godot::String(get_name()).utf8().get_data());
    }
    set_physics_process(launched_);
}

void E5Bird::collect_wing(const godot::PackedStringArray& names, float direction) {
    float lag = 0.0F;
    float share = 1.0F;
    for (const godot::String& name : names) {
        const std::int32_t bone = skeleton_->find_bone(name);
        if (bone < 0) {
            logger().warn("E5Bird: the skeleton has no bone named '{}'", name.utf8().get_data());
            continue;
        }
        const std::int32_t parent = skeleton_->get_bone_parent(bone);
        wing_bones_.push_back(
            {.bone = bone,
             .rest = skeleton_->get_bone_rest(bone).basis.get_rotation_quaternion(),
             .parent_rest = parent >= 0 ? skeleton_->get_bone_global_rest(parent).basis.get_rotation_quaternion()
                                        : godot::Quaternion(),
             .direction = direction,
             .lag = lag,
             .share = share});
        lag += segment_lag;
        share *= segment_share;
    }
}

void E5Bird::perch() {
    perched_ = true;
    wing_spread_ = 0.0F;
}

void E5Bird::flap(float delta) {
    const float rate = perched_ ? perched_flap_rate : flap_rate_;
    flap_phase_ = std::fmod(flap_phase_ + delta * rate * full_turn, full_turn);
    for (const WingBone& wing : wing_bones_) {
        // The left wing tip rises by turning about the model's front axis (+Z); the right wing mirrors it.
        float angle = wing.direction * wing.share * flap_angle_ * std::sin(flap_phase_ - wing.lag);
        if (perched_) {
            // Folded, the wings hang down along the body; as they open, a gentle beat sets in.
            const float spread = std::clamp(wing_spread_, 0.0F, 1.0F);
            angle =
                wing.direction * wing.share *
                (-folded_wing_angle * (1.0F - spread) + perched_flap_angle * spread * std::sin(flap_phase_ - wing.lag));
        }
        const godot::Quaternion swing(godot::Vector3(0.0F, 0.0F, 1.0F), angle);
        // The swing is given in skeleton space; bring it into the space the bone's rotation is expressed in.
        const godot::Quaternion local_swing = wing.parent_rest.inverse() * swing * wing.parent_rest;
        skeleton_->set_bone_pose_rotation(wing.bone, (local_swing * wing.rest).normalized());
    }
}

void E5Bird::_process(double delta) {
    if (skeleton_ != nullptr) {
        flap(static_cast<float>(delta));
    }
}

void E5Bird::launch(const godot::Vector3& position, const godot::Vector3& velocity, float side, int index,
                    const godot::Node3D* summoner) {
    state_ = {.position = to_plain(position), .velocity = to_plain(velocity), .side = side};
    index_ = index;
    // Birds released together must not beat their wings in step.
    flap_phase_ = static_cast<float>(index) * 1.7F;
    home_ = position;
    if (summoner != nullptr) {
        summoner_id_ = summoner->get_instance_id();
        home_ = summoner->get_global_position();
    }
    launched_ = true;
    perched_ = false;
    set_global_position(position);
    set_physics_process(true);
}

namespace {

// Where a bird aims on the thing it attacks; false if it can no longer be attacked.
bool attack_point(godot::Object* object, godot::Vector3& point) {
    if (const auto* const enemy = godot::Object::cast_to<E5Enemy>(object)) {
        point = enemy->get_aim_point();
        return enemy->is_alive();
    }
    if (const auto* const target = godot::Object::cast_to<E5Target>(object)) {
        point = target->get_global_position();
        return true;
    }
    return false;
}

} // namespace

bool E5Bird::find_target(const godot::Vector3& home, godot::Vector3& target_position) {
    if (attack_point(godot::ObjectDB::get_instance(target_id_), target_position)) {
        return true;
    }
    // Living enemies in range come first; practice targets only when there are none.
    // Nearest to the summoner first, and bird number `index` takes the matching
    // entry, so a flock spreads over its prey instead of mobbing one.
    std::vector<std::pair<float, godot::Node*>> in_range;
    for (const char* const group : {E5Enemy::group_name, E5Target::group_name}) {
        const godot::TypedArray<godot::Node> candidates = get_tree()->get_nodes_in_group(group);
        for (const godot::Variant& node : candidates) {
            auto* const candidate = godot::Object::cast_to<godot::Node>(node);
            godot::Vector3 point;
            if (candidate != nullptr && attack_point(candidate, point)) {
                const auto distance = static_cast<float>(point.distance_to(home));
                if (distance <= attack_range_) {
                    in_range.emplace_back(distance, candidate);
                }
            }
        }
        if (!in_range.empty()) {
            break;
        }
    }
    if (in_range.empty()) {
        target_id_ = 0;
        return false;
    }
    std::ranges::sort(in_range, {}, &std::pair<float, godot::Node*>::first);
    godot::Node* const chosen = in_range.at(static_cast<std::size_t>(index_) % in_range.size()).second;
    target_id_ = chosen->get_instance_id();
    return attack_point(chosen, target_position);
}

void E5Bird::_physics_process(double delta) {
    E5_PROFILE_SCOPE("E5Bird::_physics_process");

    if (const auto* const summoner =
            godot::Object::cast_to<godot::Node3D>(godot::ObjectDB::get_instance(summoner_id_))) {
        home_ = summoner->get_global_position();
    }
    godot::Vector3 target_position;
    const bool has_target = find_target(home_, target_position);

    const gameplay::BirdStep step = gameplay::step_bird(state_, has_target, to_plain(target_position), to_plain(home_),
                                                        params_, static_cast<float>(delta));
    state_ = step.state;

    const godot::Vector3 position = to_godot(state_.position);
    const godot::Vector3 heading = to_godot(state_.velocity).normalized();
    const godot::Vector3 up =
        std::abs(heading.y) > 0.99F ? godot::Vector3(1.0F, 0.0F, 0.0F) : godot::Vector3(0.0F, 1.0F, 0.0F);
    // The model's front is +Z.
    set_global_transform(godot::Transform3D(godot::Basis::looking_at(heading, up, true), position));

    if (step.hit) {
        E5Effect::spawn(hit_effect_, get_parent(), target_position);
        combat::hit(godot::ObjectDB::get_instance(target_id_), target_position,
                    gameplay::skill_damage(gameplay::SkillId::Kingfishers));
    }
    if (step.finished) {
        E5Effect::spawn(vanish_effect_, get_parent(), position);
        queue_free();
    }
}

} // namespace e5::bridge
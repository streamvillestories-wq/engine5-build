#include "enemy.hpp"

#include "arrow.hpp"
#include "e5/core/profiling.hpp"
#include "e5/gameplay/character_motor.hpp"
#include "e5/gameplay/inventory.hpp"
#include "effect.hpp"
#include "godot_log.hpp"
#include "pickup.hpp"
#include "player_controller.hpp"
#include "spell_bolt.hpp"
#include "terrain.hpp"

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cmath>
#include <format>
#include <string>

namespace e5::bridge {
namespace {

constexpr float bar_width = 0.8F;         // metres
constexpr float bar_height = 0.09F;       // metres
constexpr float bar_above_head = 0.22F;   // metres
constexpr float number_seconds = 0.9F;    // how long a damage number lives
constexpr float number_rise_speed = 0.9F; // m/s
constexpr float walk_clip_speed = 1.6F;   // m/s the walk clip was authored for

} // namespace

namespace {
constexpr float fall_tumble = 2.5F;         // rad/s a flying enemy tips over while it drops
constexpr float circle_share = 0.45F;       // of its speed: the sideways drift round the player
constexpr float circle_turn_seconds = 6.0F; // it changes direction this often
constexpr float fly_ease = 3.0F;            // 1/s: how quickly a flyer takes up a new velocity
constexpr float bob_rate = 1.7F;            // rad/s
constexpr float bob_height = 0.12F;         // metres
constexpr float hover_stiffness = 4.0F;     // 1/s: how firmly it is held at its height
constexpr float cast_height_share = 0.6F;   // of its height: where its spells leave
constexpr float cast_reach = 0.35F;         // metres in front of its body
constexpr float player_chest_height = 1.0F; // metres above the player's feet: what it aims at
constexpr float blow_at_share = 0.45F;      // of the attack's length: when the fist arrives
constexpr float blow_reach_share = 1.35F;   // of the attack range: how far the blow still reaches
} // namespace

void E5Enemy::_bind_methods() {
    using godot::ClassDB;
    using godot::D_METHOD;
    using godot::PropertyInfo;

    ClassDB::bind_method(D_METHOD("set_animation_library", "library"), &E5Enemy::set_animation_library);
    ClassDB::bind_method(D_METHOD("get_animation_library"), &E5Enemy::get_animation_library);
    ClassDB::bind_method(D_METHOD("set_health_bar_shader", "shader"), &E5Enemy::set_health_bar_shader);
    ClassDB::bind_method(D_METHOD("get_health_bar_shader"), &E5Enemy::get_health_bar_shader);
    ClassDB::bind_method(D_METHOD("set_hit_effect", "scene"), &E5Enemy::set_hit_effect);
    ClassDB::bind_method(D_METHOD("get_hit_effect"), &E5Enemy::get_hit_effect);
    ClassDB::bind_method(D_METHOD("set_max_health", "health"), &E5Enemy::set_max_health);
    ClassDB::bind_method(D_METHOD("get_max_health"), &E5Enemy::get_max_health);
    ClassDB::bind_method(D_METHOD("set_move_speed", "speed"), &E5Enemy::set_move_speed);
    ClassDB::bind_method(D_METHOD("get_move_speed"), &E5Enemy::get_move_speed);
    ClassDB::bind_method(D_METHOD("set_aggro_range", "metres"), &E5Enemy::set_aggro_range);
    ClassDB::bind_method(D_METHOD("get_aggro_range"), &E5Enemy::get_aggro_range);
    ClassDB::bind_method(D_METHOD("set_attack_range", "metres"), &E5Enemy::set_attack_range);
    ClassDB::bind_method(D_METHOD("get_attack_range"), &E5Enemy::get_attack_range);
    ClassDB::bind_method(D_METHOD("set_respawn_seconds", "seconds"), &E5Enemy::set_respawn_seconds);
    ClassDB::bind_method(D_METHOD("get_respawn_seconds"), &E5Enemy::get_respawn_seconds);
    ClassDB::bind_method(D_METHOD("set_body_height", "metres"), &E5Enemy::set_body_height);
    ClassDB::bind_method(D_METHOD("get_body_height"), &E5Enemy::get_body_height);
    ClassDB::bind_method(D_METHOD("set_cast_range", "metres"), &E5Enemy::set_cast_range);
    ClassDB::bind_method(D_METHOD("get_cast_range"), &E5Enemy::get_cast_range);
    ClassDB::bind_method(D_METHOD("set_cast_damage", "damage"), &E5Enemy::set_cast_damage);
    ClassDB::bind_method(D_METHOD("get_cast_damage"), &E5Enemy::get_cast_damage);
    ClassDB::bind_method(D_METHOD("set_cast_cooldown", "seconds"), &E5Enemy::set_cast_cooldown);
    ClassDB::bind_method(D_METHOD("get_cast_cooldown"), &E5Enemy::get_cast_cooldown);
    ClassDB::bind_method(D_METHOD("set_cast_effect", "effect"), &E5Enemy::set_cast_effect);
    ClassDB::bind_method(D_METHOD("get_cast_effect"), &E5Enemy::get_cast_effect);
    ClassDB::bind_method(D_METHOD("set_bolt_trail", "effect"), &E5Enemy::set_bolt_trail);
    ClassDB::bind_method(D_METHOD("get_bolt_trail"), &E5Enemy::get_bolt_trail);
    ClassDB::bind_method(D_METHOD("set_bolt_impact", "effect"), &E5Enemy::set_bolt_impact);
    ClassDB::bind_method(D_METHOD("get_bolt_impact"), &E5Enemy::get_bolt_impact);
    ClassDB::bind_method(D_METHOD("set_hover_height", "metres"), &E5Enemy::set_hover_height);
    ClassDB::bind_method(D_METHOD("get_hover_height"), &E5Enemy::get_hover_height);
    ClassDB::bind_method(D_METHOD("set_keep_distance", "metres"), &E5Enemy::set_keep_distance);
    ClassDB::bind_method(D_METHOD("get_keep_distance"), &E5Enemy::get_keep_distance);
    ClassDB::bind_method(D_METHOD("set_cast_min_range", "metres"), &E5Enemy::set_cast_min_range);
    ClassDB::bind_method(D_METHOD("get_cast_min_range"), &E5Enemy::get_cast_min_range);
    ClassDB::bind_method(D_METHOD("set_trail_effect", "effect"), &E5Enemy::set_trail_effect);
    ClassDB::bind_method(D_METHOD("get_trail_effect"), &E5Enemy::get_trail_effect);
    ClassDB::bind_method(D_METHOD("set_blow_effect", "effect"), &E5Enemy::set_blow_effect);
    ClassDB::bind_method(D_METHOD("get_blow_effect"), &E5Enemy::get_blow_effect);
    ClassDB::bind_method(D_METHOD("set_stagger_damage", "damage"), &E5Enemy::set_stagger_damage);
    ClassDB::bind_method(D_METHOD("get_stagger_damage"), &E5Enemy::get_stagger_damage);
    ClassDB::bind_method(D_METHOD("set_bolt_speed", "speed"), &E5Enemy::set_bolt_speed);
    ClassDB::bind_method(D_METHOD("get_bolt_speed"), &E5Enemy::get_bolt_speed);
    ClassDB::bind_method(D_METHOD("set_cast_seconds", "seconds"), &E5Enemy::set_cast_seconds);
    ClassDB::bind_method(D_METHOD("get_cast_seconds"), &E5Enemy::get_cast_seconds);
    ClassDB::bind_method(D_METHOD("set_cast_release_share", "share"), &E5Enemy::set_cast_release_share);
    ClassDB::bind_method(D_METHOD("get_cast_release_share"), &E5Enemy::get_cast_release_share);
    ClassDB::bind_method(D_METHOD("set_attack_seconds", "seconds"), &E5Enemy::set_attack_seconds);
    ClassDB::bind_method(D_METHOD("get_attack_seconds"), &E5Enemy::get_attack_seconds);
    ClassDB::bind_method(D_METHOD("set_attack_damage", "damage"), &E5Enemy::set_attack_damage);
    ClassDB::bind_method(D_METHOD("get_attack_damage"), &E5Enemy::get_attack_damage);
    ClassDB::bind_method(D_METHOD("set_body_radius", "metres"), &E5Enemy::set_body_radius);
    ClassDB::bind_method(D_METHOD("get_body_radius"), &E5Enemy::get_body_radius);
    ClassDB::bind_method(D_METHOD("take_damage", "amount", "position"), &E5Enemy::take_damage);
    ClassDB::bind_method(D_METHOD("get_health"), &E5Enemy::get_health);
    ClassDB::bind_method(D_METHOD("is_alive"), &E5Enemy::is_alive);

    ADD_PROPERTY(PropertyInfo(godot::Variant::OBJECT, "animation_library", godot::PROPERTY_HINT_RESOURCE_TYPE,
                              "AnimationLibrary"),
                 "set_animation_library", "get_animation_library");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::OBJECT, "health_bar_shader", godot::PROPERTY_HINT_RESOURCE_TYPE, "Shader"),
        "set_health_bar_shader", "get_health_bar_shader");
    ADD_PROPERTY(PropertyInfo(godot::Variant::OBJECT, "hit_effect", godot::PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"),
                 "set_hit_effect", "get_hit_effect");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "max_health", godot::PROPERTY_HINT_RANGE, "1,10000,1"),
                 "set_max_health", "get_max_health");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "move_speed", godot::PROPERTY_HINT_RANGE, "0,20,0.1,suffix:m/s"),
                 "set_move_speed", "get_move_speed");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "aggro_range", godot::PROPERTY_HINT_RANGE, "0,100,0.5,suffix:m"),
                 "set_aggro_range", "get_aggro_range");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "attack_range", godot::PROPERTY_HINT_RANGE, "0.2,10,0.1,suffix:m"),
                 "set_attack_range", "get_attack_range");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "respawn_seconds", godot::PROPERTY_HINT_RANGE, "0,120,0.5,suffix:s"),
        "set_respawn_seconds", "get_respawn_seconds");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "body_height", godot::PROPERTY_HINT_RANGE, "0.2,10,0.05,suffix:m"),
                 "set_body_height", "get_body_height");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "cast_range", godot::PROPERTY_HINT_RANGE, "0,40,0.5"),
                 "set_cast_range", "get_cast_range");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "cast_damage", godot::PROPERTY_HINT_RANGE, "0,200,0.5"),
                 "set_cast_damage", "get_cast_damage");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "cast_cooldown", godot::PROPERTY_HINT_RANGE, "0.5,30,0.1"),
                 "set_cast_cooldown", "get_cast_cooldown");
    ADD_PROPERTY(PropertyInfo(godot::Variant::OBJECT, "cast_effect", godot::PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"),
                 "set_cast_effect", "get_cast_effect");
    ADD_PROPERTY(PropertyInfo(godot::Variant::OBJECT, "bolt_trail", godot::PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"),
                 "set_bolt_trail", "get_bolt_trail");
    ADD_PROPERTY(PropertyInfo(godot::Variant::OBJECT, "bolt_impact", godot::PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"),
                 "set_bolt_impact", "get_bolt_impact");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "hover_height", godot::PROPERTY_HINT_RANGE, "0,20,0.1"),
                 "set_hover_height", "get_hover_height");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "keep_distance", godot::PROPERTY_HINT_RANGE, "0,40,0.5"),
                 "set_keep_distance", "get_keep_distance");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "cast_min_range", godot::PROPERTY_HINT_RANGE, "0,40,0.5"),
                 "set_cast_min_range", "get_cast_min_range");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::OBJECT, "trail_effect", godot::PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"),
        "set_trail_effect", "get_trail_effect");
    ADD_PROPERTY(PropertyInfo(godot::Variant::OBJECT, "blow_effect", godot::PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"),
                 "set_blow_effect", "get_blow_effect");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "stagger_damage", godot::PROPERTY_HINT_RANGE, "1,500,1"),
                 "set_stagger_damage", "get_stagger_damage");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "bolt_speed", godot::PROPERTY_HINT_RANGE, "1,80,0.5"),
                 "set_bolt_speed", "get_bolt_speed");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "cast_seconds", godot::PROPERTY_HINT_RANGE, "0.2,6,0.05"),
                 "set_cast_seconds", "get_cast_seconds");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "cast_release_share", godot::PROPERTY_HINT_RANGE, "0.05,1,0.01"),
                 "set_cast_release_share", "get_cast_release_share");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "attack_seconds", godot::PROPERTY_HINT_RANGE, "0.2,5,0.05"),
                 "set_attack_seconds", "get_attack_seconds");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "attack_damage", godot::PROPERTY_HINT_RANGE, "0,200,0.5"),
                 "set_attack_damage", "get_attack_damage");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "body_radius", godot::PROPERTY_HINT_RANGE, "0.05,5,0.05,suffix:m"),
                 "set_body_radius", "get_body_radius");
}

void E5Enemy::_ready() {
    add_to_group(group_name);
    clip_idle_ = godot::StringName("idle");
    clip_walk_ = godot::StringName("walk");
    clip_attack_ = godot::StringName("attack");
    clip_cast_ = godot::StringName("cast");
    clip_hit_ = godot::StringName("hit");
    clip_death_ = godot::StringName("death");

    // On an island the scene only says where it stands, not how high the ground is there.
    if (const auto* const terrain =
            godot::Object::cast_to<E5Terrain>(get_tree()->get_first_node_in_group(E5Terrain::group_name))) {
        godot::Vector3 position = get_global_position();
        position.y = terrain->height_at(static_cast<float>(position.x), static_cast<float>(position.z));
        set_global_position(position);
    }
    if (hover_height_ > 0.0F) {
        // It starts in the air, and every one bobs and circles on its own beat.
        godot::Vector3 position = get_global_position();
        position.y = ground_height() + hover_height_;
        set_global_position(position);
        hover_seconds_ = static_cast<float>(godot::UtilityFunctions::randf()) * 10.0F;
        circle_side_ = godot::UtilityFunctions::randf() < 0.5 ? -1.0F : 1.0F;
    }
    if (trail_effect_.is_valid()) {
        trail_ = godot::Object::cast_to<godot::Node3D>(trail_effect_->instantiate());
        if (trail_ != nullptr) {
            add_child(trail_);
            trail_->set_position(godot::Vector3(0.0F, body_height_ * 0.5F, 0.0F));
        }
    }
    spawn_transform_ = get_global_transform();
    collision_layer_ = get_collision_layer();
    model_yaw_ = static_cast<float>(get_rotation().y);
    state_ = gameplay::spawn_enemy(params_);

    const std::string name = godot::String(get_name()).utf8().get_data();
    model_ = get_node<godot::Node3D>(godot::NodePath("Model"));
    if (model_ == nullptr) {
        logger().warn("E5Enemy '{}' has no Node3D child named 'Model'; nothing will be shown", name);
    } else if (animation_library_.is_valid()) {
        bool complete = true;
        for (const godot::StringName& clip : {clip_idle_, clip_walk_, clip_attack_, clip_hit_, clip_death_}) {
            if (!animation_library_->has_animation(clip)) {
                logger().error("E5Enemy '{}': the animation library is missing the '{}' clip; it will not be animated",
                               name, godot::String(clip).utf8().get_data());
                complete = false;
            }
        }
        if (complete && animator_.setup(this, model_, animation_library_)) {
            animator_.set_base(clip_idle_, 1.0F);
            // The behaviour follows the clips, so a blow lands when the animation says so.
            params_.attack_seconds = animator_.clip_length(clip_attack_);
            params_.hit_seconds = std::min(animator_.clip_length(clip_hit_), 0.6F);
        }
    }
    build_health_bar();
}

void E5Enemy::build_health_bar() {
    if (health_bar_shader_.is_null()) {
        logger().warn("E5Enemy '{}' has no health bar shader; no bar is shown",
                      godot::String(get_name()).utf8().get_data());
        return;
    }
    godot::Ref<godot::ShaderMaterial> material;
    material.instantiate();
    material->set_shader(health_bar_shader_);
    godot::Ref<godot::QuadMesh> quad;
    quad.instantiate();
    quad->set_size(godot::Vector2(bar_width, bar_height));
    quad->set_material(material);

    health_bar_ = memnew(godot::MeshInstance3D);
    health_bar_->set_mesh(quad);
    health_bar_->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
    health_bar_->set_position(godot::Vector3(0.0F, body_height_ + bar_above_head, 0.0F));
    add_child(health_bar_);
    update_health_bar();
}

void E5Enemy::update_health_bar() {
    if (health_bar_ == nullptr) {
        return;
    }
    health_bar_->set_visible(is_alive());
    health_bar_->set_instance_shader_parameter("fraction", state_.health / std::max(params_.max_health, 1.0F));
}

godot::Vector3 E5Enemy::get_aim_point() const {
    return get_global_position() + godot::Vector3(0.0F, body_height_ * 0.55F, 0.0F);
}

void E5Enemy::take_damage(float amount, const godot::Vector3& position) {
    if (!is_alive() || amount <= 0.0F) {
        return;
    }
    // Collected here and applied in the next physics step, so the order of
    // hits within one step cannot change the outcome.
    pending_damage_ += amount;
    heaviest_pending_blow_ = std::max(heaviest_pending_blow_, amount);
    damage_taken_ += amount;
    show_damage_number(amount, position);
    E5Effect::spawn(hit_effect_, get_parent(), position);
}

void E5Enemy::set_held(bool held) {
    held_ = held;
    set_collision_layer(held || !is_alive() ? 0 : collision_layer_);
    set_velocity(godot::Vector3());
    if (held && animator_.is_ready()) {
        animator_.set_base(clip_hit_, 1.0F);
    }
}

void E5Enemy::set_shrink(float scale) {
    if (model_ != nullptr) {
        model_->set_scale(godot::Vector3(scale, scale, scale));
    }
}

void E5Enemy::spin(float radians) {
    model_yaw_ += radians;
    if (model_ != nullptr) {
        model_->set_rotation(godot::Vector3(0.0F, model_yaw_, 0.0F));
    }
}

void E5Enemy::fling(const godot::Vector3& velocity) {
    flung_ = true;
    set_velocity(velocity);
}

void E5Enemy::attach(godot::Node3D* stuck) {
    if (stuck != nullptr) {
        // The model turns to face the player; the body node itself does not.
        stuck->reparent(model_ != nullptr ? static_cast<godot::Node*>(model_) : this, true);
    }
}

void E5Enemy::show_damage_number(float amount, const godot::Vector3& position) {
    auto* const label = memnew(godot::Label3D);
    label->set_text(godot::String::utf8(std::format("{:.0f}", amount).c_str()));
    label->set_billboard_mode(godot::BaseMaterial3D::BILLBOARD_ENABLED);
    label->set_draw_flag(godot::Label3D::FLAG_DISABLE_DEPTH_TEST, true);
    label->set_font_size(56);
    label->set_pixel_size(0.004F);
    label->set_outline_size(14);
    label->set_modulate(godot::Color(1.0F, 0.9F, 0.35F));
    // In the world, not on the body: the number stays where the blow landed.
    get_parent()->add_child(label);
    label->set_global_position(position + godot::Vector3(0.0F, 0.25F, 0.0F));
    damage_numbers_.push_back({.label_id = label->get_instance_id()});
}

void E5Enemy::_process(double delta) {
    for (DamageNumber& number : damage_numbers_) {
        number.age += static_cast<float>(delta);
        auto* const label = godot::Object::cast_to<godot::Label3D>(godot::ObjectDB::get_instance(number.label_id));
        if (label == nullptr) {
            number.age = number_seconds;
            continue;
        }
        if (number.age >= number_seconds) {
            label->queue_free();
            continue;
        }
        label->translate(godot::Vector3(0.0F, number_rise_speed * static_cast<float>(delta), 0.0F));
        const float fade = 1.0F - number.age / number_seconds;
        label->set_modulate(godot::Color(1.0F, 0.9F, 0.35F, fade));
        label->set_outline_modulate(godot::Color(0.0F, 0.0F, 0.0F, fade));
    }
    std::erase_if(damage_numbers_, [](const DamageNumber& number) { return number.age >= number_seconds; });
}

// The blow lands part-way through the attack, and only on a player who is still in reach
// of an enemy that is still attacking: stepping back or staggering it avoids it.
void E5Enemy::update_blow(bool attack_started, float dt, float distance_to_player) {
    if (attack_started) {
        blow_in_seconds_ = params_.attack_seconds * blow_at_share;
        return;
    }
    if (blow_in_seconds_ < 0.0F) {
        return;
    }
    blow_in_seconds_ -= dt;
    if (state_.phase != gameplay::EnemyPhase::Attack) {
        blow_in_seconds_ = -1.0F;
        return;
    }
    if (blow_in_seconds_ >= 0.0F) {
        return;
    }
    // The blow comes down now, whether it finds anyone or not.
    if (blow_effect_.is_valid()) {
        const godot::Vector3 forward(std::sin(model_yaw_), 0.0F, std::cos(model_yaw_));
        E5Effect::spawn(blow_effect_, get_parent(), get_global_position() + forward * (params_.attack_range * 0.6F));
    }
    if (distance_to_player <= params_.attack_range * blow_reach_share) {
        auto* const target = godot::Object::cast_to<E5PlayerController>(
            get_tree()->get_first_node_in_group(E5PlayerController::group_name));
        if (target != nullptr) {
            target->take_damage(attack_damage_);
        }
    }
}

// Dead: a flyer drops to the ground; then it waits to come back.
void E5Enemy::update_dead(float dt) {
    if (hover_height_ > 0.0F && !is_on_floor()) {
        // What held it up is gone: it drops, and tips over as it falls.
        godot::Vector3 velocity = get_velocity();
        velocity.y -= gravity_ * dt;
        set_velocity(velocity);
        move_and_slide();
        if (model_ != nullptr) {
            model_->rotate_x(-fall_tumble * dt);
        }
    }
    state_.phase_seconds += dt;
    if (respawn_seconds_ > 0.0F && state_.phase_seconds >= respawn_seconds_) {
        respawn();
    }
}

void E5Enemy::walk(bool moving, const godot::Vector3& direction, float dt) {
    godot::Vector3 velocity = get_velocity();
    velocity.x = moving ? direction.x * move_speed_ : 0.0F;
    velocity.z = moving ? direction.z * move_speed_ : 0.0F;
    velocity.y = is_on_floor() ? 0.0F : velocity.y - gravity_ * dt;
    set_velocity(velocity);
    move_and_slide();
}

void E5Enemy::_physics_process(double delta) {
    E5_PROFILE_SCOPE("E5Enemy::_physics_process");
    const auto dt = static_cast<float>(delta);

    if (held_) {
        animator_.update(dt);
        return; // the holder moves it
    }
    if (flung_) {
        // In the air until it lands; then it carries on, and takes the damage it got meanwhile.
        godot::Vector3 velocity = get_velocity();
        velocity.y -= gravity_ * dt;
        set_velocity(velocity);
        move_and_slide();
        if (is_on_floor() && velocity.y <= 0.0F) {
            flung_ = false;
            set_velocity(godot::Vector3());
        }
        animator_.update(dt);
        return;
    }
    if (!is_alive()) {
        update_dead(dt);
        return;
    }

    const auto* const player =
        godot::Object::cast_to<godot::Node3D>(get_tree()->get_first_node_in_group(E5PlayerController::group_name));
    godot::Vector3 to_player;
    if (player != nullptr) {
        to_player = player->get_global_position() - get_global_position();
        to_player.y = 0.0F;
    }
    const gameplay::EnemyInput input{
        .has_player = player != nullptr,
        .distance_to_player = static_cast<float>(to_player.length()),
        .damage = pending_damage_,
        .heaviest_blow = heaviest_pending_blow_,
    };
    pending_damage_ = 0.0F;
    heaviest_pending_blow_ = 0.0F;

    const gameplay::EnemyStep step = gameplay::step_enemy(state_, input, params_, dt);
    state_ = step.state;
    update_health_bar();
    if (step.died) {
        die();
        return;
    }
    update_blow(step.attack_started, dt, static_cast<float>(to_player.length()));
    if (step.cast_started) {
        // The glow gathering in front of it is the warning: time to step aside, or to strike first.
        E5Effect::spawn(cast_effect_, this, cast_origin());
    }
    if (step.cast_released && player != nullptr) {
        throw_bolt(player->get_global_position());
    }

    const godot::Vector3 direction = to_player.length() > 0.01F ? to_player.normalized() : godot::Vector3();
    if (hover_height_ > 0.0F) {
        fly(step, to_player, dt);
    } else {
        walk(step.moving, direction, dt);
    }

    // Once it has noticed the player it keeps facing her.
    if (state_.aggro && model_ != nullptr && to_player.length() > 0.01F) {
        const float wanted = gameplay::facing_yaw(static_cast<float>(direction.x), static_cast<float>(direction.z));
        model_yaw_ = gameplay::turn_toward(model_yaw_, wanted, turn_speed_ * dt);
        model_->set_rotation(godot::Vector3(0.0F, model_yaw_, 0.0F));
    }
    // Backing away is walking too; a flyer has no legs to show it with.
    play_phase_animation(step.moving || step.retreating);
}

void E5Enemy::play_phase_animation(bool moving) {
    if (!animator_.is_ready()) {
        return;
    }
    switch (state_.phase) {
    case gameplay::EnemyPhase::Idle:
        animator_.set_base(clip_idle_, 1.0F);
        break;
    case gameplay::EnemyPhase::Chase:
        if (moving) {
            animator_.set_base(clip_walk_, move_speed_ / walk_clip_speed);
        } else {
            animator_.set_base(clip_idle_, 1.0F);
        }
        break;
    case gameplay::EnemyPhase::Attack:
        animator_.set_base(clip_attack_, 1.0F);
        break;
    case gameplay::EnemyPhase::Cast:
        animator_.set_base(animator_.has_clip(clip_cast_) ? clip_cast_ : clip_attack_, 1.0F);
        break;
    case gameplay::EnemyPhase::Hit:
        animator_.set_base(clip_hit_, 1.0F);
        break;
    case gameplay::EnemyPhase::Dead:
        animator_.set_base(clip_death_, 1.0F);
        break;
    }
}

void E5Enemy::die() {
    ++death_count_;
    // Nothing can hit or bump into a corpse.
    set_collision_layer(0);
    set_velocity(godot::Vector3());
    play_phase_animation(false);
    logger().info("enemy '{}' died", godot::String(get_name()).utf8().get_data());
    E5Effect::set_active(trail_, false);
    drop_loot();
}

float E5Enemy::ground_height() const {
    const godot::Vector3 position = get_global_position();
    if (const auto* const terrain =
            godot::Object::cast_to<E5Terrain>(get_tree()->get_first_node_in_group(E5Terrain::group_name))) {
        return terrain->height_at(static_cast<float>(position.x), static_cast<float>(position.z));
    }
    return 0.0F; // a flat scene: the floor is at zero
}

void E5Enemy::fly(const gameplay::EnemyStep& step, const godot::Vector3& to_player, float dt) {
    hover_seconds_ += dt;
    const godot::Vector3 direction = to_player.length() > 0.01F ? to_player.normalized() : godot::Vector3();
    godot::Vector3 wanted;
    if (step.moving) {
        wanted = direction * move_speed_;
    } else if (step.retreating) {
        wanted = -direction * move_speed_;
    }
    // At its distance it does not hang still: it drifts round the player, now and then the other way.
    if (state_.phase == gameplay::EnemyPhase::Chase) {
        if (std::fmod(hover_seconds_, circle_turn_seconds) < dt) {
            circle_side_ = -circle_side_;
        }
        wanted += godot::Vector3(-direction.z, 0.0F, direction.x) * (circle_side_ * move_speed_ * circle_share);
    }
    godot::Vector3 velocity = get_velocity();
    velocity.x =
        std::lerp(static_cast<float>(velocity.x), static_cast<float>(wanted.x), 1.0F - std::exp(-fly_ease * dt));
    velocity.z =
        std::lerp(static_cast<float>(velocity.z), static_cast<float>(wanted.z), 1.0F - std::exp(-fly_ease * dt));
    // Held at its height above the ground, with a slow bob.
    const float target = ground_height() + hover_height_ + std::sin(hover_seconds_ * bob_rate) * bob_height;
    velocity.y = (target - static_cast<float>(get_global_position().y)) * hover_stiffness;
    set_velocity(velocity);
    move_and_slide();
}

godot::Vector3 E5Enemy::cast_origin() const {
    const godot::Vector3 forward(std::sin(model_yaw_), 0.0F, std::cos(model_yaw_));
    return get_global_position() + godot::Vector3(0.0F, body_height_ * cast_height_share, 0.0F) +
           forward * (body_radius_ + cast_reach);
}

// A straight throw at where the player stands now: whoever keeps moving is missed.
void E5Enemy::throw_bolt(const godot::Vector3& player_position) {
    godot::Node* const parent = get_parent();
    if (parent == nullptr) {
        return;
    }
    const godot::Vector3 from = cast_origin();
    const godot::Vector3 target = player_position + godot::Vector3(0.0F, player_chest_height, 0.0F);
    auto* const bolt = memnew(E5SpellBolt);
    parent->add_child(bolt);
    bolt->set_global_position(from);
    bolt->set_trail_effect(bolt_trail_);
    bolt->set_impact_effect(bolt_impact_);
    bolt->set_damage(cast_damage_);
    bolt->set_hostile(true);
    bolt->launch((target - from).normalized() * bolt_speed_, get_rid());
    ++cast_count_;
}

// What it carried hops out around it.
void E5Enemy::drop_loot() {
    godot::Node* const parent = get_parent();
    if (parent == nullptr) {
        return;
    }
    const gameplay::Loot loot = gameplay::roll_loot(static_cast<std::uint32_t>(godot::UtilityFunctions::randi()));
    const godot::Vector3 feet = get_global_position();
    const godot::Vector3 from = feet + godot::Vector3(0.0F, body_height_ * 0.5F, 0.0F);
    int index = 0;
    const auto place = [&feet, &index] {
        // Spread around it by the golden angle, so no two land on each other.
        const float angle = static_cast<float>(index) * 2.4F + static_cast<float>(godot::UtilityFunctions::randf());
        const float distance = 0.9F + 0.25F * static_cast<float>(index);
        ++index;
        return feet + godot::Vector3(std::cos(angle) * distance, 0.0F, std::sin(angle) * distance);
    };
    E5Pickup::spawn(parent, from, place(), 0, 0, loot.gold);
    for (const gameplay::ItemStack& stack : loot.items) {
        if (!stack.empty()) {
            E5Pickup::spawn(parent, from, place(), static_cast<int>(stack.item), stack.count, 0);
        }
    }
}

void E5Enemy::respawn() {
    // Arrows that stuck in the body go with it.
    const godot::TypedArray<godot::Node> arrows = find_children("*", "E5Arrow", true, false);
    for (const godot::Variant& node : arrows) {
        if (auto* const arrow = godot::Object::cast_to<godot::Node>(node)) {
            arrow->queue_free();
        }
    }
    state_ = gameplay::spawn_enemy(params_);
    held_ = false;
    flung_ = false;
    set_shrink(1.0F);
    pending_damage_ = 0.0F;
    heaviest_pending_blow_ = 0.0F;
    set_global_transform(spawn_transform_);
    set_collision_layer(collision_layer_);
    E5Effect::set_active(trail_, true);
    set_velocity(godot::Vector3());
    model_yaw_ = 0.0F;
    if (model_ != nullptr) {
        model_->set_rotation(godot::Vector3());
    }
    update_health_bar();
    play_phase_animation(false);
}

} // namespace e5::bridge
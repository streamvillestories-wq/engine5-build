#include "day_night.hpp"

#include "e5/core/profiling.hpp"
#include "godot_log.hpp"

#include <godot_cpp/classes/directional_light3d.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/classes/light3d.hpp>
#include <godot_cpp/classes/noise_texture2d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/sky.hpp>
#include <godot_cpp/classes/world_environment.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cmath>
#include <numbers>

namespace e5::bridge {
namespace {

constexpr const char* sky_shader_path = "res://shaders/sky.gdshader";
constexpr float cloud_wind_x = 0.0012F; // sky-plane units per second
constexpr float cloud_wind_z = 0.0005F;
// Below this a light is switched off altogether: it would only cost its shadow pass.
constexpr float least_light = 0.002F;

[[nodiscard]] godot::Color to_color(const gameplay::Rgb& rgb) {
    return {rgb.r, rgb.g, rgb.b};
}

[[nodiscard]] godot::Vector3 to_vector(const gameplay::Vec3& v) {
    return {v.x, v.y, v.z};
}

// A directional light shines along its -Z: turn it so that it comes from `towards_source`.
void aim_light(godot::DirectionalLight3D& light, const godot::Vector3& towards_source) {
    const godot::Vector3 shining = -towards_source;
    // Straight up or down has no "up" to the side of it.
    const godot::Vector3 up =
        std::abs(shining.y) > 0.99F ? godot::Vector3(0.0F, 0.0F, 1.0F) : godot::Vector3(0.0F, 1.0F, 0.0F);
    light.set_global_basis(godot::Basis::looking_at(shining, up));
}

} // namespace

void E5DayNight::_bind_methods() {
    using godot::ClassDB;
    using godot::D_METHOD;
    using godot::PropertyInfo;
    ClassDB::bind_method(D_METHOD("set_hour", "hour"), &E5DayNight::set_hour);
    ClassDB::bind_method(D_METHOD("get_hour"), &E5DayNight::get_hour);
    ClassDB::bind_method(D_METHOD("set_day_length_minutes", "minutes"), &E5DayNight::set_day_length_minutes);
    ClassDB::bind_method(D_METHOD("get_day_length_minutes"), &E5DayNight::get_day_length_minutes);
    ClassDB::bind_method(D_METHOD("set_night_speed", "speed"), &E5DayNight::set_night_speed);
    ClassDB::bind_method(D_METHOD("get_night_speed"), &E5DayNight::get_night_speed);
    ClassDB::bind_method(D_METHOD("set_running", "running"), &E5DayNight::set_running);
    ClassDB::bind_method(D_METHOD("is_running"), &E5DayNight::is_running);
    ClassDB::bind_method(D_METHOD("set_cloud_cover", "cover"), &E5DayNight::set_cloud_cover);
    ClassDB::bind_method(D_METHOD("get_cloud_cover"), &E5DayNight::get_cloud_cover);
    ClassDB::bind_method(D_METHOD("set_night_light_boost", "boost"), &E5DayNight::set_night_light_boost);
    ClassDB::bind_method(D_METHOD("get_night_light_boost"), &E5DayNight::get_night_light_boost);
    ClassDB::bind_method(D_METHOD("set_sun_path", "path"), &E5DayNight::set_sun_path);
    ClassDB::bind_method(D_METHOD("get_sun_path"), &E5DayNight::get_sun_path);
    ClassDB::bind_method(D_METHOD("set_environment_path", "path"), &E5DayNight::set_environment_path);
    ClassDB::bind_method(D_METHOD("get_environment_path"), &E5DayNight::get_environment_path);
    ClassDB::bind_method(D_METHOD("get_night"), &E5DayNight::get_night);
    ClassDB::bind_method(D_METHOD("is_night_half"), &E5DayNight::is_night_half);
    ClassDB::bind_method(D_METHOD("get_half_progress"), &E5DayNight::get_half_progress);
    ClassDB::bind_method(D_METHOD("set_day", "day"), &E5DayNight::set_day);
    ClassDB::bind_method(D_METHOD("get_day"), &E5DayNight::get_day);
    ClassDB::bind_method(D_METHOD("get_moon_phase"), &E5DayNight::get_moon_phase);

    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "hour", godot::PROPERTY_HINT_RANGE, "0,24,0.05"), "set_hour",
                 "get_hour");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "day_length_minutes", godot::PROPERTY_HINT_RANGE, "0,240,0.5"),
                 "set_day_length_minutes", "get_day_length_minutes");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "night_speed", godot::PROPERTY_HINT_RANGE, "0.1,10,0.1"),
                 "set_night_speed", "get_night_speed");
    ADD_PROPERTY(PropertyInfo(godot::Variant::INT, "day", godot::PROPERTY_HINT_RANGE, "1,1000,1"), "set_day",
                 "get_day");
    ADD_PROPERTY(PropertyInfo(godot::Variant::BOOL, "running"), "set_running", "is_running");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "cloud_cover", godot::PROPERTY_HINT_RANGE, "0,1,0.01"),
                 "set_cloud_cover", "get_cloud_cover");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "night_light_boost", godot::PROPERTY_HINT_RANGE, "1,5,0.05"),
                 "set_night_light_boost", "get_night_light_boost");
    ADD_PROPERTY(PropertyInfo(godot::Variant::NODE_PATH, "sun"), "set_sun_path", "get_sun_path");
    ADD_PROPERTY(PropertyInfo(godot::Variant::NODE_PATH, "environment"), "set_environment_path",
                 "get_environment_path");
}

void E5DayNight::set_hour(float hour) {
    hour_ = gameplay::wrap_hour(hour);
}

void E5DayNight::_ready() {
    add_to_group(group_name);
    sun_ = godot::Object::cast_to<godot::DirectionalLight3D>(get_node_or_null(sun_path_));
    if (const auto* const world =
            godot::Object::cast_to<godot::WorldEnvironment>(get_node_or_null(environment_path_))) {
        environment_ = world->get_environment();
    }
    if (sun_ == nullptr || environment_.is_null()) {
        logger().warn("E5DayNight needs a sun and a WorldEnvironment; day and night are off");
        set_process(false);
        return;
    }

    // The moon: the sun's twin for the other half of the day, with the sun's shadow settings.
    moon_ = memnew(godot::DirectionalLight3D);
    moon_->set_name("Moon");
    moon_->set_shadow(sun_->has_shadow());
    moon_->set_param(godot::Light3D::PARAM_SHADOW_MAX_DISTANCE,
                     sun_->get_param(godot::Light3D::PARAM_SHADOW_MAX_DISTANCE));
    moon_->set_visible(false);
    add_child(moon_);

    const godot::Ref<godot::Shader> shader = godot::ResourceLoader::get_singleton()->load(sky_shader_path);
    if (shader.is_valid()) {
        sky_material_.instantiate();
        sky_material_->set_shader(shader);
        // The clouds' shapes: seamless, so the layer can drift for ever.
        godot::Ref<godot::FastNoiseLite> noise;
        noise.instantiate();
        noise->set_noise_type(godot::FastNoiseLite::TYPE_SIMPLEX_SMOOTH);
        noise->set_frequency(0.006F);
        noise->set_fractal_octaves(5);
        godot::Ref<godot::NoiseTexture2D> clouds;
        clouds.instantiate();
        clouds->set_width(512);
        clouds->set_height(512);
        clouds->set_seamless(true);
        clouds->set_generate_mipmaps(true);
        clouds->set_noise(noise);
        sky_material_->set_shader_parameter("cloud_noise", clouds);
        godot::Ref<godot::Sky> sky;
        sky.instantiate();
        sky->set_material(sky_material_);
        // The sky changes every frame, and the light it gives must follow at once.
        sky->set_radiance_size(godot::Sky::RADIANCE_SIZE_256);
        sky->set_process_mode(godot::Sky::PROCESS_MODE_REALTIME);
        environment_->set_background(godot::Environment::BG_SKY);
        environment_->set_sky(sky);
    } else {
        logger().warn("E5DayNight: {} is missing; the scene keeps its own sky", sky_shader_path);
    }

    const godot::TypedArray<godot::Node> lamps = get_tree()->get_nodes_in_group(night_light_group);
    for (const godot::Variant& node : lamps) {
        if (const auto* const light = godot::Object::cast_to<godot::Light3D>(node)) {
            night_lights_.emplace_back(light->get_instance_id(), light->get_param(godot::Light3D::PARAM_ENERGY));
        }
    }
    apply(gameplay::day_sky(hour_, params_, get_moon_phase()));
}

void E5DayNight::_process(double delta) {
    E5_PROFILE_SCOPE("E5DayNight::_process");
    const auto dt = static_cast<float>(delta);
    if (running_) {
        const float before = hour_;
        hour_ = gameplay::advance_hour(hour_, dt, params_);
        // Past midnight: a new day, and the moon one step further in its phases.
        if (hour_ < before) {
            ++day_;
        }
    }
    cloud_drift_ += godot::Vector2(cloud_wind_x, cloud_wind_z) * dt;
    apply(gameplay::day_sky(hour_, params_, get_moon_phase()));
}

void E5DayNight::apply(const gameplay::DaySky& sky) {
    night_ = sky.night;

    sun_->set_visible(sky.sun_energy > least_light);
    if (sun_->is_visible()) {
        aim_light(*sun_, to_vector(sky.sun_direction));
        sun_->set_param(godot::Light3D::PARAM_ENERGY, sky.sun_energy);
        sun_->set_color(to_color(sky.sun_color));
    }
    moon_->set_visible(sky.moon_energy > least_light);
    if (moon_->is_visible()) {
        aim_light(*moon_, to_vector(sky.moon_direction));
        moon_->set_param(godot::Light3D::PARAM_ENERGY, sky.moon_energy);
        moon_->set_color(to_color(sky.moon_color));
    }

    environment_->set_fog_light_color(to_color(sky.fog_color));
    environment_->set_ambient_light_energy(sky.ambient_energy);

    if (sky_material_.is_valid()) {
        sky_material_->set_shader_parameter("sky_top", to_color(sky.sky_top));
        sky_material_->set_shader_parameter("sky_horizon", to_color(sky.sky_horizon));
        sky_material_->set_shader_parameter("sun_direction", to_vector(sky.sun_direction));
        sky_material_->set_shader_parameter("sun_color", to_color(sky.sun_color));
        sky_material_->set_shader_parameter("sun_energy", sky.sun_energy);
        sky_material_->set_shader_parameter("moon_direction", to_vector(sky.moon_direction));
        sky_material_->set_shader_parameter("moon_color", to_color(sky.moon_color));
        sky_material_->set_shader_parameter("moon_energy", sky.moon_energy);
        sky_material_->set_shader_parameter("moon_phase", get_moon_phase());
        sky_material_->set_shader_parameter("night", sky.night);
        sky_material_->set_shader_parameter("twilight", sky.twilight);
        sky_material_->set_shader_parameter("star_turn", hour_ / 24.0F * 2.0F * std::numbers::pi_v<float>);
        sky_material_->set_shader_parameter("cloud_cover", cloud_cover_);
        sky_material_->set_shader_parameter("cloud_drift", cloud_drift_);
        sky_material_->set_shader_parameter("sky_fill", to_color(sky.sky_fill));
    }

    const float boost = std::lerp(1.0F, night_light_boost_, sky.night);
    for (const auto& [id, energy] : night_lights_) {
        if (auto* const light = godot::Object::cast_to<godot::Light3D>(godot::ObjectDB::get_instance(id))) {
            light->set_param(godot::Light3D::PARAM_ENERGY, energy * boost);
        }
    }
}

} // namespace e5::bridge

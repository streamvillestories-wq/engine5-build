#pragma once

#include "e5/gameplay/day_cycle.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace godot {
class DirectionalLight3D;
class Environment;
class Light3D;
} // namespace godot

namespace e5::bridge {

// Day and night for a scene. Put one in a scene that has a WorldEnvironment and
// a sun (a DirectionalLight3D): it runs the clock, moves the sun, adds a moon,
// replaces the sky with `shaders/sky.gdshader`, and keeps fog, ambient light
// and the sky's colours in step. What the sky looks like at an hour is decided
// by e5::gameplay::day_sky; this class only hands the numbers to the engine.
//
// Lights in the group `e5_night_light` burn brighter in the dark.
class E5DayNight : public godot::Node {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5DayNight, godot::Node)

public:
    static constexpr const char* group_name = "e5_day_night";
    static constexpr const char* night_light_group = "e5_night_light";

    void _ready() override;
    void _process(double delta) override;

    void set_hour(float hour);
    [[nodiscard]] float get_hour() const { return hour_; }
    void set_day_length_minutes(float minutes) { params_.day_length_seconds = minutes * 60.0F; }
    [[nodiscard]] float get_day_length_minutes() const { return params_.day_length_seconds / 60.0F; }
    void set_night_speed(float speed) { params_.night_speed = speed; }
    [[nodiscard]] float get_night_speed() const { return params_.night_speed; }
    void set_running(bool running) { running_ = running; }
    [[nodiscard]] bool is_running() const { return running_; }
    void set_cloud_cover(float cover) { cloud_cover_ = cover; }
    [[nodiscard]] float get_cloud_cover() const { return cloud_cover_; }
    void set_night_light_boost(float boost) { night_light_boost_ = boost; }
    [[nodiscard]] float get_night_light_boost() const { return night_light_boost_; }
    void set_sun_path(const godot::NodePath& path) { sun_path_ = path; }
    [[nodiscard]] godot::NodePath get_sun_path() const { return sun_path_; }
    void set_environment_path(const godot::NodePath& path) { environment_path_ = path; }
    [[nodiscard]] godot::NodePath get_environment_path() const { return environment_path_; }

    // 0 by day, 1 in the dark.
    [[nodiscard]] float get_night() const { return night_; }
    // For a dial: which half of the day it is, and how far that half has come (0..1).
    [[nodiscard]] bool is_night_half() const { return gameplay::is_night_hour(hour_, params_); }
    [[nodiscard]] float get_half_progress() const { return gameplay::half_progress(hour_, params_); }
    // Days are counted from 1.
    void set_day(int day) { day_ = std::max(day, 1); }
    [[nodiscard]] int get_day() const { return day_; }
    [[nodiscard]] float get_moon_phase() const { return gameplay::moon_phase(day_ - 1); }

protected:
    static void _bind_methods();

private:
    void apply(const gameplay::DaySky& sky);

    gameplay::DayParams params_;
    float hour_ = 9.0F;
    int day_ = 1;
    bool running_ = true;
    float cloud_cover_ = 0.36F;
    float night_light_boost_ = 1.7F;
    float night_ = 0.0F;
    godot::Vector2 cloud_drift_;
    godot::NodePath sun_path_{"../Sun"};
    godot::NodePath environment_path_{"../WorldEnvironment"};

    // Non-owning: nodes of the scene, and one child (the moon).
    godot::DirectionalLight3D* sun_ = nullptr;
    godot::DirectionalLight3D* moon_ = nullptr;
    godot::Ref<godot::Environment> environment_;
    godot::Ref<godot::ShaderMaterial> sky_material_;
    // Lamps and what they give by day; found by object id, as they may be freed.
    std::vector<std::pair<std::uint64_t, float>> night_lights_;
};

} // namespace e5::bridge

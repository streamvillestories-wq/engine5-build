#include "diagnostics.hpp"

#include "enemy.hpp"
#include "forest.hpp"
#include "terrain.hpp"
#include "wanderer.hpp"

#include "cloth_simulator.hpp"
#include "day_night.hpp"

#include "e5/core/profiling.hpp"
#include "godot_log.hpp"
#include "input_actions.hpp"
#include "inventory.hpp"
#include "perf_overlay.hpp"
#include "player_controller.hpp"
#include "render_metrics.hpp"
#include "target.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <algorithm>
#include <numbers>
#include <string>
#include <string_view>
#include <vector>

namespace e5::bridge {
namespace {

// Scene nodes instanced from imported glTF assets are tagged with this group so
// the report can prove that imported geometry actually made it into the scene.
constexpr const char* imported_asset_group = "e5_asset";

// The character spawns in the air and can only draw once it has landed (about 0.6 s);
// drawing takes about 1.0 s and a full charge 1.2 s more.
constexpr double auto_shot_hold_ms = 2000.0;
constexpr double auto_charge_hold_ms = 3200.0;
constexpr double auto_shoot_pause_ms = 400.0;
constexpr int power_shot_slot = 1;
// Per second, not per frame: the turn must not depend on the frame rate.
constexpr float auto_turn_pixels_per_second = 500.0F;

godot::String to_godot(std::string_view text) {
    return godot::String::utf8(text.data(), static_cast<std::int64_t>(text.size()));
}

Result<RuntimeConfig> read_runtime_config() {
    const godot::PackedStringArray raw = godot::OS::get_singleton()->get_cmdline_user_args();
    std::vector<std::string> storage;
    storage.reserve(static_cast<std::size_t>(raw.size()));
    for (const godot::String& arg : raw) {
        storage.emplace_back(arg.utf8().get_data());
    }
    const std::vector<std::string_view> views(storage.begin(), storage.end());
    return parse_runtime_args(views);
}

std::int64_t count_mesh_instances(const godot::Node* node) {
    std::int64_t count = godot::Object::cast_to<godot::MeshInstance3D>(node) != nullptr ? 1 : 0;
    for (std::int32_t i = 0; i < node->get_child_count(); ++i) {
        count += count_mesh_instances(node->get_child(i));
    }
    return count;
}

} // namespace

void E5Diagnostics::_ready() {
    auto config = read_runtime_config();
    if (!config) {
        logger().error("invalid command line: {}", config.error().message);
        quit(2);
        return;
    }
    config_ = std::move(*config);

    godot::SceneTree* const tree = get_tree();
    if (!config_.show_overlay) {
        const godot::TypedArray<godot::Node> overlays = tree->get_nodes_in_group(E5PerfOverlay::group_name);
        for (const godot::Variant& node : overlays) {
            if (auto* const overlay = godot::Object::cast_to<godot::CanvasLayer>(node)) {
                overlay->set_visible(false);
            }
        }
    }

    if (!config_.camera_name.empty()) {
        auto* const camera = godot::Object::cast_to<godot::Camera3D>(
            tree->get_root()->find_child(to_godot(config_.camera_name), true, false));
        if (camera != nullptr) {
            camera->make_current();
        } else {
            logger().error("--camera: no Camera3D named '{}' in the scene", config_.camera_name);
            quit(2);
            return;
        }
    }
    if (config_.auto_charge) {
        ensure_default_input_actions();
    }
    if (config_.auto_aim) {
        ensure_default_input_actions();
        godot::Input::get_singleton()->action_press(actions::aim);
    }
    if (config_.auto_attack) {
        ensure_default_input_actions();
    }
    if (!config_.auto_move.empty()) {
        ensure_default_input_actions();
        const char* action = actions::move_forward;
        if (config_.auto_move == "back") {
            action = actions::move_back;
        } else if (config_.auto_move == "left") {
            action = actions::move_left;
        } else if (config_.auto_move == "right") {
            action = actions::move_right;
        }
        godot::Input::get_singleton()->action_press(action);
    }

    if (config_.benchmark) {
        // Measure what the hardware can do, not the display's refresh rate.
        godot::DisplayServer::get_singleton()->window_set_vsync_mode(godot::DisplayServer::VSYNC_DISABLED);
        godot::Engine::get_singleton()->set_max_fps(0);
        enable_render_time_measurement(get_viewport()->get_viewport_rid());
        logger().info("benchmark: {:.1f} s warm-up, {:.1f} s measured", config_.benchmark_warmup_seconds,
                      config_.benchmark_seconds);
    }
}

void E5Diagnostics::_process(double /*delta*/) {
    const double frame_ms = clock_.tick_ms();
    E5_PROFILE_FRAME();
    E5_PROFILE_PLOT("frame time (ms)", frame_ms);

    // Applied on the first frame rather than in _ready: the player node may
    // become ready after this one and would overwrite the angle.
    if (!camera_yaw_applied_) {
        camera_yaw_applied_ = true;
        auto* const player = godot::Object::cast_to<E5PlayerController>(
            get_tree()->get_first_node_in_group(E5PlayerController::group_name));
        if (player != nullptr) {
            if (config_.auto_skill) {
                player->select_skill(*config_.auto_skill - 1);
            } else if (config_.auto_charge) {
                player->select_skill(power_shot_slot);
            } else if (!config_.auto_attack && (config_.auto_aim || config_.auto_fire || !config_.auto_move.empty())) {
                // The automatic input holds the right button: put the standard attack on it,
                // as these runs expect.
                player->select_skill(0);
            }
            constexpr float radians_per_degree = std::numbers::pi_v<float> / 180.0F;
            if (config_.time_of_day) {
                if (auto* const day = godot::Object::cast_to<E5DayNight>(
                        get_tree()->get_first_node_in_group(E5DayNight::group_name))) {
                    day->set_hour(*config_.time_of_day);
                    day->set_running(false);
                }
            }
            if (config_.camera_yaw_degrees) {
                player->set_camera_yaw(*config_.camera_yaw_degrees * radians_per_degree);
            }
            if (config_.camera_pitch_degrees) {
                player->set_camera_pitch(*config_.camera_pitch_degrees * radians_per_degree);
            }
        }
    }

    drive_automated_input(frame_ms);

    if (config_.benchmark && !finished_ && frame_ms > 0.0) {
        run_benchmark_frame(frame_ms);
    }
}

// Stands in for a player during unattended runs (--auto-turn, --auto-fire, --auto-charge).
void E5Diagnostics::drive_automated_input(double frame_ms) {
    if (config_.auto_turn) {
        // Through Input::parse_input_event, i.e. the same route a real mouse takes,
        // including the GUI layer that can swallow events.
        godot::Input* const input = godot::Input::get_singleton();
        input->set_mouse_mode(godot::Input::MOUSE_MODE_CAPTURED);
        godot::Ref<godot::InputEventMouseMotion> motion;
        motion.instantiate();
        motion->set_relative(godot::Vector2(auto_turn_pixels_per_second * static_cast<float>(frame_ms) * 0.001F, 0.0F));
        const godot::Vector2 centre = godot::Vector2(get_viewport()->get_visible_rect().size) * 0.5F;
        motion->set_position(centre);
        motion->set_global_position(centre);
        input->parse_input_event(motion);
    }

    // Shooting means holding aim until the bow is drawn (and charged, for the
    // power shot) and then letting go; repeat.
    // --auto-attack does the same with the left button: the standard attack.
    if (config_.auto_fire || config_.auto_charge || config_.auto_attack) {
        const char* const button = config_.auto_attack ? actions::attack : actions::aim;
        const double hold_ms = config_.auto_charge ? auto_charge_hold_ms : auto_shot_hold_ms;
        auto_shoot_elapsed_ms_ += frame_ms;
        if (auto_shoot_elapsed_ms_ >= hold_ms + auto_shoot_pause_ms) {
            auto_shoot_elapsed_ms_ = 0.0;
        }
        godot::Input* const input = godot::Input::get_singleton();
        if (auto_shoot_elapsed_ms_ < hold_ms) {
            input->action_press(button);
        } else {
            input->action_release(button);
        }
    }
}
void E5Diagnostics::run_benchmark_frame(double frame_ms) {
    E5_PROFILE_SCOPE("E5Diagnostics::run_benchmark_frame");

    // An unattended run must not grab the user's mouse. Done every frame
    // because the player captures it in its own _ready.
    if (!config_.auto_turn) {
        godot::Input::get_singleton()->set_mouse_mode(godot::Input::MOUSE_MODE_VISIBLE);
    }

    // Warm-up lets shader/pipeline compilation and texture streaming settle.
    if (warmup_elapsed_ms_ < static_cast<double>(config_.benchmark_warmup_seconds) * 1000.0) {
        warmup_elapsed_ms_ += frame_ms;
        return;
    }

    history_.record(frame_ms);
    measured_elapsed_ms_ += frame_ms;

    const RenderMetrics metrics = sample_render_metrics(get_viewport()->get_viewport_rid());
    totals_.cpu_render_ms += metrics.cpu_render_ms;
    totals_.gpu_render_ms += metrics.gpu_render_ms;
    totals_.process_ms += metrics.process_ms;
    totals_.physics_ms += metrics.physics_ms;
    totals_.draw_calls += static_cast<double>(metrics.draw_calls);
    totals_.visible_objects += static_cast<double>(metrics.visible_objects);
    totals_.primitives += static_cast<double>(metrics.primitives);
    totals_.peak_video_memory_mb = std::max(totals_.peak_video_memory_mb, metrics.video_memory_mb);
    totals_.peak_texture_memory_mb = std::max(totals_.peak_texture_memory_mb, metrics.texture_memory_mb);
    totals_.peak_static_memory_mb = std::max(totals_.peak_static_memory_mb, metrics.static_memory_mb);
    ++totals_.samples;

    if (measured_elapsed_ms_ < static_cast<double>(config_.benchmark_seconds) * 1000.0) {
        return;
    }

    finished_ = true;
    const bool screenshot_ok = config_.screenshot_output.empty() || save_screenshot();
    const bool report_ok = write_report();
    quit(screenshot_ok && report_ok ? 0 : 1);
}

bool E5Diagnostics::write_report() {
    const FrameTimeSummary frame = history_.summarize();
    const double samples = static_cast<double>(std::max<std::int64_t>(totals_.samples, 1));

    const godot::RenderingServer* const rs = godot::RenderingServer::get_singleton();
    const godot::ProjectSettings* const settings = godot::ProjectSettings::get_singleton();
    godot::SceneTree* const tree = get_tree();

    godot::Dictionary build;
    build["engine5_version"] = E5_VERSION;
    build["config"] = E5_BUILD_CONFIG;
#if defined(E5_PROFILING_TRACY)
    build["tracy"] = true;
#else
    build["tracy"] = false;
#endif
    build["godot_version"] = godot::Engine::get_singleton()->get_version_info()["string"];

    godot::Dictionary renderer;
    renderer["driver"] = rs->get_current_rendering_driver_name();
    renderer["method"] = rs->get_current_rendering_method();
    renderer["adapter"] = rs->get_video_adapter_name();
    renderer["adapter_api_version"] = rs->get_video_adapter_api_version();
    const godot::Vector2i window_size = tree->get_root()->get_size();
    renderer["window_width"] = window_size.x;
    renderer["window_height"] = window_size.y;

    godot::Dictionary frame_times;
    frame_times["frames"] = static_cast<std::int64_t>(frame.sample_count);
    frame_times["seconds"] = measured_elapsed_ms_ / 1000.0;
    frame_times["average_fps"] = frame.average_fps;
    frame_times["one_percent_low_fps"] = frame.one_percent_low_fps;
    frame_times["average_ms"] = frame.average_ms;
    frame_times["min_ms"] = frame.min_ms;
    frame_times["max_ms"] = frame.max_ms;
    frame_times["p50_ms"] = frame.p50_ms;
    frame_times["p95_ms"] = frame.p95_ms;
    frame_times["p99_ms"] = frame.p99_ms;

    godot::Dictionary averages;
    averages["cpu_render_ms"] = totals_.cpu_render_ms / samples;
    averages["gpu_render_ms"] = totals_.gpu_render_ms / samples;
    averages["process_ms"] = totals_.process_ms / samples;
    averages["physics_ms"] = totals_.physics_ms / samples;
    averages["draw_calls"] = totals_.draw_calls / samples;
    averages["visible_objects"] = totals_.visible_objects / samples;
    averages["primitives"] = totals_.primitives / samples;

    godot::Dictionary memory;
    memory["peak_video_mb"] = totals_.peak_video_memory_mb;
    memory["peak_texture_mb"] = totals_.peak_texture_memory_mb;
    memory["peak_engine_heap_mb"] = totals_.peak_static_memory_mb;

    godot::Dictionary scene;
    scene["physics_engine"] = settings->get_setting("physics/3d/physics_engine");
    std::int64_t asset_meshes = 0;
    const godot::TypedArray<godot::Node> assets = tree->get_nodes_in_group(imported_asset_group);
    for (const godot::Variant& node : assets) {
        if (const auto* const asset = godot::Object::cast_to<godot::Node>(node)) {
            asset_meshes += count_mesh_instances(asset);
        }
    }
    scene["imported_asset_roots"] = assets.size();
    scene["imported_asset_meshes"] = asset_meshes;
    std::int64_t cloth_chains = 0;
    const godot::TypedArray<godot::Node> cloth_nodes =
        tree->get_root()->find_children("*", "E5ClothSimulator", true, false);
    for (const godot::Variant& node : cloth_nodes) {
        if (const auto* const cloth = godot::Object::cast_to<E5ClothSimulator>(node)) {
            cloth_chains += cloth->get_chain_count();
        }
    }
    scene["cloth_chains"] = cloth_chains;
    std::int64_t target_hits = 0;
    std::int64_t target_score = 0;
    std::int64_t target_power_hits = 0;
    const godot::TypedArray<godot::Node> targets = tree->get_root()->find_children("*", "E5Target", true, false);
    for (const godot::Variant& node : targets) {
        if (const auto* const target = godot::Object::cast_to<E5Target>(node)) {
            target_hits += target->get_hit_count();
            target_score += target->get_total_score();
            target_power_hits += target->get_power_hit_count();
        }
    }
    scene["target_hits"] = target_hits;
    scene["target_score"] = target_score;
    scene["target_power_hits"] = target_power_hits;

    // Enemies: how many there are, and what the run did to them.
    double enemy_damage = 0.0;
    std::int64_t enemy_deaths = 0;
    std::int64_t enemy_casts = 0;
    const godot::TypedArray<godot::Node> enemies = tree->get_nodes_in_group(E5Enemy::group_name);
    for (const godot::Variant& node : enemies) {
        if (const auto* const enemy = godot::Object::cast_to<E5Enemy>(node)) {
            enemy_damage += static_cast<double>(enemy->get_damage_taken());
            enemy_deaths += enemy->get_death_count();
            enemy_casts += enemy->get_cast_count();
        }
    }
    scene["enemies"] = enemies.size();
    scene["enemy_damage"] = enemy_damage;
    if (const auto* const wanderer =
            godot::Object::cast_to<E5Wanderer>(get_tree()->get_first_node_in_group(E5Wanderer::group_name))) {
        scene["wanderer_walked"] = wanderer->get_distance_walked();
    }
    scene["enemy_deaths"] = enemy_deaths;
    scene["enemy_casts"] = enemy_casts;
    if (const auto* const day =
            godot::Object::cast_to<E5DayNight>(get_tree()->get_first_node_in_group(E5DayNight::group_name))) {
        scene["hour"] = day->get_hour();
        scene["night"] = day->get_night();
    }

    std::int64_t forest_plants = 0;
    const godot::TypedArray<godot::Node> forests = tree->get_nodes_in_group(E5Forest::group_name);
    for (const godot::Variant& node : forests) {
        if (const auto* const forest = godot::Object::cast_to<E5Forest>(node)) {
            forest_plants += forest->get_plant_count();
        }
    }
    scene["forest_plants"] = forest_plants;
    const auto* const terrain = godot::Object::cast_to<E5Terrain>(tree->get_first_node_in_group(E5Terrain::group_name));
    scene["terrain_triangles"] = terrain != nullptr ? terrain->get_triangle_count() : 0;

    // The player spawns above the floor; where it ends up shows whether
    // gravity and collision both worked.
    godot::Dictionary player_info;
    const auto* const player =
        godot::Object::cast_to<E5PlayerController>(tree->get_first_node_in_group(E5PlayerController::group_name));
    player_info["found"] = player != nullptr;
    if (player != nullptr) {
        const godot::Vector3 position = player->get_global_position();
        player_info["x"] = position.x;
        player_info["y"] = position.y;
        player_info["z"] = position.z;
        player_info["on_floor"] = player->is_on_floor();
        player_info["animation"] = player->get_current_animation();
        player_info["health"] = player->get_health();
        player_info["damage_taken"] = player->get_damage_taken();
        player_info["deaths"] = player->get_death_count();
        player_info["skill_set"] = player->get_skill_set();
        player_info["last_skill"] = player->get_last_skill();
        player_info["skills_used"] = player->get_skills_used();
        if (const E5Inventory* const inventory = player->get_inventory()) {
            player_info["gold"] = inventory->get_gold();
            player_info["items_picked"] = inventory->get_items_picked();
            player_info["potions"] = inventory->get_potion_count();
        }
        player_info["camera_yaw_degrees"] = player->get_camera_yaw() * 180.0F / std::numbers::pi_v<float>;
    }

    godot::Dictionary report;
    report["build"] = build;
    report["renderer"] = renderer;
    report["frame_times"] = frame_times;
    report["averages"] = averages;
    report["memory"] = memory;
    report["scene"] = scene;
    report["player"] = player_info;

    logger().info("benchmark: {} frames, {:.1f} FPS avg, {:.2f} ms p99, GPU {:.2f} ms, {:.0f} draw calls",
                  frame.sample_count, frame.average_fps, frame.p99_ms, totals_.gpu_render_ms / samples,
                  totals_.draw_calls / samples);

    if (config_.benchmark_output.empty()) {
        return true;
    }
    const godot::String path = to_godot(config_.benchmark_output);
    const godot::Ref<godot::FileAccess> file = godot::FileAccess::open(path, godot::FileAccess::WRITE);
    if (file.is_null()) {
        logger().error("cannot write benchmark report to '{}' (error {})", config_.benchmark_output,
                       static_cast<int>(godot::FileAccess::get_open_error()));
        return false;
    }
    file->store_string(godot::JSON::stringify(report, "  "));
    logger().info("benchmark report written to {}", config_.benchmark_output);
    return true;
}

bool E5Diagnostics::save_screenshot() const {
    const godot::Ref<godot::ViewportTexture> texture = get_viewport()->get_texture();
    const godot::Ref<godot::Image> image = texture.is_valid() ? texture->get_image() : godot::Ref<godot::Image>();
    if (image.is_null() || image->is_empty()) {
        logger().error("screenshot failed: viewport image unavailable");
        return false;
    }
    const godot::Error error = image->save_png(to_godot(config_.screenshot_output));
    if (error != godot::OK) {
        logger().error("cannot write screenshot to '{}' (error {})", config_.screenshot_output,
                       static_cast<int>(error));
        return false;
    }
    logger().info("screenshot written to {}", config_.screenshot_output);
    return true;
}

void E5Diagnostics::quit(int exit_code) {
    finished_ = true;
    get_tree()->quit(exit_code);
}

} // namespace e5::bridge

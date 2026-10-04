#include "perf_overlay.hpp"

#include "day_night.hpp"
#include "e5/core/profiling.hpp"
#include "enemy.hpp"
#include "player_controller.hpp"
#include "render_metrics.hpp"

#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line2d.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/system_font.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <string>

namespace e5::bridge {
namespace {

// Rebuilding the text every frame would make the overlay itself show up in
// the numbers it reports; four times a second is plenty to read.
constexpr double refresh_interval_ms = 250.0;
constexpr double note_ms = 6000.0;

constexpr double ms_at_60_fps = 1000.0 / 60.0;
constexpr double ms_at_30_fps = 1000.0 / 30.0;
// A frame this long is felt as a stumble whatever the average is.
constexpr double hitch_ms = 50.0;

constexpr float margin = 12.0F;
constexpr float graph_width = 480.0F; // one pixel for each frame of the history
constexpr float graph_height = 72.0F;
// The graph's top is at least this, so a steady 60 FPS does not fill it with noise.
constexpr double graph_least_top_ms = 40.0;
constexpr double graph_most_top_ms = 200.0;

constexpr const char* report_path = "user://performance_report.txt";
constexpr double bytes_per_gb = 1024.0 * 1024.0 * 1024.0;

std::string text_of(const godot::String& text) {
    return text.utf8().get_data();
}

const char* vsync_name(godot::DisplayServer::VSyncMode mode) {
    switch (mode) {
    case godot::DisplayServer::VSYNC_DISABLED:
        return "off";
    case godot::DisplayServer::VSYNC_ENABLED:
        return "on";
    case godot::DisplayServer::VSYNC_ADAPTIVE:
        return "adaptive";
    case godot::DisplayServer::VSYNC_MAILBOX:
        return "mailbox";
    }
    return "?";
}

const char* msaa_name(godot::Viewport::MSAA msaa) {
    switch (msaa) {
    case godot::Viewport::MSAA_2X:
        return "2x";
    case godot::Viewport::MSAA_4X:
        return "4x";
    case godot::Viewport::MSAA_8X:
        return "8x";
    case godot::Viewport::MSAA_DISABLED:
    case godot::Viewport::MSAA_MAX:
        break;
    }
    return "off";
}

godot::ColorRect* make_rect(godot::Node* parent, const godot::Color& colour) {
    auto* const rect = memnew(godot::ColorRect);
    rect->set_color(colour);
    rect->set_mouse_filter(godot::Control::MOUSE_FILTER_IGNORE);
    parent->add_child(rect);
    return rect;
}

// What holds the frame rate where it is. A guess from the measured times, and named as one:
// time spent waiting for the graphics card to present a frame is in none of the counters.
std::string limit_guess(double frame_ms, double cpu_ms, double gpu_ms, bool vsync, double refresh_hz, int max_fps) {
    // Busy for most of the frame: that part cannot go faster, so it sets the pace.
    constexpr double most = 0.8;
    if (gpu_ms >= most * frame_ms && gpu_ms >= cpu_ms) {
        return "the graphics card (lower the resolution, shadows or anti-aliasing)";
    }
    if (cpu_ms >= most * frame_ms) {
        return "the processor (game logic, physics or preparing the draw calls)";
    }
    if (max_fps > 0 && frame_ms >= 0.95 * 1000.0 / max_fps) {
        return std::format("the frame cap of {} FPS (the machine could do more)", max_fps);
    }
    if (vsync && refresh_hz > 1.0 && frame_ms <= 1.08 * 1000.0 / refresh_hz) {
        return std::format("the display: V-Sync at {:.0f} Hz (the machine could do more)", refresh_hz);
    }
    return "nothing that stands out: neither processor nor graphics card is busy for most of the frame";
}

double ms_between(std::uint64_t from_usec, std::uint64_t to_usec) {
    return from_usec == 0 || to_usec < from_usec ? 0.0 : static_cast<double>(to_usec - from_usec) / 1000.0;
}

std::uint64_t now_usec() {
    return godot::Time::get_singleton()->get_ticks_usec();
}

} // namespace

void E5PerfOverlay::_ready() {
    add_to_group(group_name);
    set_layer(100); // above any game UI

    backdrop_ = make_rect(this, godot::Color(0.02F, 0.03F, 0.05F, 0.78F));
    backdrop_->set_position(godot::Vector2(margin * 0.5F, margin * 0.5F));

    graph_ = memnew(godot::Control);
    graph_->set_position(godot::Vector2(margin, margin));
    graph_->set_size(godot::Vector2(graph_width, graph_height));
    graph_->set_mouse_filter(godot::Control::MOUSE_FILTER_IGNORE);
    add_child(graph_);
    godot::ColorRect* const graph_back = make_rect(graph_, godot::Color(1.0F, 1.0F, 1.0F, 0.06F));
    graph_back->set_size(godot::Vector2(graph_width, graph_height));
    graph_line_ = memnew(godot::Line2D);
    graph_line_->set_width(1.5F);
    graph_line_->set_default_color(godot::Color(1.0F, 0.95F, 0.7F));
    graph_->add_child(graph_line_);
    // After the curve, so an uneven frame rate does not bury them.
    line_60_ = make_rect(graph_, godot::Color(0.45F, 0.9F, 0.55F, 0.9F));
    line_60_->set_size(godot::Vector2(graph_width, 1.0F));
    line_30_ = make_rect(graph_, godot::Color(1.0F, 0.6F, 0.3F, 0.9F));
    line_30_->set_size(godot::Vector2(graph_width, 1.0F));

    // Columns of numbers that jump sideways cannot be read: a font with one width for all
    // characters, from the system (none of ours has one).
    godot::Ref<godot::SystemFont> font;
    font.instantiate();
    godot::PackedStringArray names;
    names.push_back("Consolas");
    names.push_back("Menlo");
    names.push_back("DejaVu Sans Mono");
    names.push_back("monospace");
    font->set_font_names(names);

    label_ = memnew(godot::Label);
    label_->add_theme_font_override("font", font);
    label_->add_theme_font_size_override("font_size", 13);
    label_->add_theme_color_override("font_color", godot::Color(1.0F, 1.0F, 1.0F));
    label_->add_theme_color_override("font_outline_color", godot::Color(0.0F, 0.0F, 0.0F));
    label_->add_theme_constant_override("outline_size", 5);
    label_->set_mouse_filter(godot::Control::MOUSE_FILTER_IGNORE);
    add_child(label_);

    enable_render_time_measurement(get_viewport()->get_viewport_rid());
    get_tree()->connect("physics_frame", callable_mp(this, &E5PerfOverlay::on_physics_frame));
    get_tree()->connect("process_frame", callable_mp(this, &E5PerfOverlay::on_process_frame));
    godot::RenderingServer* const rs = godot::RenderingServer::get_singleton();
    rs->connect("frame_pre_draw", callable_mp(this, &E5PerfOverlay::on_pre_draw));
    rs->connect("frame_post_draw", callable_mp(this, &E5PerfOverlay::on_post_draw));
    set_detailed(false);
}

void E5PerfOverlay::on_physics_frame() {
    // Several physics steps can fall into one frame: the first one starts the clock.
    if (physics_start_usec_ == 0) {
        physics_start_usec_ = now_usec();
    }
}

void E5PerfOverlay::on_process_frame() {
    process_start_usec_ = now_usec();
    running_.physics_ms += ms_between(physics_start_usec_, process_start_usec_);
    physics_start_usec_ = 0;
}

void E5PerfOverlay::on_pre_draw() {
    draw_start_usec_ = now_usec();
    running_.logic_ms += ms_between(process_start_usec_, draw_start_usec_);
}

void E5PerfOverlay::on_post_draw() {
    running_.draw_ms += ms_between(draw_start_usec_, now_usec());
    ++running_.frames;
}

void E5PerfOverlay::set_detailed(bool detailed) {
    detailed_ = detailed;
    if (label_ == nullptr) {
        return;
    }
    backdrop_->set_visible(detailed_);
    graph_->set_visible(detailed_);
    label_->set_position(godot::Vector2(margin, detailed_ ? margin + graph_height + 6.0F : 10.0F));
    refresh_text();
}

void E5PerfOverlay::_process(double /*delta*/) {
    const double frame_ms = clock_.tick_ms();
    if (frame_ms <= 0.0) {
        return; // first frame: no interval to measure yet
    }
    history_.record(frame_ms);
    if (!is_visible()) {
        return;
    }
    if (detailed_) {
        refresh_graph();
    }
    if (note_ms_left_ > 0.0) {
        note_ms_left_ -= frame_ms;
        if (note_ms_left_ <= 0.0) {
            note_.clear();
        }
    }
    ms_since_refresh_ += frame_ms;
    if (ms_since_refresh_ >= refresh_interval_ms) {
        ms_since_refresh_ = 0.0;
        refresh_text();
    }
}

void E5PerfOverlay::_unhandled_key_input(const godot::Ref<godot::InputEvent>& event) {
    const godot::Ref<godot::InputEventKey> key = event;
    if (!key.is_valid() || !key->is_pressed() || key->is_echo()) {
        return;
    }
    if (key->get_keycode() == godot::KEY_F3) {
        set_visible(!is_visible());
    } else if (key->get_keycode() == godot::KEY_F4 && !key->is_alt_pressed()) {
        if (key->is_shift_pressed()) {
            save_report();
        } else if (!is_visible()) {
            // F4 on a hidden overlay means "show me the details", not "switch what is hidden".
            set_visible(true);
            set_detailed(true);
        } else {
            set_detailed(!detailed_);
        }
    }
}

void E5PerfOverlay::refresh_graph() {
    const std::size_t count = history_.size();
    // One long frame must not flatten everything else: the scale follows the slowest 1%, and
    // a longer frame runs into the top edge.
    const double top_ms = graph_top_ms_;

    const auto height_of = [top_ms](double ms) {
        return graph_height - static_cast<float>(std::min(ms / top_ms, 1.0)) * graph_height;
    };
    graph_points_.resize(static_cast<std::int64_t>(count));
    // The newest frame is at the right edge, also while the history is still filling.
    const float first_x = graph_width - static_cast<float>(count);
    for (std::size_t i = 0; i < count; ++i) {
        graph_points_.set(static_cast<std::int64_t>(i),
                          godot::Vector2(first_x + static_cast<float>(i), height_of(history_.sample(i))));
    }
    graph_line_->set_points(graph_points_);
    line_60_->set_position(godot::Vector2(0.0F, height_of(ms_at_60_fps)));
    line_30_->set_position(godot::Vector2(0.0F, height_of(ms_at_30_fps)));
}

void E5PerfOverlay::refresh_text() {
    E5_PROFILE_SCOPE("E5PerfOverlay::refresh_text");

    if (running_.frames > 0) {
        const double frames = running_.frames;
        split_ = {.physics_ms = running_.physics_ms / frames,
                  .logic_ms = running_.logic_ms / frames,
                  .draw_ms = running_.draw_ms / frames,
                  .frames = running_.frames};
        running_ = {};
    }

    std::string text = detailed_ ? detailed_text() : short_text();
    if (!note_.empty()) {
        text += "\n" + note_;
    }
    label_->set_text(godot::String::utf8(text.c_str()));
    if (detailed_) {
        const godot::Vector2 size = label_->get_minimum_size();
        backdrop_->set_size(
            godot::Vector2(std::max(size.x, graph_width) + margin, label_->get_position().y + size.y + margin * 0.5F));
    }
}

std::string E5PerfOverlay::short_text() {
    const FrameTimeSummary frame = history_.summarize();
    const RenderMetrics metrics = sample_render_metrics(get_viewport()->get_viewport_rid());

    return std::format("FPS {:.0f}   frame {:.2f} ms (p99 {:.2f}, 1% low {:.0f} FPS)\n"
                       "CPU render {:.2f} ms   GPU render {:.2f} ms\n"
                       "process {:.2f} ms   physics {:.2f} ms\n"
                       "draw calls {}   objects {}   primitives {}\n"
                       "VRAM {:.1f} MB (textures {:.1f}, buffers {:.1f})   heap {:.1f} MB\n"
                       "nodes {}   [F3] overlay   [F4] details   [Esc] release mouse",
                       frame.average_fps, frame.average_ms, frame.p99_ms, frame.one_percent_low_fps,
                       metrics.cpu_render_ms, metrics.gpu_render_ms, metrics.process_ms, metrics.physics_ms,
                       metrics.draw_calls, metrics.visible_objects, metrics.primitives, metrics.video_memory_mb,
                       metrics.texture_memory_mb, metrics.buffer_memory_mb, metrics.static_memory_mb,
                       metrics.node_count);
}

std::string E5PerfOverlay::detailed_text() {
    const godot::OS* const os = godot::OS::get_singleton();
    const godot::DisplayServer* const display = godot::DisplayServer::get_singleton();
    const godot::RenderingServer* const rs = godot::RenderingServer::get_singleton();
    godot::Viewport* const viewport = get_viewport();
    const godot::RID viewport_rid = viewport->get_viewport_rid();

    if (system_line_.empty()) {
        const godot::Dictionary memory = os->get_memory_info();
        system_line_ =
            std::format("SYSTEM   {} {}   {} ({} threads)   {:.0f} GB memory\n"
                        "         {}   {} {} ({})",
                        text_of(os->get_name()), text_of(os->get_version()), text_of(os->get_processor_name()),
                        os->get_processor_count(), static_cast<double>(memory.get("physical", 0)) / bytes_per_gb,
                        text_of(rs->get_video_adapter_name()), text_of(rs->get_current_rendering_driver_name()),
                        text_of(rs->get_video_adapter_api_version()), text_of(rs->get_current_rendering_method()));
    }

    const FrameTimeSummary frame = history_.summarize();
    graph_top_ms_ = std::clamp(frame.p99_ms * 1.25, graph_least_top_ms, graph_most_top_ms);
    const RenderMetrics metrics = sample_render_metrics(viewport_rid);
    const DetailMetrics detail = sample_detail_metrics(viewport_rid);

    int over_30 = 0;
    int hitches = 0;
    double window_ms = 0.0;
    for (std::size_t i = 0; i < history_.size(); ++i) {
        const double ms = history_.sample(i);
        window_ms += ms;
        over_30 += ms > ms_at_30_fps ? 1 : 0;
        hitches += ms > hitch_ms ? 1 : 0;
    }

    const godot::DisplayServer::VSyncMode vsync = display->window_get_vsync_mode();
    const double refresh_hz = static_cast<double>(display->screen_get_refresh_rate());
    const int max_fps = godot::Engine::get_singleton()->get_max_fps();
    // The draw part waits for the display with V-Sync, so the measured render time on the
    // processor stands in for it here.
    const double cpu_ms = split_.physics_ms + split_.logic_ms + metrics.cpu_render_ms;
    const double rest_ms = std::max(frame.average_ms - split_.physics_ms - split_.logic_ms - split_.draw_ms, 0.0);
    const godot::Vector2i window = display->window_get_size();
    const double scale = static_cast<double>(viewport->get_scaling_3d_scale());

    // The world: only what is in the tree, so the numbers are right in any scene.
    godot::SceneTree* const tree = get_tree();
    int enemies = 0;
    int enemies_alive = 0;
    int enemies_mirrored = 0;
    const godot::TypedArray<godot::Node> enemy_nodes = tree->get_nodes_in_group(E5Enemy::group_name);
    for (const godot::Variant& node : enemy_nodes) {
        if (const auto* const enemy = godot::Object::cast_to<E5Enemy>(node)) {
            ++enemies;
            enemies_alive += enemy->is_alive() ? 1 : 0;
            enemies_mirrored += enemy->is_remote() ? 1 : 0;
        }
    }
    const std::int64_t remote_heroes = tree->get_nodes_in_group(E5PlayerController::remote_group_name).size();
    std::string place = "no hero in the scene";
    if (const auto* const hero =
            godot::Object::cast_to<godot::Node3D>(tree->get_first_node_in_group(E5PlayerController::group_name))) {
        const godot::Vector3 at = hero->get_global_position();
        place = std::format("hero at ({:.0f}, {:.0f}, {:.0f})", at.x, at.y, at.z);
    }
    if (const auto* const day =
            godot::Object::cast_to<E5DayNight>(tree->get_first_node_in_group(E5DayNight::group_name))) {
        place += std::format("   hour {:.1f}", day->get_hour());
    }

    // The network script knows the connection; this class must not know the script.
    std::string network = "not in the tree";
    if (godot::Node* const net = tree->get_root()->get_node_or_null("Net");
        net != nullptr && net->has_method("debug_text")) {
        network = text_of(net->call("debug_text"));
    }

    return std::format(
        "{} {}   {} build   {}\n"
        "{}\n"
        "FRAME    {:.0f} FPS   average {:.2f} ms   least {:.2f}   median {:.2f}   p95 {:.2f}   p99 {:.2f}   most "
        "{:.2f}\n"
        "         1% low {:.0f} FPS   over 33 ms: {}   over 50 ms: {}   (last {} frames, {:.1f} s)\n"
        "         graph: green line 16.7 ms (60 FPS), orange line 33.3 ms (30 FPS)\n"
        "TIME     of one frame: logic {:.2f} ms   physics {:.2f} ms   drawing and presenting {:.2f} ms   rest {:.2f} "
        "ms\n"
        "         render on the processor {:.2f} ms   on the graphics card {:.2f} ms   navigation {:.2f} ms\n"
        "         worst of the last second: whole frame {:.2f} ms   one physics step {:.2f} ms\n"
        "         probably limited by {}\n"
        "RENDER   draw calls {} (scene {}, shadows {}, interface {})\n"
        "         objects {} (scene {}, shadows {})   triangles {} (scene {}, shadows {})\n"
        "         window {}x{}   3D at {:.0f}x{:.0f} (scale {:.2f})   MSAA {}   V-Sync {}   {:.0f} Hz   cap {}\n"
        "         shader pipelines compiled {} (a hitch each time this grows)\n"
        "MEMORY   graphics {:.0f} MB (textures {:.0f}, buffers {:.0f})   heap {:.0f} MB (peak {:.0f})\n"
        "SCENE    nodes {} (outside the tree {})   objects {}   resources {}\n"
        "         physics: moving bodies {}   pairs {}   islands {}\n"
        "WORLD    {}   enemies alive {} of {} (mirrored {})   other heroes {}\n"
        "NETWORK  {}\n"
        "[F3] hide   [F4] short   [Shift+F4] save this as a file and copy it",
        text_of(godot::ProjectSettings::get_singleton()->get_setting("application/config/name")),
        text_of(godot::ProjectSettings::get_singleton()->get_setting("application/config/version")),
        os->is_debug_build() ? "debug" : "release",
        text_of(godot::Time::get_singleton()->get_datetime_string_from_system(false, true)), system_line_,
        frame.average_fps, frame.average_ms, frame.min_ms, frame.p50_ms, frame.p95_ms, frame.p99_ms, frame.max_ms,
        frame.one_percent_low_fps, over_30, hitches, history_.size(), window_ms / 1000.0, split_.logic_ms,
        split_.physics_ms, split_.draw_ms, rest_ms, metrics.cpu_render_ms, metrics.gpu_render_ms, detail.navigation_ms,
        metrics.process_ms, metrics.physics_ms,
        limit_guess(frame.average_ms, cpu_ms, metrics.gpu_render_ms, vsync != godot::DisplayServer::VSYNC_DISABLED,
                    refresh_hz, max_fps),
        metrics.draw_calls, detail.scene.draw_calls, detail.shadows.draw_calls, detail.interface.draw_calls,
        metrics.visible_objects, detail.scene.objects, detail.shadows.objects, metrics.primitives,
        detail.scene.primitives, detail.shadows.primitives, window.x, window.y, static_cast<double>(window.x) * scale,
        static_cast<double>(window.y) * scale, scale, msaa_name(viewport->get_msaa_3d()), vsync_name(vsync), refresh_hz,
        max_fps > 0 ? std::format("{} FPS", max_fps) : std::string("none"), detail.pipelines_compiled,
        metrics.video_memory_mb, metrics.texture_memory_mb, metrics.buffer_memory_mb, metrics.static_memory_mb,
        detail.static_memory_peak_mb, metrics.node_count, detail.orphan_node_count, detail.object_count,
        detail.resource_count, detail.physics_active_bodies, detail.physics_collision_pairs, detail.physics_islands,
        place, enemies_alive, enemies, enemies_mirrored, remote_heroes, network);
}

void E5PerfOverlay::save_report() {
    const std::string text = detailed_text();
    const godot::String report = godot::String::utf8(text.c_str());
    const godot::Ref<godot::FileAccess> file = godot::FileAccess::open(report_path, godot::FileAccess::WRITE);
    if (file.is_valid()) {
        file->store_string(report);
        godot::DisplayServer::get_singleton()->clipboard_set(report);
        note_ = "saved (and copied, paste it into a message): " +
                text_of(godot::ProjectSettings::get_singleton()->globalize_path(report_path));
    } else {
        note_ = "the report could not be written";
    }
    note_ms_left_ = note_ms;
    set_visible(true);
    refresh_text();
}

} // namespace e5::bridge

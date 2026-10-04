#include "perf_overlay.hpp"

#include "e5/core/profiling.hpp"
#include "render_metrics.hpp"

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>

#include <format>
#include <string>

namespace e5::bridge {
namespace {

// Rebuilding the text every frame would make the overlay itself show up in
// the numbers it reports; four times a second is plenty to read.
constexpr double refresh_interval_ms = 250.0;

} // namespace

void E5PerfOverlay::_ready() {
    add_to_group(group_name);
    set_layer(100); // above any game UI

    label_ = memnew(godot::Label);
    label_->set_position(godot::Vector2(12.0F, 10.0F));
    label_->add_theme_font_size_override("font_size", 14);
    label_->add_theme_color_override("font_color", godot::Color(1.0F, 1.0F, 1.0F));
    label_->add_theme_color_override("font_outline_color", godot::Color(0.0F, 0.0F, 0.0F));
    label_->add_theme_constant_override("outline_size", 5);
    add_child(label_);

    enable_render_time_measurement(get_viewport()->get_viewport_rid());
}

void E5PerfOverlay::_process(double /*delta*/) {
    const double frame_ms = clock_.tick_ms();
    if (frame_ms <= 0.0) {
        return; // first frame: no interval to measure yet
    }
    history_.record(frame_ms);

    ms_since_refresh_ += frame_ms;
    if (is_visible() && ms_since_refresh_ >= refresh_interval_ms) {
        ms_since_refresh_ = 0.0;
        refresh_text();
    }
}

void E5PerfOverlay::_unhandled_key_input(const godot::Ref<godot::InputEvent>& event) {
    const godot::Ref<godot::InputEventKey> key = event;
    if (key.is_valid() && key->is_pressed() && !key->is_echo() && key->get_keycode() == godot::KEY_F3) {
        set_visible(!is_visible());
    }
}

void E5PerfOverlay::refresh_text() {
    E5_PROFILE_SCOPE("E5PerfOverlay::refresh_text");

    const FrameTimeSummary frame = history_.summarize();
    const RenderMetrics metrics = sample_render_metrics(get_viewport()->get_viewport_rid());

    const std::string text =
        std::format("FPS {:.0f}   frame {:.2f} ms (p99 {:.2f}, 1% low {:.0f} FPS)\n"
                    "CPU render {:.2f} ms   GPU render {:.2f} ms\n"
                    "process {:.2f} ms   physics {:.2f} ms\n"
                    "draw calls {}   objects {}   primitives {}\n"
                    "VRAM {:.1f} MB (textures {:.1f}, buffers {:.1f})   heap {:.1f} MB\n"
                    "nodes {}   [F3] overlay   [Esc] release mouse",
                    frame.average_fps, frame.average_ms, frame.p99_ms, frame.one_percent_low_fps, metrics.cpu_render_ms,
                    metrics.gpu_render_ms, metrics.process_ms, metrics.physics_ms, metrics.draw_calls,
                    metrics.visible_objects, metrics.primitives, metrics.video_memory_mb, metrics.texture_memory_mb,
                    metrics.buffer_memory_mb, metrics.static_memory_mb, metrics.node_count);

    label_->set_text(godot::String::utf8(text.c_str()));
}

} // namespace e5::bridge

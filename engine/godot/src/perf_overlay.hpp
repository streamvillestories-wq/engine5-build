#pragma once

#include "e5/core/frame_stats.hpp"
#include "render_metrics.hpp"

#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

#include <cstdint>
#include <string>

namespace godot {
class ColorRect;
class Control;
class Label;
class Line2D;
} // namespace godot

namespace e5::bridge {

// On-screen performance statistics. F3 shows or hides them; F4 switches between the short
// readout (FPS, frame time, render time, draw calls, memory) and the detailed one: a graph of
// the frame times, where the time goes and what probably limits the frame rate, the render
// passes, the display settings, memory, scene, physics, world and network. Shift+F4 writes the
// detailed text to a file and the clipboard, so a player can send it.
class E5PerfOverlay : public godot::CanvasLayer {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto,cppcoreguidelines-special-member-functions)
    GDCLASS(E5PerfOverlay, godot::CanvasLayer)

public:
    static constexpr const char* group_name = "e5_perf_overlay";

    void _ready() override;
    void _process(double delta) override;
    void _unhandled_key_input(const godot::Ref<godot::InputEvent>& event) override;

    void set_detailed(bool detailed);
    [[nodiscard]] bool is_detailed() const { return detailed_; }

protected:
    static void _bind_methods() {}

private:
    // Where one frame's time on the main thread goes, in milliseconds.
    struct FrameSplit {
        double physics_ms = 0.0; // physics callbacks and the physics step
        double logic_ms = 0.0;   // every _process, timers, tweens, queued calls
        double draw_ms = 0.0;    // culling, draw lists, handing the frame over (waits with V-Sync)
        int frames = 0;
    };

    // The engine's own "process time" is the worst whole frame of the last second, drawing
    // included: it says nothing about where the time goes. These four signals bracket the
    // parts of a frame, and the clock is read at each.
    void on_physics_frame();
    void on_process_frame();
    void on_pre_draw();
    void on_post_draw();

    void refresh_text();
    void refresh_graph();
    void save_report();
    [[nodiscard]] std::string short_text();
    [[nodiscard]] std::string detailed_text();

    // 8 s of history at 60 FPS: stable percentiles, and a graph long enough to see a hitch
    // that has just happened.
    FrameTimeHistory history_{480};
    FrameClock clock_;
    double ms_since_refresh_ = 0.0;
    FrameSplit running_; // summed since the text was last rebuilt
    FrameSplit split_;   // the average of one frame over that time
    std::uint64_t physics_start_usec_ = 0;
    std::uint64_t process_start_usec_ = 0;
    std::uint64_t draw_start_usec_ = 0;
    double graph_top_ms_ = 40.0;
    double note_ms_left_ = 0.0; // how long the line about the saved report stays
    bool detailed_ = false;
    std::string note_;
    std::string system_line_; // processor, memory, graphics card: asked for once
    godot::PackedVector2Array graph_points_;
    // Non-owning children, owned by the scene tree.
    godot::Label* label_ = nullptr;
    godot::ColorRect* backdrop_ = nullptr;
    godot::Control* graph_ = nullptr;
    godot::Line2D* graph_line_ = nullptr;
    godot::ColorRect* line_60_ = nullptr;
    godot::ColorRect* line_30_ = nullptr;
};

} // namespace e5::bridge

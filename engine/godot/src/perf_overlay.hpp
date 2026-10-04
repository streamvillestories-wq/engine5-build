#pragma once

#include "e5/core/frame_stats.hpp"
#include "render_metrics.hpp"

#include <godot_cpp/classes/canvas_layer.hpp>

namespace godot {
class Label;
}

namespace e5::bridge {

// On-screen performance statistics (FPS, frame time, CPU/GPU render time,
// draw calls, primitives, memory). Toggle with F3.
class E5PerfOverlay : public godot::CanvasLayer {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto,cppcoreguidelines-special-member-functions)
    GDCLASS(E5PerfOverlay, godot::CanvasLayer)

public:
    static constexpr const char* group_name = "e5_perf_overlay";

    void _ready() override;
    void _process(double delta) override;
    void _unhandled_key_input(const godot::Ref<godot::InputEvent>& event) override;

protected:
    static void _bind_methods() {}

private:
    void refresh_text();

    // ~4 s of history at 60 FPS; enough for stable percentiles in the readout.
    FrameTimeHistory history_{240};
    FrameClock clock_;
    double ms_since_refresh_ = 0.0;
    godot::Label* label_ = nullptr; // non-owning child, owned by the scene tree
};

} // namespace e5::bridge

#pragma once

#include <godot_cpp/variant/rid.hpp>

#include <cstdint>

namespace e5::bridge {

// One frame's worth of engine counters, in display-friendly units.
struct RenderMetrics {
    double fps = 0.0;
    double cpu_render_ms = 0.0; // scene culling + draw-list building on the CPU
    double gpu_render_ms = 0.0; // measured GPU time for the viewport
    double process_ms = 0.0;    // all _process callbacks
    double physics_ms = 0.0;    // all _physics_process callbacks + physics step
    std::int64_t draw_calls = 0;
    std::int64_t visible_objects = 0;
    std::int64_t primitives = 0;
    double video_memory_mb = 0.0;
    double texture_memory_mb = 0.0;
    double buffer_memory_mb = 0.0;
    double static_memory_mb = 0.0; // engine heap allocations
    std::int64_t node_count = 0;
};

// Measures real frame-to-frame time from the monotonic clock. The `delta`
// Godot passes to _process must not be used for statistics: it is clamped
// (never below 1/8 of the physics step, i.e. 480 FPS at 60 Hz) and smoothed.
class FrameClock {
public:
    // Returns milliseconds since the previous call; 0 on the first call.
    [[nodiscard]] double tick_ms();

private:
    std::uint64_t last_usec_ = 0;
};

// GPU/CPU render timing is off by default in Godot because it has a small
// cost; it must be switched on per viewport before sample_render_metrics().
void enable_render_time_measurement(const godot::RID& viewport);

[[nodiscard]] RenderMetrics sample_render_metrics(const godot::RID& viewport);

} // namespace e5::bridge

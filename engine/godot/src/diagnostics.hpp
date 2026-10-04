#pragma once

#include "e5/core/config.hpp"
#include "e5/core/frame_stats.hpp"
#include "render_metrics.hpp"

#include <godot_cpp/classes/node.hpp>

#include <cstdint>

namespace e5::bridge {

// Per-scene diagnostics driver:
//  * marks frame boundaries for the profiler,
//  * applies runtime options passed after `--` on the command line,
//  * in `--benchmark` mode measures the scene for a fixed time, writes a JSON
//    report (and optional screenshot) and exits. The exit code is non-zero if
//    the run could not be completed, so scripts and CI can rely on it.
class E5Diagnostics : public godot::Node {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto,cppcoreguidelines-special-member-functions)
    GDCLASS(E5Diagnostics, godot::Node)

public:
    void _ready() override;
    void _process(double delta) override;

protected:
    static void _bind_methods() {}

private:
    struct MetricTotals {
        double cpu_render_ms = 0.0;
        double gpu_render_ms = 0.0;
        double process_ms = 0.0;
        double physics_ms = 0.0;
        double draw_calls = 0.0;
        double visible_objects = 0.0;
        double primitives = 0.0;
        double peak_video_memory_mb = 0.0;
        double peak_texture_memory_mb = 0.0;
        double peak_static_memory_mb = 0.0;
        std::int64_t samples = 0;
    };

    void drive_automated_input(double frame_ms);
    void run_benchmark_frame(double frame_ms);
    [[nodiscard]] bool write_report();
    [[nodiscard]] bool save_screenshot() const;
    void quit(int exit_code);

    RuntimeConfig config_;
    // Covers a 10 s run even at >10,000 FPS without reallocating.
    static constexpr std::size_t history_capacity = 131072;
    FrameTimeHistory history_{history_capacity};
    MetricTotals totals_;
    FrameClock clock_;
    double warmup_elapsed_ms_ = 0.0;
    double measured_elapsed_ms_ = 0.0;
    bool finished_ = false;
    bool camera_yaw_applied_ = false;
    double auto_shoot_elapsed_ms_ = 0.0;
    double auto_block_elapsed_ms_ = 0.0;
};

} // namespace e5::bridge

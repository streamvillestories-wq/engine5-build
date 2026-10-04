#include "render_metrics.hpp"

#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/time.hpp>

namespace e5::bridge {
namespace {

constexpr double bytes_per_mb = 1024.0 * 1024.0;
constexpr double ms_per_second = 1000.0;

} // namespace

double FrameClock::tick_ms() {
    const std::uint64_t now_usec = godot::Time::get_singleton()->get_ticks_usec();
    const double elapsed_ms = last_usec_ == 0 ? 0.0 : static_cast<double>(now_usec - last_usec_) / 1000.0;
    last_usec_ = now_usec;
    return elapsed_ms;
}

void enable_render_time_measurement(const godot::RID& viewport) {
    godot::RenderingServer::get_singleton()->viewport_set_measure_render_time(viewport, true);
}

RenderMetrics sample_render_metrics(const godot::RID& viewport) {
    using godot::Performance;
    const Performance* const perf = Performance::get_singleton();
    const godot::RenderingServer* const rs = godot::RenderingServer::get_singleton();

    RenderMetrics metrics;
    metrics.fps = perf->get_monitor(Performance::TIME_FPS);
    metrics.cpu_render_ms = rs->viewport_get_measured_render_time_cpu(viewport) + rs->get_frame_setup_time_cpu();
    metrics.gpu_render_ms = rs->viewport_get_measured_render_time_gpu(viewport);
    metrics.process_ms = perf->get_monitor(Performance::TIME_PROCESS) * ms_per_second;
    metrics.physics_ms = perf->get_monitor(Performance::TIME_PHYSICS_PROCESS) * ms_per_second;
    metrics.draw_calls = static_cast<std::int64_t>(perf->get_monitor(Performance::RENDER_TOTAL_DRAW_CALLS_IN_FRAME));
    metrics.visible_objects = static_cast<std::int64_t>(perf->get_monitor(Performance::RENDER_TOTAL_OBJECTS_IN_FRAME));
    metrics.primitives = static_cast<std::int64_t>(perf->get_monitor(Performance::RENDER_TOTAL_PRIMITIVES_IN_FRAME));
    metrics.video_memory_mb = perf->get_monitor(Performance::RENDER_VIDEO_MEM_USED) / bytes_per_mb;
    metrics.texture_memory_mb = perf->get_monitor(Performance::RENDER_TEXTURE_MEM_USED) / bytes_per_mb;
    metrics.buffer_memory_mb = perf->get_monitor(Performance::RENDER_BUFFER_MEM_USED) / bytes_per_mb;
    metrics.static_memory_mb = perf->get_monitor(Performance::MEMORY_STATIC) / bytes_per_mb;
    metrics.node_count = static_cast<std::int64_t>(perf->get_monitor(Performance::OBJECT_NODE_COUNT));
    return metrics;
}

DetailMetrics sample_detail_metrics(const godot::RID& viewport) {
    using godot::Performance;
    using godot::RenderingServer;
    const Performance* const perf = Performance::get_singleton();
    RenderingServer* const rs = RenderingServer::get_singleton();

    const auto count = [perf](Performance::Monitor monitor) {
        return static_cast<std::int64_t>(perf->get_monitor(monitor));
    };
    const auto pass = [rs, &viewport](RenderingServer::ViewportRenderInfoType type) {
        PassMetrics metrics;
        metrics.draw_calls =
            rs->viewport_get_render_info(viewport, type, RenderingServer::VIEWPORT_RENDER_INFO_DRAW_CALLS_IN_FRAME);
        metrics.objects =
            rs->viewport_get_render_info(viewport, type, RenderingServer::VIEWPORT_RENDER_INFO_OBJECTS_IN_FRAME);
        metrics.primitives =
            rs->viewport_get_render_info(viewport, type, RenderingServer::VIEWPORT_RENDER_INFO_PRIMITIVES_IN_FRAME);
        return metrics;
    };

    DetailMetrics metrics;
    metrics.navigation_ms = perf->get_monitor(Performance::TIME_NAVIGATION_PROCESS) * ms_per_second;
    metrics.scene = pass(RenderingServer::VIEWPORT_RENDER_INFO_TYPE_VISIBLE);
    metrics.shadows = pass(RenderingServer::VIEWPORT_RENDER_INFO_TYPE_SHADOW);
    metrics.interface = pass(RenderingServer::VIEWPORT_RENDER_INFO_TYPE_CANVAS);
    metrics.pipelines_compiled =
        count(Performance::PIPELINE_COMPILATIONS_CANVAS) + count(Performance::PIPELINE_COMPILATIONS_MESH) +
        count(Performance::PIPELINE_COMPILATIONS_SURFACE) + count(Performance::PIPELINE_COMPILATIONS_DRAW) +
        count(Performance::PIPELINE_COMPILATIONS_SPECIALIZATION);
    metrics.static_memory_peak_mb = perf->get_monitor(Performance::MEMORY_STATIC_MAX) / bytes_per_mb;
    metrics.object_count = count(Performance::OBJECT_COUNT);
    metrics.resource_count = count(Performance::OBJECT_RESOURCE_COUNT);
    metrics.orphan_node_count = count(Performance::OBJECT_ORPHAN_NODE_COUNT);
    metrics.physics_active_bodies = count(Performance::PHYSICS_3D_ACTIVE_OBJECTS);
    metrics.physics_collision_pairs = count(Performance::PHYSICS_3D_COLLISION_PAIRS);
    metrics.physics_islands = count(Performance::PHYSICS_3D_ISLAND_COUNT);
    return metrics;
}

} // namespace e5::bridge

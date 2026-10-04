#pragma once

#include <cstddef>
#include <vector>

namespace e5 {

struct FrameTimeSummary {
    std::size_t sample_count = 0;
    double average_ms = 0.0;
    double min_ms = 0.0;
    double max_ms = 0.0;
    double p50_ms = 0.0;
    double p95_ms = 0.0;
    double p99_ms = 0.0;
    double average_fps = 0.0;
    // Average FPS over the slowest 1% of frames: the stutter the player feels.
    double one_percent_low_fps = 0.0;
};

// Fixed-capacity ring buffer of frame times. All memory is allocated in the
// constructor so recording a sample every frame never touches the heap.
class FrameTimeHistory {
public:
    explicit FrameTimeHistory(std::size_t capacity);

    void record(double frame_ms) noexcept;
    void clear() noexcept;

    [[nodiscard]] std::size_t size() const noexcept { return count_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return samples_.size(); }

    // Not const: reuses an internal scratch buffer for the percentile sort.
    [[nodiscard]] FrameTimeSummary summarize();

private:
    std::vector<double> samples_;
    std::vector<double> scratch_;
    std::size_t next_ = 0;
    std::size_t count_ = 0;
};

} // namespace e5

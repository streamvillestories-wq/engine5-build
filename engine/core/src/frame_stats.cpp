#include "e5/core/frame_stats.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace e5 {
namespace {

// Nearest-rank percentile on an ascending-sorted, non-empty range.
double percentile(const std::vector<double>& sorted, std::size_t count, double fraction) {
    const auto rank = static_cast<std::size_t>(std::ceil(fraction * static_cast<double>(count)));
    return sorted[std::clamp<std::size_t>(rank, 1, count) - 1];
}

} // namespace

FrameTimeHistory::FrameTimeHistory(std::size_t capacity)
    : samples_(std::max<std::size_t>(capacity, 1)), scratch_(samples_.size()) {}

void FrameTimeHistory::record(double frame_ms) noexcept {
    samples_[next_] = frame_ms;
    next_ = (next_ + 1) % samples_.size();
    count_ = std::min(count_ + 1, samples_.size());
}

void FrameTimeHistory::clear() noexcept {
    next_ = 0;
    count_ = 0;
}

double FrameTimeHistory::sample(std::size_t index) const noexcept {
    if (index >= count_) {
        return 0.0;
    }
    // Once the buffer has wrapped, the oldest sample is the one about to be overwritten.
    const std::size_t oldest = count_ < samples_.size() ? 0 : next_;
    return samples_[(oldest + index) % samples_.size()];
}

FrameTimeSummary FrameTimeHistory::summarize() {
    FrameTimeSummary summary;
    summary.sample_count = count_;
    if (count_ == 0) {
        return summary;
    }

    // Until the buffer wraps, valid samples occupy [0, count_); afterwards all of it.
    const auto first = samples_.begin();
    const auto last = first + static_cast<std::ptrdiff_t>(count_);
    const auto scratch_last = std::copy(first, last, scratch_.begin());
    std::sort(scratch_.begin(), scratch_last);

    const double total = std::accumulate(scratch_.begin(), scratch_last, 0.0);
    summary.average_ms = total / static_cast<double>(count_);
    summary.min_ms = scratch_.front();
    summary.max_ms = scratch_[count_ - 1];
    summary.p50_ms = percentile(scratch_, count_, 0.50);
    summary.p95_ms = percentile(scratch_, count_, 0.95);
    summary.p99_ms = percentile(scratch_, count_, 0.99);
    summary.average_fps = summary.average_ms > 0.0 ? 1000.0 / summary.average_ms : 0.0;

    const std::size_t slow_count = std::max<std::size_t>(count_ / 100, 1);
    const double slow_total =
        std::accumulate(scratch_last - static_cast<std::ptrdiff_t>(slow_count), scratch_last, 0.0);
    const double slow_average = slow_total / static_cast<double>(slow_count);
    summary.one_percent_low_fps = slow_average > 0.0 ? 1000.0 / slow_average : 0.0;

    return summary;
}

} // namespace e5

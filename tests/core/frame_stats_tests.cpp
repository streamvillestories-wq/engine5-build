#include "e5/core/frame_stats.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

TEST_CASE("empty history summarizes to zeros", "[frame_stats]") {
    e5::FrameTimeHistory history(8);
    const auto summary = history.summarize();
    CHECK(summary.sample_count == 0);
    CHECK(summary.average_ms == 0.0);
    CHECK(summary.average_fps == 0.0);
}

TEST_CASE("summary statistics over 1..100 ms", "[frame_stats]") {
    e5::FrameTimeHistory history(100);
    for (int i = 1; i <= 100; ++i) {
        history.record(static_cast<double>(i));
    }
    const auto summary = history.summarize();
    CHECK(summary.sample_count == 100);
    CHECK(summary.average_ms == Approx(50.5));
    CHECK(summary.min_ms == Approx(1.0));
    CHECK(summary.max_ms == Approx(100.0));
    CHECK(summary.p50_ms == Approx(50.0));
    CHECK(summary.p95_ms == Approx(95.0));
    CHECK(summary.p99_ms == Approx(99.0));
    CHECK(summary.average_fps == Approx(1000.0 / 50.5));
    // Slowest 1% of 100 samples is the single 100 ms frame.
    CHECK(summary.one_percent_low_fps == Approx(10.0));
}

TEST_CASE("ring buffer keeps only the newest samples", "[frame_stats]") {
    e5::FrameTimeHistory history(4);
    for (const double ms : {100.0, 100.0, 10.0, 20.0, 30.0, 40.0}) {
        history.record(ms);
    }
    CHECK(history.size() == 4);
    const auto summary = history.summarize();
    CHECK(summary.max_ms == Approx(40.0));
    CHECK(summary.min_ms == Approx(10.0));
    CHECK(summary.average_ms == Approx(25.0));
}

TEST_CASE("clear resets the history", "[frame_stats]") {
    e5::FrameTimeHistory history(4);
    history.record(16.0);
    history.clear();
    CHECK(history.size() == 0);
    CHECK(history.summarize().sample_count == 0);
}

TEST_CASE("zero capacity is clamped to one", "[frame_stats]") {
    e5::FrameTimeHistory history(0);
    history.record(5.0);
    history.record(7.0);
    CHECK(history.capacity() == 1);
    CHECK(history.summarize().average_ms == Approx(7.0));
}

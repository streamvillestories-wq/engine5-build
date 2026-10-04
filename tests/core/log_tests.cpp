#include "e5/core/log.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <utility>
#include <vector>

TEST_CASE("logger formats and filters by level", "[log]") {
    std::vector<std::pair<e5::LogLevel, std::string>> received;
    e5::Logger logger(
        [&received](e5::LogLevel level, std::string_view message) { received.emplace_back(level, message); },
        e5::LogLevel::Info);

    logger.log(e5::LogLevel::Debug, "hidden {}", 1);
    logger.info("loaded {} assets in {:.1f} ms", 3, 12.25);
    logger.error("failed: {}", "disk");

    REQUIRE(received.size() == 2);
    CHECK(received[0].first == e5::LogLevel::Info);
    CHECK(received[0].second == "loaded 3 assets in 12.2 ms");
    CHECK(received[1].first == e5::LogLevel::Error);
    CHECK(received[1].second == "failed: disk");

    logger.set_min_level(e5::LogLevel::Debug);
    logger.log(e5::LogLevel::Debug, "now visible");
    CHECK(received.size() == 3);
}

TEST_CASE("logger without a sink is a safe no-op", "[log]") {
    const e5::Logger logger(e5::LogSink{});
    logger.error("nobody listens");
    SUCCEED();
}

TEST_CASE("log level names", "[log]") {
    CHECK(e5::to_string(e5::LogLevel::Warning) == "warning");
    CHECK(e5::to_string(e5::LogLevel::Trace) == "trace");
}

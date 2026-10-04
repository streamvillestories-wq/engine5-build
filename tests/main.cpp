#include "e5/core/profiling.hpp"

#include <catch2/catch_session.hpp>

int main(int argc, char* argv[]) {
    // Code under test may contain profiling markers, which need a live session.
    const e5::ProfilerSession profiler;
    return Catch::Session().run(argc, argv);
}

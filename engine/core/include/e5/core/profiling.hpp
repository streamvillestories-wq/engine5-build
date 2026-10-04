#pragma once

// Profiling abstraction. Engine and game code use only the E5_PROFILE_* macros,
// never a profiler's API directly, so the backend can be swapped or compiled out
// (E5_ENABLE_TRACY=OFF, as in the Release preset) with zero residual cost.
//
// These are macros rather than functions because Tracy needs a static
// source-location record at each call site.

namespace e5 {

// Owns the profiler's lifetime. Exactly one must be alive while any E5_PROFILE_*
// macro executes: the Godot bridge holds one between extension init and
// terminate, the unit-test runner holds one in main(). Explicit (rather than a
// static object) because static teardown inside an unloading DLL deadlocks.
// A no-op when profiling is compiled out.
class ProfilerSession {
public:
    ProfilerSession();
    ~ProfilerSession();
    ProfilerSession(const ProfilerSession&) = delete;
    ProfilerSession& operator=(const ProfilerSession&) = delete;
    ProfilerSession(ProfilerSession&&) = delete;
    ProfilerSession& operator=(ProfilerSession&&) = delete;
};

} // namespace e5

#if defined(E5_PROFILING_TRACY)

#include <tracy/Tracy.hpp>

// Marks the end of a frame. Call exactly once per rendered frame.
#define E5_PROFILE_FRAME() FrameMark
// Times the enclosing scope, named after the enclosing function.
#define E5_PROFILE_FUNCTION() ZoneScoped
// Times the enclosing scope under a string-literal name.
#define E5_PROFILE_SCOPE(name) ZoneScopedN(name)
// Plots a numeric value over time under a string-literal name.
#define E5_PROFILE_PLOT(name, value) TracyPlot(name, value)

#else

#define E5_PROFILE_FRAME() static_cast<void>(0)
#define E5_PROFILE_FUNCTION() static_cast<void>(0)
#define E5_PROFILE_SCOPE(name) static_cast<void>(0)
#define E5_PROFILE_PLOT(name, value) static_cast<void>(0)

#endif

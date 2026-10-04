#include "e5/core/profiling.hpp"

namespace e5 {

#if defined(E5_PROFILING_TRACY)

ProfilerSession::ProfilerSession() {
    tracy::StartupProfiler();
}

ProfilerSession::~ProfilerSession() {
    tracy::ShutdownProfiler();
}

#else

ProfilerSession::ProfilerSession() = default;
ProfilerSession::~ProfilerSession() = default;

#endif

} // namespace e5

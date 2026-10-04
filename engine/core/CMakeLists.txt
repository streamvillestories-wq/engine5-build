# e5_core: engine-agnostic foundation. Must never depend on Godot.
add_library(e5_core STATIC
    src/config.cpp
    src/frame_stats.cpp
    src/log.cpp
    src/profiling.cpp
)
add_library(e5::core ALIAS e5_core)

target_sources(e5_core PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
    include/e5/core/config.hpp
    include/e5/core/error.hpp
    include/e5/core/frame_stats.hpp
    include/e5/core/log.hpp
    include/e5/core/profiling.hpp
)

if(E5_ENABLE_TRACY)
    target_link_libraries(e5_core PUBLIC Tracy::TracyClient)
    target_compile_definitions(e5_core PUBLIC E5_PROFILING_TRACY=1)
endif()

e5_set_warnings(e5_core)
e5_enable_sanitizers(e5_core)

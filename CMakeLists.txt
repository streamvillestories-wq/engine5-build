cmake_minimum_required(VERSION 3.28)

# Dynamic CRT everywhere (/MD, /MDd in Debug). Must be set before godot-cpp is
# added: godot-cpp only supplies its own default (static CRT) when this is not
# already defined, and every object in one link must agree on the CRT.
# Matches the vcpkg triplet x64-windows-static-md. See docs/DECISIONS.md (D-007).
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "MSVC runtime")

project(engine5 VERSION 0.1.0 DESCRIPTION "Engine5 - C++ game foundation on Godot" LANGUAGES CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not supported. Use: cmake --preset <name>")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(E5_BUILD_GDEXTENSION "Build the Godot GDExtension bridge library" ON)
option(E5_BUILD_TESTS "Build unit tests" ON)
option(E5_ENABLE_TRACY "Compile Tracy profiler instrumentation into the build" ON)
option(E5_ENABLE_ASAN "Build with AddressSanitizer (standalone libraries and tests only)" OFF)
option(E5_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(Sanitizers)

if(E5_ENABLE_ASAN AND E5_BUILD_GDEXTENSION)
    # The official Godot binary is not ASan-instrumented; loading an ASan DLL into it is unsupported.
    message(FATAL_ERROR "E5_ENABLE_ASAN requires E5_BUILD_GDEXTENSION=OFF (use the 'asan' preset).")
endif()

if(E5_ENABLE_TRACY)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tracy/CMakeLists.txt")
        message(FATAL_ERROR "third_party/tracy is missing. Run: git submodule update --init --recursive")
    endif()
    set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
    set(TRACY_STATIC ON CACHE BOOL "" FORCE)
    # Collect nothing until a profiler connects, so idle overhead and memory stay flat.
    set(TRACY_ON_DEMAND ON CACHE BOOL "" FORCE)
    # The profiler lives inside a DLL that Godot unloads at shutdown. Tracy's default
    # static lifetime joins its worker threads during DLL unload, under the Windows
    # loader lock, and deadlocks. Manual lifetime lets e5::ProfilerSession start and
    # stop it from the extension's init/terminate callbacks instead.
    set(TRACY_MANUAL_LIFETIME ON CACHE BOOL "" FORCE)
    # Godot installs its own crash handler; do not replace it.
    set(TRACY_NO_CRASH_HANDLER ON CACHE BOOL "" FORCE)
    add_subdirectory(third_party/tracy EXCLUDE_FROM_ALL SYSTEM)
    # Required by TRACY_MANUAL_LIFETIME; Tracy has no CMake option for it.
    target_compile_definitions(TracyClient PUBLIC TRACY_DELAYED_INIT)
endif()

add_subdirectory(engine/core)
add_subdirectory(engine/gameplay)

if(E5_BUILD_GDEXTENSION)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/godot-cpp/CMakeLists.txt")
        message(FATAL_ERROR "third_party/godot-cpp is missing. Run: git submodule update --init --recursive")
    endif()
    set(GODOTCPP_API_VERSION "4.7" CACHE STRING "Godot API version the extension targets")
    set(GODOTCPP_SYSTEM_HEADERS ON CACHE BOOL "Treat godot-cpp headers as system headers")
    # Keep exception settings identical across every library in the DLL; mixing
    # _HAS_EXCEPTIONS=0 and =1 translation units is an ODR hazard with the MSVC STL.
    set(GODOTCPP_DISABLE_EXCEPTIONS OFF CACHE BOOL "Keep exceptions enabled, consistent with engine libs")
    set(GODOTCPP_USE_STATIC_CPP OFF CACHE BOOL "Dynamic CRT, see CMAKE_MSVC_RUNTIME_LIBRARY above")
    add_subdirectory(third_party/godot-cpp EXCLUDE_FROM_ALL SYSTEM)
    add_subdirectory(engine/godot)
endif()

if(E5_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

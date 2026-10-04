find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

add_executable(e5_unit_tests
    main.cpp
    core/config_tests.cpp
    core/frame_stats_tests.cpp
    core/log_tests.cpp
    gameplay/bird_flight_tests.cpp
    gameplay/bow_state_tests.cpp
    gameplay/bow_string_tests.cpp
    gameplay/character_motor_tests.cpp
    gameplay/cloth_rig_tests.cpp
    gameplay/enemy_tests.cpp
    gameplay/island_tests.cpp
    gameplay/paths_tests.cpp
    gameplay/trails_tests.cpp
    gameplay/projectile_tests.cpp
    gameplay/scatter_tests.cpp
    gameplay/skills_tests.cpp
    gameplay/vitals_tests.cpp
    gameplay/inventory_tests.cpp
    gameplay/day_cycle_tests.cpp
    gameplay/patrol_tests.cpp
)
target_link_libraries(e5_unit_tests PRIVATE e5::core e5::gameplay Catch2::Catch2)
e5_set_warnings(e5_unit_tests)
e5_enable_sanitizers(e5_unit_tests)

# One CTest entry for the whole executable rather than catch_discover_tests():
# per-case processes each pay profiler start-up/shutdown, which made the suite
# 20x slower. Catch2 still reports every failing case by name.
add_test(NAME engine.unit COMMAND e5_unit_tests)
set_tests_properties(engine.unit PROPERTIES LABELS unit)

# Asset pipeline tests (pure Python, no third-party packages).
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    add_test(NAME asset_pipeline.unit
        COMMAND ${Python3_EXECUTABLE} -m unittest discover -s "${CMAKE_SOURCE_DIR}/tools/asset_pipeline/tests" -t "${CMAKE_SOURCE_DIR}/tools/asset_pipeline"
    )
    set_tests_properties(asset_pipeline.unit PROPERTIES LABELS unit)
endif()

# End-to-end test: launches the real game in Godot, runs the benchmark scene,
# and checks the report (extension loaded, GLB rendered, collision held, etc.).
# Needs a GPU and a desktop session, so it is opt-in via the 'integration' label.
if(E5_BUILD_GDEXTENSION)
    find_program(E5_POWERSHELL NAMES pwsh powershell)
    if(E5_POWERSHELL)
        add_test(NAME integration.scene
            COMMAND ${E5_POWERSHELL} -NoProfile -ExecutionPolicy Bypass -File "${CMAKE_SOURCE_DIR}/scripts/run_scene_test.ps1"
        )
        set_tests_properties(integration.scene PROPERTIES LABELS integration TIMEOUT 300)
    endif()
endif()

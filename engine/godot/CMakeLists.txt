# e5_godot: the GDExtension bridge. The only place Godot types are allowed.
# It adapts Godot nodes to the engine-agnostic libraries (e5::core, e5::gameplay).
add_library(e5_godot SHARED
    src/aim_offset.cpp
    src/aim_offset.hpp
    src/character_animator.cpp
    src/character_animator.hpp
    src/arrow.cpp
    src/arrow.hpp
    src/arrow_rain.cpp
    src/bird.cpp
    src/black_hole.cpp
    src/black_hole.hpp
    src/combat.cpp
    src/enemy.cpp
    src/forest.cpp
    src/arrow_rain.hpp
    src/skill_bar_hud.cpp
    src/skill_bar_hud.hpp
    src/lightning_arc.cpp
    src/lingering.cpp
    src/lingering.hpp
    src/lightning_arc.hpp
    src/spell_bolt.cpp
    src/spell_bolt.hpp
    src/target.cpp
    src/terrain.cpp
    src/target.hpp
    src/bow_string.cpp
    src/bow_string.hpp
    src/cloth_simulator.cpp
    src/hair_root.cpp
    src/hair_root.hpp
    src/cloth_simulator.hpp
    src/diagnostics.cpp
    src/diagnostics.hpp
    src/effect.cpp
    src/effect.hpp
    src/godot_log.cpp
    src/day_night.cpp
    src/spawn_point.cpp
    src/spawn_point.hpp
    src/wanderer.cpp
    src/wanderer.hpp
    src/day_night.hpp
    src/health_hud.cpp
    src/health_hud.hpp
    src/inventory.cpp
    src/inventory.hpp
    src/pickup.cpp
    src/pickup.hpp
    src/godot_log.hpp
    src/input_actions.cpp
    src/input_actions.hpp
    src/perf_overlay.cpp
    src/perf_overlay.hpp
    src/player_controller.cpp
    src/player_controller.hpp
    src/register_types.cpp
    src/render_metrics.cpp
    src/render_metrics.hpp
)

target_link_libraries(e5_godot PRIVATE e5::core e5::gameplay godot::cpp)
target_compile_definitions(e5_godot PRIVATE
    E5_BUILD_CONFIG="$<CONFIG>"
    E5_VERSION="${PROJECT_VERSION}"
)

e5_set_warnings(e5_godot)

# Godot loads the library by the name listed in game/bin/engine5.gdextension:
#   engine5.windows.template_debug.x86_64.dll / engine5.windows.template_release.x86_64.dll
# Debug and RelWithDebInfo both produce the template_debug name (last build wins).
get_target_property(E5_GODOT_SUFFIX godot::cpp GODOTCPP_SUFFIX)
set(E5_GODOT_BIN_DIR "${CMAKE_SOURCE_DIR}/game/bin")

set_target_properties(e5_godot PROPERTIES
    PREFIX ""
    OUTPUT_NAME "engine5${E5_GODOT_SUFFIX}"
)

# Deploy step that runs on every build, not only when the library relinks.
# Debug and RelWithDebInfo share one file name in game/bin; linking straight
# into game/bin let an up-to-date preset leave the other preset's library
# there, so the editor silently ran the wrong build.
set(E5_DEPLOY_COMMANDS
    COMMAND ${CMAKE_COMMAND} -E make_directory "${E5_GODOT_BIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:e5_godot>" "${E5_GODOT_BIN_DIR}/"
)
# Release builds produce no PDB.
if(MSVC AND CMAKE_BUILD_TYPE MATCHES "^(Debug|RelWithDebInfo)$")
    list(APPEND E5_DEPLOY_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_PDB_FILE:e5_godot>" "${E5_GODOT_BIN_DIR}/"
    )
endif()
add_custom_target(e5_godot_deploy ALL ${E5_DEPLOY_COMMANDS} DEPENDS e5_godot VERBATIM
    COMMENT "Deploying extension library to game/bin"
)

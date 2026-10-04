# e5_gameplay: engine-agnostic gameplay logic (pure functions over plain data).
# Keeping it free of Godot types is what makes it unit-testable and portable.
add_library(e5_gameplay STATIC
    src/bird_flight.cpp
    src/bow_state.cpp
    src/bow_string.cpp
    src/character_motor.cpp
    src/cloth_rig.cpp
    src/enemy.cpp
    src/island.cpp
    src/paths.cpp
    src/projectile.cpp
    src/scatter.cpp
    src/skills.cpp
    src/vitals.cpp
    src/inventory.cpp
    src/day_cycle.cpp
    src/patrol.cpp
    src/trails.cpp
)
add_library(e5::gameplay ALIAS e5_gameplay)

target_sources(e5_gameplay PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
    include/e5/gameplay/bird_flight.hpp
    include/e5/gameplay/bow_state.hpp
    include/e5/gameplay/bow_string.hpp
    include/e5/gameplay/character_motor.hpp
    include/e5/gameplay/cloth_rig.hpp
    include/e5/gameplay/enemy.hpp
    include/e5/gameplay/island.hpp
    include/e5/gameplay/paths.hpp
    include/e5/gameplay/projectile.hpp
    include/e5/gameplay/scatter.hpp
    include/e5/gameplay/skills.hpp
    include/e5/gameplay/vitals.hpp
    include/e5/gameplay/inventory.hpp
    include/e5/gameplay/day_cycle.hpp
    include/e5/gameplay/patrol.hpp
    include/e5/gameplay/trails.hpp
)

target_link_libraries(e5_gameplay PUBLIC e5::core)

e5_set_warnings(e5_gameplay)
e5_enable_sanitizers(e5_gameplay)

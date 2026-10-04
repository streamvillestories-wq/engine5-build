// GDExtension entry point: the only symbol Godot looks up in the library.

#include "aim_offset.hpp"
#include "arrow.hpp"
#include "arrow_rain.hpp"
#include "bird.hpp"
#include "black_hole.hpp"
#include "bow_string.hpp"
#include "cloth_simulator.hpp"
#include "day_night.hpp"
#include "diagnostics.hpp"
#include "e5/core/profiling.hpp"
#include "effect.hpp"
#include "enemy.hpp"
#include "forest.hpp"
#include "health_hud.hpp"
#include "inventory.hpp"
#include "lightning_arc.hpp"
#include "perf_overlay.hpp"
#include "pickup.hpp"
#include "player_controller.hpp"
#include "skill_bar_hud.hpp"
#include "spawn_point.hpp"
#include "spell_bolt.hpp"
#include "target.hpp"
#include "terrain.hpp"
#include "wanderer.hpp"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include <optional>

namespace {

// Module-level state is unavoidable here: Godot calls two free functions and
// the session must live exactly between them (also across hot reloads).
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::optional<e5::ProfilerSession> profiler_session;

void initialize_engine5(godot::ModuleInitializationLevel level) {
    if (level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    profiler_session.emplace();

    // Runtime classes: the editor shows their properties but does not execute
    // their callbacks, so gameplay code never runs inside the editor.
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5PlayerController);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5PerfOverlay);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5Diagnostics);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5ClothSimulator);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5BowString);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5Arrow);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5AimOffset);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5Effect);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5ArrowRain);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5SkillBarHud);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5HealthHud);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5Inventory);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5DayNight);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5Wanderer);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5Pickup);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5SpellBolt);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5LightningArc);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5BlackHole);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5Target);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5Bird);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5Enemy);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5SpawnPoint);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5Forest);
    GDREGISTER_RUNTIME_CLASS(e5::bridge::E5Terrain);
}

void uninitialize_engine5(godot::ModuleInitializationLevel level) {
    if (level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    profiler_session.reset();
}

} // namespace

extern "C" {

GDExtensionBool GDE_EXPORT engine5_library_init(GDExtensionInterfaceGetProcAddress get_proc_address,
                                                GDExtensionClassLibraryPtr library,
                                                GDExtensionInitialization* initialization) {
    const godot::GDExtensionBinding::InitObject init(get_proc_address, library, initialization);
    init.register_initializer(initialize_engine5);
    init.register_terminator(uninitialize_engine5);
    init.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SCENE);
    return init.init();
}

} // extern "C"

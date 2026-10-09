#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <vector>

namespace e5::bridge {

class E5Enemy;

// Something an arrow leaves behind that goes on working for a while: a cloud of venom that
// hurts whatever stands in it, or brambles that hold what they caught.
//
// It hurts through combat::blast, like every other area effect. On another player's machine
// the arrow that made it is a replay with no damage: the cloud is then only shown.
class E5Lingering : public godot::Node3D {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5Lingering, godot::Node3D)

public:
    struct Setup {
        float radius = 2.5F;                   // metres
        float seconds = 5.0F;                  // how long it works
        float tick_damage = 0.0F;              // to everything inside, every `tick_seconds`
        float tick_seconds = 0.5F;             //
        float root_seconds = 0.0F;             // > 0: enemies inside when it appears cannot move for so long
        godot::Ref<godot::PackedScene> visual; // shown at its middle for as long as it works
        godot::Ref<godot::PackedScene> cage;   // shown round every enemy it holds
    };

    // `follows`: an enemy it stays on while that one lives (the one the venom arrow struck:
    // it carries the cloud to the others). May be null.
    static E5Lingering* spawn(godot::Node* parent, const godot::Vector3& position, const Setup& setup,
                              E5Enemy* follows);

    void _physics_process(double delta) override;

protected:
    static void _bind_methods() {}

private:
    Setup setup_;
    float age_ = 0.0F;
    float until_tick_ = 0.0F;
    bool ended_ = false;
    std::uint64_t follows_ = 0;         // instance id, or 0
    godot::Node3D* visual_ = nullptr;   // non-owning child
    std::vector<godot::Node3D*> cages_; // non-owning children
};

} // namespace e5::bridge

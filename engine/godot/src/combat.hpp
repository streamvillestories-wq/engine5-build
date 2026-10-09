#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector3.hpp>

// What a hit does to the thing it lands on. Arrows, explosions, kicks and birds
// all go through here, so everything that can be hit reacts to every skill.
namespace e5::bridge::combat {

// In a shared game a hit can also land on another player's hero (a remote
// E5PlayerController): she does not lose health here, the damage is collected for her own
// machine, which decides (E5PlayerController::take_outgoing_damage).

// A direct hit on `struck` (the collider an arrow ran into, the thing a bird
// pecked). Enemies take `damage`; practice targets score by ring, times
// `score_multiplier`. Returns false if `struck` is neither.
bool hit(godot::Object* struck, const godot::Vector3& position, float damage, int score_multiplier = 1);

// An area effect: everything within `radius` of `centre` is hit. `context` is
// any node in the scene. Returns how many things were hit.
int blast(godot::Node* context, const godot::Vector3& centre, float radius, float damage, int score_multiplier = 1);

// Lightning that leaps on from where something struck: from `from` to the nearest enemy within
// `reach` that it has not touched (`first`, what was struck, counts as touched), and on from
// that one, `jumps` times at most. Each takes `damage`. The arcs and `effect` at every enemy
// are drawn under `parent`. Returns how many it reached.
int chain(godot::Node* parent, const godot::Vector3& from, godot::Object* first, float damage, int jumps, float reach,
          const godot::Color& colour, const godot::Ref<godot::PackedScene>& effect);

} // namespace e5::bridge::combat
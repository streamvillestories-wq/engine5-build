#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/vector3.hpp>

// What a hit does to the thing it lands on. Arrows, explosions, kicks and birds
// all go through here, so everything that can be hit reacts to every skill.
namespace e5::bridge::combat {

// A direct hit on `struck` (the collider an arrow ran into, the thing a bird
// pecked). Enemies take `damage`; practice targets score by ring, times
// `score_multiplier`. Returns false if `struck` is neither.
bool hit(godot::Object* struck, const godot::Vector3& position, float damage, int score_multiplier = 1);

// An area effect: everything within `radius` of `centre` is hit. `context` is
// any node in the scene. Returns how many things were hit.
int blast(godot::Node* context, const godot::Vector3& centre, float radius, float damage, int score_multiplier = 1);

} // namespace e5::bridge::combat
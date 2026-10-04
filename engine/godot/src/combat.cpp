#include "combat.hpp"

#include "enemy.hpp"
#include "target.hpp"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace e5::bridge::combat {

bool hit(godot::Object* struck, const godot::Vector3& position, float damage, int score_multiplier) {
    if (auto* const enemy = godot::Object::cast_to<E5Enemy>(struck)) {
        enemy->take_damage(damage, position);
        return true;
    }
    if (auto* const target = godot::Object::cast_to<E5Target>(struck)) {
        target->register_hit(position, score_multiplier);
        return true;
    }
    return false;
}

int blast(godot::Node* context, const godot::Vector3& centre, float radius, float damage, int score_multiplier) {
    if (context == nullptr || !context->is_inside_tree()) {
        return 0;
    }
    int count = E5Target::blast(context, centre, radius, score_multiplier);
    const godot::TypedArray<godot::Node> enemies = context->get_tree()->get_nodes_in_group(E5Enemy::group_name);
    for (const godot::Variant& node : enemies) {
        auto* const enemy = godot::Object::cast_to<E5Enemy>(node);
        if (enemy == nullptr || !enemy->is_alive()) {
            continue;
        }
        // Measured to the middle of the body, with the body's own width as allowance.
        const godot::Vector3 body = enemy->get_aim_point();
        if (body.distance_to(centre) <= radius + enemy->get_body_radius()) {
            enemy->take_damage(damage, body);
            ++count;
        }
    }
    return count;
}

} // namespace e5::bridge::combat
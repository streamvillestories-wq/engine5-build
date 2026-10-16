#include "crystal_core.hpp"

#include <godot_cpp/core/class_db.hpp>

namespace e5::bridge {

void E5CrystalCore::_bind_methods() {
    using godot::ClassDB;
    using godot::D_METHOD;
    using godot::PropertyInfo;
    ClassDB::bind_method(D_METHOD("take_damage", "amount", "position"), &E5CrystalCore::take_damage);
    ClassDB::bind_method(D_METHOD("take_outgoing_damage"), &E5CrystalCore::take_outgoing_damage);
    ClassDB::bind_method(D_METHOD("get_aim_point"), &E5CrystalCore::get_aim_point);
    ClassDB::bind_method(D_METHOD("set_owner_key", "key"), &E5CrystalCore::set_owner_key);
    ClassDB::bind_method(D_METHOD("get_owner_key"), &E5CrystalCore::get_owner_key);
    ClassDB::bind_method(D_METHOD("set_hittable", "hittable"), &E5CrystalCore::set_hittable);
    ClassDB::bind_method(D_METHOD("is_hittable"), &E5CrystalCore::is_hittable);
    ClassDB::bind_method(D_METHOD("set_body_radius", "metres"), &E5CrystalCore::set_body_radius);
    ClassDB::bind_method(D_METHOD("get_body_radius"), &E5CrystalCore::get_body_radius);
    ClassDB::bind_method(D_METHOD("set_aim_height", "metres"), &E5CrystalCore::set_aim_height);
    ClassDB::bind_method(D_METHOD("get_aim_height"), &E5CrystalCore::get_aim_height);
    ADD_PROPERTY(PropertyInfo(godot::Variant::STRING, "owner_key"), "set_owner_key", "get_owner_key");
    ADD_PROPERTY(PropertyInfo(godot::Variant::BOOL, "hittable"), "set_hittable", "is_hittable");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "body_radius", godot::PROPERTY_HINT_RANGE, "0.1,10,0.05,suffix:m"),
                 "set_body_radius", "get_body_radius");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "aim_height", godot::PROPERTY_HINT_RANGE, "0,10,0.05,suffix:m"),
                 "set_aim_height", "get_aim_height");
    ADD_SIGNAL(godot::MethodInfo("struck", PropertyInfo(godot::Variant::FLOAT, "amount"),
                                 PropertyInfo(godot::Variant::VECTOR3, "position")));
}

void E5CrystalCore::_ready() {
    add_to_group(group_name);
}

void E5CrystalCore::take_damage(float amount, const godot::Vector3& position) {
    if (!hittable_ || amount <= 0.0F) {
        return;
    }
    outgoing_ += amount;
    emit_signal("struck", amount, position);
}

float E5CrystalCore::take_outgoing_damage() {
    const float taken = outgoing_;
    outgoing_ = 0.0F;
    return taken;
}

godot::Vector3 E5CrystalCore::get_aim_point() const {
    return get_global_position() + godot::Vector3(0.0F, aim_height_, 0.0F);
}

} // namespace e5::bridge

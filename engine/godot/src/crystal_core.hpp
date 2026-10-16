#pragma once

#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace e5::bridge {

// A player's crystal core in a match (e5/gameplay/crystal_core.hpp). The game server keeps its
// health; this node is what is seen and struck. Whatever the hero played here does to it through
// `combat::hit` (arrows, bolts) and `combat::blast` (blows, bursts) is collected and sent to the
// server by the match script (game/match/crystal_core.gd, which extends this), never taken here:
// the server decides, and tells every game the new health.
// It is not an E5Enemy: enemies are told apart between machines by a list made at the start, and
// cores come and go during a match.
class E5CrystalCore : public godot::StaticBody3D {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5CrystalCore, godot::StaticBody3D)

public:
    // Every core is in this group, so area effects can find them.
    static constexpr const char* group_name = "e5_crystal_core";

    void _ready() override;

    // A blow that reached it (damage already the hero's own: a shown hero's is zero). Counted
    // only while it is `hittable` (an enemy's core in the fight, not protected).
    void take_damage(float amount, const godot::Vector3& position);
    // What was collected since the last call, for the server.
    float take_outgoing_damage();
    // The middle of the crystal: where area effects measure to.
    [[nodiscard]] godot::Vector3 get_aim_point() const;

    void set_owner_key(const godot::String& key) { owner_key_ = key; }
    [[nodiscard]] godot::String get_owner_key() const { return owner_key_; }
    void set_hittable(bool hittable) { hittable_ = hittable; }
    [[nodiscard]] bool is_hittable() const { return hittable_; }
    void set_body_radius(float metres) { body_radius_ = metres; }
    [[nodiscard]] float get_body_radius() const { return body_radius_; }
    void set_aim_height(float metres) { aim_height_ = metres; }
    [[nodiscard]] float get_aim_height() const { return aim_height_; }

protected:
    static void _bind_methods();

private:
    godot::String owner_key_; // the match's key of the player whose core it is
    bool hittable_ = false;
    float body_radius_ = 1.3F; // metres: the plinth's half width, allowance for area effects
    float aim_height_ = 1.4F;  // metres over its foot: the crystal's middle
    float outgoing_ = 0.0F;
};

} // namespace e5::bridge

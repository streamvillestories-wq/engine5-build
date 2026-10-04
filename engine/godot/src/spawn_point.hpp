#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace e5::bridge {

// A place where enemies live: a camp, a den, a nest. It makes `count` enemies
// of one kind around itself when the scene starts, each on the ground at a
// spot of its own within `radius` (never in water or on a slope too steep to
// stand on). That spot is the enemy's home: it comes back there when it loses
// its prey, and is made anew there `respawn_seconds` after it dies.
//
// A scene then says where its enemies live with one node for each place
// instead of one for each enemy, and a map can show the place by its name.
// The spots are chosen from `seed` and the place itself, so they are the same
// on every run and every machine.
class E5SpawnPoint : public godot::Node3D {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5SpawnPoint, godot::Node3D)

public:
    // Every spawn point is in this group.
    static constexpr const char* group_name = "e5_spawn_point";

    void _ready() override;

    void set_enemy_scene(const godot::Ref<godot::PackedScene>& scene) { enemy_scene_ = scene; }
    [[nodiscard]] godot::Ref<godot::PackedScene> get_enemy_scene() const { return enemy_scene_; }
    // A mixed camp: further kinds of enemy. The camp's enemies take turns through
    // enemy_scene and these, so a kind named twice comes twice as often.
    void set_more_scenes(const godot::TypedArray<godot::PackedScene>& scenes) { more_scenes_ = scenes; }
    [[nodiscard]] godot::TypedArray<godot::PackedScene> get_more_scenes() const { return more_scenes_; }
    void set_count(int count) { count_ = count; }
    [[nodiscard]] int get_count() const { return count_; }
    void set_radius(float metres) { radius_ = metres; }
    [[nodiscard]] float get_radius() const { return radius_; }
    void set_respawn_seconds(float seconds) { respawn_seconds_ = seconds; }
    [[nodiscard]] float get_respawn_seconds() const { return respawn_seconds_; }
    void set_seed(int seed) { seed_ = seed; }
    [[nodiscard]] int get_seed() const { return seed_; }
    // What the place is called on the map ("Wolf Den").
    void set_label(const godot::String& label) { label_ = label; }
    [[nodiscard]] godot::String get_label() const { return label_; }
    // The kind of enemy, for the map's mark ("wolf", "spider", ...); free text the map script knows.
    void set_kind(const godot::String& kind) { kind_ = kind; }
    [[nodiscard]] godot::String get_kind() const { return kind_; }

    // How many of its enemies are alive right now.
    [[nodiscard]] int get_alive_count() const;

protected:
    static void _bind_methods();

private:
    void spawn();

    godot::Ref<godot::PackedScene> enemy_scene_;
    godot::TypedArray<godot::PackedScene> more_scenes_;
    int count_ = 3;
    float radius_ = 6.0F;
    float respawn_seconds_ = 45.0F;
    int seed_ = 1;
    godot::String label_;
    godot::String kind_;
};

} // namespace e5::bridge

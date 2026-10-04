#pragma once

#include "e5/gameplay/island.hpp"
#include "e5/gameplay/paths.hpp"
#include "e5/gameplay/trails.hpp"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <vector>

namespace e5::bridge {

// The ground of an island. Its shape is the function in e5::gameplay
// (island.hpp); this node turns that function into something to look at and
// to stand on: a mesh in square chunks (so only what is in view is drawn) and
// one height-field collision shape. Nothing is loaded from disk: the same
// properties always build the same island.
//
// Paths (dirt roads) are lines through a few points each, given in `paths`.
// They are drawn into the ground material and forests keep off them; they do
// not change the shape of the ground. `trails` are paths that do: they are cut
// into the slope, level from side to side and never steeper than `trail_grade`
// (e5/gameplay/trails.hpp), which is how the mountains are walked.
//
// Sea level is the node's own height (y = 0 in its space). Anything that needs
// to stand on the ground (forests, enemies, the player's start) asks
// `height_at`.
class E5Terrain : public godot::StaticBody3D {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5Terrain, godot::StaticBody3D)

public:
    // The terrain is in this group, so other nodes can find it.
    static constexpr const char* group_name = "e5_terrain";
    // A Node3D in this group is stood on the ground when the terrain is built: its height
    // becomes the ground's at its place plus its metadata "ground_offset" (metres, may be
    // negative to set it into the ground).
    static constexpr const char* on_ground_group_name = "e5_on_ground";

    void _ready() override;

    // Height of the ground at a place in the world, in world space.
    [[nodiscard]] float height_at(float world_x, float world_z) const;
    // Steepness there: 0 = level, 1 = 45 degrees.
    [[nodiscard]] float slope_at(float world_x, float world_z) const;
    // Metres from a place in the world to the middle of the nearest path; very far if there are none.
    [[nodiscard]] float path_distance_at(float world_x, float world_z) const;
    // Number of triangles in all chunks together, for statistics.
    [[nodiscard]] int get_triangle_count() const { return triangle_count_; }

    void set_radius(float metres) { params_.radius = metres; }
    [[nodiscard]] float get_radius() const { return params_.radius; }
    void set_hill_height(float metres) { params_.hill_height = metres; }
    [[nodiscard]] float get_hill_height() const { return params_.hill_height; }
    void set_lowland_height(float metres) { params_.lowland_height = metres; }
    [[nodiscard]] float get_lowland_height() const { return params_.lowland_height; }
    void set_sea_floor(float metres) { params_.sea_floor = metres; }
    [[nodiscard]] float get_sea_floor() const { return params_.sea_floor; }
    void set_hill_size(float metres) { params_.hill_size = metres; }
    [[nodiscard]] float get_hill_size() const { return params_.hill_size; }
    void set_clearing_radius(float metres) { params_.clearing_radius = metres; }
    [[nodiscard]] float get_clearing_radius() const { return params_.clearing_radius; }
    void set_seed(int seed) { params_.seed = static_cast<std::uint32_t>(seed); }
    [[nodiscard]] int get_seed() const { return static_cast<int>(params_.seed); }
    void set_mountain_height(float metres) { params_.mountain_height = metres; }
    [[nodiscard]] float get_mountain_height() const { return params_.mountain_height; }
    void set_mountain_radius(float metres) { params_.mountain_radius = metres; }
    [[nodiscard]] float get_mountain_radius() const { return params_.mountain_radius; }
    void set_mountain_position(const godot::Vector2& position) {
        params_.mountain_x = static_cast<float>(position.x);
        params_.mountain_z = static_cast<float>(position.y);
    }
    [[nodiscard]] godot::Vector2 get_mountain_position() const { return {params_.mountain_x, params_.mountain_z}; }
    void set_mountain_length(float metres) { params_.mountain_length = metres; }
    [[nodiscard]] float get_mountain_length() const { return params_.mountain_length; }
    void set_mountain_direction(float radians) { params_.mountain_direction = radians; }
    [[nodiscard]] float get_mountain_direction() const { return params_.mountain_direction; }
    void set_paths(const godot::Array& paths) { paths_ = paths; }
    [[nodiscard]] godot::Array get_paths() const { return paths_; }
    void set_trails(const godot::Array& trails) { trails_ = trails; }
    [[nodiscard]] godot::Array get_trails() const { return trails_; }
    void set_trail_grade(float grade) { trail_grade_ = grade; }
    [[nodiscard]] float get_trail_grade() const { return trail_grade_; }
    // For laying out a trail: a way from one place to another (x, z) that is nowhere steeper
    // than `trail_grade`, as points to put into `trails`. Empty if there is none.
    [[nodiscard]] godot::PackedVector2Array plan_trail(const godot::Vector2& from, const godot::Vector2& to) const;
    // For a map: every path and trail as its smoothed line (x, z), paths first, and how many
    // of them are paths.
    [[nodiscard]] godot::Array get_path_lines() const;
    [[nodiscard]] int get_path_count() const { return path_count_; }
    // For a map: a picture of the ground from above, one pixel a metre, north (-z) up:
    // sea by depth, sand, grass, rock and snow, shaded as if lit from the north-west.
    [[nodiscard]] godot::Ref<godot::Image> make_map_image() const;
    // Metres across the square the ground (and the map picture) covers.
    [[nodiscard]] float get_side() const { return static_cast<float>(cells_across()); }
    void set_path_width(float metres) { path_width_ = metres; }
    [[nodiscard]] float get_path_width() const { return path_width_; }
    void set_extent(float metres) { extent_ = metres; }
    [[nodiscard]] float get_extent() const { return extent_; }
    void set_material(const godot::Ref<godot::Material>& material) { material_ = material; }
    [[nodiscard]] godot::Ref<godot::Material> get_material() const { return material_; }

protected:
    static void _bind_methods();

private:
    void build_chunk(int first_column, int first_row, int cells);
    void build_collision(int cells_across);
    // Smooths the paths and hands the material a picture of where they run.
    void build_paths(int cells_across);
    // The height of the ground at a place in the node's space, trails cut in.
    [[nodiscard]] float ground(float x, float z) const;
    [[nodiscard]] int cells_across() const;

    gameplay::IslandParams params_;
    // Metres from the centre to the edge of the generated ground. Beyond it there is
    // only the water surface, so it should reach well past the coast.
    float extent_ = 230.0F;
    godot::Ref<godot::Material> material_;
    // Each entry a PackedVector2Array: the points (x, z in the node's space) one path runs through.
    godot::Array paths_;
    float path_width_ = 2.4F; // metres
    godot::Array trails_;
    float trail_grade_ = 0.42F; // about 23 degrees
    gameplay::TrailField trail_field_;
    std::vector<std::vector<gameplay::PathPoint>> smoothed_paths_; // paths, then trails
    int path_count_ = 0;
    std::vector<float> heights_; // the collision grid: one a metre, row by row
    int triangle_count_ = 0;
};

} // namespace e5::bridge

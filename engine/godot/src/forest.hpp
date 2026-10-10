#pragma once

#include "e5/gameplay/scatter.hpp"

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/shape3d.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

namespace e5::bridge {

class E5Terrain;

// Many plants at little cost. Give it the plant scenes made by
// scripts/prepare_plants.ps1, an area, a spacing and a seed; it places the
// plants itself (deterministically, see e5/gameplay/scatter.hpp) and draws
// each one in the cheapest form its distance allows:
//
//   up close            the full mesh, then the mid mesh      one instance per plant,
//   further             the coarse mesh                       switched per plant
//   beyond `Impostor`   a flat card with a baked picture      batched per grid cell
//   shadows             the coarse mesh, near the camera only batched per grid cell
//
// The meshes, materials and distances are read from the plant scenes, so a
// plant looks the same in a forest as placed by hand. Trunks collide; nothing
// else does. The forest is built once when the node becomes ready and does not
// follow later changes to the node's transform or properties.
class E5Forest : public godot::Node3D {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5Forest, godot::Node3D)

public:
    // Every forest is in this group.
    static constexpr const char* group_name = "e5_forest";
    // A Node3D in this group keeps every forest away from itself, within the metres given
    // by its metadata "clear_radius": a landmark, a camp, anything placed by hand.
    static constexpr const char* clearing_group_name = "e5_clearing";

    void _ready() override;
    void _process(double delta) override;
    void _exit_tree() override;

    // A plant someone can hide in (the Archer's Disguise): what it looks like and where it stands.
    struct Plant {
        int index = -1; // in its forest; -1: none, or a plant that stands by itself
        godot::Transform3D transform;
        godot::Ref<godot::Mesh> mesh; // empty: there is none
        godot::Ref<godot::Material> material;
    };
    // The plant of this forest nearest to `position` (over the ground) within `radius` whose
    // scene's path contains `kind` ("bush"), if there is one.
    [[nodiscard]] Plant plant_near(const godot::Vector3& position, float radius, const godot::String& kind) const;
    // Takes that plant out of the picture, or puts it back. (Its shadow and, from far away, its
    // card stay: those are drawn in batches.)
    void set_plant_hidden(int index, bool hidden);
    // The same of a plant scene that stands by itself, not in a forest (`transform` is not set).
    [[nodiscard]] static Plant plant_of_scene(const godot::Ref<godot::PackedScene>& scene);

    [[nodiscard]] int get_plant_count() const { return plant_count_; }

    void set_plant_scenes(const godot::TypedArray<godot::PackedScene>& scenes) { plant_scenes_ = scenes; }
    [[nodiscard]] godot::TypedArray<godot::PackedScene> get_plant_scenes() const { return plant_scenes_; }
    void set_size(const godot::Vector2& size) { size_ = size; }
    [[nodiscard]] godot::Vector2 get_size() const { return size_; }
    void set_spacing(float metres) { spacing_ = metres; }
    [[nodiscard]] float get_spacing() const { return spacing_; }
    void set_jitter(float jitter) { jitter_ = jitter; }
    [[nodiscard]] float get_jitter() const { return jitter_; }
    void set_clearing_radius(float metres) { clearing_radius_ = metres; }
    [[nodiscard]] float get_clearing_radius() const { return clearing_radius_; }
    void set_seed(int seed) { seed_ = seed; }
    [[nodiscard]] int get_seed() const { return seed_; }
    void set_cell_size(float metres) { cell_size_ = metres; }
    [[nodiscard]] float get_cell_size() const { return cell_size_; }
    void set_shadow_distance(float metres) { shadow_distance_ = metres; }
    [[nodiscard]] float get_shadow_distance() const { return shadow_distance_; }
    void set_near_distance(float metres) { near_distance_ = metres; }
    [[nodiscard]] float get_near_distance() const { return near_distance_; }
    void set_far_distance(float metres) { far_distance_ = metres; }
    [[nodiscard]] float get_far_distance() const { return far_distance_; }
    // Only in one kind of country: 0 meadow, 1 pine forest, 2 dry highland, 3 marsh
    // (E5Terrain.region_at); -1 = anywhere.
    void set_region(int region) { region_ = region; }
    [[nodiscard]] int get_region() const { return region_; }
    void set_min_height(float metres) { min_height_ = metres; }
    [[nodiscard]] float get_min_height() const { return min_height_; }
    void set_max_height(float metres) { max_height_ = metres; }
    [[nodiscard]] float get_max_height() const { return max_height_; }
    void set_max_slope(float slope) { max_slope_ = slope; }
    [[nodiscard]] float get_max_slope() const { return max_slope_; }
    void set_path_clearance(float metres) { path_clearance_ = metres; }
    [[nodiscard]] float get_path_clearance() const { return path_clearance_; }
    void set_path_reach(float metres) { path_reach_ = metres; }
    [[nodiscard]] float get_path_reach() const { return path_reach_; }
    void set_scale_min(float scale) { scale_min_ = scale; }
    [[nodiscard]] float get_scale_min() const { return scale_min_; }
    void set_scale_max(float scale) { scale_max_ = scale; }
    [[nodiscard]] float get_scale_max() const { return scale_max_; }

protected:
    static void _bind_methods();

private:
    // What a plant scene contains, taken apart once.
    struct Species {
        godot::Ref<godot::Mesh> full;
        godot::Ref<godot::Mesh> mid;
        godot::Ref<godot::Mesh> coarse;
        godot::Ref<godot::Mesh> card; // the impostor; may be missing
        godot::Ref<godot::Material> material;
        float near_distance = 10.0F;      // metres: full mesh up to here
        float far_distance = 22.0F;       // mid mesh up to here
        float impostor_distance = 0.0F;   // coarse mesh up to here; 0 = for ever
        godot::Ref<godot::Shape3D> trunk; // may be missing
        godot::Vector3 trunk_offset;
        float foot_radius = 0.4F; // metres: how wide the plant stands on the ground
        godot::String path;       // of its scene: says what kind of plant it is
    };
    // One mesh of one plant and the distances between which it can be seen at all.
    struct Form {
        godot::RID instance; // owned: freed in _exit_tree
        float begin = 0.0F;
        float end = 0.0F; // 0 = no limit
        bool visible = false;
    };
    struct Drawn {
        godot::Vector3 position;
        std::array<Form, 3> forms; // full, mid, coarse
        std::size_t species = 0;
        godot::Basis basis;  // its turn and size
        bool hidden = false; // someone has taken it (set_plant_hidden)
    };

    [[nodiscard]] static bool read_species(const godot::Ref<godot::PackedScene>& scene, Species& species);
    void add_plant(const Species& species, const godot::Transform3D& transform);
    [[nodiscard]] godot::RID add_instance(const godot::Ref<godot::Mesh>& mesh,
                                          const godot::Ref<godot::Material>& material,
                                          const godot::Transform3D& transform, float begin, float end);
    // The point from which detail distances are measured: the player, so that turning the
    // camera around her changes nothing; the camera itself when it is not hers.
    [[nodiscard]] godot::Vector3 lod_origin() const;
    // A plant that passed the terrain's conditions, placed relative to the forest node.
    struct Planted {
        std::size_t species = 0;
        godot::Transform3D local;
    };
    void build_cells(const std::vector<Planted>& plants);
    // Places kept free by hand (clearing_group_name): x and z in the world, with the radius in y.
    [[nodiscard]] std::vector<godot::Vector3> read_clearings() const;
    // The height at which a plant stands at a place on the terrain, set into the ground a
    // little; nothing if the place does not suit this forest.
    [[nodiscard]] std::optional<float> ground_for(const E5Terrain& terrain, float world_x, float world_z,
                                                  float foot_radius, float scale) const;

    godot::TypedArray<godot::PackedScene> plant_scenes_;
    godot::Vector2 size_{60.0F, 60.0F}; // metres along x and z, centred on the node
    float spacing_ = 6.0F;
    float jitter_ = 0.35F;
    float clearing_radius_ = 0.0F;
    int seed_ = 1;
    float cell_size_ = 8.0F;        // metres; the batches of cards and shadows are this large
    float shadow_distance_ = 45.0F; // metres; at or beyond where the sun's shadows end, or shadows pop in by the cell
    // To let the detailed forms give way sooner or later than the plant scenes say. 0 = as the scene says.
    float near_distance_ = 0.0F; // metres: full mesh up to here
    float far_distance_ = 0.0F;  // metres: mid mesh up to here
    // On a terrain: plants grow only between these heights (world space) and on ground no steeper than this.
    int region_ = -1;
    float min_height_ = 1.2F;     // metres; above the beach
    float max_height_ = 1000.0F;  // metres
    float max_slope_ = 0.55F;     // 0 = level, 1 = 45 degrees
    float path_clearance_ = 3.0F; // metres from the middle of a path that stay free of plants
    float path_reach_ = 0.0F;     // metres; plants grow only this near a path. 0 = anywhere
    float scale_min_ = 0.85F;
    float scale_max_ = 1.2F;

    std::vector<Species> species_;
    std::vector<Drawn> drawn_;
    int plant_count_ = 0;
};

} // namespace e5::bridge

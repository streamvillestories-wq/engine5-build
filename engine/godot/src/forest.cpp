#include "forest.hpp"

#include "e5/core/profiling.hpp"
#include "godot_log.hpp"
#include "player_controller.hpp"
#include "terrain.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/cylinder_shape3d.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/string.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <tuple>

namespace e5::bridge {
namespace {

constexpr float half_diagonal = 0.7071F; // of a square cell, as a share of its side
// The orbiting camera is never further than this from the player. Detail distances are
// measured from the player while that holds, so that turning the camera changes nothing.
constexpr float camera_reach = 12.0F;
const char* const lod_origin_name = "lod_origin"; // the global shader parameter, see project.godot

// Half the width of the band around a switching distance in which two forms dissolve into
// each other. Must match lod_half_band in foliage_wind.gdshader and plant_impostor.gdshader.
constexpr float band_min = 4.0F;
constexpr float band_share = 0.3F;
float half_band(float boundary) {
    return std::max(band_min, boundary * band_share) * 0.5F;
}

// How far a plant is set into the ground: its foot is uneven, the drawn ground is flat
// between its samples, and on a slope the downhill side of the foot would hang in the air.
constexpr float sink_base = 0.12F; // metres
constexpr float foot_share = 1.6F; // the foot with its roots, as a multiple of the trunk's radius

// One batch: the plants of one species inside one grid cell.
using CellKey = std::tuple<int, int, std::size_t>; // column, row, species
struct Cell {
    std::vector<godot::Transform3D> plants; // relative to the forest node
};

godot::MultiMeshInstance3D* make_batch(const godot::Ref<godot::Mesh>& mesh,
                                       const std::vector<godot::Transform3D>& plants, const godot::Vector3& centre) {
    godot::Ref<godot::MultiMesh> batch;
    batch.instantiate();
    batch->set_transform_format(godot::MultiMesh::TRANSFORM_3D);
    batch->set_mesh(mesh);
    batch->set_instance_count(static_cast<std::int32_t>(plants.size()));
    for (std::size_t index = 0; index < plants.size(); ++index) {
        // Relative to the cell's centre, so the batch's bounds sit around its node.
        godot::Transform3D local = plants[index];
        local.origin -= centre;
        batch->set_instance_transform(static_cast<std::int32_t>(index), local);
    }
    auto* const node = memnew(godot::MultiMeshInstance3D);
    node->set_multimesh(batch);
    node->set_position(centre);
    return node;
}

} // namespace

void E5Forest::_bind_methods() {
    using godot::ClassDB;
    using godot::D_METHOD;
    using godot::PropertyInfo;

    ClassDB::bind_method(D_METHOD("set_plant_scenes", "scenes"), &E5Forest::set_plant_scenes);
    ClassDB::bind_method(D_METHOD("get_plant_scenes"), &E5Forest::get_plant_scenes);
    ClassDB::bind_method(D_METHOD("set_size", "size"), &E5Forest::set_size);
    ClassDB::bind_method(D_METHOD("get_size"), &E5Forest::get_size);
    ClassDB::bind_method(D_METHOD("set_spacing", "metres"), &E5Forest::set_spacing);
    ClassDB::bind_method(D_METHOD("get_spacing"), &E5Forest::get_spacing);
    ClassDB::bind_method(D_METHOD("set_jitter", "jitter"), &E5Forest::set_jitter);
    ClassDB::bind_method(D_METHOD("get_jitter"), &E5Forest::get_jitter);
    ClassDB::bind_method(D_METHOD("set_clearing_radius", "metres"), &E5Forest::set_clearing_radius);
    ClassDB::bind_method(D_METHOD("get_clearing_radius"), &E5Forest::get_clearing_radius);
    ClassDB::bind_method(D_METHOD("set_seed", "seed"), &E5Forest::set_seed);
    ClassDB::bind_method(D_METHOD("get_seed"), &E5Forest::get_seed);
    ClassDB::bind_method(D_METHOD("set_cell_size", "metres"), &E5Forest::set_cell_size);
    ClassDB::bind_method(D_METHOD("get_cell_size"), &E5Forest::get_cell_size);
    ClassDB::bind_method(D_METHOD("set_shadow_distance", "metres"), &E5Forest::set_shadow_distance);
    ClassDB::bind_method(D_METHOD("get_shadow_distance"), &E5Forest::get_shadow_distance);
    ClassDB::bind_method(D_METHOD("set_near_distance", "metres"), &E5Forest::set_near_distance);
    ClassDB::bind_method(D_METHOD("get_near_distance"), &E5Forest::get_near_distance);
    ClassDB::bind_method(D_METHOD("set_far_distance", "metres"), &E5Forest::set_far_distance);
    ClassDB::bind_method(D_METHOD("get_far_distance"), &E5Forest::get_far_distance);
    ClassDB::bind_method(D_METHOD("set_region", "region"), &E5Forest::set_region);
    ClassDB::bind_method(D_METHOD("get_region"), &E5Forest::get_region);
    ClassDB::bind_method(D_METHOD("set_min_height", "metres"), &E5Forest::set_min_height);
    ClassDB::bind_method(D_METHOD("get_min_height"), &E5Forest::get_min_height);
    ClassDB::bind_method(D_METHOD("set_max_height", "metres"), &E5Forest::set_max_height);
    ClassDB::bind_method(D_METHOD("get_max_height"), &E5Forest::get_max_height);
    ClassDB::bind_method(D_METHOD("set_max_slope", "slope"), &E5Forest::set_max_slope);
    ClassDB::bind_method(D_METHOD("get_max_slope"), &E5Forest::get_max_slope);
    ClassDB::bind_method(D_METHOD("set_path_clearance", "metres"), &E5Forest::set_path_clearance);
    ClassDB::bind_method(D_METHOD("get_path_clearance"), &E5Forest::get_path_clearance);
    ClassDB::bind_method(D_METHOD("set_path_reach", "metres"), &E5Forest::set_path_reach);
    ClassDB::bind_method(D_METHOD("get_path_reach"), &E5Forest::get_path_reach);
    ClassDB::bind_method(D_METHOD("set_scale_min", "scale"), &E5Forest::set_scale_min);
    ClassDB::bind_method(D_METHOD("get_scale_min"), &E5Forest::get_scale_min);
    ClassDB::bind_method(D_METHOD("set_scale_max", "scale"), &E5Forest::set_scale_max);
    ClassDB::bind_method(D_METHOD("get_scale_max"), &E5Forest::get_scale_max);
    ClassDB::bind_method(D_METHOD("get_plant_count"), &E5Forest::get_plant_count);

    // "an array of PackedScene resources", in the form the editor expects.
    const godot::String scene_array_hint = godot::String::num_int64(godot::Variant::OBJECT) + godot::String("/") +
                                           godot::String::num_int64(godot::PROPERTY_HINT_RESOURCE_TYPE) +
                                           godot::String(":PackedScene");
    ADD_PROPERTY(PropertyInfo(godot::Variant::ARRAY, "plant_scenes", godot::PROPERTY_HINT_ARRAY_TYPE, scene_array_hint),
                 "set_plant_scenes", "get_plant_scenes");
    ADD_PROPERTY(PropertyInfo(godot::Variant::VECTOR2, "size", godot::PROPERTY_HINT_NONE, "suffix:m"), "set_size",
                 "get_size");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "spacing", godot::PROPERTY_HINT_RANGE, "1,50,0.1,suffix:m"),
                 "set_spacing", "get_spacing");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "jitter", godot::PROPERTY_HINT_RANGE, "0,0.5,0.01"), "set_jitter",
                 "get_jitter");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "clearing_radius", godot::PROPERTY_HINT_RANGE, "0,500,0.5,suffix:m"),
        "set_clearing_radius", "get_clearing_radius");
    ADD_PROPERTY(PropertyInfo(godot::Variant::INT, "seed"), "set_seed", "get_seed");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "cell_size", godot::PROPERTY_HINT_RANGE, "2,64,0.5,suffix:m"),
                 "set_cell_size", "get_cell_size");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "shadow_distance", godot::PROPERTY_HINT_RANGE, "0,200,1,suffix:m"),
                 "set_shadow_distance", "get_shadow_distance");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "near_distance", godot::PROPERTY_HINT_RANGE, "0,100,0.5,suffix:m"),
                 "set_near_distance", "get_near_distance");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "far_distance", godot::PROPERTY_HINT_RANGE, "0,200,0.5,suffix:m"),
                 "set_far_distance", "get_far_distance");
    ADD_PROPERTY(PropertyInfo(godot::Variant::INT, "region", godot::PROPERTY_HINT_RANGE, "-1,3,1"), "set_region",
                 "get_region");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "min_height", godot::PROPERTY_HINT_RANGE, "-100,1000,0.1,suffix:m"),
        "set_min_height", "get_min_height");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "max_height", godot::PROPERTY_HINT_RANGE, "-100,1000,0.1,suffix:m"),
        "set_max_height", "get_max_height");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "max_slope", godot::PROPERTY_HINT_RANGE, "0,3,0.01"),
                 "set_max_slope", "get_max_slope");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "path_clearance", godot::PROPERTY_HINT_RANGE, "0,20,0.1,suffix:m"),
                 "set_path_clearance", "get_path_clearance");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "path_reach", godot::PROPERTY_HINT_RANGE, "0,200,0.5,suffix:m"),
                 "set_path_reach", "get_path_reach");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "scale_min", godot::PROPERTY_HINT_RANGE, "0.1,4,0.01"),
                 "set_scale_min", "get_scale_min");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "scale_max", godot::PROPERTY_HINT_RANGE, "0.1,4,0.01"),
                 "set_scale_max", "get_scale_max");
}

bool E5Forest::read_species(const godot::Ref<godot::PackedScene>& scene, Species& species) {
    if (scene.is_null()) {
        return false;
    }
    godot::Node* const root = scene->instantiate();
    if (root == nullptr) {
        return false;
    }
    // The scenes written by prepare_plants.ps1 name their meshes <plant>, <plant>_mid,
    // <plant>_far and <plant>_shadow, and the card "Impostor".
    const godot::TypedArray<godot::Node> meshes = root->find_children("*", "MeshInstance3D", true, false);
    for (const godot::Variant& item : meshes) {
        const auto* const node = godot::Object::cast_to<godot::MeshInstance3D>(item);
        if (node == nullptr) {
            continue;
        }
        const godot::String name = node->get_name();
        if (name == godot::String("Impostor")) {
            species.card = node->get_mesh();
            // The card's material knows where it begins to show.
            const godot::Ref<godot::ShaderMaterial> card_material =
                species.card.is_valid() ? species.card->surface_get_material(0) : godot::Ref<godot::Material>();
            if (card_material.is_valid()) {
                species.impostor_distance = card_material->get_shader_parameter("fade_begin");
            }
        } else if (name.ends_with("_mid")) {
            species.mid = node->get_mesh();
            species.near_distance = node->get_instance_shader_parameter("lod_begin");
        } else if (name.ends_with("_far")) {
            species.coarse = node->get_mesh();
            species.far_distance = node->get_instance_shader_parameter("lod_begin");
        } else if (!name.ends_with("_shadow")) {
            species.full = node->get_mesh();
            species.material = node->get_surface_override_material(0);
        }
    }
    const godot::TypedArray<godot::Node> shapes = root->find_children("*", "CollisionShape3D", true, false);
    for (const godot::Variant& item : shapes) {
        if (const auto* const shape = godot::Object::cast_to<godot::CollisionShape3D>(item)) {
            species.trunk = shape->get_shape();
            species.trunk_offset = shape->get_position();
            const godot::Ref<godot::CylinderShape3D> cylinder = species.trunk;
            if (cylinder.is_valid()) {
                species.foot_radius = cylinder->get_radius() * foot_share;
            }
            break;
        }
    }
    memdelete(root);

    const bool complete =
        species.full.is_valid() && species.mid.is_valid() && species.coarse.is_valid() && species.material.is_valid();
    if (!complete) {
        logger().error(
            "E5Forest: '{}' is not a plant scene from prepare_plants.ps1 (a mesh or its material is missing)",
            scene->get_path().utf8().get_data());
    }
    return complete;
}

godot::RID E5Forest::add_instance(const godot::Ref<godot::Mesh>& mesh, const godot::Ref<godot::Material>& material,
                                  const godot::Transform3D& transform, float begin, float end) {
    godot::RenderingServer* const server = godot::RenderingServer::get_singleton();
    const godot::RID instance = server->instance_create2(mesh->get_rid(), get_world_3d()->get_scenario());
    server->instance_set_transform(instance, transform);
    server->instance_geometry_set_material_override(instance, material->get_rid());
    // The plant's own shadow would cost a full draw per cascade; the cell's batch casts it instead.
    server->instance_geometry_set_cast_shadows_setting(instance, godot::RenderingServer::SHADOW_CASTING_SETTING_OFF);
    // The shader dissolves the mesh in and out around these distances (see foliage_wind.gdshader);
    // _process stops it from being drawn where it has fully dissolved.
    server->instance_geometry_set_shader_parameter(instance, "lod_begin", begin);
    server->instance_geometry_set_shader_parameter(instance, "lod_end", end);
    server->instance_set_visible(instance, false);
    return instance;
}

void E5Forest::add_plant(const Species& species, const godot::Transform3D& transform) {
    // Each form has its own band of distances; neighbours dissolve into each other in the
    // shader, per plant. The engine's own cross-fade is not used: it draws both forms as
    // transparent geometry while it lasts, and in a forest, where some plant is always in
    // transition, that cost more than all the triangles together.
    const float card_distance = species.card.is_valid() ? species.impostor_distance : 0.0F;
    const auto form = [&](const godot::Ref<godot::Mesh>& mesh, float begin, float end) {
        return Form{.instance = add_instance(mesh, species.material, transform, begin, end),
                    .begin = begin > 0.0F ? begin - half_band(begin) : 0.0F,
                    .end = end > 0.0F ? end + half_band(end) : 0.0F};
    };
    drawn_.push_back({.position = transform.origin,
                      .forms = {form(species.full, 0.0F, species.near_distance),
                                form(species.mid, species.near_distance, species.far_distance),
                                form(species.coarse, species.far_distance, card_distance)}});
}

godot::Vector3 E5Forest::lod_origin() const {
    const godot::Viewport* const viewport = get_viewport();
    const godot::Camera3D* const camera = viewport != nullptr ? viewport->get_camera_3d() : nullptr;
    const auto* const player =
        godot::Object::cast_to<godot::Node3D>(get_tree()->get_first_node_in_group(E5PlayerController::group_name));
    if (camera == nullptr) {
        return player != nullptr ? player->get_global_position() : get_global_position();
    }
    const godot::Vector3 eye = camera->get_global_position();
    if (player != nullptr && eye.distance_to(player->get_global_position()) < camera_reach) {
        return player->get_global_position();
    }
    return eye; // a free or debug camera
}

void E5Forest::_process(double /*delta*/) {
    E5_PROFILE_SCOPE("E5Forest::_process");
    godot::RenderingServer* const server = godot::RenderingServer::get_singleton();
    const godot::Vector3 origin = lod_origin();
    // Every forest sets the same value; the shaders of all plants read it.
    server->global_shader_parameter_set(lod_origin_name, origin);
    for (Drawn& plant : drawn_) {
        const auto distance = static_cast<float>(origin.distance_to(plant.position));
        for (Form& form : plant.forms) {
            const bool visible = distance >= form.begin && (form.end <= 0.0F || distance <= form.end);
            if (visible != form.visible) {
                form.visible = visible;
                server->instance_set_visible(form.instance, visible);
            }
        }
    }
}

void E5Forest::build_cells(const std::vector<Planted>& plants) {
    std::map<CellKey, Cell> cells;
    for (const Planted& plant : plants) {
        cells[{static_cast<int>(std::floor(plant.local.origin.x / cell_size_)),
               static_cast<int>(std::floor(plant.local.origin.z / cell_size_)), plant.species}]
            .plants.push_back(plant.local);
    }

    const float cell_radius = cell_size_ * half_diagonal;
    // Far away the cards are batched again, by blocks many cells wide: a wide view over a
    // large island was thousands of draw calls of a few cards each. A block takes over from
    // where its middle is `far_cards_from` away; the cells go on a block's radius further,
    // so that every card is drawn by one or the other, and in between by both, in the same
    // place and looking the same.
    constexpr float block_size = 64.0F;
    constexpr float far_cards_from = 100.0F;
    constexpr float block_radius = block_size * half_diagonal;
    std::map<CellKey, Cell> blocks;
    for (const Planted& plant : plants) {
        if (species_.at(plant.species).card.is_valid()) {
            blocks[{static_cast<int>(std::floor(plant.local.origin.x / block_size)),
                    static_cast<int>(std::floor(plant.local.origin.z / block_size)), plant.species}]
                .plants.push_back(plant.local);
        }
    }
    for (const auto& [key, block] : blocks) {
        const auto& [column, row, species_index] = key;
        const godot::Vector3 centre((static_cast<float>(column) + 0.5F) * block_size, block.plants.front().origin.y,
                                    (static_cast<float>(row) + 0.5F) * block_size);
        godot::MultiMeshInstance3D* const cards = make_batch(species_.at(species_index).card, block.plants, centre);
        cards->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
        cards->set_visibility_range_begin(far_cards_from);
        cards->set_visibility_range_fade_mode(godot::GeometryInstance3D::VISIBILITY_RANGE_FADE_DISABLED);
        add_child(cards);
    }

    for (const auto& [key, cell] : cells) {
        const auto& [column, row, species_index] = key;
        const Species& species = species_.at(species_index);
        const godot::Vector3 centre((static_cast<float>(column) + 0.5F) * cell_size_, cell.plants.front().origin.y,
                                    (static_cast<float>(row) + 0.5F) * cell_size_);

        if (species.card.is_valid()) {
            godot::MultiMeshInstance3D* const cards = make_batch(species.card, cell.plants, centre);
            cards->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
            // The cards dissolve in per plant, in the shader; the batch only has to be drawn from
            // where its nearest card can begin to show. The engine measures from the camera,
            // the shader from the player, hence the camera's reach.
            cards->set_visibility_range_begin(std::max(
                species.impostor_distance - half_band(species.impostor_distance) - cell_radius - camera_reach, 0.0F));
            // Further off the blocks below draw the same cards in far fewer batches.
            cards->set_visibility_range_end(far_cards_from + block_radius + cell_radius);
            cards->set_visibility_range_fade_mode(godot::GeometryInstance3D::VISIBILITY_RANGE_FADE_DISABLED);
            add_child(cards);
        }
        if (shadow_distance_ > 0.0F) {
            godot::MultiMeshInstance3D* const shadows = make_batch(species.coarse, cell.plants, centre);
            shadows->set_material_override(species.material);
            shadows->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_SHADOWS_ONLY);
            shadows->set_visibility_range_end(shadow_distance_ + cell_radius);
            add_child(shadows);
        }
    }
}

std::vector<godot::Vector3> E5Forest::read_clearings() const {
    std::vector<godot::Vector3> clearings;
    const godot::TypedArray<godot::Node> keepers = get_tree()->get_nodes_in_group(clearing_group_name);
    for (const godot::Variant& item : keepers) {
        if (const auto* const keeper = godot::Object::cast_to<godot::Node3D>(item)) {
            const godot::Vector3 centre = keeper->get_global_position();
            const float radius = keeper->get_meta("clear_radius", 0.0F);
            clearings.emplace_back(centre.x, radius, centre.z);
        }
    }
    return clearings;
}

std::optional<float> E5Forest::ground_for(const E5Terrain& terrain, float world_x, float world_z, float foot_radius,
                                          float scale) const {
    const float height = terrain.height_at(world_x, world_z);
    const float slope = terrain.slope_at(world_x, world_z);
    if (height < min_height_ || height > max_height_ || slope > max_slope_) {
        return std::nullopt;
    }
    if (region_ >= 0 && terrain.region_at(world_x, world_z) != region_) {
        return std::nullopt; // this forest belongs to another kind of country
    }
    const float to_path = terrain.path_distance_at(world_x, world_z);
    if (to_path < path_clearance_ || (path_reach_ > 0.0F && to_path > path_reach_)) {
        return std::nullopt; // nothing grows on a road, and some forests only line the roads
    }
    return height - sink_base * scale - slope * foot_radius;
}

void E5Forest::_ready() {
    E5_PROFILE_SCOPE("E5Forest::_ready");
    add_to_group(group_name);

    for (const godot::Variant& scene : plant_scenes_) {
        Species species;
        if (read_species(scene, species)) {
            if (near_distance_ > 0.0F) {
                species.near_distance = near_distance_;
            }
            if (far_distance_ > 0.0F) {
                species.far_distance = far_distance_;
            }
            species_.push_back(species);
        }
    }
    if (species_.empty()) {
        logger().warn("E5Forest '{}' has no usable plant scenes; nothing is planted",
                      godot::String(get_name()).utf8().get_data());
        return;
    }

    const std::vector<gameplay::ScatterPoint> points = gameplay::scatter_plants({
        .width = static_cast<float>(size_.x),
        .depth = static_cast<float>(size_.y),
        .spacing = spacing_,
        .jitter = jitter_,
        .clearing_radius = clearing_radius_,
        .scale_min = scale_min_,
        .scale_max = scale_max_,
        .species_count = static_cast<int>(species_.size()),
        .seed = static_cast<std::uint32_t>(seed_),
    });

    // With a terrain in the scene the plants stand on it, and only where it suits them:
    // not in the water or on the beach, not on steep ground.
    const auto* const terrain =
        godot::Object::cast_to<E5Terrain>(get_tree()->get_first_node_in_group(E5Terrain::group_name));
    const godot::Vector3 origin = get_global_position();

    auto* const trunks = memnew(godot::StaticBody3D);
    trunks->set_name("Trunks");
    add_child(trunks);

    const std::vector<godot::Vector3> clearings = read_clearings();

    std::vector<Planted> planted;
    planted.reserve(points.size());
    for (const gameplay::ScatterPoint& point : points) {
        const auto species_index = static_cast<std::size_t>(point.species);
        const Species& species = species_.at(species_index);
        const float world_x = static_cast<float>(origin.x) + point.x;
        const float world_z = static_cast<float>(origin.z) + point.z;
        const bool kept_free = std::ranges::any_of(clearings, [world_x, world_z](const godot::Vector3& clearing) {
            return std::hypot(world_x - static_cast<float>(clearing.x), world_z - static_cast<float>(clearing.z)) <
                   static_cast<float>(clearing.y);
        });
        if (kept_free) {
            continue;
        }
        float ground = 0.0F; // relative to the forest node
        if (terrain != nullptr) {
            const std::optional<float> height =
                ground_for(*terrain, world_x, world_z, species.foot_radius * point.scale, point.scale);
            if (!height) {
                continue;
            }
            ground = *height - static_cast<float>(origin.y);
        }
        const godot::Basis turned(godot::Vector3(0.0F, 1.0F, 0.0F), point.yaw);
        const godot::Vector3 position(point.x, ground, point.z);
        const godot::Transform3D local(turned.scaled(godot::Vector3(point.scale, point.scale, point.scale)), position);
        // The forest node's turn and scale are ignored: a forest is placed, not rotated.
        add_plant(species, godot::Transform3D(local.basis, origin + position));
        planted.push_back({.species = species_index, .local = local});

        if (species.trunk.is_valid()) {
            // Unscaled: physics shapes should not be scaled, and a trunk a little too thin or thick is harmless.
            auto* const trunk = memnew(godot::CollisionShape3D);
            trunk->set_shape(species.trunk);
            godot::Vector3 offset = turned.xform(species.trunk_offset * godot::Vector3(point.scale, 1.0F, point.scale));
            offset.y = species.trunk_offset.y;
            trunk->set_position(position + offset);
            trunks->add_child(trunk);
        }
    }
    build_cells(planted);
    plant_count_ = static_cast<int>(planted.size());
    logger().info("forest '{}': {} plants of {} kind(s)", godot::String(get_name()).utf8().get_data(), plant_count_,
                  species_.size());
}

void E5Forest::_exit_tree() {
    godot::RenderingServer* const server = godot::RenderingServer::get_singleton();
    for (const Drawn& plant : drawn_) {
        for (const Form& form : plant.forms) {
            server->free_rid(form.instance);
        }
    }
    drawn_.clear();
}

} // namespace e5::bridge

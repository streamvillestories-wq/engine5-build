#include "terrain.hpp"

#include "e5/core/profiling.hpp"
#include "godot_log.hpp"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/height_map_shape3d.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace e5::bridge {
namespace {

// The ground is sampled every metre: fine enough for the hills, and the same
// grid the collision shape uses, so what is seen is what is stood on.
constexpr int chunk_cells = 48; // a chunk is this many metres on a side
// Chunks that lie entirely under water are drawn with cells this many times
// larger: nobody sees the sea floor closely.
constexpr int submerged_step = 4;
constexpr float submerged_below = -1.5F; // metres; a chunk whose highest point is below this counts as submerged

// The picture of the paths: each pixel holds how near the middle of a path is, from
// 255 on it down to 0 at `path_picture_reach` metres and beyond. A distance, not a
// finished edge, so the shader can draw a soft, uneven border at any width.
constexpr int path_picture_size = 1024;
constexpr float path_picture_reach = 6.0F; // metres
constexpr float path_step = 1.0F;          // metres between the points of a smoothed path

// Planning a trail: on a grid this fine, a little gentler than a trail may be (the curve laid
// through the planned points cuts corners and so comes out steeper), a point this often.
constexpr float trail_plan_step = 3.0F;
constexpr float trail_plan_margin = 0.85F;
constexpr float trail_plan_spacing = 12.0F;

// On the map: where the grass gives way to rock and where the snow starts, in metres above
// the sea. The ground shader has the same two numbers (tree_line, snow_line).
constexpr float map_tree_line = 34.0F;
constexpr float map_snow_line = 62.0F;

} // namespace

void E5Terrain::_bind_methods() {
    using godot::ClassDB;
    using godot::D_METHOD;
    using godot::PropertyInfo;

    ClassDB::bind_method(D_METHOD("set_radius", "metres"), &E5Terrain::set_radius);
    ClassDB::bind_method(D_METHOD("get_radius"), &E5Terrain::get_radius);
    ClassDB::bind_method(D_METHOD("set_hill_height", "metres"), &E5Terrain::set_hill_height);
    ClassDB::bind_method(D_METHOD("get_hill_height"), &E5Terrain::get_hill_height);
    ClassDB::bind_method(D_METHOD("set_lowland_height", "metres"), &E5Terrain::set_lowland_height);
    ClassDB::bind_method(D_METHOD("get_lowland_height"), &E5Terrain::get_lowland_height);
    ClassDB::bind_method(D_METHOD("set_sea_floor", "metres"), &E5Terrain::set_sea_floor);
    ClassDB::bind_method(D_METHOD("get_sea_floor"), &E5Terrain::get_sea_floor);
    ClassDB::bind_method(D_METHOD("set_hill_size", "metres"), &E5Terrain::set_hill_size);
    ClassDB::bind_method(D_METHOD("get_hill_size"), &E5Terrain::get_hill_size);
    ClassDB::bind_method(D_METHOD("set_clearing_radius", "metres"), &E5Terrain::set_clearing_radius);
    ClassDB::bind_method(D_METHOD("get_clearing_radius"), &E5Terrain::get_clearing_radius);
    ClassDB::bind_method(D_METHOD("set_seed", "seed"), &E5Terrain::set_seed);
    ClassDB::bind_method(D_METHOD("get_seed"), &E5Terrain::get_seed);
    ClassDB::bind_method(D_METHOD("set_mountain_height", "metres"), &E5Terrain::set_mountain_height);
    ClassDB::bind_method(D_METHOD("get_mountain_height"), &E5Terrain::get_mountain_height);
    ClassDB::bind_method(D_METHOD("set_mountain_radius", "metres"), &E5Terrain::set_mountain_radius);
    ClassDB::bind_method(D_METHOD("get_mountain_radius"), &E5Terrain::get_mountain_radius);
    ClassDB::bind_method(D_METHOD("set_mountain_position", "position"), &E5Terrain::set_mountain_position);
    ClassDB::bind_method(D_METHOD("get_mountain_position"), &E5Terrain::get_mountain_position);
    ClassDB::bind_method(D_METHOD("set_mountain_length", "metres"), &E5Terrain::set_mountain_length);
    ClassDB::bind_method(D_METHOD("get_mountain_length"), &E5Terrain::get_mountain_length);
    ClassDB::bind_method(D_METHOD("set_mountain_direction", "radians"), &E5Terrain::set_mountain_direction);
    ClassDB::bind_method(D_METHOD("get_mountain_direction"), &E5Terrain::get_mountain_direction);
    ClassDB::bind_method(D_METHOD("set_regions", "regions"), &E5Terrain::set_regions);
    ClassDB::bind_method(D_METHOD("get_regions"), &E5Terrain::get_regions);
    ClassDB::bind_method(D_METHOD("region_at", "world_x", "world_z"), &E5Terrain::region_at);
    ClassDB::bind_method(D_METHOD("set_river", "points"), &E5Terrain::set_river);
    ClassDB::bind_method(D_METHOD("get_river"), &E5Terrain::get_river);
    ClassDB::bind_method(D_METHOD("set_river_width", "metres"), &E5Terrain::set_river_width);
    ClassDB::bind_method(D_METHOD("get_river_width"), &E5Terrain::get_river_width);
    ClassDB::bind_method(D_METHOD("set_lakes", "lakes"), &E5Terrain::set_lakes);
    ClassDB::bind_method(D_METHOD("get_lakes"), &E5Terrain::get_lakes);
    ClassDB::bind_method(D_METHOD("set_hills", "hills"), &E5Terrain::set_hills);
    ClassDB::bind_method(D_METHOD("get_hills"), &E5Terrain::get_hills);
    ClassDB::bind_method(D_METHOD("set_ridges", "ridges"), &E5Terrain::set_ridges);
    ClassDB::bind_method(D_METHOD("get_ridges"), &E5Terrain::get_ridges);
    ClassDB::bind_method(D_METHOD("set_rolling", "metres"), &E5Terrain::set_rolling);
    ClassDB::bind_method(D_METHOD("get_rolling"), &E5Terrain::get_rolling);
    ClassDB::bind_method(D_METHOD("set_sites", "sites"), &E5Terrain::set_sites);
    ClassDB::bind_method(D_METHOD("get_sites"), &E5Terrain::get_sites);
    ClassDB::bind_method(D_METHOD("set_islets", "islets"), &E5Terrain::set_islets);
    ClassDB::bind_method(D_METHOD("get_islets"), &E5Terrain::get_islets);
    ClassDB::bind_method(D_METHOD("set_bays", "bays"), &E5Terrain::set_bays);
    ClassDB::bind_method(D_METHOD("get_bays"), &E5Terrain::get_bays);
    ClassDB::bind_method(D_METHOD("set_cliffs", "cliffs"), &E5Terrain::set_cliffs);
    ClassDB::bind_method(D_METHOD("get_cliffs"), &E5Terrain::get_cliffs);
    ClassDB::bind_method(D_METHOD("set_mountain_pass", "pass"), &E5Terrain::set_mountain_pass);
    ClassDB::bind_method(D_METHOD("get_mountain_pass"), &E5Terrain::get_mountain_pass);
    ClassDB::bind_method(D_METHOD("river_distance_at", "world_x", "world_z"), &E5Terrain::river_distance_at);
    ClassDB::bind_method(D_METHOD("lake_distance_at", "world_x", "world_z"), &E5Terrain::lake_distance_at);
    ClassDB::bind_method(D_METHOD("set_paths", "paths"), &E5Terrain::set_paths);
    ClassDB::bind_method(D_METHOD("get_paths"), &E5Terrain::get_paths);
    ClassDB::bind_method(D_METHOD("set_trails", "trails"), &E5Terrain::set_trails);
    ClassDB::bind_method(D_METHOD("get_trails"), &E5Terrain::get_trails);
    ClassDB::bind_method(D_METHOD("set_trail_grade", "grade"), &E5Terrain::set_trail_grade);
    ClassDB::bind_method(D_METHOD("get_trail_grade"), &E5Terrain::get_trail_grade);
    ClassDB::bind_method(D_METHOD("plan_trail", "from", "to"), &E5Terrain::plan_trail);
    ClassDB::bind_method(D_METHOD("get_path_lines"), &E5Terrain::get_path_lines);
    ClassDB::bind_method(D_METHOD("get_path_count"), &E5Terrain::get_path_count);
    ClassDB::bind_method(D_METHOD("make_map_image"), &E5Terrain::make_map_image);
    ClassDB::bind_method(D_METHOD("get_side"), &E5Terrain::get_side);
    ClassDB::bind_method(D_METHOD("set_path_width", "metres"), &E5Terrain::set_path_width);
    ClassDB::bind_method(D_METHOD("get_path_width"), &E5Terrain::get_path_width);
    ClassDB::bind_method(D_METHOD("path_distance_at", "world_x", "world_z"), &E5Terrain::path_distance_at);
    ClassDB::bind_method(D_METHOD("set_extent", "metres"), &E5Terrain::set_extent);
    ClassDB::bind_method(D_METHOD("get_extent"), &E5Terrain::get_extent);
    ClassDB::bind_method(D_METHOD("set_material", "material"), &E5Terrain::set_material);
    ClassDB::bind_method(D_METHOD("get_material"), &E5Terrain::get_material);
    ClassDB::bind_method(D_METHOD("height_at", "world_x", "world_z"), &E5Terrain::height_at);
    ClassDB::bind_method(D_METHOD("slope_at", "world_x", "world_z"), &E5Terrain::slope_at);
    ClassDB::bind_method(D_METHOD("get_triangle_count"), &E5Terrain::get_triangle_count);

    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "radius", godot::PROPERTY_HINT_RANGE, "20,2000,1,suffix:m"),
                 "set_radius", "get_radius");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "hill_height", godot::PROPERTY_HINT_RANGE, "0,200,0.5,suffix:m"),
                 "set_hill_height", "get_hill_height");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "lowland_height", godot::PROPERTY_HINT_RANGE, "0.5,50,0.1,suffix:m"),
        "set_lowland_height", "get_lowland_height");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "sea_floor", godot::PROPERTY_HINT_RANGE, "-100,-1,0.5,suffix:m"),
                 "set_sea_floor", "get_sea_floor");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "hill_size", godot::PROPERTY_HINT_RANGE, "10,500,1,suffix:m"),
                 "set_hill_size", "get_hill_size");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "clearing_radius", godot::PROPERTY_HINT_RANGE, "0,200,1,suffix:m"),
                 "set_clearing_radius", "get_clearing_radius");
    ADD_PROPERTY(PropertyInfo(godot::Variant::INT, "seed"), "set_seed", "get_seed");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "mountain_height", godot::PROPERTY_HINT_RANGE, "0,500,1,suffix:m"),
                 "set_mountain_height", "get_mountain_height");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "mountain_radius", godot::PROPERTY_HINT_RANGE, "10,1000,1,suffix:m"),
        "set_mountain_radius", "get_mountain_radius");
    ADD_PROPERTY(PropertyInfo(godot::Variant::VECTOR2, "mountain_position", godot::PROPERTY_HINT_NONE, "suffix:m"),
                 "set_mountain_position", "get_mountain_position");
    ADD_PROPERTY(
        PropertyInfo(godot::Variant::FLOAT, "mountain_length", godot::PROPERTY_HINT_RANGE, "0,2000,1,suffix:m"),
        "set_mountain_length", "get_mountain_length");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "mountain_direction", godot::PROPERTY_HINT_RANGE,
                              "-180,180,1,radians_as_degrees"),
                 "set_mountain_direction", "get_mountain_direction");
    ADD_PROPERTY(PropertyInfo(godot::Variant::PACKED_FLOAT32_ARRAY, "regions"), "set_regions", "get_regions");
    ADD_PROPERTY(PropertyInfo(godot::Variant::VECTOR2, "mountain_pass"), "set_mountain_pass", "get_mountain_pass");
    ADD_PROPERTY(PropertyInfo(godot::Variant::PACKED_VECTOR2_ARRAY, "river"), "set_river", "get_river");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "river_width", godot::PROPERTY_HINT_RANGE, "1,40,0.5,suffix:m"),
                 "set_river_width", "get_river_width");
    ADD_PROPERTY(PropertyInfo(godot::Variant::PACKED_VECTOR3_ARRAY, "lakes"), "set_lakes", "get_lakes");
    ADD_PROPERTY(PropertyInfo(godot::Variant::PACKED_VECTOR3_ARRAY, "islets"), "set_islets", "get_islets");
    ADD_PROPERTY(PropertyInfo(godot::Variant::PACKED_VECTOR3_ARRAY, "sites"), "set_sites", "get_sites");
    ADD_PROPERTY(PropertyInfo(godot::Variant::PACKED_FLOAT32_ARRAY, "hills"), "set_hills", "get_hills");
    ADD_PROPERTY(PropertyInfo(godot::Variant::ARRAY, "ridges"), "set_ridges", "get_ridges");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "rolling", godot::PROPERTY_HINT_RANGE, "0,6,0.1,suffix:m"),
                 "set_rolling", "get_rolling");
    ADD_PROPERTY(PropertyInfo(godot::Variant::PACKED_VECTOR3_ARRAY, "bays"), "set_bays", "get_bays");
    ADD_PROPERTY(PropertyInfo(godot::Variant::VECTOR3, "cliffs"), "set_cliffs", "get_cliffs");
    ADD_PROPERTY(PropertyInfo(godot::Variant::ARRAY, "paths"), "set_paths", "get_paths");
    ADD_PROPERTY(PropertyInfo(godot::Variant::ARRAY, "trails"), "set_trails", "get_trails");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "trail_grade", godot::PROPERTY_HINT_RANGE, "0.05,1,0.01"),
                 "set_trail_grade", "get_trail_grade");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "path_width", godot::PROPERTY_HINT_RANGE, "0.5,10,0.1,suffix:m"),
                 "set_path_width", "get_path_width");
    ADD_PROPERTY(PropertyInfo(godot::Variant::FLOAT, "extent", godot::PROPERTY_HINT_RANGE, "48,2000,1,suffix:m"),
                 "set_extent", "get_extent");
    ADD_PROPERTY(PropertyInfo(godot::Variant::OBJECT, "material", godot::PROPERTY_HINT_RESOURCE_TYPE, "Material"),
                 "set_material", "get_material");
}

namespace {

constexpr float radians_per_degree = 3.14159265F / 180.0F;

// 0 at `from`, 1 at `to`, smooth between; `from` may be the larger.
float smooth_step_between(float from, float to, float value) {
    const float t = std::clamp((value - from) / (to - from), 0.0F, 1.0F);
    return t * t * (3.0F - 2.0F * t);
}

std::vector<gameplay::IslandDisc> discs_from(const godot::PackedVector3Array& values) {
    std::vector<gameplay::IslandDisc> discs;
    for (const godot::Vector3& value : values) {
        discs.push_back({.x = static_cast<float>(value.x),
                         .z = static_cast<float>(value.y),
                         .radius = static_cast<float>(value.z)});
    }
    return discs;
}

godot::PackedVector3Array discs_to(const std::vector<gameplay::IslandDisc>& discs) {
    godot::PackedVector3Array values;
    for (const gameplay::IslandDisc& disc : discs) {
        values.push_back(godot::Vector3(disc.x, disc.z, disc.radius));
    }
    return values;
}

} // namespace

void E5Terrain::set_regions(const godot::PackedFloat32Array& regions) {
    params_.regions.clear();
    for (std::int64_t index = 0; index + 3 < regions.size(); index += 4) {
        const int kind = std::clamp(static_cast<int>(regions[index]), 0, 3);
        params_.regions.push_back({.kind = static_cast<gameplay::IslandRegionKind>(kind),
                                   .x = regions[index + 1],
                                   .z = regions[index + 2],
                                   .radius = regions[index + 3]});
    }
}

// push_back takes a double (Apple's compiler refuses the silent widening of a float).
godot::PackedFloat32Array E5Terrain::get_regions() const {
    godot::PackedFloat32Array regions;
    for (const gameplay::IslandRegion& region : params_.regions) {
        regions.push_back(static_cast<double>(region.kind));
        regions.push_back(static_cast<double>(region.x));
        regions.push_back(static_cast<double>(region.z));
        regions.push_back(static_cast<double>(region.radius));
    }
    return regions;
}

int E5Terrain::region_at(float world_x, float world_z) const {
    const godot::Vector3 origin = get_global_position();
    return static_cast<int>(gameplay::island_region(params_, world_x - static_cast<float>(origin.x),
                                                    world_z - static_cast<float>(origin.z)));
}

void E5Terrain::build_regions(int cells_across) {
    const godot::Ref<godot::ShaderMaterial> material = material_;
    if (material.is_null()) {
        return;
    }
    // A pixel every two metres is plenty: the borders are tens of metres wide.
    const int size = std::max(cells_across / 2, 2);
    godot::PackedByteArray bytes;
    bytes.resize(static_cast<std::int64_t>(size) * size * 3);
    const float side = static_cast<float>(cells_across);
    for (int row = 0; row < size; ++row) {
        for (int column = 0; column < size; ++column) {
            const float x = (static_cast<float>(column) + 0.5F) / static_cast<float>(size) * side - side * 0.5F;
            const float z = (static_cast<float>(row) + 0.5F) / static_cast<float>(size) * side - side * 0.5F;
            const gameplay::RegionShares shares = gameplay::island_regions(params_, x, z);
            const std::int64_t at = (static_cast<std::int64_t>(row) * size + column) * 3;
            bytes[at] = static_cast<std::uint8_t>(std::lround(shares.pine * 255.0F));
            bytes[at + 1] = static_cast<std::uint8_t>(std::lround(shares.dry * 255.0F));
            bytes[at + 2] = static_cast<std::uint8_t>(std::lround(shares.marsh * 255.0F));
        }
    }
    const godot::Ref<godot::Image> image =
        godot::Image::create_from_data(size, size, false, godot::Image::FORMAT_RGB8, bytes);
    material->set_shader_parameter("region_picture", godot::ImageTexture::create_from_image(image));
    material->set_shader_parameter("region_picture_side", side);
}

void E5Terrain::set_river(const godot::PackedVector2Array& points) {
    params_.river.clear();
    for (const godot::Vector2& point : points) {
        params_.river.push_back({.x = static_cast<float>(point.x), .z = static_cast<float>(point.y)});
    }
}

godot::PackedVector2Array E5Terrain::get_river() const {
    godot::PackedVector2Array points;
    for (const gameplay::IslandPoint& point : params_.river) {
        points.push_back(godot::Vector2(point.x, point.z));
    }
    return points;
}

void E5Terrain::set_lakes(const godot::PackedVector3Array& lakes) {
    params_.lakes = discs_from(lakes);
}

godot::PackedVector3Array E5Terrain::get_lakes() const {
    return discs_to(params_.lakes);
}

void E5Terrain::set_hills(const godot::PackedFloat32Array& hills) {
    params_.hills.clear();
    for (std::int64_t index = 0; index + 3 < hills.size(); index += 4) {
        params_.hills.push_back(
            {.x = hills[index], .z = hills[index + 1], .radius = hills[index + 2], .height = hills[index + 3]});
    }
}

godot::PackedFloat32Array E5Terrain::get_hills() const {
    godot::PackedFloat32Array hills;
    for (const gameplay::IslandHill& hill : params_.hills) {
        hills.push_back(static_cast<double>(hill.x));
        hills.push_back(static_cast<double>(hill.z));
        hills.push_back(static_cast<double>(hill.radius));
        hills.push_back(static_cast<double>(hill.height));
    }
    return hills;
}

void E5Terrain::set_ridges(const godot::Array& ridges) {
    params_.ridges.clear();
    for (const godot::Variant& entry : ridges) {
        const godot::PackedFloat32Array numbers = entry;
        if (numbers.size() < 6) {
            continue;
        }
        gameplay::IslandRidge ridge{.points = {}, .width = numbers[1], .height = numbers[0]};
        for (std::int64_t index = 2; index + 1 < numbers.size(); index += 2) {
            ridge.points.push_back({.x = numbers[index], .z = numbers[index + 1]});
        }
        params_.ridges.push_back(std::move(ridge));
    }
}

godot::Array E5Terrain::get_ridges() const {
    godot::Array ridges;
    for (const gameplay::IslandRidge& ridge : params_.ridges) {
        godot::PackedFloat32Array numbers;
        numbers.push_back(static_cast<double>(ridge.height));
        numbers.push_back(static_cast<double>(ridge.width));
        for (const gameplay::IslandPoint& point : ridge.points) {
            numbers.push_back(static_cast<double>(point.x));
            numbers.push_back(static_cast<double>(point.z));
        }
        ridges.push_back(numbers);
    }
    return ridges;
}

void E5Terrain::set_sites(const godot::PackedVector3Array& sites) {
    params_.sites = discs_from(sites);
}

godot::PackedVector3Array E5Terrain::get_sites() const {
    return discs_to(params_.sites);
}

void E5Terrain::set_islets(const godot::PackedVector3Array& islets) {
    params_.islets = discs_from(islets);
}

godot::PackedVector3Array E5Terrain::get_islets() const {
    return discs_to(params_.islets);
}

void E5Terrain::set_bays(const godot::PackedVector3Array& bays) {
    params_.bays.clear();
    for (const godot::Vector3& bay : bays) {
        params_.bays.push_back({.direction = static_cast<float>(bay.x) * radians_per_degree,
                                .width = static_cast<float>(bay.y) * radians_per_degree,
                                .depth = static_cast<float>(bay.z)});
    }
}

godot::PackedVector3Array E5Terrain::get_bays() const {
    godot::PackedVector3Array bays;
    for (const gameplay::IslandBay& bay : params_.bays) {
        bays.push_back(godot::Vector3(bay.direction / radians_per_degree, bay.width / radians_per_degree, bay.depth));
    }
    return bays;
}

void E5Terrain::set_cliffs(const godot::Vector3& cliffs) {
    params_.cliff_from = static_cast<float>(cliffs.x) * radians_per_degree;
    params_.cliff_to = static_cast<float>(cliffs.y) * radians_per_degree;
    params_.cliff_height = static_cast<float>(cliffs.z);
}

godot::Vector3 E5Terrain::get_cliffs() const {
    return {params_.cliff_from / radians_per_degree, params_.cliff_to / radians_per_degree, params_.cliff_height};
}

float E5Terrain::river_distance_at(float world_x, float world_z) const {
    const godot::Vector3 origin = get_global_position();
    return gameplay::island_river_distance(params_, world_x - static_cast<float>(origin.x),
                                           world_z - static_cast<float>(origin.z));
}

float E5Terrain::lake_distance_at(float world_x, float world_z) const {
    const godot::Vector3 origin = get_global_position();
    return gameplay::island_lake_distance(params_, world_x - static_cast<float>(origin.x),
                                          world_z - static_cast<float>(origin.z));
}

float E5Terrain::ground(float x, float z) const {
    float height = trail_field_.apply(x, z, gameplay::island_height(params_, x, z));
    // A road keeps its feet dry: where it runs through shallows (a marsh's pools, the wet
    // banks of a river) the ground under it is raised to a causeway. Not in the river or
    // a lake themselves: there a bridge belongs.
    constexpr float causeway_height = 0.6F;
    constexpr float causeway_reach = 7.0F;
    if (height < causeway_height && height > -2.0F && path_count_ > 0) {
        float nearest = causeway_reach;
        for (int index = 0; index < path_count_; ++index) {
            nearest =
                std::min(nearest, gameplay::distance_to_path(smoothed_paths_[static_cast<std::size_t>(index)], x, z));
        }
        if (nearest < causeway_reach) {
            const float open_water =
                std::min(gameplay::island_river_distance(params_, x, z) - params_.river_width * 0.5F,
                         gameplay::island_lake_distance(params_, x, z));
            const float raised =
                smooth_step_between(causeway_reach, 2.5F, nearest) * smooth_step_between(0.5F, 3.0F, open_water);
            height = std::lerp(height, causeway_height, raised);
        }
    }
    return height;
}

int E5Terrain::cells_across() const {
    // A whole number of chunks, centred on the node.
    return std::max(static_cast<int>(std::ceil(extent_ * 2.0F / static_cast<float>(chunk_cells))), 1) * chunk_cells;
}

float E5Terrain::height_at(float world_x, float world_z) const {
    const godot::Vector3 origin = get_global_position();
    return static_cast<float>(origin.y) +
           ground(world_x - static_cast<float>(origin.x), world_z - static_cast<float>(origin.z));
}

float E5Terrain::slope_at(float world_x, float world_z) const {
    const godot::Vector3 origin = get_global_position();
    const float x = world_x - static_cast<float>(origin.x);
    const float z = world_z - static_cast<float>(origin.z);
    return std::hypot(ground(x + 1.0F, z) - ground(x - 1.0F, z), ground(x, z + 1.0F) - ground(x, z - 1.0F)) * 0.5F;
}

godot::PackedVector2Array E5Terrain::plan_trail(const godot::Vector2& from, const godot::Vector2& to) const {
    const std::vector<gameplay::PathPoint> way = gameplay::plan_trail(
        [this](float x, float z) { return gameplay::island_height(params_, x, z); },
        {.x = static_cast<float>(from.x), .z = static_cast<float>(from.y)},
        {.x = static_cast<float>(to.x), .z = static_cast<float>(to.y)}, static_cast<float>(cells_across()),
        trail_plan_step, trail_grade_ * trail_plan_margin, trail_plan_spacing);
    godot::PackedVector2Array points;
    for (const gameplay::PathPoint& point : way) {
        points.push_back(godot::Vector2(point.x, point.z));
    }
    return points;
}

godot::Array E5Terrain::get_path_lines() const {
    godot::Array lines;
    for (const std::vector<gameplay::PathPoint>& path : smoothed_paths_) {
        godot::PackedVector2Array line;
        line.resize(static_cast<std::int64_t>(path.size()));
        for (std::size_t index = 0; index < path.size(); ++index) {
            line.set(static_cast<std::int64_t>(index), godot::Vector2(path[index].x, path[index].z));
        }
        lines.push_back(line);
    }
    return lines;
}

godot::Ref<godot::Image> E5Terrain::make_map_image() const {
    const int samples = cells_across() + 1;
    if (heights_.size() != static_cast<std::size_t>(samples) * static_cast<std::size_t>(samples)) {
        return {};
    }
    const int size = samples - 1;
    godot::PackedByteArray bytes;
    bytes.resize(static_cast<std::int64_t>(size) * size * 3);
    std::uint8_t* const pixels = bytes.ptrw();
    const auto height_of = [&](int column, int row) {
        return heights_[static_cast<std::size_t>(std::clamp(row, 0, samples - 1)) * static_cast<std::size_t>(samples) +
                        static_cast<std::size_t>(std::clamp(column, 0, samples - 1))];
    };
    struct Rgb {
        float r, g, b;
    };
    const auto mix = [](Rgb a, Rgb b, float t) {
        t = std::clamp(t, 0.0F, 1.0F);
        return Rgb{.r = std::lerp(a.r, b.r, t), .g = std::lerp(a.g, b.g, t), .b = std::lerp(a.b, b.b, t)};
    };
    const auto step = [](float low, float high, float value) {
        const float t = std::clamp((value - low) / (high - low), 0.0F, 1.0F);
        return t * t * (3.0F - 2.0F * t);
    };
    // The colours of a drawn map rather than of the ground itself: quiet, so that marks stand out.
    constexpr Rgb deep{.r = 0.16F, .g = 0.3F, .b = 0.42F};
    constexpr Rgb shallow{.r = 0.42F, .g = 0.64F, .b = 0.68F};
    constexpr Rgb sand{.r = 0.87F, .g = 0.8F, .b = 0.62F};
    constexpr Rgb meadow{.r = 0.6F, .g = 0.7F, .b = 0.42F};
    constexpr Rgb upland{.r = 0.44F, .g = 0.56F, .b = 0.36F};
    constexpr Rgb rock{.r = 0.6F, .g = 0.57F, .b = 0.53F};
    constexpr Rgb snow{.r = 0.96F, .g = 0.97F, .b = 0.99F};
    for (int row = 0; row < size; ++row) {
        for (int column = 0; column < size; ++column) {
            const float height = height_of(column, row);
            const float rise_x = (height_of(column + 1, row) - height_of(column - 1, row)) * 0.5F;
            const float rise_z = (height_of(column, row + 1) - height_of(column, row - 1)) * 0.5F;
            const float steep = std::hypot(rise_x, rise_z);
            Rgb colour{};
            if (height < 0.0F) {
                colour = mix(shallow, deep, step(0.0F, -5.0F, height));
            } else {
                colour = mix(sand, meadow, step(0.9F, 1.8F, height));
                colour = mix(colour, upland, step(6.0F, 30.0F, height));
                colour =
                    mix(colour, rock,
                        std::max(step(0.55F, 0.95F, steep), step(map_tree_line - 6.0F, map_tree_line + 6.0F, height)));
                colour =
                    mix(colour, snow,
                        step(map_snow_line - 5.0F, map_snow_line + 6.0F, height) * (1.0F - step(1.3F, 2.2F, steep)));
                // Lit from the north-west: slopes facing it are lighter, those turned away darker.
                const float lit = std::clamp((rise_x + rise_z) * 0.42F, -0.45F, 0.4F);
                colour = Rgb{
                    .r = colour.r * (1.0F + lit), .g = colour.g * (1.0F + lit), .b = colour.b * (1.0F + lit * 0.9F)};
                // A height line every ten metres, fainter on the lowland.
                const float level = height / 10.0F;
                const float to_line = std::abs(level - std::round(level)) * 10.0F / std::max(steep, 0.08F);
                const float line = (1.0F - step(0.5F, 1.2F, to_line)) * (height > 4.0F ? 0.16F : 0.0F);
                colour = mix(colour, Rgb{.r = 0.25F, .g = 0.2F, .b = 0.15F}, line);
            }
            std::uint8_t* const pixel = pixels + (static_cast<std::ptrdiff_t>(row) * size + column) * 3;
            pixel[0] = static_cast<std::uint8_t>(std::clamp(colour.r, 0.0F, 1.0F) * 255.0F);
            pixel[1] = static_cast<std::uint8_t>(std::clamp(colour.g, 0.0F, 1.0F) * 255.0F);
            pixel[2] = static_cast<std::uint8_t>(std::clamp(colour.b, 0.0F, 1.0F) * 255.0F);
        }
    }
    return godot::Image::create_from_data(size, size, false, godot::Image::FORMAT_RGB8, bytes);
}

float E5Terrain::path_distance_at(float world_x, float world_z) const {
    const godot::Vector3 origin = get_global_position();
    const float x = world_x - static_cast<float>(origin.x);
    const float z = world_z - static_cast<float>(origin.z);
    float nearest = std::numeric_limits<float>::max();
    for (const std::vector<gameplay::PathPoint>& path : smoothed_paths_) {
        nearest = std::min(nearest, gameplay::distance_to_path(path, x, z));
    }
    return nearest;
}

void E5Terrain::build_paths(int cells_across) {
    smoothed_paths_.clear();
    const auto smooth_all = [this](const godot::Array& lines) {
        for (const godot::Variant& entry : lines) {
            const godot::PackedVector2Array given = entry;
            std::vector<gameplay::PathPoint> points;
            points.reserve(static_cast<std::size_t>(given.size()));
            for (const godot::Vector2& point : given) {
                points.push_back({.x = static_cast<float>(point.x), .z = static_cast<float>(point.y)});
            }
            if (points.size() >= 2) {
                smoothed_paths_.push_back(gameplay::smooth_path(points, path_step));
            }
        }
    };
    smooth_all(paths_);
    path_count_ = static_cast<int>(smoothed_paths_.size());
    smooth_all(trails_);
    // The trails are cut into the ground; from here on ground() gives the cut height.
    trail_field_.build(std::span(smoothed_paths_).subspan(static_cast<std::size_t>(path_count_)),
                       [this](float x, float z) { return gameplay::island_height(params_, x, z); },
                       static_cast<float>(cells_across), {.max_grade = trail_grade_});
    auto* const material = godot::Object::cast_to<godot::ShaderMaterial>(material_.ptr());
    if (material == nullptr || smoothed_paths_.empty()) {
        return;
    }

    // Only the pixels near a path are visited: around each of its points, a square as wide as the reach.
    const auto side = static_cast<float>(cells_across);
    const float metres_per_pixel = side / static_cast<float>(path_picture_size);
    const int reach_pixels = static_cast<int>(std::ceil(path_picture_reach / metres_per_pixel)) + 1;
    std::vector<std::uint8_t> nearness(static_cast<std::size_t>(path_picture_size) * path_picture_size, 0);
    for (const std::vector<gameplay::PathPoint>& path : smoothed_paths_) {
        for (const gameplay::PathPoint& point : path) {
            const int centre_column = static_cast<int>((point.x + side * 0.5F) / metres_per_pixel);
            const int centre_row = static_cast<int>((point.z + side * 0.5F) / metres_per_pixel);
            for (int row = std::max(centre_row - reach_pixels, 0);
                 row <= std::min(centre_row + reach_pixels, path_picture_size - 1); ++row) {
                for (int column = std::max(centre_column - reach_pixels, 0);
                     column <= std::min(centre_column + reach_pixels, path_picture_size - 1); ++column) {
                    const float x = (static_cast<float>(column) + 0.5F) * metres_per_pixel - side * 0.5F;
                    const float z = (static_cast<float>(row) + 0.5F) * metres_per_pixel - side * 0.5F;
                    const float near = 1.0F - gameplay::distance_to_path(path, x, z) / path_picture_reach;
                    const auto value = static_cast<std::uint8_t>(std::clamp(near, 0.0F, 1.0F) * 255.0F);
                    std::uint8_t& pixel =
                        nearness[static_cast<std::size_t>(row) * path_picture_size + static_cast<std::size_t>(column)];
                    pixel = std::max(pixel, value);
                }
            }
        }
    }
    godot::PackedByteArray bytes;
    bytes.resize(static_cast<std::int64_t>(nearness.size()));
    std::ranges::copy(nearness, bytes.ptrw());
    const godot::Ref<godot::Image> image =
        godot::Image::create_from_data(path_picture_size, path_picture_size, false, godot::Image::FORMAT_L8, bytes);
    material->set_shader_parameter("path_picture", godot::ImageTexture::create_from_image(image));
    material->set_shader_parameter("path_picture_side", side);
    material->set_shader_parameter("path_picture_reach", path_picture_reach);
    material->set_shader_parameter("path_width", path_width_);
}

void E5Terrain::build_chunk(int first_column, int first_row, int cells) {
    const auto x0 = static_cast<float>(first_column);
    const auto z0 = static_cast<float>(first_row);

    // Coarser cells for a chunk nobody looks at closely.
    float highest = -1000.0F;
    for (int row = 0; row <= cells; row += 4) {
        for (int column = 0; column <= cells; column += 4) {
            highest = std::max(highest, ground(x0 + static_cast<float>(column), z0 + static_cast<float>(row)));
        }
    }
    const int step = highest < submerged_below ? submerged_step : 1;
    const int points = cells / step + 1;

    godot::PackedVector3Array vertices;
    godot::PackedVector3Array normals;
    godot::PackedInt32Array indices;
    vertices.resize(static_cast<std::int64_t>(points) * points);
    normals.resize(vertices.size());
    for (int row = 0; row < points; ++row) {
        for (int column = 0; column < points; ++column) {
            const float x = x0 + static_cast<float>(column * step);
            const float z = z0 + static_cast<float>(row * step);
            const std::int64_t index = static_cast<std::int64_t>(row) * points + column;
            vertices.set(index, godot::Vector3(x, ground(x, z), z));
            // The normal comes from the function, not from the triangles: chunks then meet without a visible seam.
            const float rise_x = ground(x + 1.0F, z) - ground(x - 1.0F, z);
            const float rise_z = ground(x, z + 1.0F) - ground(x, z - 1.0F);
            normals.set(index, godot::Vector3(-rise_x, 2.0F, -rise_z).normalized());
        }
    }
    for (int row = 0; row < points - 1; ++row) {
        for (int column = 0; column < points - 1; ++column) {
            const std::int32_t a = row * points + column;
            const std::int32_t b = a + 1;
            const std::int32_t c = a + points;
            const std::int32_t d = c + 1;
            // Clockwise seen from above, which is the front side.
            for (const std::int32_t corner : {a, b, c, b, d, c}) {
                indices.push_back(corner);
            }
        }
    }
    triangle_count_ += static_cast<int>(indices.size() / 3);

    godot::Array arrays;
    arrays.resize(godot::Mesh::ARRAY_MAX);
    arrays[godot::Mesh::ARRAY_VERTEX] = vertices;
    arrays[godot::Mesh::ARRAY_NORMAL] = normals;
    arrays[godot::Mesh::ARRAY_INDEX] = indices;
    godot::Ref<godot::ArrayMesh> mesh;
    mesh.instantiate();
    mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES, arrays);

    auto* const instance = memnew(godot::MeshInstance3D);
    instance->set_mesh(mesh);
    if (material_.is_valid()) {
        instance->set_material_override(material_);
    }
    add_child(instance);
}

void E5Terrain::build_collision(int cells_across) {
    // One sample per metre, centred on the node: exactly the grid of the mesh.
    const int samples = cells_across + 1;
    godot::PackedFloat32Array heights;
    heights.resize(static_cast<std::int64_t>(samples) * samples);
    const float half = static_cast<float>(cells_across) * 0.5F;
    for (int row = 0; row < samples; ++row) {
        for (int column = 0; column < samples; ++column) {
            heights.set(static_cast<std::int64_t>(row) * samples + column,
                        static_cast<double>(ground(static_cast<float>(column) - half, static_cast<float>(row) - half)));
        }
    }
    heights_.assign(heights.ptr(), heights.ptr() + heights.size()); // kept for the map
    godot::Ref<godot::HeightMapShape3D> shape;
    shape.instantiate();
    shape->set_map_width(samples);
    shape->set_map_depth(samples);
    shape->set_map_data(heights);
    auto* const collision = memnew(godot::CollisionShape3D);
    collision->set_shape(shape);
    add_child(collision);
}

void E5Terrain::_ready() {
    E5_PROFILE_SCOPE("E5Terrain::_ready");
    add_to_group(group_name);

    const int cells_across = this->cells_across();
    const int chunks_across = cells_across / chunk_cells;
    const int first = -cells_across / 2;
    build_paths(cells_across);
    build_regions(cells_across);
    for (int row = 0; row < chunks_across; ++row) {
        for (int column = 0; column < chunks_across; ++column) {
            build_chunk(first + column * chunk_cells, first + row * chunk_cells, chunk_cells);
        }
    }
    build_collision(cells_across);

    const godot::TypedArray<godot::Node> standing = get_tree()->get_nodes_in_group(on_ground_group_name);
    for (const godot::Variant& item : standing) {
        if (auto* const node = godot::Object::cast_to<godot::Node3D>(item)) {
            godot::Vector3 position = node->get_global_position();
            const float offset = node->get_meta("ground_offset", 0.0F);
            position.y = height_at(static_cast<float>(position.x), static_cast<float>(position.z)) + offset;
            node->set_global_position(position);
        }
    }
    logger().info("terrain '{}': {} x {} m, {} chunks, {} triangles", godot::String(get_name()).utf8().get_data(),
                  cells_across, cells_across, chunks_across * chunks_across, triangle_count_);
}

} // namespace e5::bridge

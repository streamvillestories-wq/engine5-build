#pragma once

#include <godot_cpp/classes/immediate_mesh.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <random>

namespace e5::bridge {

// A bolt of lightning between two points in the world: a jagged, glowing
// ribbon that re-forks many times a second and is gone in under half a second.
// Purely for show; whoever creates it deals the damage.
// The generator is reseeded per arc in spawn(); the default seed never shows.
class E5LightningArc : public godot::Node3D { // NOLINT(bugprone-random-generator-seed)
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5LightningArc, godot::Node3D)

public:
    // Creates an arc under `parent` from one world position to another.
    static E5LightningArc* spawn(godot::Node* parent, const godot::Vector3& from, const godot::Vector3& to,
                                 const godot::Color& colour);

    void _ready() override;
    void _process(double delta) override;

protected:
    static void _bind_methods() {}

private:
    void rebuild();
    void add_ribbon(const godot::Vector3& eye, float width, const godot::Color& colour);

    godot::Ref<godot::ImmediateMesh> mesh_;
    godot::Vector3 from_;
    godot::Vector3 to_;
    godot::Color colour_{0.6F, 0.8F, 1.0F};
    std::vector<godot::Vector3> points_;
    float age_ = 0.0F;
    float until_refork_ = 0.0F;
    // Cosmetic only: every machine may fork its lightning differently.
    std::minstd_rand random_; // NOLINT(bugprone-random-generator-seed): reseeded in spawn()
};

} // namespace e5::bridge

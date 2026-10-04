#include "arrow_rain.hpp"

#include "arrow.hpp"
#include "e5/gameplay/skills.hpp"

#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace e5::bridge {
namespace {

// How long the node lingers after the last arrow, so the marker fades with the volley.
constexpr float linger_seconds = 0.4F;
constexpr float splash_radius = 1.2F; // metres around each arrow of the rain

} // namespace

void E5ArrowRain::configure(const godot::RID& shooter, const godot::Ref<godot::PackedScene>& marker,
                            const godot::Ref<godot::PackedScene>& impact_effect) {
    shooter_ = shooter;
    marker_ = marker;
    impact_effect_ = impact_effect;
}

void E5ArrowRain::_ready() {
    if (marker_.is_valid()) {
        if (auto* const marker = godot::Object::cast_to<godot::Node3D>(marker_->instantiate())) {
            add_child(marker);
        }
    }
}

void E5ArrowRain::_physics_process(double delta) {
    elapsed_ += static_cast<float>(delta);

    // Arrow i is due at delay + i/count of the duration.
    while (dropped_ < count_ &&
           elapsed_ >= delay_ + duration_ * static_cast<float>(dropped_) / static_cast<float>(count_)) {
        drop_arrow(dropped_);
        ++dropped_;
    }
    if (dropped_ >= count_ && elapsed_ >= delay_ + duration_ + linger_seconds) {
        queue_free();
    }
}

void E5ArrowRain::drop_arrow(int index) {
    const gameplay::Vec3 offset = gameplay::rain_arrow_offset(index, count_, radius_);
    const godot::Vector3 start = get_global_position() + godot::Vector3(offset.x, height_, offset.z);
    const godot::Vector3 down(0.0F, -1.0F, 0.0F);

    // Arrows belong to the world, not to this short-lived node.
    auto* const arrow = memnew(E5Arrow);
    get_parent()->add_child(arrow);
    arrow->set_global_transform(
        godot::Transform3D(godot::Basis::looking_at(down, godot::Vector3(1.0F, 0.0F, 0.0F)), start));
    arrow->set_impact_effect(impact_effect_);
    arrow->set_damage(harmless_ ? 0.0F : gameplay::skill_damage(gameplay::SkillId::ArrowRain));
    // A rain is an area attack: each arrow also hurts what stands close to where it lands,
    // otherwise an enemy between two arrows of the pattern would walk through untouched.
    arrow->set_blast_radius(splash_radius);
    arrow->launch(down * speed_, shooter_);
}

} // namespace e5::bridge
#include "health_hud.hpp"

#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/font_variation.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/text_server.hpp>
#include <godot_cpp/classes/text_server_manager.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <cmath>
#include <format>

namespace e5::bridge {
namespace {

constexpr float bar_width = 420.0F;
constexpr float bar_height = 12.0F;
constexpr float bar_margin = 12.0F;              // room in the rectangle for glow and shadow; the shader's `margin`
constexpr float bar_centre_from_bottom = 105.0F; // above the skill bar
constexpr float health_per_notch = 25.0F;
constexpr float fill_rate = 9.0F;           // 1/s: how fast the bar closes on the real value
constexpr float trail_hold_seconds = 0.45F; // the pale part waits this long after a blow
constexpr float trail_rate = 0.55F;         // share of the bar per second it then falls
constexpr float hit_fade = 5.0F;            // 1/s
constexpr float gain_fade = 2.5F;           // 1/s
constexpr float number_rate = 10.0F;        // 1/s
constexpr float low_health = 0.3F;          // below this share the danger signs start
constexpr float low_vignette = 0.5F;        // the red at the edges at zero health
constexpr float dead_vignette = 0.75F;
constexpr float gain_threshold = 0.04F; // a jump up of this share shimmers: a potion, not regeneration

[[nodiscard]] godot::Ref<godot::Font> font(const char* path, int weight) {
    godot::ResourceLoader* const loader = godot::ResourceLoader::get_singleton();
    if (!loader->exists(path)) {
        return {};
    }
    godot::Ref<godot::FontVariation> variation;
    variation.instantiate();
    variation->set_base_font(loader->load(path));
    godot::Dictionary axes;
    axes[godot::TextServerManager::get_singleton()->get_primary_interface()->name_to_tag("wght")] = weight;
    variation->set_variation_opentype(axes);
    return variation;
}

[[nodiscard]] godot::Ref<godot::ShaderMaterial> material(const char* path) {
    godot::ResourceLoader* const loader = godot::ResourceLoader::get_singleton();
    godot::Ref<godot::ShaderMaterial> result;
    if (loader->exists(path)) {
        result.instantiate();
        result->set_shader(loader->load(path));
    }
    return result;
}

[[nodiscard]] godot::Label* label(const godot::Ref<godot::Font>& face, int size, const godot::Color& colour) {
    auto* const text = memnew(godot::Label);
    text->set_mouse_filter(godot::Control::MOUSE_FILTER_IGNORE);
    if (face.is_valid()) {
        text->add_theme_font_override("font", face);
    }
    text->add_theme_font_size_override("font_size", size);
    text->add_theme_color_override("font_color", colour);
    text->add_theme_color_override("font_shadow_color", godot::Color(0.0F, 0.0F, 0.0F, 0.7F));
    text->add_theme_constant_override("shadow_offset_x", 0);
    text->add_theme_constant_override("shadow_offset_y", 1);
    text->add_theme_constant_override("shadow_outline_size", 4);
    return text;
}

} // namespace

void E5HealthHud::_ready() {
    set_layer(89); // just below the skill bar
    const godot::Ref<godot::Font> bold = font("res://ui/fonts/Inter.ttf", 700);
    const godot::Ref<godot::Font> regular = font("res://ui/fonts/Inter.ttf", 500);
    const godot::Ref<godot::Font> title = font("res://ui/fonts/Cinzel.ttf", 700);

    vignette_ = memnew(godot::ColorRect);
    vignette_->set_anchors_preset(godot::Control::PRESET_FULL_RECT);
    vignette_->set_mouse_filter(godot::Control::MOUSE_FILTER_IGNORE);
    vignette_material_ = material("res://shaders/hurt_vignette.gdshader");
    vignette_->set_material(vignette_material_);
    vignette_->set_color(godot::Color(0.0F, 0.0F, 0.0F, 0.0F)); // what shows if the shader is missing: nothing
    add_child(vignette_);

    bar_ = memnew(godot::ColorRect);
    bar_->set_mouse_filter(godot::Control::MOUSE_FILTER_IGNORE);
    bar_->set_anchors_preset(godot::Control::PRESET_CENTER_BOTTOM);
    bar_->set_offset(godot::SIDE_LEFT, -bar_width * 0.5F - bar_margin);
    bar_->set_offset(godot::SIDE_RIGHT, bar_width * 0.5F + bar_margin);
    bar_->set_offset(godot::SIDE_TOP, -bar_centre_from_bottom - bar_height * 0.5F - bar_margin);
    bar_->set_offset(godot::SIDE_BOTTOM, -bar_centre_from_bottom + bar_height * 0.5F + bar_margin);
    bar_material_ = material("res://shaders/player_health_bar.gdshader");
    bar_->set_material(bar_material_);
    bar_->set_color(godot::Color(0.3F, 0.84F, 0.42F, bar_material_.is_valid() ? 1.0F : 0.0F));
    if (bar_material_.is_valid()) {
        bar_material_->set_shader_parameter(
            "size", godot::Vector2(bar_width + 2.0F * bar_margin, bar_height + 2.0F * bar_margin));
        bar_material_->set_shader_parameter("margin", bar_margin);
    }
    add_child(bar_);

    // Left of the bar, on its line: the number, and the maximum small behind it.
    number_ = label(bold, 22, godot::Color(1.0F, 1.0F, 1.0F, 0.96F));
    number_->set_anchors_preset(godot::Control::PRESET_CENTER_BOTTOM);
    number_->set_horizontal_alignment(godot::HORIZONTAL_ALIGNMENT_RIGHT);
    number_->set_vertical_alignment(godot::VERTICAL_ALIGNMENT_CENTER);
    number_->set_offset(godot::SIDE_LEFT, -bar_width * 0.5F - 118.0F);
    number_->set_offset(godot::SIDE_RIGHT, -bar_width * 0.5F - 52.0F);
    number_->set_offset(godot::SIDE_TOP, -bar_centre_from_bottom - 16.0F);
    number_->set_offset(godot::SIDE_BOTTOM, -bar_centre_from_bottom + 16.0F);
    add_child(number_);
    maximum_ = label(regular, 12, godot::Color(1.0F, 1.0F, 1.0F, 0.55F));
    maximum_->set_anchors_preset(godot::Control::PRESET_CENTER_BOTTOM);
    maximum_->set_vertical_alignment(godot::VERTICAL_ALIGNMENT_CENTER);
    maximum_->set_offset(godot::SIDE_LEFT, -bar_width * 0.5F - 48.0F);
    maximum_->set_offset(godot::SIDE_RIGHT, -bar_width * 0.5F - 10.0F);
    maximum_->set_offset(godot::SIDE_TOP, -bar_centre_from_bottom - 12.0F);
    maximum_->set_offset(godot::SIDE_BOTTOM, -bar_centre_from_bottom + 18.0F);
    add_child(maximum_);

    auto* const notice = memnew(godot::VBoxContainer);
    notice->set_mouse_filter(godot::Control::MOUSE_FILTER_IGNORE);
    notice->set_anchors_preset(godot::Control::PRESET_FULL_RECT);
    notice->set_alignment(godot::BoxContainer::ALIGNMENT_CENTER);
    notice->add_theme_constant_override("separation", 6);
    notice->set_visible(false);
    add_child(notice);
    notice_ = notice;
    godot::Label* const fallen = label(title, 52, godot::Color(0.96F, 0.9F, 0.86F, 1.0F));
    fallen->set_text("You Have Fallen");
    fallen->set_horizontal_alignment(godot::HORIZONTAL_ALIGNMENT_CENTER);
    notice->add_child(fallen);
    countdown_ = label(regular, 16, godot::Color(1.0F, 1.0F, 1.0F, 0.7F));
    countdown_->set_horizontal_alignment(godot::HORIZONTAL_ALIGNMENT_CENTER);
    notice->add_child(countdown_);
}

void E5HealthHud::show_health(float health, float max_health) {
    const float before = max_health_ > 0.0F ? health_ / max_health_ : 0.0F;
    health_ = std::max(health, 0.0F);
    max_health_ = std::max(max_health, 1.0F);
    // A jump up in one step is a potion; regeneration comes in crumbs and does not shimmer.
    if (health_ / max_health_ - before > gain_threshold && !dead_) {
        gain_ = 1.0F;
    }
}

void E5HealthHud::flash(float strength) {
    hit_ = std::clamp(std::max(hit_, 0.45F + strength * 2.0F), 0.0F, 1.0F);
    trail_hold_ = trail_hold_seconds;
}

void E5HealthHud::show_respawn(float seconds_left) {
    const bool dead = seconds_left >= 0.0F;
    if (dead_ && !dead) {
        // Back on her feet: the bar starts full, nothing trails.
        fraction_ = 1.0F;
        trail_fraction_ = 1.0F;
    }
    dead_ = dead;
    if (notice_ != nullptr) {
        notice_->set_visible(dead);
    }
    if (dead && countdown_ != nullptr) {
        countdown_->set_text(godot::String::utf8(std::format("Returning in {:.0f}", std::ceil(seconds_left)).c_str()));
    }
}

void E5HealthHud::_process(double delta) {
    if (bar_ == nullptr) {
        return;
    }
    const auto dt = static_cast<float>(delta);
    seconds_ += dt;
    const float target = std::clamp(health_ / max_health_, 0.0F, 1.0F);
    const float ease = 1.0F - std::exp(-fill_rate * dt);

    fraction_ += (target - fraction_) * ease;
    // The pale part stays where the bar was, waits, then falls after it; it never lags a gain.
    trail_hold_ = std::max(trail_hold_ - dt, 0.0F);
    if (trail_fraction_ < fraction_) {
        trail_fraction_ = fraction_;
    } else if (trail_hold_ <= 0.0F) {
        trail_fraction_ = std::max(trail_fraction_ - trail_rate * dt, fraction_);
    }
    hit_ = std::max(hit_ - hit_fade * dt * std::max(hit_, 0.15F), 0.0F);
    gain_ = std::max(gain_ - gain_fade * dt, 0.0F);
    // 0 above the danger line, 1 at nothing left.
    const float low = dead_ ? 0.0F : std::clamp((low_health - target) / low_health, 0.0F, 1.0F);

    if (bar_material_.is_valid()) {
        bar_material_->set_shader_parameter("fill", fraction_);
        bar_material_->set_shader_parameter("trail", trail_fraction_);
        bar_material_->set_shader_parameter("hit", hit_);
        bar_material_->set_shader_parameter("gain", gain_);
        bar_material_->set_shader_parameter("low", low > 0.0F ? 0.35F + 0.65F * low : 0.0F);
        bar_material_->set_shader_parameter("notches", std::max(std::round(max_health_ / health_per_notch), 1.0F));
    }

    shown_number_ += (health_ - shown_number_) * (1.0F - std::exp(-number_rate * dt));
    number_->set_text(godot::String::num_int64(static_cast<int64_t>(std::ceil(shown_number_ - 0.05F))));
    maximum_->set_text(godot::String::utf8(std::format("/ {:.0f}", max_health_).c_str()));
    // The number takes the bar's alarm: white, then red and beating.
    const float beat = 0.5F + 0.5F * std::sin(seconds_ * 7.0F);
    number_->add_theme_color_override(
        "font_color", godot::Color(1.0F, 1.0F, 1.0F, 0.96F).lerp(godot::Color(1.0F, 0.35F + 0.2F * beat, 0.3F), low));

    if (vignette_material_.is_valid()) {
        const float held = dead_ ? dead_vignette : low * low_vignette * (0.75F + 0.25F * beat);
        vignette_material_->set_shader_parameter("strength", std::max(held, hit_ * 0.85F));
    }
}

} // namespace e5::bridge

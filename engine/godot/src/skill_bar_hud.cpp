#include "skill_bar_hud.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace e5::bridge {
namespace {

constexpr float slot_width = 92.0F;
constexpr float slot_height = 58.0F;
constexpr int bottom_margin = 18;

godot::Ref<godot::StyleBoxFlat> make_style(const godot::Color& background, const godot::Color& border,
                                           int border_width) {
    godot::Ref<godot::StyleBoxFlat> style;
    style.instantiate();
    style->set_bg_color(background);
    style->set_border_color(border);
    style->set_border_width_all(border_width);
    style->set_corner_radius_all(6);
    style->set_content_margin_all(6.0F);
    return style;
}

godot::Label* make_label(int font_size, const godot::Color& colour) {
    auto* const label = memnew(godot::Label);
    label->add_theme_font_size_override("font_size", font_size);
    label->add_theme_color_override("font_color", colour);
    label->set_mouse_filter(godot::Control::MOUSE_FILTER_IGNORE);
    return label;
}

} // namespace

void E5SkillBarHud::_ready() {
    set_layer(90); // above the game, below the performance overlay

    normal_style_ = make_style(godot::Color(0.05F, 0.06F, 0.07F, 0.62F), godot::Color(1.0F, 1.0F, 1.0F, 0.18F), 1);
    selected_style_ = make_style(godot::Color(0.1F, 0.22F, 0.1F, 0.82F), godot::Color(0.55F, 1.0F, 0.45F, 1.0F), 3);

    auto* const row = memnew(godot::HBoxContainer);
    row->set_mouse_filter(godot::Control::MOUSE_FILTER_IGNORE);
    row->add_theme_constant_override("separation", 6);
    add_child(row);

    for (std::size_t index = 0; index < slot_count; ++index) {
        auto* const panel = memnew(godot::PanelContainer);
        panel->set_mouse_filter(godot::Control::MOUSE_FILTER_IGNORE);
        panel->set_custom_minimum_size(godot::Vector2(slot_width, slot_height));
        panel->add_theme_stylebox_override("panel", normal_style_);
        row->add_child(panel);

        auto* const column = memnew(godot::VBoxContainer);
        column->set_mouse_filter(godot::Control::MOUSE_FILTER_IGNORE);
        panel->add_child(column);

        // Keys run 1..9 and then 0 for the tenth slot.
        godot::Label* const key = make_label(12, godot::Color(1.0F, 1.0F, 1.0F, 0.6F));
        key->set_text(index < 10 ? godot::String::num_int64(static_cast<std::int64_t>((index + 1) % 10))
                                 : godot::String(index == 10 ? "-" : "F"));
        column->add_child(key);

        godot::Label* const name = make_label(14, godot::Color(1.0F, 1.0F, 1.0F, 0.95F));
        name->set_horizontal_alignment(godot::HORIZONTAL_ALIGNMENT_CENTER);
        column->add_child(name);

        slots_[index] = {.panel = panel, .name = name};
    }

    // Bottom centre; growing to both sides keeps it centred whatever its width.
    row->set_anchors_and_offsets_preset(godot::Control::PRESET_CENTER_BOTTOM, godot::Control::PRESET_MODE_MINSIZE,
                                        bottom_margin);
    row->set_h_grow_direction(godot::Control::GROW_DIRECTION_BOTH);
    row->set_v_grow_direction(godot::Control::GROW_DIRECTION_BEGIN);
}

void E5SkillBarHud::set_slot_name(std::size_t index, const godot::String& name) {
    if (index < slot_count && slots_[index].name != nullptr) {
        slots_[index].name->set_text(name);
    }
}

void E5SkillBarHud::set_selected(std::size_t index) {
    for (std::size_t slot = 0; slot < slot_count; ++slot) {
        if (slots_[slot].panel != nullptr) {
            slots_[slot].panel->add_theme_stylebox_override("panel", slot == index ? selected_style_ : normal_style_);
        }
    }
}

} // namespace e5::bridge
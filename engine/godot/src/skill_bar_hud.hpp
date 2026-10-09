#pragma once

#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/variant/string.hpp>

#include <array>
#include <cstddef>

namespace godot {
class Label;
class PanelContainer;
} // namespace godot

namespace e5::bridge {

// The row of ten skill slots at the bottom of the screen. Display only: it is
// told what each slot holds and which one is selected. Nothing in it reacts
// to the mouse, so it can never swallow camera input.
class E5SkillBarHud : public godot::CanvasLayer {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5SkillBarHud, godot::CanvasLayer)

public:
    static constexpr std::size_t slot_count = 12;

    void _ready() override;

    void set_slot_name(std::size_t index, const godot::String& name);
    void set_selected(std::size_t index);

protected:
    static void _bind_methods() {}

private:
    struct Slot {
        godot::PanelContainer* panel = nullptr; // non-owning children
        godot::Label* name = nullptr;
    };

    std::array<Slot, slot_count> slots_;
    godot::Ref<godot::StyleBoxFlat> normal_style_;
    godot::Ref<godot::StyleBoxFlat> selected_style_;
};

} // namespace e5::bridge
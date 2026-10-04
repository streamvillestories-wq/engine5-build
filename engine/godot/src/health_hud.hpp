#pragma once

#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/shader_material.hpp>

namespace godot {
class ColorRect;
class Control;
class Label;
} // namespace godot

namespace e5::bridge {

// The player's health on screen: a bar above the skill bar with the number
// beside it, red closing in from the edges of the picture when hurt or nearly
// dead, and a notice while dead. The bar is one shader
// (shaders/player_health_bar.gdshader); this class feeds it and animates the
// values. Built in code; whoever owns it calls `show_health` every step.
class E5HealthHud : public godot::CanvasLayer {
    // Findings inside Godot's macro expansion are not ours to fix.
    // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
    GDCLASS(E5HealthHud, godot::CanvasLayer)

public:
    void _ready() override;
    void _process(double delta) override;

    void show_health(float health, float max_health);
    // The moment of a blow: stronger for a heavier one (damage as a share of the maximum).
    void flash(float strength);
    // While dead: the notice and the seconds until she is back; a negative value hides it.
    void show_respawn(float seconds_left);

protected:
    static void _bind_methods() {}

private:
    // Non-owning: child nodes owned by the scene tree.
    godot::ColorRect* bar_ = nullptr;
    godot::Label* number_ = nullptr;  // current health, large
    godot::Label* maximum_ = nullptr; // "/ 130", small
    godot::ColorRect* vignette_ = nullptr;
    godot::Control* notice_ = nullptr;
    godot::Label* countdown_ = nullptr;
    godot::Ref<godot::ShaderMaterial> bar_material_;
    godot::Ref<godot::ShaderMaterial> vignette_material_;

    float health_ = 100.0F;
    float max_health_ = 100.0F;
    float fraction_ = 1.0F;       // what the bar shows; eases towards the real value
    float trail_fraction_ = 1.0F; // where it was: follows after a pause
    float trail_hold_ = 0.0F;     // seconds the trail still waits
    float hit_ = 0.0F;
    float gain_ = 0.0F;
    float shown_number_ = 100.0F; // counts towards the real value
    float seconds_ = 0.0F;
    bool dead_ = false;
};

} // namespace e5::bridge

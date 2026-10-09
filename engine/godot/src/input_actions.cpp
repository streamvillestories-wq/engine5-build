#include "input_actions.hpp"

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <array>
#include <cstddef>

namespace e5::bridge {

void ensure_default_input_actions() {
    struct Binding {
        const char* action;
        godot::Key key;
    };
    // Physical keycodes: WASD stays in the same place on non-QWERTY layouts.
    constexpr std::array bindings{
        Binding{.action = actions::move_forward, .key = godot::KEY_W},
        Binding{.action = actions::move_back, .key = godot::KEY_S},
        Binding{.action = actions::move_left, .key = godot::KEY_A},
        Binding{.action = actions::move_right, .key = godot::KEY_D},
        Binding{.action = actions::jump, .key = godot::KEY_SPACE},
        Binding{.action = actions::sprint, .key = godot::KEY_SHIFT},
        Binding{.action = actions::walk, .key = godot::KEY_CTRL},
        Binding{.action = actions::dodge_alt, .key = godot::KEY_T},
        Binding{.action = actions::emote, .key = godot::KEY_N},
        Binding{.action = actions::interact, .key = godot::KEY_E},
        Binding{.action = actions::use_potion, .key = godot::KEY_Q},
        Binding{.action = actions::block, .key = godot::KEY_R},
        Binding{.action = actions::inventory, .key = godot::KEY_TAB},
        Binding{.action = actions::inventory, .key = godot::KEY_I},
    };

    struct MouseBinding {
        const char* action;
        godot::MouseButton button;
    };
    constexpr std::array mouse_bindings{
        MouseBinding{.action = actions::aim, .button = godot::MOUSE_BUTTON_RIGHT},
        MouseBinding{.action = actions::attack, .button = godot::MOUSE_BUTTON_LEFT},
    };

    godot::InputMap* const input_map = godot::InputMap::get_singleton();
    for (const MouseBinding& binding : mouse_bindings) {
        const godot::StringName action(binding.action);
        if (input_map->has_action(action)) {
            continue;
        }
        godot::Ref<godot::InputEventMouseButton> event;
        event.instantiate();
        event->set_button_index(binding.button);
        input_map->add_action(action);
        input_map->action_add_event(action, event);
    }
    // Slots 1..9 sit on the number keys in order; the tenth is 0, as on the keyboard row.
    constexpr std::array slot_keys{godot::KEY_1,     godot::KEY_2, godot::KEY_3, godot::KEY_4, godot::KEY_5,
                                   godot::KEY_6,     godot::KEY_7, godot::KEY_8, godot::KEY_9, godot::KEY_0,
                                   godot::KEY_MINUS, godot::KEY_F}; // F: the archer's dagger is on the twelfth
    for (int slot = 0; slot < actions::skill_slot_count; ++slot) {
        const godot::StringName action(godot::String(actions::skill_prefix) + godot::String::num_int64(slot + 1));
        if (input_map->has_action(action)) {
            continue;
        }
        godot::Ref<godot::InputEventKey> event;
        event.instantiate();
        event->set_physical_keycode(slot_keys.at(static_cast<std::size_t>(slot)));
        input_map->add_action(action);
        input_map->action_add_event(action, event);
    }
    for (const Binding& binding : bindings) {
        const godot::StringName action(binding.action);
        if (input_map->has_action(action)) {
            continue;
        }
        godot::Ref<godot::InputEventKey> event;
        event.instantiate();
        event->set_physical_keycode(binding.key);
        input_map->add_action(action);
        input_map->action_add_event(action, event);
    }
}

} // namespace e5::bridge

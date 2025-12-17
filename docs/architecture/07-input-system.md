# Input System Architecture

The input system translates raw platform input into game actions, supporting keyboard, mouse, touch, and gamepad.

## Design Goals

1. **Action-based** - Game code responds to "actions" not raw keys
2. **Rebindable** - Players can customize controls
3. **Multi-input** - Same action can be triggered by keyboard or touch
4. **State + Events** - Both "is key down" and "was key just pressed"

## Input Flow

```
Platform Events → Input System → Action Mapping → Game
     │                  │              │
     │                  │              └─► "jump" action
     └─► KeyDown(W)     └─► W pressed
```

## Raw Input State

```cpp
// engine/input.h

namespace cafe {

// Key codes (platform-independent)
enum class Key {
    Unknown = 0,

    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // Numbers
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

    // Function keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    // Navigation
    Up, Down, Left, Right,

    // Modifiers
    LeftShift, RightShift, LeftControl, RightControl,
    LeftAlt, RightAlt, LeftCommand, RightCommand,

    // Other
    Space, Enter, Escape, Tab, Backspace, Delete,

    COUNT
};

enum class MouseButton {
    Left, Right, Middle, COUNT
};

// Current frame input state
struct InputState {
    // Keyboard
    std::bitset<static_cast<size_t>(Key::COUNT)> keys_down;
    std::bitset<static_cast<size_t>(Key::COUNT)> keys_pressed;   // Just pressed this frame
    std::bitset<static_cast<size_t>(Key::COUNT)> keys_released;  // Just released this frame

    // Mouse
    float mouse_x = 0, mouse_y = 0;
    float mouse_dx = 0, mouse_dy = 0;  // Movement this frame
    float scroll_x = 0, scroll_y = 0;
    std::bitset<static_cast<size_t>(MouseButton::COUNT)> mouse_down;
    std::bitset<static_cast<size_t>(MouseButton::COUNT)> mouse_pressed;
    std::bitset<static_cast<size_t>(MouseButton::COUNT)> mouse_released;

    // Touch (for mobile)
    struct Touch {
        int id;
        float x, y;
        bool active;
    };
    std::array<Touch, 10> touches;
    int touch_count = 0;

    // Text input (for text fields)
    std::string text_input;
};

class Input {
private:
    InputState current_;
    InputState previous_;

public:
    // Called at start of frame
    void begin_frame() {
        previous_ = current_;

        // Clear per-frame state
        current_.keys_pressed.reset();
        current_.keys_released.reset();
        current_.mouse_pressed.reset();
        current_.mouse_released.reset();
        current_.mouse_dx = current_.mouse_dy = 0;
        current_.scroll_x = current_.scroll_y = 0;
        current_.text_input.clear();
    }

    // Process platform event
    void handle_event(const InputEvent& event) {
        switch (event.type) {
            case InputEvent::KeyDown: {
                Key key = translate_keycode(event.key_code);
                if (!current_.keys_down[static_cast<size_t>(key)]) {
                    current_.keys_pressed[static_cast<size_t>(key)] = true;
                }
                current_.keys_down[static_cast<size_t>(key)] = true;
                break;
            }
            case InputEvent::KeyUp: {
                Key key = translate_keycode(event.key_code);
                current_.keys_down[static_cast<size_t>(key)] = false;
                current_.keys_released[static_cast<size_t>(key)] = true;
                break;
            }
            case InputEvent::MouseMove:
                current_.mouse_dx += event.x - current_.mouse_x;
                current_.mouse_dy += event.y - current_.mouse_y;
                current_.mouse_x = event.x;
                current_.mouse_y = event.y;
                break;
            case InputEvent::MouseDown: {
                auto btn = static_cast<MouseButton>(event.mouse_button);
                current_.mouse_down[static_cast<size_t>(btn)] = true;
                current_.mouse_pressed[static_cast<size_t>(btn)] = true;
                break;
            }
            case InputEvent::MouseUp: {
                auto btn = static_cast<MouseButton>(event.mouse_button);
                current_.mouse_down[static_cast<size_t>(btn)] = false;
                current_.mouse_released[static_cast<size_t>(btn)] = true;
                break;
            }
            case InputEvent::Scroll:
                current_.scroll_x += event.x;
                current_.scroll_y += event.y;
                break;
            case InputEvent::TextInput:
                current_.text_input += event.text;
                break;
            // Touch events similar to mouse
        }
    }

    // Queries
    bool key_down(Key key) const {
        return current_.keys_down[static_cast<size_t>(key)];
    }

    bool key_pressed(Key key) const {
        return current_.keys_pressed[static_cast<size_t>(key)];
    }

    bool key_released(Key key) const {
        return current_.keys_released[static_cast<size_t>(key)];
    }

    bool mouse_down(MouseButton btn) const {
        return current_.mouse_down[static_cast<size_t>(btn)];
    }

    bool mouse_pressed(MouseButton btn) const {
        return current_.mouse_pressed[static_cast<size_t>(btn)];
    }

    float mouse_x() const { return current_.mouse_x; }
    float mouse_y() const { return current_.mouse_y; }
    float scroll_y() const { return current_.scroll_y; }

    const std::string& text_input() const { return current_.text_input; }

private:
    Key translate_keycode(int native_code);  // Platform-specific
};

} // namespace cafe
```

## Action Mapping

```cpp
// engine/input_map.h

namespace cafe {

using ActionID = uint32_t;
constexpr ActionID INVALID_ACTION = 0;

// Input binding types
struct KeyBinding {
    Key key;
    bool with_shift = false;
    bool with_ctrl = false;
    bool with_alt = false;
};

struct MouseBinding {
    MouseButton button;
};

struct Binding {
    enum Type { None, Keyboard, Mouse, Touch };
    Type type = None;

    union {
        KeyBinding key;
        MouseBinding mouse;
    };

    static Binding from_key(Key k) {
        Binding b;
        b.type = Keyboard;
        b.key = {k, false, false, false};
        return b;
    }

    static Binding from_mouse(MouseButton btn) {
        Binding b;
        b.type = Mouse;
        b.mouse = {btn};
        return b;
    }
};

class InputMap {
private:
    struct Action {
        std::string name;
        std::vector<Binding> bindings;
    };

    std::unordered_map<ActionID, Action> actions_;
    std::unordered_map<std::string, ActionID> name_to_id_;
    ActionID next_id_ = 1;

    Input* input_;

public:
    InputMap(Input* input) : input_(input) {}

    // Define actions
    ActionID register_action(const std::string& name) {
        ActionID id = next_id_++;
        actions_[id] = {name, {}};
        name_to_id_[name] = id;
        return id;
    }

    void bind(ActionID action, Binding binding) {
        if (actions_.count(action)) {
            actions_[action].bindings.push_back(binding);
        }
    }

    void bind(const std::string& action_name, Binding binding) {
        auto it = name_to_id_.find(action_name);
        if (it != name_to_id_.end()) {
            bind(it->second, binding);
        }
    }

    void unbind_all(ActionID action) {
        if (actions_.count(action)) {
            actions_[action].bindings.clear();
        }
    }

    // Query action state
    bool is_action_down(ActionID action) const {
        auto it = actions_.find(action);
        if (it == actions_.end()) return false;

        for (const auto& binding : it->second.bindings) {
            if (is_binding_down(binding)) return true;
        }
        return false;
    }

    bool is_action_pressed(ActionID action) const {
        auto it = actions_.find(action);
        if (it == actions_.end()) return false;

        for (const auto& binding : it->second.bindings) {
            if (is_binding_pressed(binding)) return true;
        }
        return false;
    }

    bool is_action_released(ActionID action) const {
        auto it = actions_.find(action);
        if (it == actions_.end()) return false;

        for (const auto& binding : it->second.bindings) {
            if (is_binding_released(binding)) return true;
        }
        return false;
    }

    // By name (convenience)
    bool is_down(const std::string& name) const {
        auto it = name_to_id_.find(name);
        return it != name_to_id_.end() && is_action_down(it->second);
    }

    bool is_pressed(const std::string& name) const {
        auto it = name_to_id_.find(name);
        return it != name_to_id_.end() && is_action_pressed(it->second);
    }

    // Save/load bindings
    void save_bindings(const std::string& path);
    void load_bindings(const std::string& path);

private:
    bool is_binding_down(const Binding& b) const {
        switch (b.type) {
            case Binding::Keyboard:
                if (!input_->key_down(b.key.key)) return false;
                if (b.key.with_shift && !input_->key_down(Key::LeftShift) && !input_->key_down(Key::RightShift)) return false;
                if (b.key.with_ctrl && !input_->key_down(Key::LeftControl) && !input_->key_down(Key::RightControl)) return false;
                return true;
            case Binding::Mouse:
                return input_->mouse_down(b.mouse.button);
            default:
                return false;
        }
    }

    bool is_binding_pressed(const Binding& b) const {
        switch (b.type) {
            case Binding::Keyboard:
                return input_->key_pressed(b.key.key);
            case Binding::Mouse:
                return input_->mouse_pressed(b.mouse.button);
            default:
                return false;
        }
    }

    bool is_binding_released(const Binding& b) const {
        switch (b.type) {
            case Binding::Keyboard:
                return input_->key_released(b.key.key);
            case Binding::Mouse:
                return input_->mouse_released(b.mouse.button);
            default:
                return false;
        }
    }
};

} // namespace cafe
```

## Game Usage Example

```cpp
// game/input_setup.cpp

void CafeGame::setup_input() {
    // Register actions
    input_map_->register_action("pause");
    input_map_->register_action("select");
    input_map_->register_action("pan_camera");
    input_map_->register_action("zoom_in");
    input_map_->register_action("zoom_out");
    input_map_->register_action("speed_up");
    input_map_->register_action("speed_down");

    // Default bindings
    input_map_->bind("pause", Binding::from_key(Key::Escape));
    input_map_->bind("pause", Binding::from_key(Key::P));

    input_map_->bind("select", Binding::from_mouse(MouseButton::Left));

    input_map_->bind("pan_camera", Binding::from_mouse(MouseButton::Right));

    input_map_->bind("zoom_in", Binding::from_key(Key::Num1));
    input_map_->bind("zoom_out", Binding::from_key(Key::Num2));

    input_map_->bind("speed_up", Binding::from_key(Key::Num3));
    input_map_->bind("speed_down", Binding::from_key(Key::Num4));

    // Load player customizations (if any)
    input_map_->load_bindings("controls.json");
}

void CafeGame::handle_input() {
    if (input_map_->is_pressed("pause")) {
        toggle_pause();
    }

    if (input_map_->is_pressed("select")) {
        handle_click(input_->mouse_x(), input_->mouse_y());
    }

    if (input_map_->is_down("pan_camera")) {
        camera_.pan(input_->mouse_dx(), input_->mouse_dy());
    }

    // Scroll wheel zoom
    float scroll = input_->scroll_y();
    if (scroll > 0) camera_.zoom_in();
    if (scroll < 0) camera_.zoom_out();

    // Speed controls
    if (input_map_->is_pressed("speed_up")) {
        game_speed_ = std::min(game_speed_ + 1, 3);
    }
    if (input_map_->is_pressed("speed_down")) {
        game_speed_ = std::max(game_speed_ - 1, 0);
    }
}
```

## Touch Support

```cpp
// For mobile: convert touch to mouse-like actions

void Input::handle_touch_as_mouse() {
    if (touch_count_ > 0) {
        const Touch& primary = touches_[0];

        // Treat first touch like mouse
        if (primary.just_started) {
            handle_event({InputEvent::MouseDown, .mouse_button = 0, .x = primary.x, .y = primary.y});
        } else if (primary.just_ended) {
            handle_event({InputEvent::MouseUp, .mouse_button = 0, .x = primary.x, .y = primary.y});
        } else {
            handle_event({InputEvent::MouseMove, .x = primary.x, .y = primary.y});
        }
    }

    // Two-finger pinch for zoom
    if (touch_count_ >= 2) {
        float dist = distance(touches_[0], touches_[1]);
        float prev_dist = distance(prev_touches_[0], prev_touches_[1]);
        float zoom_delta = (dist - prev_dist) / 100.0f;
        handle_event({InputEvent::Scroll, .y = zoom_delta});
    }
}
```

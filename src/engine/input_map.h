#ifndef CAFE_INPUT_MAP_H
#define CAFE_INPUT_MAP_H

#include "../platform/platform.h"
#include "../renderer/renderer.h"  // For Vec2
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace cafe {

// ============================================================================
// Input Action - Named input that can be triggered by multiple keys/buttons
// ============================================================================

struct InputBinding {
    Key key = Key::Unknown;
    MouseButton mouse_button = MouseButton::Left;
    bool is_mouse = false;

    InputBinding() = default;
    explicit InputBinding(Key k) : key(k), is_mouse(false) {}
    explicit InputBinding(MouseButton mb) : mouse_button(mb), is_mouse(true) {}

    static InputBinding from_key(Key k) { return InputBinding(k); }
    static InputBinding from_mouse(MouseButton mb) { return InputBinding(mb); }
};

struct InputAction {
    std::string name;
    std::vector<InputBinding> bindings;

    InputAction() = default;
    explicit InputAction(const std::string& n) : name(n) {}

    // Add binding helpers
    InputAction& add_key(Key key) {
        bindings.push_back(InputBinding::from_key(key));
        return *this;
    }

    InputAction& add_mouse(MouseButton button) {
        bindings.push_back(InputBinding::from_mouse(button));
        return *this;
    }

    void clear_bindings() {
        bindings.clear();
    }
};

// ============================================================================
// Input Axis - Analog input from two keys or gamepad axis
// ============================================================================

struct InputAxis {
    std::string name;
    Key positive_key = Key::Unknown;
    Key negative_key = Key::Unknown;
    float dead_zone = 0.1f;
    float sensitivity = 1.0f;

    InputAxis() = default;
    explicit InputAxis(const std::string& n) : name(n) {}

    InputAxis& set_keys(Key negative, Key positive) {
        negative_key = negative;
        positive_key = positive;
        return *this;
    }
};

// ============================================================================
// Input Map - Manages actions and axes
// ============================================================================

class InputMap {
public:
    InputMap() = default;

    // Set the window to read input from
    void set_window(Window* window) { window_ = window; }

    // ========================================================================
    // Action Management
    // ========================================================================

    // Define a new action
    InputAction& define_action(const std::string& name);

    // Get action (returns nullptr if not found)
    InputAction* get_action(const std::string& name);
    const InputAction* get_action(const std::string& name) const;

    // Remove action
    void remove_action(const std::string& name);

    // ========================================================================
    // Axis Management
    // ========================================================================

    // Define a new axis
    InputAxis& define_axis(const std::string& name);

    // Get axis
    InputAxis* get_axis(const std::string& name);
    const InputAxis* get_axis(const std::string& name) const;

    // Remove axis
    void remove_axis(const std::string& name);

    // ========================================================================
    // Input Queries
    // ========================================================================

    // Check if action is currently held
    bool is_action_held(const std::string& name) const;

    // Check if action was just pressed this frame
    bool is_action_pressed(const std::string& name) const;

    // Check if action was just released this frame
    bool is_action_released(const std::string& name) const;

    // Get axis value (-1.0 to 1.0)
    float get_axis_value(const std::string& name) const;

    // ========================================================================
    // Convenience: Movement Vectors
    // ========================================================================

    // Get movement vector from "move_x" and "move_y" axes
    Vec2 get_movement() const;

    // Get movement from specific axes
    Vec2 get_movement(const std::string& x_axis, const std::string& y_axis) const;

    // ========================================================================
    // Common Presets
    // ========================================================================

    // Add standard WASD movement axes
    void add_wasd_movement();

    // Add arrow key movement axes
    void add_arrow_movement();

    // Add both WASD and arrows
    void add_standard_movement();

    // ========================================================================
    // Callbacks
    // ========================================================================

    // Register callback for when action is pressed
    using ActionCallback = std::function<void()>;
    void on_action_pressed(const std::string& name, ActionCallback callback);
    void on_action_released(const std::string& name, ActionCallback callback);

    // Update (call each frame to trigger callbacks)
    void update();

    // ========================================================================
    // Serialization (for saving/loading keybinds)
    // ========================================================================

    // TODO: Add save/load functionality for user preferences

private:
    Window* window_ = nullptr;

    std::unordered_map<std::string, InputAction> actions_;
    std::unordered_map<std::string, InputAxis> axes_;

    // Callbacks
    std::unordered_map<std::string, std::vector<ActionCallback>> pressed_callbacks_;
    std::unordered_map<std::string, std::vector<ActionCallback>> released_callbacks_;

    // Previous frame state for press/release detection
    std::unordered_map<std::string, bool> prev_action_state_;

    // Helper to check if a binding is active
    bool is_binding_active(const InputBinding& binding) const;
    bool was_binding_just_pressed(const InputBinding& binding) const;
    bool was_binding_just_released(const InputBinding& binding) const;
};

} // namespace cafe

#endif // CAFE_INPUT_MAP_H

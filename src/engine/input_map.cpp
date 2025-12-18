#include "input_map.h"
#include <cmath>

namespace cafe {

// ============================================================================
// Action Management
// ============================================================================

InputAction& InputMap::define_action(const std::string& name) {
    auto& action = actions_[name];
    action.name = name;
    return action;
}

InputAction* InputMap::get_action(const std::string& name) {
    auto it = actions_.find(name);
    return it != actions_.end() ? &it->second : nullptr;
}

const InputAction* InputMap::get_action(const std::string& name) const {
    auto it = actions_.find(name);
    return it != actions_.end() ? &it->second : nullptr;
}

void InputMap::remove_action(const std::string& name) {
    actions_.erase(name);
    pressed_callbacks_.erase(name);
    released_callbacks_.erase(name);
    prev_action_state_.erase(name);
}

// ============================================================================
// Axis Management
// ============================================================================

InputAxis& InputMap::define_axis(const std::string& name) {
    auto& axis = axes_[name];
    axis.name = name;
    return axis;
}

InputAxis* InputMap::get_axis(const std::string& name) {
    auto it = axes_.find(name);
    return it != axes_.end() ? &it->second : nullptr;
}

const InputAxis* InputMap::get_axis(const std::string& name) const {
    auto it = axes_.find(name);
    return it != axes_.end() ? &it->second : nullptr;
}

void InputMap::remove_axis(const std::string& name) {
    axes_.erase(name);
}

// ============================================================================
// Input Queries
// ============================================================================

bool InputMap::is_binding_active(const InputBinding& binding) const {
    if (!window_) return false;

    if (binding.is_mouse) {
        return window_->is_mouse_button_down(binding.mouse_button);
    } else {
        return window_->is_key_down(binding.key);
    }
}

bool InputMap::was_binding_just_pressed(const InputBinding& binding) const {
    if (!window_) return false;

    if (binding.is_mouse) {
        // Note: Window doesn't have is_mouse_button_pressed yet
        // For now, just use is_mouse_button_down
        return window_->is_mouse_button_down(binding.mouse_button);
    } else {
        return window_->is_key_pressed(binding.key);
    }
}

bool InputMap::was_binding_just_released(const InputBinding& binding) const {
    if (!window_) return false;

    if (binding.is_mouse) {
        return false;  // Not implemented
    } else {
        return window_->is_key_released(binding.key);
    }
}

bool InputMap::is_action_held(const std::string& name) const {
    const InputAction* action = get_action(name);
    if (!action) return false;

    for (const auto& binding : action->bindings) {
        if (is_binding_active(binding)) {
            return true;
        }
    }
    return false;
}

bool InputMap::is_action_pressed(const std::string& name) const {
    const InputAction* action = get_action(name);
    if (!action) return false;

    for (const auto& binding : action->bindings) {
        if (was_binding_just_pressed(binding)) {
            return true;
        }
    }
    return false;
}

bool InputMap::is_action_released(const std::string& name) const {
    const InputAction* action = get_action(name);
    if (!action) return false;

    for (const auto& binding : action->bindings) {
        if (was_binding_just_released(binding)) {
            return true;
        }
    }
    return false;
}

float InputMap::get_axis_value(const std::string& name) const {
    const InputAxis* axis = get_axis(name);
    if (!axis || !window_) return 0.0f;

    float value = 0.0f;

    if (axis->positive_key != Key::Unknown && window_->is_key_down(axis->positive_key)) {
        value += 1.0f;
    }
    if (axis->negative_key != Key::Unknown && window_->is_key_down(axis->negative_key)) {
        value -= 1.0f;
    }

    // Apply dead zone
    if (std::abs(value) < axis->dead_zone) {
        value = 0.0f;
    }

    // Apply sensitivity
    value *= axis->sensitivity;

    // Clamp to -1, 1
    return std::max(-1.0f, std::min(1.0f, value));
}

// ============================================================================
// Movement Vectors
// ============================================================================

Vec2 InputMap::get_movement() const {
    return get_movement("move_x", "move_y");
}

Vec2 InputMap::get_movement(const std::string& x_axis, const std::string& y_axis) const {
    Vec2 movement;
    movement.x = get_axis_value(x_axis);
    movement.y = get_axis_value(y_axis);

    // Normalize if diagonal movement
    float length_sq = movement.x * movement.x + movement.y * movement.y;
    if (length_sq > 1.0f) {
        float length = std::sqrt(length_sq);
        movement.x /= length;
        movement.y /= length;
    }

    return movement;
}

// ============================================================================
// Common Presets
// ============================================================================

void InputMap::add_wasd_movement() {
    define_axis("move_x")
        .set_keys(Key::A, Key::D);

    define_axis("move_y")
        .set_keys(Key::W, Key::S);
}

void InputMap::add_arrow_movement() {
    // Note: If axes already exist, this will overwrite them
    // A better implementation would combine multiple key pairs per axis

    auto* x_axis = get_axis("move_x");
    if (!x_axis) {
        define_axis("move_x")
            .set_keys(Key::Left, Key::Right);
    }

    auto* y_axis = get_axis("move_y");
    if (!y_axis) {
        define_axis("move_y")
            .set_keys(Key::Up, Key::Down);
    }
}

void InputMap::add_standard_movement() {
    // Create axes that respond to both WASD and arrows
    // For simplicity, we'll use WASD as primary
    // A more complete implementation would support multiple key pairs

    define_axis("move_x")
        .set_keys(Key::A, Key::D);

    define_axis("move_y")
        .set_keys(Key::W, Key::S);

    // Also define common actions
    define_action("confirm")
        .add_key(Key::Enter)
        .add_key(Key::Space);

    define_action("cancel")
        .add_key(Key::Escape);
}

// ============================================================================
// Callbacks
// ============================================================================

void InputMap::on_action_pressed(const std::string& name, ActionCallback callback) {
    pressed_callbacks_[name].push_back(std::move(callback));
}

void InputMap::on_action_released(const std::string& name, ActionCallback callback) {
    released_callbacks_[name].push_back(std::move(callback));
}

void InputMap::update() {
    // Check for action state changes and fire callbacks
    for (const auto& [name, action] : actions_) {
        bool current_state = is_action_held(name);
        bool prev_state = prev_action_state_[name];

        // Just pressed
        if (current_state && !prev_state) {
            auto it = pressed_callbacks_.find(name);
            if (it != pressed_callbacks_.end()) {
                for (const auto& callback : it->second) {
                    callback();
                }
            }
        }

        // Just released
        if (!current_state && prev_state) {
            auto it = released_callbacks_.find(name);
            if (it != released_callbacks_.end()) {
                for (const auto& callback : it->second) {
                    callback();
                }
            }
        }

        prev_action_state_[name] = current_state;
    }
}

} // namespace cafe

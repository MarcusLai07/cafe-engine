#ifdef __EMSCRIPTEN__

#include "../platform.h"
#include <emscripten.h>
#include <emscripten/html5.h>
#include <cstring>
#include <chrono>

namespace cafe {

// ============================================================================
// Key Code Translation (Emscripten DOM key codes -> cafe::Key)
// ============================================================================

static Key translate_key_code(const char* code) {
    // DOM KeyboardEvent.code values
    if (strcmp(code, "KeyA") == 0) return Key::A;
    if (strcmp(code, "KeyB") == 0) return Key::B;
    if (strcmp(code, "KeyC") == 0) return Key::C;
    if (strcmp(code, "KeyD") == 0) return Key::D;
    if (strcmp(code, "KeyE") == 0) return Key::E;
    if (strcmp(code, "KeyF") == 0) return Key::F;
    if (strcmp(code, "KeyG") == 0) return Key::G;
    if (strcmp(code, "KeyH") == 0) return Key::H;
    if (strcmp(code, "KeyI") == 0) return Key::I;
    if (strcmp(code, "KeyJ") == 0) return Key::J;
    if (strcmp(code, "KeyK") == 0) return Key::K;
    if (strcmp(code, "KeyL") == 0) return Key::L;
    if (strcmp(code, "KeyM") == 0) return Key::M;
    if (strcmp(code, "KeyN") == 0) return Key::N;
    if (strcmp(code, "KeyO") == 0) return Key::O;
    if (strcmp(code, "KeyP") == 0) return Key::P;
    if (strcmp(code, "KeyQ") == 0) return Key::Q;
    if (strcmp(code, "KeyR") == 0) return Key::R;
    if (strcmp(code, "KeyS") == 0) return Key::S;
    if (strcmp(code, "KeyT") == 0) return Key::T;
    if (strcmp(code, "KeyU") == 0) return Key::U;
    if (strcmp(code, "KeyV") == 0) return Key::V;
    if (strcmp(code, "KeyW") == 0) return Key::W;
    if (strcmp(code, "KeyX") == 0) return Key::X;
    if (strcmp(code, "KeyY") == 0) return Key::Y;
    if (strcmp(code, "KeyZ") == 0) return Key::Z;

    if (strcmp(code, "Digit0") == 0) return Key::Num0;
    if (strcmp(code, "Digit1") == 0) return Key::Num1;
    if (strcmp(code, "Digit2") == 0) return Key::Num2;
    if (strcmp(code, "Digit3") == 0) return Key::Num3;
    if (strcmp(code, "Digit4") == 0) return Key::Num4;
    if (strcmp(code, "Digit5") == 0) return Key::Num5;
    if (strcmp(code, "Digit6") == 0) return Key::Num6;
    if (strcmp(code, "Digit7") == 0) return Key::Num7;
    if (strcmp(code, "Digit8") == 0) return Key::Num8;
    if (strcmp(code, "Digit9") == 0) return Key::Num9;

    if (strcmp(code, "ArrowLeft") == 0) return Key::Left;
    if (strcmp(code, "ArrowRight") == 0) return Key::Right;
    if (strcmp(code, "ArrowUp") == 0) return Key::Up;
    if (strcmp(code, "ArrowDown") == 0) return Key::Down;

    if (strcmp(code, "Space") == 0) return Key::Space;
    if (strcmp(code, "Enter") == 0) return Key::Enter;
    if (strcmp(code, "Tab") == 0) return Key::Tab;
    if (strcmp(code, "Backspace") == 0) return Key::Backspace;
    if (strcmp(code, "Escape") == 0) return Key::Escape;

    if (strcmp(code, "ShiftLeft") == 0) return Key::LeftShift;
    if (strcmp(code, "ShiftRight") == 0) return Key::RightShift;
    if (strcmp(code, "ControlLeft") == 0) return Key::LeftControl;
    if (strcmp(code, "ControlRight") == 0) return Key::RightControl;
    if (strcmp(code, "AltLeft") == 0) return Key::LeftAlt;
    if (strcmp(code, "AltRight") == 0) return Key::RightAlt;

    return Key::Unknown;
}

// ============================================================================
// Web Window Implementation
// ============================================================================

class WebWindow : public Window {
private:
    int width_ = 0;
    int height_ = 0;
    bool is_open_ = true;

    // Input state
    static constexpr int kKeyCount = static_cast<int>(Key::KeyCount);
    static constexpr int kMouseButtonCount = static_cast<int>(MouseButton::ButtonCount);

    bool keys_down_[kKeyCount] = {};
    bool keys_down_prev_[kKeyCount] = {};
    bool mouse_buttons_[kMouseButtonCount] = {};
    float mouse_x_ = 0.0f;
    float mouse_y_ = 0.0f;

public:
    WebWindow(const WindowConfig& config) {
        width_ = config.width;
        height_ = config.height;

        // Set canvas size
        emscripten_set_canvas_element_size("#canvas", width_, height_);

        // Register input callbacks
        emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, this, true, key_callback);
        emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, this, true, key_callback);
        emscripten_set_mousedown_callback("#canvas", this, true, mouse_callback);
        emscripten_set_mouseup_callback("#canvas", this, true, mouse_callback);
        emscripten_set_mousemove_callback("#canvas", this, true, mouse_move_callback);
    }

    ~WebWindow() override = default;

    bool is_open() const override { return is_open_; }
    void close() override { is_open_ = false; }

    int width() const override { return width_; }
    int height() const override { return height_; }
    float scale_factor() const override {
        return static_cast<float>(emscripten_get_device_pixel_ratio());
    }

    void set_title(const std::string& title) override {
        emscripten_set_window_title(title.c_str());
    }

    void* native_handle() override {
        return nullptr;  // WebGL uses canvas directly
    }

    // Input - Keyboard
    bool is_key_down(Key key) const override {
        int index = static_cast<int>(key);
        if (index < 0 || index >= kKeyCount) return false;
        return keys_down_[index];
    }

    bool is_key_pressed(Key key) const override {
        int index = static_cast<int>(key);
        if (index < 0 || index >= kKeyCount) return false;
        return keys_down_[index] && !keys_down_prev_[index];
    }

    bool is_key_released(Key key) const override {
        int index = static_cast<int>(key);
        if (index < 0 || index >= kKeyCount) return false;
        return !keys_down_[index] && keys_down_prev_[index];
    }

    // Input - Mouse
    bool is_mouse_button_down(MouseButton button) const override {
        int index = static_cast<int>(button);
        if (index < 0 || index >= kMouseButtonCount) return false;
        return mouse_buttons_[index];
    }

    float mouse_x() const override { return mouse_x_; }
    float mouse_y() const override { return mouse_y_; }

    void update_input() override {
        std::memcpy(keys_down_prev_, keys_down_, sizeof(keys_down_));
    }

    // Static callbacks for Emscripten
    static EM_BOOL key_callback(int event_type, const EmscriptenKeyboardEvent* e, void* user_data) {
        WebWindow* win = static_cast<WebWindow*>(user_data);
        Key key = translate_key_code(e->code);
        int index = static_cast<int>(key);

        if (index > 0 && index < kKeyCount) {
            win->keys_down_[index] = (event_type == EMSCRIPTEN_EVENT_KEYDOWN);
        }

        return EM_TRUE;  // Consume event
    }

    static EM_BOOL mouse_callback(int event_type, const EmscriptenMouseEvent* e, void* user_data) {
        WebWindow* win = static_cast<WebWindow*>(user_data);
        int button = e->button;

        if (button >= 0 && button < kMouseButtonCount) {
            win->mouse_buttons_[button] = (event_type == EMSCRIPTEN_EVENT_MOUSEDOWN);
        }

        return EM_TRUE;
    }

    static EM_BOOL mouse_move_callback(int, const EmscriptenMouseEvent* e, void* user_data) {
        WebWindow* win = static_cast<WebWindow*>(user_data);
        win->mouse_x_ = static_cast<float>(e->targetX);
        win->mouse_y_ = static_cast<float>(e->targetY);
        return EM_TRUE;
    }
};

// ============================================================================
// Web Platform Implementation
// ============================================================================

class WebPlatform : public Platform {
private:
    std::chrono::high_resolution_clock::time_point start_time_;

public:
    WebPlatform() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }

    ~WebPlatform() override = default;

    std::unique_ptr<Window> create_window(const WindowConfig& config) override {
        return std::make_unique<WebWindow>(config);
    }

    void poll_events() override {
        // Emscripten handles events through callbacks
        // No explicit polling needed
    }

    double get_time() const override {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(now - start_time_);
        return duration.count();
    }

    const char* name() const override {
        return "Web (Emscripten)";
    }
};

// ============================================================================
// Factory Function
// ============================================================================

std::unique_ptr<Platform> create_platform() {
    return std::make_unique<WebPlatform>();
}

} // namespace cafe

#endif // __EMSCRIPTEN__

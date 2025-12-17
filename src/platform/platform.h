#ifndef CAFE_PLATFORM_H
#define CAFE_PLATFORM_H

#include <string>
#include <memory>
#include <functional>

namespace cafe {

// Forward declarations
class Window;

// ============================================================================
// Input Types
// ============================================================================

// Common key codes (subset of keys needed for games)
enum class Key {
    Unknown = 0,

    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // Numbers
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

    // Function keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    // Arrow keys
    Left, Right, Up, Down,

    // Modifiers
    LeftShift, RightShift, LeftControl, RightControl,
    LeftAlt, RightAlt, LeftSuper, RightSuper,

    // Special keys
    Space, Enter, Tab, Backspace, Escape,
    Insert, Delete, Home, End, PageUp, PageDown,

    // Punctuation (common ones)
    Comma, Period, Slash, Semicolon, Quote,
    LeftBracket, RightBracket, Backslash, Grave, Minus, Equal,

    KeyCount  // Total number of keys
};

// Mouse buttons
enum class MouseButton {
    Left = 0,
    Right = 1,
    Middle = 2,
    ButtonCount = 3
};

// Platform-agnostic window configuration
struct WindowConfig {
    std::string title = "Cafe Engine";
    int width = 1280;
    int height = 720;
    bool resizable = true;
    bool fullscreen = false;
};

// Abstract window interface
// Each platform implements this (macOS, Web, iOS, etc.)
class Window {
public:
    virtual ~Window() = default;

    // Lifecycle
    virtual bool is_open() const = 0;
    virtual void close() = 0;

    // Properties
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual float scale_factor() const = 0;  // For Retina displays
    virtual void set_title(const std::string& title) = 0;

    // Native handle (for renderer)
    virtual void* native_handle() = 0;  // NSView* on macOS, HWND on Windows, etc.

    // Input - Keyboard
    virtual bool is_key_down(Key key) const = 0;      // Is key currently held
    virtual bool is_key_pressed(Key key) const = 0;   // Was key just pressed this frame
    virtual bool is_key_released(Key key) const = 0;  // Was key just released this frame

    // Input - Mouse
    virtual bool is_mouse_button_down(MouseButton button) const = 0;
    virtual float mouse_x() const = 0;  // Mouse X in window coordinates
    virtual float mouse_y() const = 0;  // Mouse Y in window coordinates

    // Called by Platform after polling events to update input state
    virtual void update_input() = 0;
};

// Abstract platform interface
class Platform {
public:
    virtual ~Platform() = default;

    // Create a window
    virtual std::unique_ptr<Window> create_window(const WindowConfig& config) = 0;

    // Event loop
    virtual void poll_events() = 0;

    // Time
    virtual double get_time() const = 0;  // Seconds since start

    // Platform info
    virtual const char* name() const = 0;
};

// Factory function - implemented per platform
std::unique_ptr<Platform> create_platform();

} // namespace cafe

#endif // CAFE_PLATFORM_H

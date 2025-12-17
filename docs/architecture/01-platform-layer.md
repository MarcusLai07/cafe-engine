# Platform Layer Architecture

The platform layer abstracts operating system and hardware differences, providing a unified interface for the rest of the engine.

## Responsibilities

1. **Window management** - Create, resize, close application windows
2. **Graphics context** - Initialize and manage GPU API (Metal, WebGL, etc.)
3. **Input capture** - Keyboard, mouse, touch events
4. **Audio output** - Platform audio API access
5. **File I/O** - Read/write files, locate asset directories
6. **Time** - High-precision timing for game loop

## Abstract Interface

```cpp
// platform/platform.h

namespace cafe {

struct PlatformConfig {
    const char* window_title;
    int window_width;
    int window_height;
    bool fullscreen;
    bool resizable;
};

struct InputEvent {
    enum Type { KeyDown, KeyUp, MouseMove, MouseDown, MouseUp, TouchBegin, TouchMove, TouchEnd };
    Type type;
    int key_code;      // For keyboard events
    int mouse_button;  // For mouse events
    float x, y;        // Position
    int touch_id;      // For multi-touch
};

class Platform {
public:
    virtual ~Platform() = default;

    // Lifecycle
    virtual bool initialize(const PlatformConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual bool should_quit() = 0;

    // Window
    virtual void get_window_size(int* width, int* height) = 0;
    virtual float get_display_scale() = 0;  // For Retina/HiDPI

    // Input
    virtual void poll_events() = 0;
    virtual bool pop_event(InputEvent* event) = 0;

    // Time
    virtual double get_time() = 0;  // Seconds since start

    // File I/O
    virtual bool read_file(const char* path, std::vector<uint8_t>* data) = 0;
    virtual bool write_file(const char* path, const void* data, size_t size) = 0;
    virtual const char* get_assets_path() = 0;
    virtual const char* get_save_path() = 0;

    // Graphics context (implementation-specific)
    virtual void* get_native_handle() = 0;  // MTLDevice*, WebGL context, etc.
};

// Factory function - implemented per platform
std::unique_ptr<Platform> create_platform();

} // namespace cafe
```

## Platform Implementations

### macOS (Cocoa + Metal)

```cpp
// platform/macos/macos_platform.mm

@interface GameWindow : NSWindow
@end

@interface GameView : NSView <NSWindowDelegate>
@property (nonatomic) id<MTLDevice> metalDevice;
@property (nonatomic) CAMetalLayer* metalLayer;
@end

class MacOSPlatform : public Platform {
private:
    NSApplication* app_;
    GameWindow* window_;
    GameView* view_;
    id<MTLDevice> device_;
    std::queue<InputEvent> events_;

public:
    bool initialize(const PlatformConfig& config) override {
        // 1. Initialize NSApplication
        // 2. Create NSWindow with config dimensions
        // 3. Create GameView with CAMetalLayer
        // 4. Get default MTLDevice
        // 5. Set up input event handlers
    }

    void poll_events() override {
        NSEvent* event;
        while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                          untilDate:nil
                                             inMode:NSDefaultRunLoopMode
                                            dequeue:YES])) {
            // Convert NSEvent to InputEvent
            // Push to events_ queue
            [NSApp sendEvent:event];
        }
    }

    void* get_native_handle() override {
        return (__bridge void*)device_;
    }
};
```

### Web (Emscripten + WebGL)

```cpp
// platform/web/web_platform.cpp

class WebPlatform : public Platform {
private:
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE gl_context_;
    std::queue<InputEvent> events_;

public:
    bool initialize(const PlatformConfig& config) override {
        // 1. Set canvas size via emscripten_set_canvas_size
        // 2. Create WebGL context
        // 3. Register input callbacks
        emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true, key_callback);
        emscripten_set_mousedown_callback("canvas", this, true, mouse_callback);
    }

    static EM_BOOL key_callback(int type, const EmscriptenKeyboardEvent* e, void* data) {
        auto* self = static_cast<WebPlatform*>(data);
        InputEvent event;
        event.type = (type == EMSCRIPTEN_EVENT_KEYDOWN) ? InputEvent::KeyDown : InputEvent::KeyUp;
        event.key_code = e->keyCode;
        self->events_.push(event);
        return EM_TRUE;
    }

    double get_time() override {
        return emscripten_get_now() / 1000.0;
    }

    const char* get_assets_path() override {
        return "/assets/";  // Virtual filesystem path
    }
};
```

### iOS (UIKit + Metal)

```cpp
// platform/ios/ios_platform.mm

// Similar to macOS but using UIWindow, UIView, MTKView
// Touch events instead of mouse
// Different file paths (app bundle, documents directory)
```

### Windows (Win32 + DirectX 11)

```cpp
// platform/windows/windows_platform.cpp

class WindowsPlatform : public Platform {
private:
    HWND hwnd_;
    ID3D11Device* device_;
    ID3D11DeviceContext* context_;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        // Handle WM_KEYDOWN, WM_MOUSEMOVE, etc.
        // Convert to InputEvent
    }
};
```

### Android (NDK + OpenGL ES)

```cpp
// platform/android/android_platform.cpp

// Native activity via android_native_app_glue
// EGL context for OpenGL ES
// JNI calls for file access, etc.
```

## File Path Conventions

Each platform has different conventions for asset and save locations:

| Platform | Assets Path | Save Path |
|----------|-------------|-----------|
| macOS | `Bundle/Contents/Resources/` | `~/Library/Application Support/CafeEngine/` |
| iOS | `Bundle/` | `Documents/` |
| Windows | `./assets/` | `%APPDATA%/CafeEngine/` |
| Android | `assets/` (APK) | Internal storage |
| Web | `/assets/` (virtual FS) | LocalStorage or IndexedDB |

## Input Key Codes

Define a platform-independent key code enum:

```cpp
enum class KeyCode {
    Unknown = 0,

    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // Numbers
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

    // Function keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    // Navigation
    Up, Down, Left, Right, Home, End, PageUp, PageDown,

    // Modifiers
    Shift, Control, Alt, Command,

    // Other
    Space, Enter, Escape, Tab, Backspace, Delete,

    COUNT
};

// Each platform implements:
KeyCode translate_native_keycode(int native_code);
```

## Implementation Order

1. **Phase 2**: macOS platform (Cocoa window, Metal context, input)
2. **Phase 3**: Web platform (Emscripten, WebGL, browser events)
3. **Phase 7**: iOS, Windows, Android platforms

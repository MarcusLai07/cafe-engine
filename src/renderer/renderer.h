#ifndef CAFE_RENDERER_H
#define CAFE_RENDERER_H

#include <memory>
#include <string>

namespace cafe {

// Forward declarations
class Window;

// RGBA color
struct Color {
    float r, g, b, a;

    static Color black() { return {0.0f, 0.0f, 0.0f, 1.0f}; }
    static Color white() { return {1.0f, 1.0f, 1.0f, 1.0f}; }
    static Color red() { return {1.0f, 0.0f, 0.0f, 1.0f}; }
    static Color green() { return {0.0f, 1.0f, 0.0f, 1.0f}; }
    static Color blue() { return {0.0f, 0.0f, 1.0f, 1.0f}; }
    static Color cornflower_blue() { return {0.39f, 0.58f, 0.93f, 1.0f}; }
};

// 2D vector
struct Vec2 {
    float x, y;
};

// Abstract renderer interface
// Each backend implements this (Metal, WebGL, etc.)
class Renderer {
public:
    virtual ~Renderer() = default;

    // Lifecycle
    virtual bool initialize(Window* window) = 0;
    virtual void shutdown() = 0;

    // Frame
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;

    // Clear
    virtual void set_clear_color(const Color& color) = 0;
    virtual void clear() = 0;

    // Drawing
    virtual void draw_quad(Vec2 position, Vec2 size, const Color& color) = 0;

    // Info
    virtual const char* backend_name() const = 0;
};

// Factory function - implemented per platform
std::unique_ptr<Renderer> create_renderer();

} // namespace cafe

#endif // CAFE_RENDERER_H

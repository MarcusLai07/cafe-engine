#ifndef CAFE_RENDERER_H
#define CAFE_RENDERER_H

#include <memory>
#include <string>
#include <cstdint>

namespace cafe {

// Forward declarations
class Window;

// ============================================================================
// Basic Types
// ============================================================================

// RGBA color (0.0 to 1.0)
struct Color {
    float r, g, b, a;

    static Color black() { return {0.0f, 0.0f, 0.0f, 1.0f}; }
    static Color white() { return {1.0f, 1.0f, 1.0f, 1.0f}; }
    static Color red() { return {1.0f, 0.0f, 0.0f, 1.0f}; }
    static Color green() { return {0.0f, 1.0f, 0.0f, 1.0f}; }
    static Color blue() { return {0.0f, 0.0f, 1.0f, 1.0f}; }
    static Color yellow() { return {1.0f, 1.0f, 0.0f, 1.0f}; }
    static Color cyan() { return {0.0f, 1.0f, 1.0f, 1.0f}; }
    static Color magenta() { return {1.0f, 0.0f, 1.0f, 1.0f}; }
    static Color cornflower_blue() { return {0.39f, 0.58f, 0.93f, 1.0f}; }
};

// 2D vector
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
};

// Rectangle (position + size)
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    Rect() = default;
    Rect(float x_, float y_, float w_, float h_) : x(x_), y(y_), width(w_), height(h_) {}
};

// ============================================================================
// Texture
// ============================================================================

// Opaque texture handle (actual data is backend-specific)
using TextureHandle = uint32_t;
constexpr TextureHandle INVALID_TEXTURE = 0;

// Texture filtering mode
enum class TextureFilter {
    Nearest,  // Pixelated (good for pixel art)
    Linear    // Smooth (good for photos)
};

// Texture wrap mode
enum class TextureWrap {
    Clamp,    // Clamp to edge
    Repeat    // Tile the texture
};

// Texture creation info
struct TextureInfo {
    int width = 0;
    int height = 0;
    TextureFilter filter = TextureFilter::Nearest;
    TextureWrap wrap = TextureWrap::Clamp;
};

// Region within a texture (for sprite sheets)
// UV coordinates: (0,0) = top-left, (1,1) = bottom-right
struct TextureRegion {
    TextureHandle texture = INVALID_TEXTURE;
    float u0 = 0.0f, v0 = 0.0f;  // Top-left UV
    float u1 = 1.0f, v1 = 1.0f;  // Bottom-right UV

    TextureRegion() = default;
    TextureRegion(TextureHandle tex) : texture(tex) {}
    TextureRegion(TextureHandle tex, float u0_, float v0_, float u1_, float v1_)
        : texture(tex), u0(u0_), v0(v0_), u1(u1_), v1(v1_) {}

    // Create region from pixel coordinates
    static TextureRegion from_pixels(TextureHandle tex, int tex_width, int tex_height,
                                      int x, int y, int w, int h) {
        float tw = static_cast<float>(tex_width);
        float th = static_cast<float>(tex_height);
        return TextureRegion(tex,
            x / tw, y / th,
            (x + w) / tw, (y + h) / th);
    }
};

// ============================================================================
// Sprite (for batch rendering)
// ============================================================================

struct Sprite {
    Vec2 position;           // Center position in world coordinates
    Vec2 size;               // Width and height
    TextureRegion region;    // Texture and UV coordinates
    Color tint = Color::white();  // Color tint/modulation
    float rotation = 0.0f;   // Rotation in radians
    Vec2 origin = {0.5f, 0.5f};  // Origin point (0-1, relative to size)
};

// ============================================================================
// Abstract Renderer Interface
// ============================================================================

// Each backend implements this (Metal, WebGL, etc.)
class Renderer {
public:
    virtual ~Renderer() = default;

    // Lifecycle
    virtual bool initialize(Window* window) = 0;
    virtual void shutdown() = 0;

    // Frame management
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;

    // Clear
    virtual void set_clear_color(const Color& color) = 0;
    virtual void clear() = 0;

    // Viewport and projection
    virtual void set_viewport(int x, int y, int width, int height) = 0;
    virtual void set_projection(float left, float right, float bottom, float top) = 0;

    // Texture management
    virtual TextureHandle create_texture(const uint8_t* pixels, const TextureInfo& info) = 0;
    virtual void destroy_texture(TextureHandle texture) = 0;
    virtual TextureInfo get_texture_info(TextureHandle texture) const = 0;

    // Immediate mode drawing (simple, not batched)
    virtual void draw_quad(Vec2 position, Vec2 size, const Color& color) = 0;
    virtual void draw_textured_quad(Vec2 position, Vec2 size,
                                     const TextureRegion& region,
                                     const Color& tint = Color::white()) = 0;

    // Batch rendering (efficient for many sprites)
    virtual void begin_batch() = 0;
    virtual void draw_sprite(const Sprite& sprite) = 0;
    virtual void end_batch() = 0;

    // Info
    virtual const char* backend_name() const = 0;
    virtual int max_texture_size() const = 0;
};

// Factory function - implemented per platform
std::unique_ptr<Renderer> create_renderer();

} // namespace cafe

#endif // CAFE_RENDERER_H

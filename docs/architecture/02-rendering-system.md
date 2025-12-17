# Rendering System Architecture

The rendering system provides a unified interface for drawing 2D graphics across all platforms and graphics APIs.

## Design Goals

1. **API agnostic** - Same interface works for Metal, WebGL, DirectX, OpenGL
2. **Batch optimized** - Minimize draw calls by batching similar sprites
3. **Pixel perfect** - Support for crisp pixel art at any resolution
4. **Isometric ready** - Proper depth sorting for isometric rendering

## Renderer Interface

```cpp
// renderer/renderer.h

namespace cafe {

struct Color {
    float r, g, b, a;
    static Color white() { return {1, 1, 1, 1}; }
    static Color black() { return {0, 0, 0, 1}; }
};

struct Rect {
    float x, y, width, height;
};

struct Vertex {
    float x, y;        // Position
    float u, v;        // Texture coordinates
    Color color;       // Vertex color (for tinting)
};

using TextureHandle = uint32_t;
using ShaderHandle = uint32_t;

constexpr TextureHandle INVALID_TEXTURE = 0;
constexpr ShaderHandle INVALID_SHADER = 0;

struct RendererConfig {
    int render_width;   // Internal render resolution
    int render_height;
    int window_width;   // Actual window size
    int window_height;
    bool pixel_perfect; // Snap to pixel boundaries
};

class Renderer {
public:
    virtual ~Renderer() = default;

    // Lifecycle
    virtual bool initialize(void* platform_handle, const RendererConfig& config) = 0;
    virtual void shutdown() = 0;

    // Frame
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
    virtual void present() = 0;

    // Textures
    virtual TextureHandle create_texture(int width, int height, const void* pixels) = 0;
    virtual void destroy_texture(TextureHandle handle) = 0;
    virtual void get_texture_size(TextureHandle handle, int* width, int* height) = 0;

    // Drawing (batched internally)
    virtual void draw_sprite(TextureHandle texture, const Rect& src, const Rect& dst, Color tint = Color::white()) = 0;
    virtual void draw_rect(const Rect& rect, Color color) = 0;
    virtual void draw_line(float x1, float y1, float x2, float y2, Color color) = 0;

    // State
    virtual void set_camera(float x, float y, float zoom) = 0;
    virtual void set_blend_mode(BlendMode mode) = 0;

    // Batch control
    virtual void flush() = 0;  // Force draw current batch
};

enum class BlendMode {
    Alpha,      // Standard alpha blending
    Additive,   // Add colors (for glow effects)
    Multiply,   // Multiply colors (for shadows)
    None        // No blending (opaque)
};

// Factory - creates appropriate backend
std::unique_ptr<Renderer> create_renderer(GraphicsAPI api);

enum class GraphicsAPI {
    Metal,
    WebGL,
    DirectX11,
    OpenGL
};

} // namespace cafe
```

## Sprite Batching

To minimize draw calls, sprites with the same texture are batched together:

```cpp
// renderer/sprite_batch.h

class SpriteBatch {
private:
    static constexpr size_t MAX_SPRITES = 10000;
    static constexpr size_t VERTICES_PER_SPRITE = 4;
    static constexpr size_t INDICES_PER_SPRITE = 6;

    struct BatchEntry {
        TextureHandle texture;
        uint32_t start_index;
        uint32_t sprite_count;
    };

    std::vector<Vertex> vertices_;
    std::vector<uint16_t> indices_;
    std::vector<BatchEntry> batches_;
    TextureHandle current_texture_ = INVALID_TEXTURE;

public:
    void begin() {
        vertices_.clear();
        indices_.clear();
        batches_.clear();
        current_texture_ = INVALID_TEXTURE;
    }

    void add_sprite(TextureHandle texture, const Rect& src, const Rect& dst, Color tint) {
        // Start new batch if texture changed
        if (texture != current_texture_) {
            if (current_texture_ != INVALID_TEXTURE) {
                // Close current batch
            }
            batches_.push_back({texture, (uint32_t)vertices_.size(), 0});
            current_texture_ = texture;
        }

        // Add 4 vertices (quad corners)
        // Add 6 indices (two triangles)
        // Increment batch sprite count
    }

    void end() {
        // Finalize last batch
    }

    const std::vector<BatchEntry>& get_batches() const { return batches_; }
    const Vertex* get_vertices() const { return vertices_.data(); }
    const uint16_t* get_indices() const { return indices_.data(); }
};
```

## Metal Backend

```cpp
// renderer/metal/metal_renderer.mm

class MetalRenderer : public Renderer {
private:
    id<MTLDevice> device_;
    id<MTLCommandQueue> command_queue_;
    id<MTLRenderPipelineState> pipeline_;
    id<MTLBuffer> vertex_buffer_;
    id<MTLBuffer> index_buffer_;
    CAMetalLayer* layer_;

    SpriteBatch batch_;
    std::unordered_map<TextureHandle, id<MTLTexture>> textures_;
    uint32_t next_texture_id_ = 1;

public:
    bool initialize(void* platform_handle, const RendererConfig& config) override {
        device_ = (__bridge id<MTLDevice>)platform_handle;
        command_queue_ = [device_ newCommandQueue];

        // Load shaders from metallib
        // Create render pipeline state
        // Create vertex/index buffers
    }

    void begin_frame() override {
        batch_.begin();
    }

    void draw_sprite(TextureHandle texture, const Rect& src, const Rect& dst, Color tint) override {
        batch_.add_sprite(texture, src, dst, tint);
    }

    void end_frame() override {
        batch_.end();
        flush();
    }

    void flush() override {
        id<MTLCommandBuffer> command_buffer = [command_queue_ commandBuffer];
        id<CAMetalDrawable> drawable = [layer_ nextDrawable];

        // Update vertex/index buffers with batch data
        // For each batch:
        //   Bind texture
        //   Draw indexed primitives

        [command_buffer presentDrawable:drawable];
        [command_buffer commit];
    }
};
```

## WebGL Backend

```cpp
// renderer/webgl/webgl_renderer.cpp

class WebGLRenderer : public Renderer {
private:
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context_;
    GLuint shader_program_;
    GLuint vao_, vbo_, ebo_;

    SpriteBatch batch_;
    std::unordered_map<TextureHandle, GLuint> textures_;

public:
    bool initialize(void* platform_handle, const RendererConfig& config) override {
        // WebGL context already created by platform layer
        context_ = (EMSCRIPTEN_WEBGL_CONTEXT_HANDLE)platform_handle;
        emscripten_webgl_make_context_current(context_);

        // Compile vertex/fragment shaders
        // Create VAO, VBO, EBO
    }

    void flush() override {
        glBindVertexArray(vao_);

        // Upload vertex data
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, ...);

        // For each batch:
        for (const auto& batch : batch_.get_batches()) {
            glBindTexture(GL_TEXTURE_2D, textures_[batch.texture]);
            glDrawElements(GL_TRIANGLES, batch.sprite_count * 6, GL_UNSIGNED_SHORT, ...);
        }
    }
};
```

## Shaders

### Metal Shader (sprite.metal)

```metal
struct VertexIn {
    float2 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
    float4 color [[attribute(2)]];
};

struct VertexOut {
    float4 position [[position]];
    float2 texcoord;
    float4 color;
};

vertex VertexOut vertex_main(VertexIn in [[stage_in]],
                             constant float4x4& projection [[buffer(1)]]) {
    VertexOut out;
    out.position = projection * float4(in.position, 0.0, 1.0);
    out.texcoord = in.texcoord;
    out.color = in.color;
    return out;
}

fragment float4 fragment_main(VertexOut in [[stage_in]],
                              texture2d<float> tex [[texture(0)]],
                              sampler smp [[sampler(0)]]) {
    return tex.sample(smp, in.texcoord) * in.color;
}
```

### WebGL Shader (sprite.glsl)

```glsl
// Vertex
attribute vec2 a_position;
attribute vec2 a_texcoord;
attribute vec4 a_color;

uniform mat4 u_projection;

varying vec2 v_texcoord;
varying vec4 v_color;

void main() {
    gl_Position = u_projection * vec4(a_position, 0.0, 1.0);
    v_texcoord = a_texcoord;
    v_color = a_color;
}

// Fragment
precision mediump float;

varying vec2 v_texcoord;
varying vec4 v_color;

uniform sampler2D u_texture;

void main() {
    gl_FragColor = texture2D(u_texture, v_texcoord) * v_color;
}
```

## Isometric Rendering

For isometric depth sorting:

```cpp
// Sort sprites by their isometric depth before batching
float isometric_depth(float world_x, float world_y, float world_z) {
    // In isometric, things further "back" (higher Y) draw first
    // Things at same Y but higher Z draw later (on top)
    return world_y - world_z * 0.5f;
}

// Before rendering each frame:
std::sort(sprites.begin(), sprites.end(), [](const Sprite& a, const Sprite& b) {
    return isometric_depth(a.x, a.y, a.z) < isometric_depth(b.x, b.y, b.z);
});
```

## Pixel Perfect Rendering

For crisp pixel art:

```cpp
void Renderer::configure_pixel_perfect(int game_width, int game_height) {
    // 1. Render to internal framebuffer at game resolution
    // 2. Scale to window using nearest-neighbor filtering
    // 3. Center with letterboxing if aspect ratios differ

    int scale = std::min(window_width_ / game_width, window_height_ / game_height);
    int scaled_width = game_width * scale;
    int scaled_height = game_height * scale;
    int offset_x = (window_width_ - scaled_width) / 2;
    int offset_y = (window_height_ - scaled_height) / 2;
}
```

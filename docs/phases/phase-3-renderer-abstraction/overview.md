# Phase 3: Renderer Abstraction

**Duration**: Months 6-8
**Goal**: Abstract rendering so same code runs on Metal and WebGL
**Deliverable**: Demo running on both Mac native and web browser

## Overview

This phase is critical for cross-platform support. You'll:
- Design an abstract renderer interface
- Implement Metal backend (refactor from Phase 2)
- Set up Emscripten toolchain
- Implement WebGL backend
- Build sprite and tile rendering systems

## Tasks

### 3.1 Design Renderer Interface

**Key Principle:** The interface should express *what* to draw, not *how* to draw it.

**Interface Design:**
```cpp
// renderer/renderer.h

namespace cafe {

class Renderer {
public:
    virtual ~Renderer() = default;

    // Lifecycle
    virtual bool initialize(void* platform_handle) = 0;
    virtual void shutdown() = 0;

    // Frame management
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
    virtual void present() = 0;

    // Texture management
    virtual TextureHandle create_texture(int width, int height, const void* pixels) = 0;
    virtual void destroy_texture(TextureHandle handle) = 0;

    // Drawing (batched internally)
    virtual void draw_sprite(TextureHandle texture,
                            float src_x, float src_y, float src_w, float src_h,
                            float dst_x, float dst_y, float dst_w, float dst_h,
                            Color tint = Color::white()) = 0;

    virtual void draw_rect(float x, float y, float w, float h, Color color) = 0;

    // Camera
    virtual void set_camera(float x, float y, float zoom = 1.0f) = 0;

    // State
    virtual void set_blend_mode(BlendMode mode) = 0;
};

// Factory function
std::unique_ptr<Renderer> create_renderer(GraphicsBackend backend);

enum class GraphicsBackend {
    Metal,
    WebGL,
    // Future: DirectX11, OpenGL
};

}
```

**Design Decisions:**
- Use handles (uint32_t) instead of raw pointers for GPU resources
- Batch draw calls internally to minimize GPU overhead
- Keep the interface simple - no complex state management

---

### 3.2 Metal Backend Implementation

Refactor your Phase 2 code into the abstract interface:

```cpp
// renderer/metal/metal_renderer.h

class MetalRenderer : public Renderer {
private:
    id<MTLDevice> device_;
    id<MTLCommandQueue> command_queue_;
    id<MTLRenderPipelineState> sprite_pipeline_;
    CAMetalLayer* layer_;

    // Texture storage
    std::unordered_map<TextureHandle, id<MTLTexture>> textures_;
    TextureHandle next_texture_id_ = 1;

    // Sprite batching
    SpriteBatch batch_;
    id<MTLBuffer> vertex_buffer_;
    id<MTLBuffer> index_buffer_;

    // Camera
    float camera_x_ = 0, camera_y_ = 0, camera_zoom_ = 1.0f;

public:
    bool initialize(void* platform_handle) override {
        layer_ = (__bridge CAMetalLayer*)platform_handle;
        device_ = layer_.device;
        command_queue_ = [device_ newCommandQueue];

        setup_sprite_pipeline();
        create_buffers();

        return true;
    }

    void draw_sprite(TextureHandle texture,
                    float src_x, float src_y, float src_w, float src_h,
                    float dst_x, float dst_y, float dst_w, float dst_h,
                    Color tint) override {
        // Add to batch
        batch_.add(texture, src_x, src_y, src_w, src_h,
                   dst_x, dst_y, dst_w, dst_h, tint);

        // Flush if batch full or texture changed
        if (batch_.needs_flush()) {
            flush_batch();
        }
    }

private:
    void flush_batch() {
        if (batch_.empty()) return;

        // Upload vertices
        memcpy(vertex_buffer_.contents, batch_.vertices(), batch_.vertex_size());

        // Draw each batch segment (grouped by texture)
        for (const auto& segment : batch_.segments()) {
            // Bind texture
            // Draw indexed triangles
        }

        batch_.clear();
    }
};
```

---

### 3.3 Emscripten Setup

**Install Emscripten:**
```bash
# Install via Homebrew
brew install emscripten

# Or manual install
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

**CMake Configuration for Web:**
```cmake
# CMakeLists.txt

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    target_link_options(cafe_engine PRIVATE
        -sUSE_WEBGL2=1
        -sALLOW_MEMORY_GROWTH=1
        -sEXPORTED_FUNCTIONS=['_main']
        -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']
        --preload-file ${CMAKE_SOURCE_DIR}/assets@/assets
    )

    # No window creation needed - browser provides canvas
    target_compile_definitions(cafe_engine PRIVATE PLATFORM_WEB)
else()
    # macOS build
    target_compile_definitions(cafe_engine PRIVATE PLATFORM_MACOS)
    target_link_libraries(cafe_engine PRIVATE
        "-framework Cocoa"
        "-framework Metal"
        "-framework QuartzCore"
    )
endif()
```

**Build for Web:**
```bash
mkdir build-web && cd build-web
emcmake cmake ..
emmake make
# Output: cafe_engine.html, cafe_engine.js, cafe_engine.wasm
```

---

### 3.4 WebGL Backend Implementation

```cpp
// renderer/webgl/webgl_renderer.cpp

#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>

class WebGLRenderer : public Renderer {
private:
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context_;
    GLuint sprite_program_;
    GLuint vao_, vbo_, ebo_;

    std::unordered_map<TextureHandle, GLuint> textures_;
    TextureHandle next_texture_id_ = 1;

    SpriteBatch batch_;

public:
    bool initialize(void* platform_handle) override {
        // Create WebGL context
        EmscriptenWebGLContextAttributes attrs;
        emscripten_webgl_init_context_attributes(&attrs);
        attrs.majorVersion = 2;  // WebGL 2

        context_ = emscripten_webgl_create_context("#canvas", &attrs);
        emscripten_webgl_make_context_current(context_);

        setup_sprite_shader();
        create_buffers();

        return true;
    }

    TextureHandle create_texture(int width, int height, const void* pixels) override {
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        // Pixel art settings
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        TextureHandle handle = next_texture_id_++;
        textures_[handle] = tex;
        return handle;
    }

private:
    void setup_sprite_shader() {
        const char* vertex_src = R"(#version 300 es
            layout(location = 0) in vec2 a_position;
            layout(location = 1) in vec2 a_texcoord;
            layout(location = 2) in vec4 a_color;

            uniform mat4 u_projection;

            out vec2 v_texcoord;
            out vec4 v_color;

            void main() {
                gl_Position = u_projection * vec4(a_position, 0.0, 1.0);
                v_texcoord = a_texcoord;
                v_color = a_color;
            }
        )";

        const char* fragment_src = R"(#version 300 es
            precision mediump float;

            in vec2 v_texcoord;
            in vec4 v_color;

            uniform sampler2D u_texture;

            out vec4 fragColor;

            void main() {
                fragColor = texture(u_texture, v_texcoord) * v_color;
            }
        )";

        sprite_program_ = compile_program(vertex_src, fragment_src);
    }
};
```

---

### 3.5 Sprite & Texture System

**Sprite Batching:**
```cpp
// renderer/sprite_batch.h

struct SpriteVertex {
    float x, y;      // Position
    float u, v;      // Texture coord
    float r, g, b, a; // Color
};

class SpriteBatch {
    std::vector<SpriteVertex> vertices_;
    std::vector<uint16_t> indices_;

    struct Segment {
        TextureHandle texture;
        uint32_t index_offset;
        uint32_t index_count;
    };
    std::vector<Segment> segments_;

    TextureHandle current_texture_ = 0;
    static constexpr size_t MAX_SPRITES = 10000;

public:
    void add(TextureHandle texture, /* params */) {
        // Start new segment if texture changed
        if (texture != current_texture_) {
            if (!segments_.empty()) {
                segments_.back().index_count = indices_.size() - segments_.back().index_offset;
            }
            segments_.push_back({texture, (uint32_t)indices_.size(), 0});
            current_texture_ = texture;
        }

        // Add 4 vertices (quad)
        uint16_t base = vertices_.size();
        vertices_.push_back({dst_x, dst_y, src_x/tex_w, src_y/tex_h, tint.r, tint.g, tint.b, tint.a});
        // ... add other 3 corners

        // Add 6 indices (two triangles)
        indices_.push_back(base + 0);
        indices_.push_back(base + 1);
        indices_.push_back(base + 2);
        indices_.push_back(base + 2);
        indices_.push_back(base + 3);
        indices_.push_back(base + 0);
    }

    bool needs_flush() const {
        return vertices_.size() >= MAX_SPRITES * 4;
    }

    void clear() {
        vertices_.clear();
        indices_.clear();
        segments_.clear();
        current_texture_ = 0;
    }
};
```

---

### 3.6 Isometric Tile Rendering

**Tile Map Structure:**
```cpp
// engine/tilemap.h

struct Tile {
    uint16_t texture_id;  // Index into tileset
    uint8_t flags;        // Walkable, etc.
};

class TileMap {
    std::vector<Tile> tiles_;
    int width_, height_;
    int tile_width_ = 32;
    int tile_height_ = 16;  // Isometric: half height

public:
    void render(Renderer* r, TextureHandle tileset, float camera_x, float camera_y) {
        // Isometric rendering order: back to front, left to right
        for (int y = 0; y < height_; y++) {
            for (int x = 0; x < width_; x++) {
                const Tile& tile = get(x, y);

                // Convert tile coords to screen coords (isometric projection)
                float screen_x = (x - y) * (tile_width_ / 2) - camera_x;
                float screen_y = (x + y) * (tile_height_ / 2) - camera_y;

                // Source rect in tileset
                int tiles_per_row = tileset_width / tile_width_;
                int tx = (tile.texture_id % tiles_per_row) * tile_width_;
                int ty = (tile.texture_id / tiles_per_row) * tile_height_;

                r->draw_sprite(tileset,
                              tx, ty, tile_width_, tile_height_,
                              screen_x, screen_y, tile_width_, tile_height_);
            }
        }
    }

    // Convert screen position to tile coordinates
    void screen_to_tile(float sx, float sy, int* tile_x, int* tile_y) {
        // Inverse isometric projection
        float tw = tile_width_ / 2.0f;
        float th = tile_height_ / 2.0f;
        *tile_x = (int)((sx / tw + sy / th) / 2);
        *tile_y = (int)((sy / th - sx / tw) / 2);
    }
};
```

---

## Milestone: Cross-Platform Demo

Create a demo that:
- Renders an isometric tile map
- Shows animated sprites on the map
- Responds to input (click to select tile)
- Runs identically on Mac and Web

**Test on Mac:**
```bash
./build/cafe_engine
```

**Test on Web:**
```bash
cd build-web
python -m http.server 8000
# Open http://localhost:8000/cafe_engine.html
```

---

## Checklist

- [ ] Renderer interface designed and documented
- [ ] Metal backend implements full interface
- [ ] Emscripten toolchain working
- [ ] WebGL backend implements full interface
- [ ] Sprite batching working efficiently
- [ ] Textures load and display correctly
- [ ] Isometric tile rendering working
- [ ] Same demo runs on Mac and Web

## Next Phase

Proceed to [Phase 4: Core Engine Systems](../phase-4-core-engine/overview.md) to build entity and resource management.

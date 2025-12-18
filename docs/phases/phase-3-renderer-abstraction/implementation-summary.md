# Phase 3: Implementation Summary

This document explains what we built in Phase 3 and why, written for beginners learning C++ graphics programming.

## What We Achieved

We transformed the engine from "only works on Mac with Metal" to "can work on Mac AND web browsers" by:

1. Creating an **abstract interface** that hides GPU differences
2. Implementing **Metal backend** (for Mac)
3. Implementing **WebGL backend** (for browsers)
4. Building a **sprite system** for 2D game graphics
5. Building an **isometric tile system** for the cafe game

---

## File-by-File Breakdown

### 1. Renderer Interface (`src/renderer/renderer.h`)

**Purpose:** Define WHAT a renderer can do, without saying HOW it does it.

**Key Concept - Abstraction:**
```cpp
// This is an "abstract class" - it defines a contract
// Any renderer (Metal, WebGL, DirectX) must implement these functions
class Renderer {
public:
    virtual ~Renderer() = default;

    // Pure virtual functions (= 0) MUST be implemented by subclasses
    virtual bool initialize(Window* window) = 0;
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
    virtual void draw_quad(Vec2 position, Vec2 size, const Color& color) = 0;
    // ... more functions
};
```

**What we added:**

| Addition | Purpose |
|----------|---------|
| `TextureHandle` | A number that identifies a texture (like a file handle) |
| `TextureInfo` | Settings for creating textures (size, filtering) |
| `TextureRegion` | A rectangle within a texture (for sprite sheets) |
| `Sprite` | All info needed to draw one sprite (position, size, texture, rotation) |
| `set_projection()` | Set up 2D coordinate system |
| `create_texture()` | Upload image data to GPU |
| `begin_batch()`/`end_batch()` | Efficient drawing of many sprites |

**Why abstraction matters:**
```cpp
// Game code doesn't care if it's Metal or WebGL
renderer->draw_sprite(player_sprite);  // Works on both platforms!
```

---

### 2. Metal Backend (`src/renderer/metal/metal_renderer.mm`)

**Purpose:** Implement the renderer interface using Apple's Metal API.

**File extension:** `.mm` means Objective-C++ (C++ mixed with Apple's Objective-C)

**Key Components:**

#### A. Shaders (GPU Programs)
```cpp
// Metal Shading Language (MSL) - runs on the GPU
static const char* kShaderSource = R"(
    // Vertex shader: positions vertices on screen
    vertex VertexOut vertex_main(VertexIn in [[stage_in]],
                                  constant Uniforms& uniforms [[buffer(1)]]) {
        VertexOut out;
        // Transform position using projection matrix
        out.position = uniforms.projection * float4(in.position, 0.0, 1.0);
        out.texcoord = in.texcoord;
        out.color = in.color;
        return out;
    }

    // Fragment shader: colors each pixel
    fragment float4 fragment_textured(VertexOut in [[stage_in]],
                                       texture2d<float> tex [[texture(0)]],
                                       sampler samp [[sampler(0)]]) {
        // Sample texture and multiply by vertex color
        return tex.sample(samp, in.texcoord) * in.color;
    }
)";
```

#### B. Render Pipelines
```cpp
// Two pipelines: one for colored shapes, one for textured sprites
id<MTLRenderPipelineState> color_pipeline_;     // For solid colors
id<MTLRenderPipelineState> textured_pipeline_;  // For sprites with images
```

#### C. Sprite Batching
```cpp
// Instead of drawing sprites one-by-one (slow), we batch them
static constexpr int kMaxQuadsPerBatch = 1000;

void draw_sprite(const Sprite& sprite) {
    // Add sprite's vertices to batch buffer
    // When buffer is full, flush to GPU
    if (batch_quad_count_ >= kMaxQuadsPerBatch) {
        flush_batch();
    }
    // Add 6 vertices (2 triangles = 1 quad)
    // ... vertex calculations with rotation ...
}
```

#### D. Texture Management
```cpp
// Store textures in a map: handle -> Metal texture object
std::unordered_map<TextureHandle, id<MTLTexture>> textures_;
TextureHandle next_texture_handle_ = 1;

TextureHandle create_texture(const uint8_t* pixels, const TextureInfo& info) {
    // 1. Create Metal texture descriptor
    // 2. Upload pixel data to GPU
    // 3. Store in map and return handle
}
```

---

### 3. Web Platform Layer (`src/platform/web/web_platform.cpp`)

**Purpose:** Handle window, input, and timing for web browsers using Emscripten.

**Key Concept - Platform Abstraction:**
```cpp
// Same interface as macOS, different implementation
class WebWindow : public Window {
    // Emscripten uses HTML5 canvas instead of native window
    // Input comes from DOM events instead of Cocoa events
};
```

**Input Handling:**
```cpp
// Translate web keyboard codes to our engine's key enum
static Key translate_key_code(const char* code) {
    if (strcmp(code, "KeyW") == 0) return Key::W;
    if (strcmp(code, "ArrowUp") == 0) return Key::Up;
    // ... more keys
}

// Emscripten callback - called when user presses a key
static EM_BOOL key_callback(int event_type,
                            const EmscriptenKeyboardEvent* e,
                            void* user_data) {
    WebWindow* win = static_cast<WebWindow*>(user_data);
    Key key = translate_key_code(e->code);
    win->keys_down_[key] = (event_type == EMSCRIPTEN_EVENT_KEYDOWN);
    return EM_TRUE;
}
```

---

### 4. WebGL Renderer (`src/renderer/webgl/webgl_renderer.cpp`)

**Purpose:** Implement the renderer interface using WebGL2 (OpenGL ES 3.0).

**Key Differences from Metal:**

| Metal | WebGL |
|-------|-------|
| MSL shaders | GLSL ES shaders |
| `id<MTLTexture>` | `GLuint` texture ID |
| `MTLRenderCommandEncoder` | `glDrawArrays()` |
| Explicit command buffers | Immediate mode API |

**GLSL Shaders:**
```cpp
static const char* kVertexShader = R"(#version 300 es
    layout(location = 0) in vec2 a_position;
    layout(location = 1) in vec2 a_texcoord;
    layout(location = 2) in vec4 a_color;

    uniform mat4 u_projection;  // 2D projection matrix

    out vec2 v_texcoord;
    out vec4 v_color;

    void main() {
        gl_Position = u_projection * vec4(a_position, 0.0, 1.0);
        v_texcoord = a_texcoord;
        v_color = a_color;
    }
)";
```

---

### 5. Image Loading (`src/engine/image.h` and `image.cpp`)

**Purpose:** Load image files (PNG, JPG) into memory.

**We use stb_image:** A single-header library (no compilation needed)

```cpp
// In image.cpp - this line creates the implementation
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

std::unique_ptr<Image> Image::load_from_file(const std::string& path) {
    auto image = std::make_unique<Image>();

    // stbi_load reads the file and returns pixel data
    // We force 4 channels (RGBA) for consistency
    image->data_ = stbi_load(path.c_str(),
                              &image->width_,
                              &image->height_,
                              &image->channels_,
                              4);  // Force RGBA

    if (!image->data_) {
        return nullptr;  // File not found or invalid
    }
    return image;
}
```

**Memory Management with RAII:**
```cpp
Image::~Image() {
    if (data_ && owned_) {
        stbi_image_free(data_);  // Automatically free when Image is destroyed
    }
}
```

---

### 6. Sprite Sheet System (`src/engine/sprite_sheet.h` and `sprite_sheet.cpp`)

**Purpose:** Manage sprite atlases (many sprites in one texture) and animations.

**Why sprite sheets?**
- Loading one large texture is faster than many small ones
- GPU can batch-draw sprites from the same texture
- Common in 2D games (think of a character walking animation)

```
+-------+-------+-------+-------+
| Frame | Frame | Frame | Frame |  <- One texture file
|   0   |   1   |   2   |   3   |
+-------+-------+-------+-------+
   Walk animation: 0 -> 1 -> 2 -> 3 -> repeat
```

**Key Classes:**

```cpp
// One frame in the sprite sheet
struct SpriteFrame {
    std::string name;      // "walk_01", "idle", etc.
    TextureRegion region;  // UV coordinates in the texture
    int width, height;     // Size in pixels
};

// Animation = sequence of frames
struct Animation {
    std::string name;              // "walk", "run", "attack"
    std::vector<int> frame_indices;  // Which frames to play
    float frame_duration;          // Seconds per frame
    bool looping;                  // Repeat when done?
};

// The sprite sheet itself
class SpriteSheet {
    void define_grid(int cell_width, int cell_height);  // Auto-slice into grid
    void define_animation(const std::string& name, int start, int end);
};

// Plays animations with timing
class AnimationPlayer {
    void play(const std::string& animation_name);
    void update(float delta_time);
    TextureRegion current_region();  // Get current frame to draw
};
```

**Usage:**
```cpp
SpriteSheet character;
character.load(renderer, "character.png");
character.define_grid(16, 16);  // Each frame is 16x16 pixels
character.define_animation("walk", 0, 3, 0.15f);  // Frames 0-3, 0.15s each

AnimationPlayer player(&character);
player.play("walk");

// In game loop:
player.update(delta_time);
sprite.region = player.current_region();
renderer->draw_sprite(sprite);
```

---

### 7. Isometric Tile System (`src/engine/isometric.h` and `isometric.cpp`)

**Purpose:** Render 2D tiles in an isometric (2.5D) view for the cafe game.

**What is isometric?**
```
    Standard 2D:          Isometric:
    +---+---+---+            /\
    | 0 | 1 | 2 |           /  \
    +---+---+---+          /\  /\
    | 3 | 4 | 5 |         /  \/  \
    +---+---+---+        /\  /\  /\
                        /  \/  \/  \
```

**Coordinate Conversion:**
```cpp
// Tile (x, y) -> Screen position
Vec2 Isometric::tile_to_screen(float tile_x, float tile_y) {
    // Classic 2:1 isometric projection
    float screen_x = (tile_x - tile_y) * (tile_width_ * 0.5f);
    float screen_y = (tile_x + tile_y) * (tile_height_ * 0.5f);

    // Apply camera offset (for scrolling)
    screen_x -= camera_x_;
    screen_y -= camera_y_;

    return {screen_x, screen_y};
}

// Screen click -> Which tile?
Vec2 Isometric::screen_to_tile(float screen_x, float screen_y) {
    // Reverse the math above
    // Used for: "which tile did the player click?"
}
```

**Depth Sorting (draw order):**
```cpp
// Problem: tiles can overlap, need to draw back-to-front
// Solution: tiles with higher (x + y) are "in front"

int tile_depth(int tile_x, int tile_y) {
    return tile_x + tile_y;  // Simple but effective!
}

// In for_each_visible():
std::sort(visible_tiles.begin(), visible_tiles.end(),
          [](const TileEntry& a, const TileEntry& b) {
              return a.depth < b.depth;  // Lower depth = draw first
          });
```

**TileMap Class:**
```cpp
class TileMap {
    int width_, height_;           // Map size in tiles
    std::vector<Tile> tiles_;      // The actual tile data
    SpriteSheet* tileset_;         // Graphics for tiles

    void render(Renderer* renderer, const Rect& viewport) {
        // 1. Find which tiles are visible on screen
        // 2. Sort by depth (back to front)
        // 3. Draw each tile using sprite batching
    }
};
```

---

### 8. Build Configuration (`CMakeLists.txt` and `CMakeLists_web.cmake`)

**Main CMakeLists.txt changes:**
```cmake
# Added new source files
set(CAFE_SOURCES
    src/main.cpp
    src/engine/game_loop.cpp
    src/engine/image.cpp          # NEW: Image loading
    src/engine/sprite_sheet.cpp   # NEW: Sprite system
    src/engine/isometric.cpp      # NEW: Isometric tiles
)

# Added third-party includes (for stb_image)
target_include_directories(cafe_engine PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party
)
```

**Web build (CMakeLists_web.cmake):**
```cmake
# Emscripten-specific settings
set(CMAKE_EXECUTABLE_SUFFIX ".html")

# WebGL2 and memory settings
set_target_properties(cafe_engine PROPERTIES
    LINK_FLAGS "\
        -s USE_WEBGL2=1 \        # Enable WebGL 2.0
        -s FULL_ES3=1 \          # Full OpenGL ES 3.0 support
        -s ALLOW_MEMORY_GROWTH=1 \  # Dynamic memory
        -s WASM=1 \              # Compile to WebAssembly
        --shell-file ${CMAKE_SOURCE_DIR}/web/shell.html \
    "
)
```

---

### 9. HTML Shell (`web/shell.html`)

**Purpose:** The web page that hosts the game.

```html
<canvas id="canvas" oncontextmenu="event.preventDefault()"></canvas>

<script>
    var Module = {
        canvas: document.getElementById('canvas'),
        onRuntimeInitialized: function() {
            // Called when WASM is ready
            document.getElementById('status').innerHTML = 'Running';
        }
    };
</script>
{{{ SCRIPT }}}  <!-- Emscripten injects the game code here -->
```

---

### 10. Build Script (`scripts/build-web.sh`)

**Purpose:** Simplify the web build process.

```bash
#!/bin/bash

# Check if Emscripten is installed
check_emscripten() {
    if ! command -v emcc &> /dev/null; then
        echo "Emscripten not found! Install it first."
        exit 1
    fi
}

# Build the project
do_build() {
    emcmake cmake "$PROJECT_DIR" -DCMAKE_BUILD_TYPE=Release
    emmake make
}

# Serve locally for testing
do_serve() {
    python3 -m http.server 8080
    # Open http://localhost:8080/cafe_engine.html
}
```

---

## Common Patterns You'll See

### 1. RAII (Resource Acquisition Is Initialization)
```cpp
// Resources are automatically cleaned up when objects are destroyed
class Image {
    ~Image() {
        if (data_) stbi_image_free(data_);  // Auto-cleanup
    }
};

// Usage - no manual cleanup needed!
{
    auto image = Image::load_from_file("sprite.png");
    // use image...
}  // <-- image destroyed here, memory freed automatically
```

### 2. Smart Pointers
```cpp
// std::unique_ptr = "I own this, delete it when I'm done"
std::unique_ptr<Image> image = Image::load_from_file("test.png");

// No need to call 'delete image' - happens automatically
```

### 3. Factory Functions
```cpp
// Instead of exposing implementation details:
std::unique_ptr<Renderer> create_renderer();  // Returns Metal or WebGL

// Game code doesn't know or care which one:
auto renderer = create_renderer();
renderer->draw_quad(...);  // Works on any platform!
```

### 4. Handles vs Pointers
```cpp
// Handle = just a number identifying a resource
using TextureHandle = uint32_t;
TextureHandle tex = renderer->create_texture(pixels, info);

// Why not pointers?
// - Handles work across DLL boundaries
// - Handles can be validated (is this texture still valid?)
// - Handles hide implementation details
```

---

## Bug We Fixed: Vector Overflow Crash

**The Problem:**
```
libc++abi: terminating due to uncaught exception of type std::length_error: vector
```

**What Happened:**
The `for_each_visible` function was calculating which tiles to render, but the coordinate system was confused:
- We passed world coordinates to a function expecting screen coordinates
- This caused the tile bounds to calculate astronomically large values
- The vector tried to reserve billions of elements and crashed

**The Fix:**
1. Changed viewport to use screen coordinates (0,0 to width,height)
2. Added bounds validation before calculating tile range
3. Added a cap on vector reservation (max 10,000 tiles)

**Lesson:** Always be clear about which coordinate system you're using!

---

## The Big Picture

```
+-------------------------------------------------------------+
|                        GAME CODE                            |
|   main.cpp - Uses sprites, tilemaps, animations             |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|                     ENGINE SYSTEMS                          |
|   SpriteSheet, AnimationPlayer, TileMap, Isometric          |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|                   ABSTRACT INTERFACES                       |
|   Renderer (renderer.h), Platform (platform.h)              |
+-------------------------------------------------------------+
                    |                   |
          +---------+                   +---------+
          v                                       v
+----------------------+             +----------------------+
|   METAL BACKEND      |             |   WEBGL BACKEND      |
|   metal_renderer.mm  |             |   webgl_renderer.cpp |
|   macos_platform.mm  |             |   web_platform.cpp   |
+----------------------+             +----------------------+
          |                                       |
          v                                       v
+----------------------+             +----------------------+
|      macOS / Metal   |             |   Browser / WebGL    |
+----------------------+             +----------------------+
```

---

## Files Created/Modified in Phase 3

| File | Type | Purpose |
|------|------|---------|
| `src/renderer/renderer.h` | Modified | Extended interface with textures, sprites |
| `src/renderer/metal/metal_renderer.mm` | Modified | Full rewrite with batching, textures |
| `src/platform/web/web_platform.cpp` | New | Web platform layer |
| `src/renderer/webgl/webgl_renderer.cpp` | New | WebGL renderer |
| `src/engine/image.h` | New | Image loading interface |
| `src/engine/image.cpp` | New | stb_image implementation |
| `src/engine/sprite_sheet.h` | New | Sprite sheet and animation |
| `src/engine/sprite_sheet.cpp` | New | Sprite sheet implementation |
| `src/engine/isometric.h` | New | Isometric coordinate system |
| `src/engine/isometric.cpp` | New | TileMap and rendering |
| `CMakeLists.txt` | Modified | Added new sources and includes |
| `CMakeLists_web.cmake` | New | Web build configuration |
| `web/shell.html` | New | HTML template for web |
| `scripts/build-web.sh` | New | Build helper script |
| `third_party/stb/stb_image.h` | New | Image loading library |

---

## What's Next (Phase 4)

With rendering abstracted, we can now build:
- **Resource Management** - Loading/caching assets efficiently
- **Entity-Component System** - Flexible game object architecture
- **Scene Management** - Loading levels, transitions
- **Audio System** - Sound effects and music
- **Input Mapping** - Rebindable controls

The cafe game is getting closer!

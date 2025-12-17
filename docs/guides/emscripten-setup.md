# Emscripten Setup Guide

Guide to setting up Emscripten for compiling Cafe Engine to WebAssembly.

## What is Emscripten?

Emscripten is a toolchain that compiles C/C++ to WebAssembly (WASM), allowing the game to run in web browsers.

## Prerequisites

- macOS, Linux, or Windows
- Git
- Python 3.6+
- CMake 3.20+

## Installation

### Option A: Homebrew (macOS)
```bash
brew install emscripten

# Verify
emcc --version
```

### Option B: Manual Installation (Recommended for Latest)
```bash
# Clone emsdk
cd ~
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

# Install latest SDK
./emsdk install latest
./emsdk activate latest

# Add to shell profile
echo 'source ~/emsdk/emsdk_env.sh' >> ~/.zshrc
source ~/.zshrc

# Verify
emcc --version
# Should show Emscripten version 3.x+
```

## Project Setup

### Update CMakeLists.txt

Add web build configuration:

```cmake
cmake_minimum_required(VERSION 3.20)
project(CafeEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Detect Emscripten
if(EMSCRIPTEN)
    message(STATUS "Building for Web (Emscripten)")
    set(PLATFORM_WEB TRUE)
else()
    message(STATUS "Building for macOS")
    set(PLATFORM_MACOS TRUE)
endif()

# Source files
if(PLATFORM_WEB)
    file(GLOB_RECURSE SOURCES
        src/*.cpp
        src/platform/web/*.cpp
        src/renderer/webgl/*.cpp
    )
else()
    file(GLOB_RECURSE SOURCES
        src/*.cpp
        src/*.mm
        src/platform/macos/*.mm
        src/renderer/metal/*.mm
    )
endif()

add_executable(cafe_engine ${SOURCES})

target_include_directories(cafe_engine PRIVATE include)

if(PLATFORM_WEB)
    # Emscripten settings
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    target_link_options(cafe_engine PRIVATE
        -sUSE_WEBGL2=1
        -sALLOW_MEMORY_GROWTH=1
        -sFULL_ES3=1
        -sWASM=1
        -sASYNCIFY=1
        --preload-file ${CMAKE_SOURCE_DIR}/assets@/assets
        -sEXPORTED_FUNCTIONS=['_main']
        -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']
    )

    target_compile_definitions(cafe_engine PRIVATE PLATFORM_WEB)

else()
    # macOS settings
    target_link_libraries(cafe_engine PRIVATE
        "-framework Cocoa"
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore"
    )

    target_compile_definitions(cafe_engine PRIVATE PLATFORM_MACOS)
endif()
```

### Create Web Platform Layer

```cpp
// src/platform/web/web_platform.cpp

#ifdef PLATFORM_WEB

#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include "platform/platform.h"

class WebPlatform : public Platform {
private:
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context_;
    int canvas_width_ = 1280;
    int canvas_height_ = 720;
    std::queue<InputEvent> events_;

public:
    bool initialize(const PlatformConfig& config) override {
        // Set canvas size
        emscripten_set_canvas_element_size("#canvas",
            config.window_width, config.window_height);
        canvas_width_ = config.window_width;
        canvas_height_ = config.window_height;

        // Create WebGL context
        EmscriptenWebGLContextAttributes attrs;
        emscripten_webgl_init_context_attributes(&attrs);
        attrs.majorVersion = 2;  // WebGL 2.0 = OpenGL ES 3.0
        attrs.minorVersion = 0;

        context_ = emscripten_webgl_create_context("#canvas", &attrs);
        if (context_ <= 0) {
            printf("Failed to create WebGL context\n");
            return false;
        }
        emscripten_webgl_make_context_current(context_);

        // Register input callbacks
        emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,
            this, true, key_callback);
        emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,
            this, true, key_callback);
        emscripten_set_mousedown_callback("#canvas",
            this, true, mouse_callback);
        emscripten_set_mouseup_callback("#canvas",
            this, true, mouse_callback);
        emscripten_set_mousemove_callback("#canvas",
            this, true, mouse_callback);

        return true;
    }

    void* get_native_handle() override {
        return (void*)(intptr_t)context_;
    }

    double get_time() override {
        return emscripten_get_now() / 1000.0;
    }

    bool read_file(const char* path, std::vector<uint8_t>* data) override {
        // Emscripten virtual filesystem
        FILE* f = fopen(path, "rb");
        if (!f) return false;

        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        fseek(f, 0, SEEK_SET);

        data->resize(size);
        fread(data->data(), 1, size, f);
        fclose(f);
        return true;
    }

private:
    static EM_BOOL key_callback(int type,
                                const EmscriptenKeyboardEvent* e,
                                void* data) {
        WebPlatform* self = static_cast<WebPlatform*>(data);
        InputEvent event;
        event.type = (type == EMSCRIPTEN_EVENT_KEYDOWN)
            ? InputEvent::KeyDown : InputEvent::KeyUp;
        event.key_code = e->keyCode;
        self->events_.push(event);
        return EM_TRUE;
    }

    static EM_BOOL mouse_callback(int type,
                                  const EmscriptenMouseEvent* e,
                                  void* data) {
        WebPlatform* self = static_cast<WebPlatform*>(data);
        InputEvent event;

        switch (type) {
            case EMSCRIPTEN_EVENT_MOUSEDOWN:
                event.type = InputEvent::MouseDown;
                event.mouse_button = e->button;
                break;
            case EMSCRIPTEN_EVENT_MOUSEUP:
                event.type = InputEvent::MouseUp;
                event.mouse_button = e->button;
                break;
            case EMSCRIPTEN_EVENT_MOUSEMOVE:
                event.type = InputEvent::MouseMove;
                break;
        }

        event.x = e->targetX;
        event.y = e->targetY;
        self->events_.push(event);
        return EM_TRUE;
    }
};

std::unique_ptr<Platform> create_platform() {
    return std::make_unique<WebPlatform>();
}

#endif // PLATFORM_WEB
```

### Create Web Main Loop

```cpp
// src/main_web.cpp

#ifdef PLATFORM_WEB

#include <emscripten.h>

// Global game state (needed for Emscripten callback)
static Game* g_game = nullptr;
static Platform* g_platform = nullptr;
static Renderer* g_renderer = nullptr;
static double g_prev_time = 0;

void main_loop() {
    double current_time = g_platform->get_time();
    double dt = current_time - g_prev_time;
    g_prev_time = current_time;

    // Poll events
    g_platform->poll_events();
    InputEvent event;
    while (g_platform->pop_event(&event)) {
        g_game->handle_input(event);
    }

    // Update and render
    g_game->update(dt);

    g_renderer->begin_frame();
    g_game->render(g_renderer, 1.0);
    g_renderer->end_frame();
    g_renderer->present();
}

int main() {
    g_platform = create_platform().release();
    g_platform->initialize({
        .window_title = "Cafe Engine",
        .window_width = 1280,
        .window_height = 720
    });

    g_renderer = create_renderer(GraphicsBackend::WebGL).release();
    g_renderer->initialize(g_platform->get_native_handle());

    g_game = new CafeGame();
    g_game->initialize();

    g_prev_time = g_platform->get_time();

    // Start main loop (doesn't return!)
    emscripten_set_main_loop(main_loop, 0, true);

    return 0;
}

#endif // PLATFORM_WEB
```

## Building for Web

### Create Build Script

```bash
#!/bin/bash
# scripts/build_web.sh

set -e

# Ensure Emscripten is available
if ! command -v emcc &> /dev/null; then
    echo "Emscripten not found. Run: source ~/emsdk/emsdk_env.sh"
    exit 1
fi

# Create build directory
mkdir -p build-web
cd build-web

# Configure with Emscripten
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
emmake make -j$(nproc)

echo "Build complete!"
echo "Output files:"
ls -la cafe_engine.*
```

### Build Commands

```bash
# First time setup
mkdir build-web
cd build-web
emcmake cmake ..
emmake make

# Subsequent builds
cd build-web
emmake make

# Or use the script
./scripts/build_web.sh
```

### Output Files

After building, you'll have:
- `cafe_engine.html` - HTML page with canvas
- `cafe_engine.js` - JavaScript glue code
- `cafe_engine.wasm` - WebAssembly binary
- `cafe_engine.data` - Preloaded assets (if any)

## Running Locally

Browsers require a web server for WASM (can't just open the HTML file).

### Option A: Python Server
```bash
cd build-web
python3 -m http.server 8000

# Open http://localhost:8000/cafe_engine.html
```

### Option B: Node.js Server
```bash
npx serve build-web

# Open the URL shown
```

### Option C: VS Code Live Server
Install "Live Server" extension, right-click `cafe_engine.html`, select "Open with Live Server".

## Custom HTML Template

Create a custom shell to control the HTML page:

```html
<!-- shell.html -->
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Cafe Engine</title>
    <style>
        body {
            margin: 0;
            padding: 0;
            background: #1a1a2e;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
        }
        #canvas {
            border: 2px solid #4a4a6a;
            image-rendering: pixelated;
        }
        #loading {
            position: absolute;
            color: white;
            font-family: sans-serif;
        }
    </style>
</head>
<body>
    <div id="loading">Loading...</div>
    <canvas id="canvas"></canvas>

    <script>
        var Module = {
            canvas: document.getElementById('canvas'),
            onRuntimeInitialized: function() {
                document.getElementById('loading').style.display = 'none';
            }
        };
    </script>
    {{{ SCRIPT }}}
</body>
</html>
```

Add to CMake:
```cmake
if(EMSCRIPTEN)
    target_link_options(cafe_engine PRIVATE
        --shell-file ${CMAKE_SOURCE_DIR}/shell.html
    )
endif()
```

## Emscripten-Specific APIs

### File System
```cpp
// Preloaded files (via --preload-file)
FILE* f = fopen("/assets/sprite.png", "rb");

// Persistent storage (IndexedDB)
EM_ASM({
    FS.syncfs(false, function(err) {
        if (err) console.log('Sync error:', err);
    });
});
```

### JavaScript Interop
```cpp
// Call JavaScript from C++
EM_ASM({
    console.log('Hello from C++!');
    alert(UTF8ToString($0));
}, "Message from game");

// Return value from JavaScript
int result = EM_ASM_INT({
    return window.innerWidth;
});
```

## Performance Tips

### Optimize Build
```cmake
if(EMSCRIPTEN)
    target_compile_options(cafe_engine PRIVATE -O3)
    target_link_options(cafe_engine PRIVATE -O3)
endif()
```

### Reduce WASM Size
```cmake
target_link_options(cafe_engine PRIVATE
    -s MINIMAL_RUNTIME=1
    -s FILESYSTEM=0  # If not using filesystem
    --closure 1      # Minify JS
)
```

### Compress Assets
```bash
# Use smaller image formats
# Compress audio to smaller bitrates
# Consider loading assets on-demand instead of preloading all
```

## Troubleshooting

### "SharedArrayBuffer is not defined"
Add headers for cross-origin isolation:
```
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

### Black screen
- Check browser console for errors
- Verify WebGL2 support: https://get.webgl.org/webgl2/
- Check canvas element exists and has correct ID

### Assets not loading
- Verify `--preload-file` path is correct
- Check browser network tab for 404s
- File paths in WASM are case-sensitive

### Audio not playing
- Browsers require user interaction before audio
- Add click handler to start audio context

## Deployment

### itch.io
1. Zip the build-web folder contents
2. Upload to itch.io
3. Set "Kind of project" to HTML

### GitHub Pages
```bash
# Create gh-pages branch
git checkout -b gh-pages
cp -r build-web/* .
git add .
git commit -m "Deploy web build"
git push origin gh-pages
```

### Custom Hosting
Upload files to any static web host. Ensure server serves:
- `.wasm` files as `application/wasm`
- `.data` files as `application/octet-stream`

## Next Steps

1. Complete basic web build
2. Verify WebGL rendering works
3. Test input handling
4. Implement WebGL renderer (Phase 3)
5. Test on multiple browsers (Chrome, Firefox, Safari)

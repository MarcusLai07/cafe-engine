# Debugging Guide

Common issues and how to solve them when developing Cafe Engine.

## Build Issues

### CMake Configuration Errors

**"Could not find compiler"**
```bash
# Fix: Install/reset Xcode command line tools
xcode-select --install
sudo xcode-select --reset
```

**"Unknown CMake command"**
```bash
# Fix: Update CMake
brew upgrade cmake
# Minimum required: 3.20
```

**"Could not find Metal framework"**
```bash
# Fix: Ensure Xcode SDK is set
export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
```

### Compilation Errors

**Undefined symbols for Objective-C**
```
Undefined symbols for architecture arm64:
  "_OBJC_CLASS_$_NSWindow"
```
Fix: Ensure file has `.mm` extension (not `.cpp`) for Objective-C++ code.

**Template errors**
```
error: no matching function for call to 'make_unique'
```
Fix: Add `#include <memory>` and ensure C++17/20 standard is set.

**Missing headers**
```
fatal error: 'Metal/Metal.h' file not found
```
Fix: Add framework to CMakeLists.txt:
```cmake
target_link_libraries(cafe_engine PRIVATE "-framework Metal")
```

### Linker Errors

**Duplicate symbols**
```
duplicate symbol '_main' in:
    main.o
    game.o
```
Fix: Ensure only one file defines `main()`.

**Undefined reference to vtable**
```
undefined reference to 'vtable for Component'
```
Fix: Implement all pure virtual functions or provide destructor:
```cpp
virtual ~Component() = default;
```

## Runtime Issues

### Crashes

#### Segmentation Fault (SIGSEGV)

**Null pointer dereference:**
```cpp
// Bad
Entity* e = find_entity("player");
e->update(dt);  // Crash if nullptr!

// Good
Entity* e = find_entity("player");
if (e) {
    e->update(dt);
}
```

**Use after free:**
```cpp
// Bad
Entity* e = entities.back();
entities.pop_back();
e->render();  // Crash! e is invalid

// Good
entities.back()->render();
entities.pop_back();
```

**Out of bounds access:**
```cpp
// Bad
std::vector<int> v = {1, 2, 3};
int x = v[5];  // Undefined behavior!

// Good
if (index < v.size()) {
    int x = v[index];
}

// Better: use .at() for bounds checking during debug
int x = v.at(index);  // Throws exception if out of bounds
```

#### EXC_BAD_ACCESS

Usually means accessing freed memory or invalid pointer.

**Debug with AddressSanitizer:**
```cmake
# Add to CMakeLists.txt for debug builds
if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(cafe_engine PRIVATE -fsanitize=address)
    target_link_options(cafe_engine PRIVATE -fsanitize=address)
endif()
```

Run and the sanitizer will point to the exact line.

### Memory Leaks

**Symptoms:**
- Memory usage grows over time
- Eventually runs out of memory

**Detection:**
```bash
# macOS Instruments
instruments -t Leaks ./build/cafe_engine

# Or use AddressSanitizer with leak detection
ASAN_OPTIONS=detect_leaks=1 ./build/cafe_engine
```

**Common causes:**
```cpp
// Leaked: raw new without delete
Texture* tex = new Texture("sprite.png");
// Fix: Use unique_ptr
auto tex = std::make_unique<Texture>("sprite.png");

// Leaked: forgot to release GPU resources
TextureHandle create_texture() {
    TextureHandle h = renderer->create_texture(...);
    // Must call renderer->destroy_texture(h) later!
    return h;
}
```

### Graphics Issues

#### Black Screen

**Checklist:**
1. Is clear color set?
   ```cpp
   // Metal
   passDesc.colorAttachments[0].clearColor = MTLClearColorMake(0.1, 0.1, 0.2, 1.0);
   passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
   ```

2. Is drawable presented?
   ```cpp
   [commandBuffer presentDrawable:drawable];
   [commandBuffer commit];
   ```

3. Are shaders compiled without errors?
   ```cpp
   NSError* error = nil;
   id<MTLRenderPipelineState> pipeline = [device newRenderPipelineStateWithDescriptor:desc error:&error];
   if (error) {
       NSLog(@"Shader error: %@", error);
   }
   ```

4. Is vertex data valid?
   - Check vertex buffer has correct stride
   - Verify vertex format matches shader input

#### Flickering

**Cause:** Drawing before previous frame completes.

**Fix:** Ensure proper synchronization:
```cpp
// Wait for previous frame's commands to complete
[commandBuffer waitUntilCompleted];
```

#### Wrong Colors

**Cause:** Pixel format mismatch.

**Fix:** Ensure consistency:
```cpp
// Layer format
metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;

// Pipeline format must match
pipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

// Texture format
textureDesc.pixelFormat = MTLPixelFormatRGBA8Unorm;  // Note: RGBA vs BGRA
```

### Input Issues

#### Keys Not Responding

**macOS:**
```objc
// View must accept first responder
- (BOOL)acceptsFirstResponder {
    return YES;
}

// Window must be key
[window makeKeyAndOrderFront:nil];
```

#### Wrong Mouse Position

**Cause:** Coordinate system mismatch.

**Fix:** Convert coordinates:
```objc
// Cocoa: origin at bottom-left
// Game: origin at top-left
float game_y = view.bounds.size.height - cocoa_y;
```

### Audio Issues

#### No Sound

**Checklist:**
1. Is audio system initialized?
2. Is volume > 0?
3. Is sound file loaded successfully?
4. On web: Has user interacted first?

**Web audio requires user interaction:**
```cpp
// Start audio context on first click
EM_ASM({
    document.addEventListener('click', function() {
        if (window.cafeAudio && window.cafeAudio.ctx.state === 'suspended') {
            window.cafeAudio.ctx.resume();
        }
    }, {once: true});
});
```

## Debugging Tools

### Print Debugging

```cpp
// Simple logging
#define LOG(fmt, ...) printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

LOG("Player position: %f, %f", player->x(), player->y());
```

### Debug Visualization

```cpp
// Draw debug info
void render_debug(Renderer* r) {
    #ifdef DEBUG
    // Draw collision boxes
    for (auto& entity : entities) {
        Rect bounds = entity.bounds();
        r->draw_rect(bounds, Color::red());
    }

    // Draw FPS
    r->draw_text(10, 10, "FPS: " + std::to_string(fps));

    // Draw pathfinding grid
    pathfinder->render_debug(r);
    #endif
}
```

### Xcode Debugger

**Set breakpoint:**
- Click line number in gutter
- Or: `Debug → Breakpoints → Add Breakpoint`

**Conditional breakpoint:**
Right-click breakpoint → Edit:
```
entity->name() == "player"
```

**Watch variables:**
- Hover over variable while paused
- Or: `Debug → Debug Workflow → Add Expression`

**View memory:**
`Debug → Debug Workflow → View Memory`

### LLDB Commands

```bash
# In Xcode console or terminal

# Print variable
(lldb) p entity->name()

# Print memory
(lldb) memory read --size 4 --count 10 pointer

# Backtrace
(lldb) bt

# Continue
(lldb) c

# Step over
(lldb) n

# Step into
(lldb) s
```

### Profiling

**Time Profiler (Xcode):**
```bash
# Find performance bottlenecks
Product → Profile → Time Profiler
```

**Manual timing:**
```cpp
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();
// Code to measure
auto end = std::chrono::high_resolution_clock::now();
auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
LOG("Operation took %lld ms", ms);
```

## Web-Specific Debugging

### Browser Console

Press F12 or Cmd+Option+I to open developer tools.

**Common errors:**

`Uncaught RuntimeError: memory access out of bounds`
- Array index out of bounds in C++ code
- Enable `-fsanitize=address` for better error messages

`LinkError: WebAssembly.instantiate()`
- WASM file corrupted or incompatible
- Rebuild with matching Emscripten version

### Source Maps

Enable for better debugging:
```cmake
target_link_options(cafe_engine PRIVATE -gsource-map)
```

Then C++ source shows in browser debugger.

## Prevention

### Compile-Time Checks
```cpp
// Static assertions
static_assert(sizeof(Vertex) == 24, "Vertex size changed!");

// Use [[nodiscard]] for functions that shouldn't ignore return
[[nodiscard]] bool load_texture(const char* path);
```

### Runtime Checks (Debug Only)
```cpp
#ifdef DEBUG
#define ASSERT(cond, msg) \
    if (!(cond)) { \
        fprintf(stderr, "Assertion failed: %s\n  %s:%d\n", msg, __FILE__, __LINE__); \
        abort(); \
    }
#else
#define ASSERT(cond, msg)
#endif

void Entity::set_position(float x, float y) {
    ASSERT(!std::isnan(x) && !std::isnan(y), "Position is NaN!");
    x_ = x;
    y_ = y;
}
```

### Code Review Checklist
- [ ] All `new` has matching `delete` (or use smart pointers)
- [ ] All resources (GPU, file handles) are released
- [ ] Array accesses are bounds-checked
- [ ] Null pointers are checked before use
- [ ] Memory isn't accessed after free

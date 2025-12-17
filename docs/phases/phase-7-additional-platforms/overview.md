# Phase 7: Additional Platforms

**Duration**: Months 18-20
**Goal**: Extend engine to all target platforms
**Deliverable**: Game runs on Mac, Web, iOS, Windows, Android

## Overview

With the game working on Mac and Web, this phase adds:
- iOS (shares Metal with Mac)
- Windows (Win32 + DirectX 11 or OpenGL)
- Android (NDK + OpenGL ES)
- Unified touch input

## Tasks

### 7.1 iOS Platform Layer

**Similarities to Mac:**
- Same Metal renderer backend
- Same C++ game code
- Objective-C++ platform layer

**Differences:**
- UIKit instead of AppKit
- Touch instead of mouse
- Different file paths
- App Store requirements

**Implementation:**
```objc
// platform/ios/ios_platform.mm

#import <UIKit/UIKit.h>
#import <MetalKit/MetalKit.h>

@interface GameViewController : UIViewController
@property (nonatomic) MTKView* metalView;
@property (nonatomic) id<MTLDevice> device;
@end

@implementation GameViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    _device = MTLCreateSystemDefaultDevice();

    _metalView = [[MTKView alloc] initWithFrame:self.view.bounds device:_device];
    _metalView.delegate = self;
    _metalView.preferredFramesPerSecond = 60;
    [self.view addSubview:_metalView];

    // Initialize engine with Metal device
    engine_initialize((__bridge void*)_device);
}

- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    engine_resize(size.width, size.height);
}

- (void)drawInMTKView:(MTKView*)view {
    engine_frame((__bridge void*)view.currentDrawable,
                 (__bridge void*)view.currentRenderPassDescriptor);
}

// Touch input
- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    for (UITouch* touch in touches) {
        CGPoint loc = [touch locationInView:self.view];
        engine_touch_begin((int)touch.hash, loc.x, loc.y);
    }
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    for (UITouch* touch in touches) {
        CGPoint loc = [touch locationInView:self.view];
        engine_touch_move((int)touch.hash, loc.x, loc.y);
    }
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    for (UITouch* touch in touches) {
        CGPoint loc = [touch locationInView:self.view];
        engine_touch_end((int)touch.hash, loc.x, loc.y);
    }
}

@end
```

**iOS File Paths:**
```cpp
const char* IOSPlatform::get_assets_path() {
    NSString* path = [[NSBundle mainBundle] resourcePath];
    return [path UTF8String];
}

const char* IOSPlatform::get_save_path() {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(
        NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* path = [paths firstObject];
    return [path UTF8String];
}
```

**Checklist:**
- [ ] Xcode project for iOS
- [ ] MTKView setup
- [ ] Touch input handling
- [ ] Proper file paths
- [ ] Game runs on iOS Simulator
- [ ] Game runs on physical device

---

### 7.2 Windows Platform Layer

**Components:**
- Win32 for windowing
- DirectX 11 or OpenGL for rendering
- XInput for gamepad (optional)

**Win32 Window:**
```cpp
// platform/windows/windows_platform.cpp

#include <windows.h>

class WindowsPlatform : public Platform {
    HWND hwnd_;
    bool should_quit_ = false;

public:
    bool initialize(const PlatformConfig& config) override {
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = "CafeEngineWindow";
        RegisterClassEx(&wc);

        hwnd_ = CreateWindowEx(
            0, "CafeEngineWindow", config.window_title,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            config.window_width, config.window_height,
            NULL, NULL, GetModuleHandle(NULL), this
        );

        ShowWindow(hwnd_, SW_SHOW);
        return true;
    }

    void poll_events() override {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                should_quit_ = true;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        WindowsPlatform* platform = /* get from hwnd user data */;

        switch (msg) {
            case WM_KEYDOWN:
                platform->push_event({InputEvent::KeyDown, translate_key(wparam)});
                return 0;
            case WM_MOUSEMOVE:
                platform->push_event({InputEvent::MouseMove,
                    .x = (float)LOWORD(lparam),
                    .y = (float)HIWORD(lparam)});
                return 0;
            case WM_CLOSE:
                PostQuitMessage(0);
                return 0;
        }
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
};
```

**DirectX 11 Renderer:**
```cpp
// renderer/d3d11/d3d11_renderer.cpp

class D3D11Renderer : public Renderer {
    ID3D11Device* device_;
    ID3D11DeviceContext* context_;
    IDXGISwapChain* swap_chain_;
    ID3D11RenderTargetView* render_target_;

public:
    bool initialize(void* platform_handle) override {
        HWND hwnd = (HWND)platform_handle;

        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 2;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION,
            &sd, &swap_chain_, &device_, nullptr, &context_
        );

        // Create render target view, shaders, etc.
        return true;
    }

    void present() override {
        swap_chain_->Present(1, 0);  // VSync on
    }
};
```

**Checklist:**
- [ ] Win32 window creation
- [ ] DirectX 11 or OpenGL initialization
- [ ] Keyboard/mouse input
- [ ] File paths (APPDATA)
- [ ] Game runs on Windows

---

### 7.3 Android Platform Layer

**Components:**
- Native Activity (C++ only, no Java)
- OpenGL ES 3.0
- Touch input
- Asset loading from APK

**Native Activity:**
```cpp
// platform/android/android_main.cpp

#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>

struct AndroidEngine {
    android_app* app;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    bool initialized = false;
};

static void handle_cmd(android_app* app, int32_t cmd) {
    AndroidEngine* engine = (AndroidEngine*)app->userData;

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            // Initialize EGL
            engine->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            eglInitialize(engine->display, nullptr, nullptr);

            // Create surface and context
            // ...

            engine->initialized = true;
            engine_initialize();
            break;

        case APP_CMD_TERM_WINDOW:
            engine_shutdown();
            eglDestroySurface(engine->display, engine->surface);
            engine->initialized = false;
            break;
    }
}

static int32_t handle_input(android_app* app, AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int action = AMotionEvent_getAction(event);
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);

        switch (action & AMOTION_EVENT_ACTION_MASK) {
            case AMOTION_EVENT_ACTION_DOWN:
                engine_touch_begin(0, x, y);
                break;
            case AMOTION_EVENT_ACTION_MOVE:
                engine_touch_move(0, x, y);
                break;
            case AMOTION_EVENT_ACTION_UP:
                engine_touch_end(0, x, y);
                break;
        }
        return 1;
    }
    return 0;
}

void android_main(android_app* app) {
    AndroidEngine engine = {};
    app->userData = &engine;
    app->onAppCmd = handle_cmd;
    app->onInputEvent = handle_input;

    while (true) {
        int events;
        android_poll_source* source;

        while (ALooper_pollAll(engine.initialized ? 0 : -1,
                              nullptr, &events, (void**)&source) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) return;
        }

        if (engine.initialized) {
            engine_frame();
            eglSwapBuffers(engine.display, engine.surface);
        }
    }
}
```

**Asset Loading from APK:**
```cpp
bool AndroidPlatform::read_file(const char* path, std::vector<uint8_t>* data) {
    AAsset* asset = AAssetManager_open(
        asset_manager_, path, AASSET_MODE_BUFFER);
    if (!asset) return false;

    size_t size = AAsset_getLength(asset);
    data->resize(size);
    AAsset_read(asset, data->data(), size);
    AAsset_close(asset);
    return true;
}
```

**Checklist:**
- [ ] Native Activity setup
- [ ] EGL/OpenGL ES initialization
- [ ] Touch input
- [ ] Asset loading from APK
- [ ] Game runs on Android emulator
- [ ] Game runs on physical device

---

### 7.4 Touch Input Abstraction

**Unified Touch API:**
```cpp
// engine/touch_input.h

struct TouchPoint {
    int id;
    float x, y;
    float start_x, start_y;  // Where touch began
    bool active;
};

class TouchInput {
    std::array<TouchPoint, 10> touches_;
    int touch_count_ = 0;

public:
    void on_touch_begin(int id, float x, float y) {
        for (auto& touch : touches_) {
            if (!touch.active) {
                touch = {id, x, y, x, y, true};
                touch_count_++;
                break;
            }
        }
    }

    // Convert touch to mouse-like events for desktop-style UI
    void emulate_mouse_for_primary_touch(Input* input) {
        if (touch_count_ > 0) {
            const auto& primary = get_primary_touch();
            // Generate mouse events
        }
    }

    // Gesture detection
    bool detect_pinch(float* zoom_delta) {
        if (touch_count_ >= 2) {
            float current_dist = distance(touches_[0], touches_[1]);
            // Compare to previous frame
            *zoom_delta = (current_dist - prev_dist_) / 100.0f;
            return true;
        }
        return false;
    }

    bool detect_swipe(Vec2* direction) {
        // Check for quick drag gesture
    }
};
```

**Platform-Specific Touch Handling:**
```cpp
#ifdef PLATFORM_IOS
    // UITouch provides touch.hash as stable ID
#endif

#ifdef PLATFORM_ANDROID
    // AMotionEvent provides pointer ID
#endif

#ifdef PLATFORM_WEB
    // TouchEvent provides touch.identifier
#endif
```

**Checklist:**
- [ ] TouchInput class
- [ ] Works on iOS
- [ ] Works on Android
- [ ] Works on Web (touch devices)
- [ ] Gesture detection (pinch, swipe)

---

### 7.5 Platform Builds & CI

**CMake Platform Detection:**
```cmake
if(APPLE)
    if(IOS)
        set(PLATFORM_IOS TRUE)
        add_compile_definitions(PLATFORM_IOS)
    else()
        set(PLATFORM_MACOS TRUE)
        add_compile_definitions(PLATFORM_MACOS)
    endif()
elseif(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    add_compile_definitions(PLATFORM_WINDOWS)
elseif(ANDROID)
    set(PLATFORM_ANDROID TRUE)
    add_compile_definitions(PLATFORM_ANDROID)
elseif(EMSCRIPTEN)
    set(PLATFORM_WEB TRUE)
    add_compile_definitions(PLATFORM_WEB)
endif()
```

**Build Scripts:**
```bash
# scripts/build_all.sh

# macOS
cmake -B build-macos -DCMAKE_BUILD_TYPE=Release
cmake --build build-macos

# iOS
cmake -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS
cmake --build build-ios --config Release

# Windows (from Windows machine)
cmake -B build-windows -G "Visual Studio 17 2022"
cmake --build build-windows --config Release

# Android
cmake -B build-android \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a
cmake --build build-android

# Web
emcmake cmake -B build-web
emmake make -C build-web
```

**Checklist:**
- [ ] Build scripts for each platform
- [ ] All platforms build successfully
- [ ] Game runs on all 5 platforms
- [ ] Platform-specific optimizations

---

## Testing Matrix

| Feature | Mac | Web | iOS | Windows | Android |
|---------|-----|-----|-----|---------|---------|
| Window/Canvas | ✓ | ✓ | ✓ | ✓ | ✓ |
| Rendering | ✓ | ✓ | ✓ | ✓ | ✓ |
| Audio | ✓ | ✓ | ✓ | ✓ | ✓ |
| Input | ✓ | ✓ | ✓ | ✓ | ✓ |
| Save/Load | ✓ | ✓ | ✓ | ✓ | ✓ |
| Touch | - | ✓ | ✓ | - | ✓ |

---

## Checklist

- [ ] iOS platform layer complete
- [ ] Windows platform layer complete
- [ ] Android platform layer complete
- [ ] Touch input abstraction
- [ ] Build scripts for all platforms
- [ ] Game tested on all platforms

## Next Phase

Proceed to [Phase 8: Content & Release](../phase-8-content-release/overview.md) to finish and ship the game.

# Phase 2: Platform Layer - Mac

**Duration**: Months 3-5
**Goal**: Create native macOS window with Metal rendering
**Deliverable**: Native window displaying animated graphics with input handling

## Overview

This phase introduces you to:
- macOS application structure (Cocoa)
- Objective-C++ (mixing C++ with Objective-C)
- Metal graphics API
- Real-time rendering fundamentals

By the end, you'll have a window showing animated graphics that responds to keyboard and mouse input.

## Tasks

### 2.1 Cocoa Window Creation

**Learn:**
- NSApplication lifecycle
- NSWindow and NSView
- Objective-C++ (.mm files)
- Event loop basics

**Implementation Steps:**

1. Create an Objective-C++ file (`main.mm`)
2. Initialize NSApplication
3. Create NSWindow with specified size
4. Create custom NSView subclass
5. Show window and run event loop

**Code Structure:**
```cpp
// platform/macos/main.mm

#import <Cocoa/Cocoa.h>

@interface GameView : NSView
@end

@implementation GameView
- (BOOL)acceptsFirstResponder { return YES; }
@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow* window;
@end

@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    NSRect frame = NSMakeRect(0, 0, 1280, 720);

    self.window = [[NSWindow alloc]
        initWithContentRect:frame
        styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
        backing:NSBackingStoreBuffered
        defer:NO];

    [self.window setTitle:@"Cafe Engine"];
    [self.window center];

    GameView* view = [[GameView alloc] initWithFrame:frame];
    [self.window setContentView:view];
    [self.window makeKeyAndOrderFront:nil];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    return YES;
}
@end

int main(int argc, const char* argv[]) {
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        AppDelegate* delegate = [[AppDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
}
```

**CMakeLists.txt Addition:**
```cmake
# For macOS, link Cocoa framework
if(APPLE)
    target_link_libraries(cafe_engine PRIVATE
        "-framework Cocoa"
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore"
    )
endif()
```

---

### 2.2 Metal Initialization

**Learn:**
- MTLDevice (GPU handle)
- MTLCommandQueue (work submission)
- CAMetalLayer (drawable surface)
- Render pipeline basics

**Implementation Steps:**

1. Add CAMetalLayer to view
2. Get default MTLDevice
3. Create MTLCommandQueue
4. Set up display link for rendering

**Code:**
```objc
// GameView with Metal support

@interface GameView : NSView
@property (nonatomic) id<MTLDevice> device;
@property (nonatomic) id<MTLCommandQueue> commandQueue;
@property (nonatomic) CAMetalLayer* metalLayer;
@property (nonatomic) CVDisplayLinkRef displayLink;
@end

@implementation GameView

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.wantsLayer = YES;

        // Get Metal device
        _device = MTLCreateSystemDefaultDevice();
        _commandQueue = [_device newCommandQueue];

        // Create Metal layer
        _metalLayer = [CAMetalLayer layer];
        _metalLayer.device = _device;
        _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        _metalLayer.framebufferOnly = YES;
        self.layer = _metalLayer;

        // Set up display link for vsync
        CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
        CVDisplayLinkSetOutputCallback(_displayLink, displayLinkCallback, (__bridge void*)self);
        CVDisplayLinkStart(_displayLink);
    }
    return self;
}

static CVReturn displayLinkCallback(CVDisplayLinkRef displayLink,
                                    const CVTimeStamp* now,
                                    const CVTimeStamp* outputTime,
                                    CVOptionFlags flagsIn,
                                    CVOptionFlags* flagsOut,
                                    void* context) {
    GameView* view = (__bridge GameView*)context;
    dispatch_async(dispatch_get_main_queue(), ^{
        [view render];
    });
    return kCVReturnSuccess;
}

- (void)render {
    // Will implement in 2.3
}

@end
```

---

### 2.3 Basic Rendering

**Learn:**
- Metal shader language (MSL)
- Vertex and fragment shaders
- Render pipeline state
- Drawing commands

**Implementation Steps:**

1. Write vertex and fragment shaders
2. Create render pipeline state
3. Create vertex buffer
4. Implement render loop

**Shader Code (shaders.metal):**
```metal
#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float2 position [[attribute(0)]];
    float4 color [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
};

vertex VertexOut vertex_main(VertexIn in [[stage_in]]) {
    VertexOut out;
    out.position = float4(in.position, 0.0, 1.0);
    out.color = in.color;
    return out;
}

fragment float4 fragment_main(VertexOut in [[stage_in]]) {
    return in.color;
}
```

**Render Implementation:**
```objc
- (void)setupPipeline {
    // Load shaders
    id<MTLLibrary> library = [_device newDefaultLibrary];
    id<MTLFunction> vertexFunc = [library newFunctionWithName:@"vertex_main"];
    id<MTLFunction> fragmentFunc = [library newFunctionWithName:@"fragment_main"];

    // Create pipeline descriptor
    MTLRenderPipelineDescriptor* desc = [[MTLRenderPipelineDescriptor alloc] init];
    desc.vertexFunction = vertexFunc;
    desc.fragmentFunction = fragmentFunc;
    desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

    // Vertex descriptor
    MTLVertexDescriptor* vertexDesc = [[MTLVertexDescriptor alloc] init];
    vertexDesc.attributes[0].format = MTLVertexFormatFloat2;
    vertexDesc.attributes[0].offset = 0;
    vertexDesc.attributes[0].bufferIndex = 0;
    vertexDesc.attributes[1].format = MTLVertexFormatFloat4;
    vertexDesc.attributes[1].offset = sizeof(float) * 2;
    vertexDesc.attributes[1].bufferIndex = 0;
    vertexDesc.layouts[0].stride = sizeof(float) * 6;
    desc.vertexDescriptor = vertexDesc;

    NSError* error = nil;
    _pipelineState = [_device newRenderPipelineStateWithDescriptor:desc error:&error];
}

- (void)render {
    id<CAMetalDrawable> drawable = [_metalLayer nextDrawable];
    if (!drawable) return;

    MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
    passDesc.colorAttachments[0].texture = drawable.texture;
    passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
    passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
    passDesc.colorAttachments[0].clearColor = MTLClearColorMake(0.1, 0.1, 0.15, 1.0);

    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:passDesc];

    [encoder setRenderPipelineState:_pipelineState];
    [encoder setVertexBuffer:_vertexBuffer offset:0 atIndex:0];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];

    [encoder endEncoding];
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
}
```

---

### 2.4 Input Handling

**Learn:**
- NSEvent types
- Key codes
- Mouse events
- Event queue pattern

**Implementation:**
```objc
// In GameView

- (void)keyDown:(NSEvent*)event {
    // Convert to engine key code
    InputEvent engineEvent;
    engineEvent.type = InputEvent::KeyDown;
    engineEvent.key_code = [event keyCode];
    [self pushEvent:engineEvent];
}

- (void)keyUp:(NSEvent*)event {
    InputEvent engineEvent;
    engineEvent.type = InputEvent::KeyUp;
    engineEvent.key_code = [event keyCode];
    [self pushEvent:engineEvent];
}

- (void)mouseDown:(NSEvent*)event {
    NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
    InputEvent engineEvent;
    engineEvent.type = InputEvent::MouseDown;
    engineEvent.mouse_button = 0;
    engineEvent.x = loc.x;
    engineEvent.y = self.bounds.size.height - loc.y; // Flip Y
    [self pushEvent:engineEvent];
}

- (void)mouseMoved:(NSEvent*)event {
    NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
    InputEvent engineEvent;
    engineEvent.type = InputEvent::MouseMove;
    engineEvent.x = loc.x;
    engineEvent.y = self.bounds.size.height - loc.y;
    [self pushEvent:engineEvent];
}

- (void)scrollWheel:(NSEvent*)event {
    InputEvent engineEvent;
    engineEvent.type = InputEvent::Scroll;
    engineEvent.x = [event scrollingDeltaX];
    engineEvent.y = [event scrollingDeltaY];
    [self pushEvent:engineEvent];
}
```

---

### 2.5 Game Loop

**Learn:**
- Fixed timestep updates
- Variable rendering
- Time measurement
- Frame rate independence

**Implementation:**
```cpp
// engine/game_loop.cpp

class MacGameLoop {
    double previous_time_;
    double accumulator_ = 0;
    static constexpr double FIXED_DT = 1.0 / 60.0;

public:
    void tick() {
        double current_time = get_time();
        double frame_time = current_time - previous_time_;
        previous_time_ = current_time;

        // Clamp large frame times
        if (frame_time > 0.25) frame_time = 0.25;

        accumulator_ += frame_time;

        // Fixed timestep updates
        while (accumulator_ >= FIXED_DT) {
            update(FIXED_DT);
            accumulator_ -= FIXED_DT;
        }

        // Render with interpolation
        double alpha = accumulator_ / FIXED_DT;
        render(alpha);
    }
};
```

---

## Milestone: Spinning Square Demo

Create a demo that shows:
- A colored square rotating on screen
- Square changes color when you press keys
- Mouse click changes rotation direction
- Scroll wheel changes size
- Frame rate display

This proves all systems are working together.

---

## Project Structure After Phase 2

```
cafe-engine/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── platform/
│   │   ├── platform.h
│   │   └── macos/
│   │       ├── macos_platform.mm
│   │       └── macos_input.mm
│   ├── renderer/
│   │   ├── renderer.h
│   │   └── metal/
│   │       ├── metal_renderer.mm
│   │       └── shaders.metal
│   └── engine/
│       ├── game_loop.cpp
│       └── input.cpp
└── docs/
```

---

## Checklist

- [ ] Window opens and displays on macOS
- [ ] Metal device and command queue initialized
- [ ] Can draw colored shapes
- [ ] Keyboard input captured
- [ ] Mouse input captured (position, clicks, scroll)
- [ ] Game loop running at stable frame rate
- [ ] Spinning square demo complete

## Next Phase

Proceed to [Phase 3: Renderer Abstraction](../phase-3-renderer-abstraction/overview.md) to add WebGL support.

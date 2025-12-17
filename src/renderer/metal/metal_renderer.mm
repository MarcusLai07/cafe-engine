#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "../renderer.h"
#include "../../platform/platform.h"

// ============================================================================
// Metal Shading Language (MSL) Shaders
// ============================================================================

static const char* kShaderSource = R"(
#include <metal_stdlib>
using namespace metal;

// Vertex input from CPU
struct VertexIn {
    float2 position [[attribute(0)]];
    float4 color    [[attribute(1)]];
};

// Data passed from vertex to fragment shader
struct VertexOut {
    float4 position [[position]];
    float4 color;
};

// Vertex shader: transform position and pass color through
vertex VertexOut vertex_main(VertexIn in [[stage_in]]) {
    VertexOut out;
    // Position in clip space (-1 to 1)
    out.position = float4(in.position, 0.0, 1.0);
    out.color = in.color;
    return out;
}

// Fragment shader: output the interpolated color
fragment float4 fragment_main(VertexOut in [[stage_in]]) {
    return in.color;
}
)";

// Vertex structure matching the shader
struct Vertex {
    float position[2];  // x, y
    float color[4];     // r, g, b, a
};

namespace cafe {

// ============================================================================
// Metal Renderer Implementation
// ============================================================================

class MetalRenderer : public Renderer {
private:
    // Core Metal objects
    id<MTLDevice> device_ = nil;
    id<MTLCommandQueue> command_queue_ = nil;
    CAMetalLayer* metal_layer_ = nil;

    // Pipeline state
    id<MTLRenderPipelineState> pipeline_state_ = nil;
    id<MTLBuffer> vertex_buffer_ = nil;

    // Current frame state (use __strong to ensure ARC retains them properly)
    __strong id<CAMetalDrawable> current_drawable_ = nil;
    __strong id<MTLCommandBuffer> current_command_buffer_ = nil;
    __strong id<MTLTexture> current_texture_ = nil;  // Cache texture to avoid accessing drawable
    __strong id<MTLRenderCommandEncoder> current_encoder_ = nil;  // For draw commands
    bool frame_valid_ = false;  // Track if current frame is valid for rendering

    // Settings
    Color clear_color_ = Color::cornflower_blue();

public:
    MetalRenderer() = default;
    ~MetalRenderer() override {
        shutdown();
    }

    bool initialize(Window* window) override {
        @autoreleasepool {
            // Get the Metal layer from the window's view
            NSView* view = (__bridge NSView*)window->native_handle();
            if (!view) {
                return false;
            }

            // The view should already have a CAMetalLayer (set up in macos_platform.mm)
            metal_layer_ = (CAMetalLayer*)[view layer];
            if (!metal_layer_ || ![metal_layer_ isKindOfClass:[CAMetalLayer class]]) {
                return false;
            }

            // Create Metal device (GPU)
            device_ = MTLCreateSystemDefaultDevice();
            if (!device_) {
                return false;
            }

            // Configure the Metal layer
            metal_layer_.device = device_;
            metal_layer_.pixelFormat = MTLPixelFormatBGRA8Unorm;
            metal_layer_.framebufferOnly = YES;

            // Enable display sync (vsync)
            metal_layer_.displaySyncEnabled = YES;

            // Create command queue
            command_queue_ = [device_ newCommandQueue];
            if (!command_queue_) {
                return false;
            }

            // Compile shaders
            NSError* error = nil;
            NSString* shader_source = [NSString stringWithUTF8String:kShaderSource];
            id<MTLLibrary> library = [device_ newLibraryWithSource:shader_source
                                                           options:nil
                                                             error:&error];
            if (!library) {
                NSLog(@"Failed to compile shaders: %@", error);
                return false;
            }

            id<MTLFunction> vertex_func = [library newFunctionWithName:@"vertex_main"];
            id<MTLFunction> fragment_func = [library newFunctionWithName:@"fragment_main"];
            if (!vertex_func || !fragment_func) {
                NSLog(@"Failed to find shader functions");
                return false;
            }

            // Create vertex descriptor
            MTLVertexDescriptor* vertex_desc = [[MTLVertexDescriptor alloc] init];
            // Position attribute (float2)
            vertex_desc.attributes[0].format = MTLVertexFormatFloat2;
            vertex_desc.attributes[0].offset = offsetof(Vertex, position);
            vertex_desc.attributes[0].bufferIndex = 0;
            // Color attribute (float4)
            vertex_desc.attributes[1].format = MTLVertexFormatFloat4;
            vertex_desc.attributes[1].offset = offsetof(Vertex, color);
            vertex_desc.attributes[1].bufferIndex = 0;
            // Buffer layout
            vertex_desc.layouts[0].stride = sizeof(Vertex);
            vertex_desc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

            // Create render pipeline
            MTLRenderPipelineDescriptor* pipeline_desc = [[MTLRenderPipelineDescriptor alloc] init];
            pipeline_desc.vertexFunction = vertex_func;
            pipeline_desc.fragmentFunction = fragment_func;
            pipeline_desc.vertexDescriptor = vertex_desc;
            pipeline_desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

            // Enable alpha blending
            pipeline_desc.colorAttachments[0].blendingEnabled = YES;
            pipeline_desc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
            pipeline_desc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
            pipeline_desc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
            pipeline_desc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

            pipeline_state_ = [device_ newRenderPipelineStateWithDescriptor:pipeline_desc
                                                                      error:&error];
            if (!pipeline_state_) {
                NSLog(@"Failed to create pipeline state: %@", error);
                return false;
            }

            // Create vertex buffer (enough for a quad = 6 vertices)
            vertex_buffer_ = [device_ newBufferWithLength:sizeof(Vertex) * 6
                                                  options:MTLResourceStorageModeShared];
            if (!vertex_buffer_) {
                NSLog(@"Failed to create vertex buffer");
                return false;
            }

            NSLog(@"Metal initialized: %@", device_.name);
            return true;
        }
    }

    void shutdown() override {
        @autoreleasepool {
            frame_valid_ = false;
            current_encoder_ = nil;
            current_texture_ = nil;
            current_drawable_ = nil;
            current_command_buffer_ = nil;
            vertex_buffer_ = nil;
            pipeline_state_ = nil;
            command_queue_ = nil;
            device_ = nil;
            metal_layer_ = nil;
        }
    }

    void begin_frame() override {
        frame_valid_ = false;  // Reset at start of frame
        current_texture_ = nil;
        current_encoder_ = nil;

        // Safety check - ensure layer is still valid
        if (!metal_layer_ || !command_queue_) {
            return;
        }

        // Check if layer's device is still valid (can become nil when window closes)
        if (!metal_layer_.device) {
            return;
        }

        // Get next drawable from the layer
        current_drawable_ = [metal_layer_ nextDrawable];
        if (!current_drawable_) {
            return;
        }

        // Cache the texture immediately while drawable is valid
        current_texture_ = current_drawable_.texture;
        if (!current_texture_) {
            current_drawable_ = nil;
            return;
        }

        // Create command buffer for this frame
        current_command_buffer_ = [command_queue_ commandBuffer];
        if (!current_command_buffer_) {
            current_drawable_ = nil;
            current_texture_ = nil;
            return;
        }

        // All resources acquired successfully
        frame_valid_ = true;
    }

    void end_frame() override {
        // Capture to locals
        id<CAMetalDrawable> drawable = current_drawable_;
        id<MTLCommandBuffer> cmd_buffer = current_command_buffer_;
        id<MTLRenderCommandEncoder> encoder = current_encoder_;

        // Clear class members first
        current_drawable_ = nil;
        current_command_buffer_ = nil;
        current_texture_ = nil;
        current_encoder_ = nil;
        frame_valid_ = false;

        // End encoder if still open
        if (encoder) {
            [encoder endEncoding];
        }

        if (!drawable || !cmd_buffer) {
            return;
        }

        // Present the drawable and commit (using locals)
        [cmd_buffer presentDrawable:drawable];
        [cmd_buffer commit];
    }

    void set_clear_color(const Color& color) override {
        clear_color_ = color;
    }

    void clear() override {
        // Capture to locals at the start to avoid ARC issues with C++ class members
        id<MTLTexture> texture = current_texture_;
        id<MTLCommandBuffer> cmd_buffer = current_command_buffer_;

        // Check frame validity - must have successfully acquired resources in begin_frame()
        if (!frame_valid_ || !texture || !cmd_buffer) {
            return;
        }

        // Create render pass descriptor using local texture reference
        MTLRenderPassDescriptor* pass_desc = [MTLRenderPassDescriptor renderPassDescriptor];
        pass_desc.colorAttachments[0].texture = texture;
        pass_desc.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass_desc.colorAttachments[0].storeAction = MTLStoreActionStore;
        pass_desc.colorAttachments[0].clearColor = MTLClearColorMake(
            clear_color_.r,
            clear_color_.g,
            clear_color_.b,
            clear_color_.a
        );

        // Create encoder and keep it open for draw commands
        current_encoder_ = [cmd_buffer renderCommandEncoderWithDescriptor:pass_desc];
    }

    void draw_quad(Vec2 position, Vec2 size, const Color& color) override {
        // Capture to locals
        id<MTLRenderCommandEncoder> encoder = current_encoder_;
        id<MTLRenderPipelineState> pipeline = pipeline_state_;
        id<MTLBuffer> vbuffer = vertex_buffer_;

        if (!frame_valid_ || !encoder || !pipeline || !vbuffer) {
            return;
        }

        // Calculate quad corners in clip space (-1 to 1)
        // Position is the center, size is width/height
        float left = position.x - size.x / 2.0f;
        float right = position.x + size.x / 2.0f;
        float bottom = position.y - size.y / 2.0f;
        float top = position.y + size.y / 2.0f;

        // Create 6 vertices for two triangles (quad)
        Vertex vertices[6] = {
            // Triangle 1 (bottom-left, bottom-right, top-right)
            {{left, bottom}, {color.r, color.g, color.b, color.a}},
            {{right, bottom}, {color.r, color.g, color.b, color.a}},
            {{right, top}, {color.r, color.g, color.b, color.a}},
            // Triangle 2 (bottom-left, top-right, top-left)
            {{left, bottom}, {color.r, color.g, color.b, color.a}},
            {{right, top}, {color.r, color.g, color.b, color.a}},
            {{left, top}, {color.r, color.g, color.b, color.a}},
        };

        // Copy vertex data to buffer
        memcpy([vbuffer contents], vertices, sizeof(vertices));

        // Set pipeline and vertex buffer, then draw
        [encoder setRenderPipelineState:pipeline];
        [encoder setVertexBuffer:vbuffer offset:0 atIndex:0];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
    }

    const char* backend_name() const override {
        return "Metal";
    }

    // Accessors for future use (shaders, etc.)
    id<MTLDevice> device() const { return device_; }
    id<MTLCommandQueue> command_queue() const { return command_queue_; }
};

// ============================================================================
// Factory Function
// ============================================================================

std::unique_ptr<Renderer> create_renderer() {
    return std::make_unique<MetalRenderer>();
}

} // namespace cafe

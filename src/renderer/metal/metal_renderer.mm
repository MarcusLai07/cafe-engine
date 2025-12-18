#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "../renderer.h"
#include "../../platform/platform.h"
#include <vector>
#include <unordered_map>
#include <cmath>

// ============================================================================
// Metal Shading Language (MSL) Shaders
// ============================================================================

static const char* kShaderSource = R"(
#include <metal_stdlib>
using namespace metal;

// Uniforms passed from CPU
struct Uniforms {
    float4x4 projection;
};

// Vertex input from CPU
struct VertexIn {
    float2 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
    float4 color    [[attribute(2)]];
};

// Data passed from vertex to fragment shader
struct VertexOut {
    float4 position [[position]];
    float2 texcoord;
    float4 color;
};

// Vertex shader: transform position and pass data through
vertex VertexOut vertex_main(VertexIn in [[stage_in]],
                              constant Uniforms& uniforms [[buffer(1)]]) {
    VertexOut out;
    out.position = uniforms.projection * float4(in.position, 0.0, 1.0);
    out.texcoord = in.texcoord;
    out.color = in.color;
    return out;
}

// Fragment shader for colored quads (no texture)
fragment float4 fragment_color(VertexOut in [[stage_in]]) {
    return in.color;
}

// Fragment shader for textured quads
fragment float4 fragment_textured(VertexOut in [[stage_in]],
                                   texture2d<float> tex [[texture(0)]],
                                   sampler samp [[sampler(0)]]) {
    float4 tex_color = tex.sample(samp, in.texcoord);
    return tex_color * in.color;
}
)";

// Vertex structure matching the shader
struct Vertex {
    float position[2];  // x, y
    float texcoord[2];  // u, v
    float color[4];     // r, g, b, a
};

// Uniforms structure matching the shader
struct Uniforms {
    float projection[16];  // 4x4 matrix
};

namespace cafe {

// ============================================================================
// Internal texture storage
// ============================================================================

struct TextureData {
    id<MTLTexture> texture;
    TextureInfo info;
};

// ============================================================================
// Metal Renderer Implementation
// ============================================================================

class MetalRenderer : public Renderer {
private:
    // Core Metal objects
    id<MTLDevice> device_ = nil;
    id<MTLCommandQueue> command_queue_ = nil;
    CAMetalLayer* metal_layer_ = nil;

    // Pipeline states
    id<MTLRenderPipelineState> color_pipeline_ = nil;      // For colored quads
    id<MTLRenderPipelineState> textured_pipeline_ = nil;   // For textured quads
    id<MTLBuffer> vertex_buffer_ = nil;
    id<MTLBuffer> uniform_buffer_ = nil;
    id<MTLSamplerState> sampler_nearest_ = nil;
    id<MTLSamplerState> sampler_linear_ = nil;

    // Batch rendering
    static constexpr size_t MAX_BATCH_VERTICES = 6 * 1000;  // 1000 quads
    std::vector<Vertex> batch_vertices_;
    TextureHandle current_batch_texture_ = INVALID_TEXTURE;
    bool batching_ = false;

    // Texture storage
    std::unordered_map<TextureHandle, TextureData> textures_;
    TextureHandle next_texture_id_ = 1;

    // Current frame state
    __strong id<CAMetalDrawable> current_drawable_ = nil;
    __strong id<MTLCommandBuffer> current_command_buffer_ = nil;
    __strong id<MTLTexture> current_texture_ = nil;
    __strong id<MTLRenderCommandEncoder> current_encoder_ = nil;
    bool frame_valid_ = false;

    // Settings
    Color clear_color_ = Color::cornflower_blue();
    Uniforms uniforms_;
    int viewport_width_ = 0;
    int viewport_height_ = 0;

public:
    MetalRenderer() {
        batch_vertices_.reserve(MAX_BATCH_VERTICES);
        // Initialize projection to identity
        std::memset(&uniforms_, 0, sizeof(uniforms_));
        uniforms_.projection[0] = 1.0f;
        uniforms_.projection[5] = 1.0f;
        uniforms_.projection[10] = 1.0f;
        uniforms_.projection[15] = 1.0f;
    }

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

            metal_layer_ = (CAMetalLayer*)[view layer];
            if (!metal_layer_ || ![metal_layer_ isKindOfClass:[CAMetalLayer class]]) {
                return false;
            }

            // Create Metal device
            device_ = MTLCreateSystemDefaultDevice();
            if (!device_) {
                return false;
            }

            // Configure the Metal layer
            metal_layer_.device = device_;
            metal_layer_.pixelFormat = MTLPixelFormatBGRA8Unorm;
            metal_layer_.framebufferOnly = YES;
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
            id<MTLFunction> fragment_color = [library newFunctionWithName:@"fragment_color"];
            id<MTLFunction> fragment_textured = [library newFunctionWithName:@"fragment_textured"];

            if (!vertex_func || !fragment_color || !fragment_textured) {
                NSLog(@"Failed to find shader functions");
                return false;
            }

            // Create vertex descriptor
            MTLVertexDescriptor* vertex_desc = [[MTLVertexDescriptor alloc] init];
            // Position (float2)
            vertex_desc.attributes[0].format = MTLVertexFormatFloat2;
            vertex_desc.attributes[0].offset = offsetof(Vertex, position);
            vertex_desc.attributes[0].bufferIndex = 0;
            // Texcoord (float2)
            vertex_desc.attributes[1].format = MTLVertexFormatFloat2;
            vertex_desc.attributes[1].offset = offsetof(Vertex, texcoord);
            vertex_desc.attributes[1].bufferIndex = 0;
            // Color (float4)
            vertex_desc.attributes[2].format = MTLVertexFormatFloat4;
            vertex_desc.attributes[2].offset = offsetof(Vertex, color);
            vertex_desc.attributes[2].bufferIndex = 0;
            // Layout
            vertex_desc.layouts[0].stride = sizeof(Vertex);
            vertex_desc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

            // Create color pipeline (no texture)
            MTLRenderPipelineDescriptor* color_desc = [[MTLRenderPipelineDescriptor alloc] init];
            color_desc.vertexFunction = vertex_func;
            color_desc.fragmentFunction = fragment_color;
            color_desc.vertexDescriptor = vertex_desc;
            color_desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
            color_desc.colorAttachments[0].blendingEnabled = YES;
            color_desc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
            color_desc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
            color_desc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
            color_desc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

            color_pipeline_ = [device_ newRenderPipelineStateWithDescriptor:color_desc error:&error];
            if (!color_pipeline_) {
                NSLog(@"Failed to create color pipeline: %@", error);
                return false;
            }

            // Create textured pipeline
            MTLRenderPipelineDescriptor* tex_desc = [[MTLRenderPipelineDescriptor alloc] init];
            tex_desc.vertexFunction = vertex_func;
            tex_desc.fragmentFunction = fragment_textured;
            tex_desc.vertexDescriptor = vertex_desc;
            tex_desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
            tex_desc.colorAttachments[0].blendingEnabled = YES;
            tex_desc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
            tex_desc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
            tex_desc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
            tex_desc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

            textured_pipeline_ = [device_ newRenderPipelineStateWithDescriptor:tex_desc error:&error];
            if (!textured_pipeline_) {
                NSLog(@"Failed to create textured pipeline: %@", error);
                return false;
            }

            // Create vertex buffer for batch rendering
            vertex_buffer_ = [device_ newBufferWithLength:sizeof(Vertex) * MAX_BATCH_VERTICES
                                                  options:MTLResourceStorageModeShared];
            if (!vertex_buffer_) {
                return false;
            }

            // Create uniform buffer
            uniform_buffer_ = [device_ newBufferWithLength:sizeof(Uniforms)
                                                   options:MTLResourceStorageModeShared];
            if (!uniform_buffer_) {
                return false;
            }

            // Create samplers
            MTLSamplerDescriptor* sampler_desc = [[MTLSamplerDescriptor alloc] init];
            sampler_desc.minFilter = MTLSamplerMinMagFilterNearest;
            sampler_desc.magFilter = MTLSamplerMinMagFilterNearest;
            sampler_desc.sAddressMode = MTLSamplerAddressModeClampToEdge;
            sampler_desc.tAddressMode = MTLSamplerAddressModeClampToEdge;
            sampler_nearest_ = [device_ newSamplerStateWithDescriptor:sampler_desc];

            sampler_desc.minFilter = MTLSamplerMinMagFilterLinear;
            sampler_desc.magFilter = MTLSamplerMinMagFilterLinear;
            sampler_linear_ = [device_ newSamplerStateWithDescriptor:sampler_desc];

            // Set default viewport/projection
            viewport_width_ = window->width();
            viewport_height_ = window->height();
            set_projection(-1.0f, 1.0f, -1.0f, 1.0f);

            NSLog(@"Metal initialized: %@", device_.name);
            return true;
        }
    }

    void shutdown() override {
        @autoreleasepool {
            // Destroy all textures
            textures_.clear();

            frame_valid_ = false;
            current_encoder_ = nil;
            current_texture_ = nil;
            current_drawable_ = nil;
            current_command_buffer_ = nil;
            vertex_buffer_ = nil;
            uniform_buffer_ = nil;
            color_pipeline_ = nil;
            textured_pipeline_ = nil;
            sampler_nearest_ = nil;
            sampler_linear_ = nil;
            command_queue_ = nil;
            device_ = nil;
            metal_layer_ = nil;
        }
    }

    void begin_frame() override {
        frame_valid_ = false;
        current_texture_ = nil;
        current_encoder_ = nil;

        if (!metal_layer_ || !command_queue_ || !metal_layer_.device) {
            return;
        }

        current_drawable_ = [metal_layer_ nextDrawable];
        if (!current_drawable_) {
            return;
        }

        current_texture_ = current_drawable_.texture;
        if (!current_texture_) {
            current_drawable_ = nil;
            return;
        }

        current_command_buffer_ = [command_queue_ commandBuffer];
        if (!current_command_buffer_) {
            current_drawable_ = nil;
            current_texture_ = nil;
            return;
        }

        frame_valid_ = true;
    }

    void end_frame() override {
        // Flush any pending batch
        if (batching_) {
            flush_batch();
        }

        id<CAMetalDrawable> drawable = current_drawable_;
        id<MTLCommandBuffer> cmd_buffer = current_command_buffer_;
        id<MTLRenderCommandEncoder> encoder = current_encoder_;

        current_drawable_ = nil;
        current_command_buffer_ = nil;
        current_texture_ = nil;
        current_encoder_ = nil;
        frame_valid_ = false;

        if (encoder) {
            [encoder endEncoding];
        }

        if (!drawable || !cmd_buffer) {
            return;
        }

        [cmd_buffer presentDrawable:drawable];
        [cmd_buffer commit];
    }

    void set_clear_color(const Color& color) override {
        clear_color_ = color;
    }

    void clear() override {
        id<MTLTexture> texture = current_texture_;
        id<MTLCommandBuffer> cmd_buffer = current_command_buffer_;

        if (!frame_valid_ || !texture || !cmd_buffer) {
            return;
        }

        MTLRenderPassDescriptor* pass_desc = [MTLRenderPassDescriptor renderPassDescriptor];
        pass_desc.colorAttachments[0].texture = texture;
        pass_desc.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass_desc.colorAttachments[0].storeAction = MTLStoreActionStore;
        pass_desc.colorAttachments[0].clearColor = MTLClearColorMake(
            clear_color_.r, clear_color_.g, clear_color_.b, clear_color_.a);

        current_encoder_ = [cmd_buffer renderCommandEncoderWithDescriptor:pass_desc];

        // Upload uniforms
        if (current_encoder_ && uniform_buffer_) {
            std::memcpy([uniform_buffer_ contents], &uniforms_, sizeof(uniforms_));
        }
    }

    void set_viewport(int x, int y, int width, int height) override {
        viewport_width_ = width;
        viewport_height_ = height;
        // Metal viewport is set when encoding draw commands
        (void)x;
        (void)y;
    }

    void set_projection(float left, float right, float bottom, float top) override {
        // Orthographic projection matrix
        float w = right - left;
        float h = top - bottom;
        std::memset(&uniforms_, 0, sizeof(uniforms_));
        uniforms_.projection[0] = 2.0f / w;
        uniforms_.projection[5] = 2.0f / h;
        uniforms_.projection[10] = -1.0f;
        uniforms_.projection[12] = -(right + left) / w;
        uniforms_.projection[13] = -(top + bottom) / h;
        uniforms_.projection[15] = 1.0f;
    }

    TextureHandle create_texture(const uint8_t* pixels, const TextureInfo& info) override {
        if (!device_ || !pixels || info.width <= 0 || info.height <= 0) {
            return INVALID_TEXTURE;
        }

        @autoreleasepool {
            MTLTextureDescriptor* desc = [[MTLTextureDescriptor alloc] init];
            desc.pixelFormat = MTLPixelFormatRGBA8Unorm;
            desc.width = info.width;
            desc.height = info.height;
            desc.usage = MTLTextureUsageShaderRead;

            id<MTLTexture> tex = [device_ newTextureWithDescriptor:desc];
            if (!tex) {
                return INVALID_TEXTURE;
            }

            MTLRegion region = MTLRegionMake2D(0, 0, info.width, info.height);
            [tex replaceRegion:region mipmapLevel:0 withBytes:pixels bytesPerRow:info.width * 4];

            TextureHandle handle = next_texture_id_++;
            textures_[handle] = {tex, info};
            return handle;
        }
    }

    void destroy_texture(TextureHandle texture) override {
        textures_.erase(texture);
    }

    TextureInfo get_texture_info(TextureHandle texture) const override {
        auto it = textures_.find(texture);
        if (it != textures_.end()) {
            return it->second.info;
        }
        return {};
    }

    void draw_quad(Vec2 position, Vec2 size, const Color& color) override {
        id<MTLRenderCommandEncoder> encoder = current_encoder_;
        if (!frame_valid_ || !encoder || !color_pipeline_ || !vertex_buffer_) {
            return;
        }

        float left = position.x - size.x / 2.0f;
        float right = position.x + size.x / 2.0f;
        float bottom = position.y - size.y / 2.0f;
        float top = position.y + size.y / 2.0f;

        Vertex vertices[6] = {
            {{left, bottom}, {0, 0}, {color.r, color.g, color.b, color.a}},
            {{right, bottom}, {1, 0}, {color.r, color.g, color.b, color.a}},
            {{right, top}, {1, 1}, {color.r, color.g, color.b, color.a}},
            {{left, bottom}, {0, 0}, {color.r, color.g, color.b, color.a}},
            {{right, top}, {1, 1}, {color.r, color.g, color.b, color.a}},
            {{left, top}, {0, 1}, {color.r, color.g, color.b, color.a}},
        };

        std::memcpy([vertex_buffer_ contents], vertices, sizeof(vertices));

        [encoder setRenderPipelineState:color_pipeline_];
        [encoder setVertexBuffer:vertex_buffer_ offset:0 atIndex:0];
        [encoder setVertexBuffer:uniform_buffer_ offset:0 atIndex:1];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
    }

    void draw_textured_quad(Vec2 position, Vec2 size,
                            const TextureRegion& region,
                            const Color& tint) override {
        id<MTLRenderCommandEncoder> encoder = current_encoder_;
        auto it = textures_.find(region.texture);
        if (!frame_valid_ || !encoder || !textured_pipeline_ || it == textures_.end()) {
            return;
        }

        float left = position.x - size.x / 2.0f;
        float right = position.x + size.x / 2.0f;
        float bottom = position.y - size.y / 2.0f;
        float top = position.y + size.y / 2.0f;

        Vertex vertices[6] = {
            {{left, bottom}, {region.u0, region.v1}, {tint.r, tint.g, tint.b, tint.a}},
            {{right, bottom}, {region.u1, region.v1}, {tint.r, tint.g, tint.b, tint.a}},
            {{right, top}, {region.u1, region.v0}, {tint.r, tint.g, tint.b, tint.a}},
            {{left, bottom}, {region.u0, region.v1}, {tint.r, tint.g, tint.b, tint.a}},
            {{right, top}, {region.u1, region.v0}, {tint.r, tint.g, tint.b, tint.a}},
            {{left, top}, {region.u0, region.v0}, {tint.r, tint.g, tint.b, tint.a}},
        };

        std::memcpy([vertex_buffer_ contents], vertices, sizeof(vertices));

        [encoder setRenderPipelineState:textured_pipeline_];
        [encoder setVertexBuffer:vertex_buffer_ offset:0 atIndex:0];
        [encoder setVertexBuffer:uniform_buffer_ offset:0 atIndex:1];
        [encoder setFragmentTexture:it->second.texture atIndex:0];
        [encoder setFragmentSamplerState:sampler_nearest_ atIndex:0];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
    }

    void begin_batch() override {
        batch_vertices_.clear();
        current_batch_texture_ = INVALID_TEXTURE;
        batching_ = true;
    }

    void draw_sprite(const Sprite& sprite) override {
        if (!batching_) return;

        // Flush if texture changes or batch is full
        if (current_batch_texture_ != sprite.region.texture) {
            if (!batch_vertices_.empty()) {
                flush_batch();
            }
            current_batch_texture_ = sprite.region.texture;
        }

        if (batch_vertices_.size() + 6 > MAX_BATCH_VERTICES) {
            flush_batch();
        }

        // Calculate corners with rotation around center
        float cx = sprite.position.x;
        float cy = sprite.position.y;
        float w2 = sprite.size.x / 2.0f;
        float h2 = sprite.size.y / 2.0f;
        float cos_r = std::cos(sprite.rotation);
        float sin_r = std::sin(sprite.rotation);

        auto rot = [&](float ox, float oy) -> std::pair<float, float> {
            return {cx + ox * cos_r - oy * sin_r, cy + ox * sin_r + oy * cos_r};
        };

        auto [x0, y0] = rot(-w2, -h2);  // bottom-left
        auto [x1, y1] = rot(w2, -h2);   // bottom-right
        auto [x2, y2] = rot(w2, h2);    // top-right
        auto [x3, y3] = rot(-w2, h2);   // top-left

        const auto& r = sprite.region;
        const auto& c = sprite.tint;

        batch_vertices_.push_back({{(float)x0, (float)y0}, {r.u0, r.v1}, {c.r, c.g, c.b, c.a}});
        batch_vertices_.push_back({{(float)x1, (float)y1}, {r.u1, r.v1}, {c.r, c.g, c.b, c.a}});
        batch_vertices_.push_back({{(float)x2, (float)y2}, {r.u1, r.v0}, {c.r, c.g, c.b, c.a}});
        batch_vertices_.push_back({{(float)x0, (float)y0}, {r.u0, r.v1}, {c.r, c.g, c.b, c.a}});
        batch_vertices_.push_back({{(float)x2, (float)y2}, {r.u1, r.v0}, {c.r, c.g, c.b, c.a}});
        batch_vertices_.push_back({{(float)x3, (float)y3}, {r.u0, r.v0}, {c.r, c.g, c.b, c.a}});
    }

    void end_batch() override {
        if (!batch_vertices_.empty()) {
            flush_batch();
        }
        batching_ = false;
    }

    const char* backend_name() const override {
        return "Metal";
    }

    int max_texture_size() const override {
        if (!device_) return 0;
        // Most modern Apple GPUs support at least 16384
        return 16384;
    }

private:
    void flush_batch() {
        if (batch_vertices_.empty()) return;

        id<MTLRenderCommandEncoder> encoder = current_encoder_;
        if (!frame_valid_ || !encoder || !vertex_buffer_) {
            batch_vertices_.clear();
            return;
        }

        std::memcpy([vertex_buffer_ contents], batch_vertices_.data(),
                    batch_vertices_.size() * sizeof(Vertex));

        // Choose pipeline based on texture
        auto it = textures_.find(current_batch_texture_);
        if (it != textures_.end()) {
            [encoder setRenderPipelineState:textured_pipeline_];
            [encoder setFragmentTexture:it->second.texture atIndex:0];
            [encoder setFragmentSamplerState:sampler_nearest_ atIndex:0];
        } else {
            [encoder setRenderPipelineState:color_pipeline_];
        }

        [encoder setVertexBuffer:vertex_buffer_ offset:0 atIndex:0];
        [encoder setVertexBuffer:uniform_buffer_ offset:0 atIndex:1];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                    vertexStart:0
                    vertexCount:batch_vertices_.size()];

        batch_vertices_.clear();
    }
};

// ============================================================================
// Factory Function
// ============================================================================

std::unique_ptr<Renderer> create_renderer() {
    return std::make_unique<MetalRenderer>();
}

} // namespace cafe

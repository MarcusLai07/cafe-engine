#ifdef __EMSCRIPTEN__

#include "../renderer.h"
#include "../../platform/platform.h"

#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>

#include <vector>
#include <unordered_map>
#include <cmath>
#include <cstring>

// ============================================================================
// WebGL Shaders (GLSL ES 3.0)
// ============================================================================

static const char* kVertexShader = R"(#version 300 es
precision highp float;

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

static const char* kFragmentColor = R"(#version 300 es
precision highp float;

in vec2 v_texcoord;
in vec4 v_color;

out vec4 fragColor;

void main() {
    fragColor = v_color;
}
)";

static const char* kFragmentTextured = R"(#version 300 es
precision highp float;

in vec2 v_texcoord;
in vec4 v_color;

uniform sampler2D u_texture;

out vec4 fragColor;

void main() {
    vec4 tex_color = texture(u_texture, v_texcoord);
    fragColor = tex_color * v_color;
}
)";

// Vertex structure
struct Vertex {
    float position[2];
    float texcoord[2];
    float color[4];
};

namespace cafe {

// ============================================================================
// Internal texture storage
// ============================================================================

struct TextureData {
    GLuint texture;
    TextureInfo info;
};

// ============================================================================
// WebGL Renderer Implementation
// ============================================================================

class WebGLRenderer : public Renderer {
private:
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE gl_context_ = 0;

    // Shader programs
    GLuint color_program_ = 0;
    GLuint textured_program_ = 0;
    GLint color_proj_loc_ = -1;
    GLint textured_proj_loc_ = -1;
    GLint textured_sampler_loc_ = -1;

    // Buffers
    GLuint vao_ = 0;
    GLuint vbo_ = 0;

    // Batch rendering
    static constexpr size_t MAX_BATCH_VERTICES = 6 * 1000;
    std::vector<Vertex> batch_vertices_;
    TextureHandle current_batch_texture_ = INVALID_TEXTURE;
    bool batching_ = false;

    // Texture storage
    std::unordered_map<TextureHandle, TextureData> textures_;
    TextureHandle next_texture_id_ = 1;

    // State
    Color clear_color_ = Color::cornflower_blue();
    float projection_[16];
    int viewport_width_ = 0;
    int viewport_height_ = 0;

    GLuint compile_shader(GLenum type, const char* source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char log[512];
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            emscripten_log(EM_LOG_ERROR, "Shader compile error: %s", log);
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    GLuint create_program(const char* vs_source, const char* fs_source) {
        GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_source);
        GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_source);
        if (!vs || !fs) return 0;

        GLuint program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        glDeleteShader(vs);
        glDeleteShader(fs);

        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char log[512];
            glGetProgramInfoLog(program, sizeof(log), nullptr, log);
            emscripten_log(EM_LOG_ERROR, "Program link error: %s", log);
            glDeleteProgram(program);
            return 0;
        }
        return program;
    }

public:
    WebGLRenderer() {
        batch_vertices_.reserve(MAX_BATCH_VERTICES);
        std::memset(projection_, 0, sizeof(projection_));
        projection_[0] = 1.0f;
        projection_[5] = 1.0f;
        projection_[10] = 1.0f;
        projection_[15] = 1.0f;
    }

    ~WebGLRenderer() override {
        shutdown();
    }

    bool initialize(Window* window) override {
        viewport_width_ = window->width();
        viewport_height_ = window->height();

        // Create WebGL2 context
        EmscriptenWebGLContextAttributes attrs;
        emscripten_webgl_init_context_attributes(&attrs);
        attrs.majorVersion = 2;
        attrs.minorVersion = 0;
        attrs.alpha = false;
        attrs.depth = false;
        attrs.stencil = false;
        attrs.antialias = false;
        attrs.preserveDrawingBuffer = false;

        gl_context_ = emscripten_webgl_create_context("#canvas", &attrs);
        if (gl_context_ <= 0) {
            emscripten_log(EM_LOG_ERROR, "Failed to create WebGL2 context");
            return false;
        }

        emscripten_webgl_make_context_current(gl_context_);

        // Create shader programs
        color_program_ = create_program(kVertexShader, kFragmentColor);
        textured_program_ = create_program(kVertexShader, kFragmentTextured);
        if (!color_program_ || !textured_program_) {
            return false;
        }

        color_proj_loc_ = glGetUniformLocation(color_program_, "u_projection");
        textured_proj_loc_ = glGetUniformLocation(textured_program_, "u_projection");
        textured_sampler_loc_ = glGetUniformLocation(textured_program_, "u_texture");

        // Create VAO and VBO
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * MAX_BATCH_VERTICES, nullptr, GL_DYNAMIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(0);

        // Texcoord attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));
        glEnableVertexAttribArray(1);

        // Color attribute
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);

        // Enable blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        set_projection(-1.0f, 1.0f, -1.0f, 1.0f);

        emscripten_log(EM_LOG_CONSOLE, "WebGL2 initialized");
        return true;
    }

    void shutdown() override {
        for (auto& [handle, data] : textures_) {
            glDeleteTextures(1, &data.texture);
        }
        textures_.clear();

        if (vbo_) glDeleteBuffers(1, &vbo_);
        if (vao_) glDeleteVertexArrays(1, &vao_);
        if (color_program_) glDeleteProgram(color_program_);
        if (textured_program_) glDeleteProgram(textured_program_);

        if (gl_context_) {
            emscripten_webgl_destroy_context(gl_context_);
            gl_context_ = 0;
        }
    }

    void begin_frame() override {
        emscripten_webgl_make_context_current(gl_context_);
        glViewport(0, 0, viewport_width_, viewport_height_);
    }

    void end_frame() override {
        if (batching_) {
            flush_batch();
        }
        // WebGL auto-presents
    }

    void set_clear_color(const Color& color) override {
        clear_color_ = color;
    }

    void clear() override {
        glClearColor(clear_color_.r, clear_color_.g, clear_color_.b, clear_color_.a);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void set_viewport(int x, int y, int width, int height) override {
        viewport_width_ = width;
        viewport_height_ = height;
        glViewport(x, y, width, height);
    }

    void set_projection(float left, float right, float bottom, float top) override {
        float w = right - left;
        float h = top - bottom;
        std::memset(projection_, 0, sizeof(projection_));
        projection_[0] = 2.0f / w;
        projection_[5] = 2.0f / h;
        projection_[10] = -1.0f;
        projection_[12] = -(right + left) / w;
        projection_[13] = -(top + bottom) / h;
        projection_[15] = 1.0f;
    }

    TextureHandle create_texture(const uint8_t* pixels, const TextureInfo& info) override {
        if (!pixels || info.width <= 0 || info.height <= 0) {
            return INVALID_TEXTURE;
        }

        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        GLenum filter = (info.filter == TextureFilter::Nearest) ? GL_NEAREST : GL_LINEAR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

        GLenum wrap = (info.wrap == TextureWrap::Repeat) ? GL_REPEAT : GL_CLAMP_TO_EDGE;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, info.width, info.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        TextureHandle handle = next_texture_id_++;
        textures_[handle] = {tex, info};
        return handle;
    }

    void destroy_texture(TextureHandle texture) override {
        auto it = textures_.find(texture);
        if (it != textures_.end()) {
            glDeleteTextures(1, &it->second.texture);
            textures_.erase(it);
        }
    }

    TextureInfo get_texture_info(TextureHandle texture) const override {
        auto it = textures_.find(texture);
        if (it != textures_.end()) {
            return it->second.info;
        }
        return {};
    }

    void draw_quad(Vec2 position, Vec2 size, const Color& color) override {
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

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glUseProgram(color_program_);
        glUniformMatrix4fv(color_proj_loc_, 1, GL_FALSE, projection_);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    void draw_textured_quad(Vec2 position, Vec2 size,
                            const TextureRegion& region,
                            const Color& tint) override {
        auto it = textures_.find(region.texture);
        if (it == textures_.end()) return;

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

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glUseProgram(textured_program_);
        glUniformMatrix4fv(textured_proj_loc_, 1, GL_FALSE, projection_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, it->second.texture);
        glUniform1i(textured_sampler_loc_, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    void begin_batch() override {
        batch_vertices_.clear();
        current_batch_texture_ = INVALID_TEXTURE;
        batching_ = true;
    }

    void draw_sprite(const Sprite& sprite) override {
        if (!batching_) return;

        if (current_batch_texture_ != sprite.region.texture) {
            if (!batch_vertices_.empty()) {
                flush_batch();
            }
            current_batch_texture_ = sprite.region.texture;
        }

        if (batch_vertices_.size() + 6 > MAX_BATCH_VERTICES) {
            flush_batch();
        }

        float cx = sprite.position.x;
        float cy = sprite.position.y;
        float w2 = sprite.size.x / 2.0f;
        float h2 = sprite.size.y / 2.0f;
        float cos_r = std::cos(sprite.rotation);
        float sin_r = std::sin(sprite.rotation);

        auto rot = [&](float ox, float oy) -> std::pair<float, float> {
            return {cx + ox * cos_r - oy * sin_r, cy + ox * sin_r + oy * cos_r};
        };

        auto [x0, y0] = rot(-w2, -h2);
        auto [x1, y1] = rot(w2, -h2);
        auto [x2, y2] = rot(w2, h2);
        auto [x3, y3] = rot(-w2, h2);

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
        return "WebGL2";
    }

    int max_texture_size() const override {
        GLint size;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &size);
        return size;
    }

private:
    void flush_batch() {
        if (batch_vertices_.empty()) return;

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, batch_vertices_.size() * sizeof(Vertex), batch_vertices_.data());

        auto it = textures_.find(current_batch_texture_);
        if (it != textures_.end()) {
            glUseProgram(textured_program_);
            glUniformMatrix4fv(textured_proj_loc_, 1, GL_FALSE, projection_);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, it->second.texture);
            glUniform1i(textured_sampler_loc_, 0);
        } else {
            glUseProgram(color_program_);
            glUniformMatrix4fv(color_proj_loc_, 1, GL_FALSE, projection_);
        }

        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(batch_vertices_.size()));
        batch_vertices_.clear();
    }
};

// ============================================================================
// Factory Function
// ============================================================================

std::unique_ptr<Renderer> create_renderer() {
    return std::make_unique<WebGLRenderer>();
}

} // namespace cafe

#endif // __EMSCRIPTEN__

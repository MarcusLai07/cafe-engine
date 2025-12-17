# Resource Management Architecture

The resource system handles loading, caching, and lifecycle of game assets (textures, audio, data files).

## Design Goals

1. **Unified loading** - Same API regardless of platform
2. **Caching** - Don't load the same asset twice
3. **Handle-based** - Use handles, not raw pointers (allows hot reload)
4. **Async-ready** - Architecture supports background loading (future)

## Resource Types

| Type | File Formats | Used For |
|------|--------------|----------|
| Texture | PNG, BMP | Sprites, tiles, UI |
| Audio | WAV, OGG | Sound effects, music |
| Font | BMFont | Text rendering |
| Data | JSON | Configs, levels, entities |
| Shader | MSL, GLSL | Render effects |

## Resource Manager

```cpp
// engine/resource.h

namespace cafe {

using ResourceHandle = uint32_t;
constexpr ResourceHandle INVALID_RESOURCE = 0;

enum class ResourceType {
    Texture,
    Audio,
    Font,
    Data,
    Shader
};

enum class ResourceState {
    NotLoaded,
    Loading,
    Loaded,
    Failed
};

struct ResourceInfo {
    std::string path;
    ResourceType type;
    ResourceState state;
    size_t memory_size;
};

class ResourceManager {
private:
    Platform* platform_;
    Renderer* renderer_;

    std::unordered_map<std::string, ResourceHandle> path_to_handle_;
    std::unordered_map<ResourceHandle, ResourceInfo> resource_info_;
    ResourceHandle next_handle_ = 1;

    // Type-specific storage
    std::unordered_map<ResourceHandle, TextureData> textures_;
    std::unordered_map<ResourceHandle, AudioData> audio_;
    std::unordered_map<ResourceHandle, FontData> fonts_;
    std::unordered_map<ResourceHandle, JsonData> data_;

public:
    ResourceManager(Platform* platform, Renderer* renderer)
        : platform_(platform), renderer_(renderer) {}

    // Load with automatic type detection from extension
    ResourceHandle load(const std::string& path) {
        // Check cache first
        auto it = path_to_handle_.find(path);
        if (it != path_to_handle_.end()) {
            return it->second;
        }

        // Determine type from extension
        ResourceType type = get_type_from_extension(path);

        // Create handle and info
        ResourceHandle handle = next_handle_++;
        path_to_handle_[path] = handle;
        resource_info_[handle] = {path, type, ResourceState::Loading, 0};

        // Load based on type
        bool success = false;
        switch (type) {
            case ResourceType::Texture:
                success = load_texture(handle, path);
                break;
            case ResourceType::Audio:
                success = load_audio(handle, path);
                break;
            case ResourceType::Font:
                success = load_font(handle, path);
                break;
            case ResourceType::Data:
                success = load_data(handle, path);
                break;
            default:
                break;
        }

        resource_info_[handle].state = success ? ResourceState::Loaded : ResourceState::Failed;
        return handle;
    }

    // Typed accessors
    TextureHandle get_texture(ResourceHandle handle) {
        auto it = textures_.find(handle);
        return (it != textures_.end()) ? it->second.gpu_handle : INVALID_TEXTURE;
    }

    AudioHandle get_audio(ResourceHandle handle) {
        auto it = audio_.find(handle);
        return (it != audio_.end()) ? it->second.handle : INVALID_AUDIO;
    }

    const JsonData* get_data(ResourceHandle handle) {
        auto it = data_.find(handle);
        return (it != data_.end()) ? &it->second : nullptr;
    }

    // Unload
    void unload(ResourceHandle handle) {
        auto info_it = resource_info_.find(handle);
        if (info_it == resource_info_.end()) return;

        // Remove from type-specific storage
        switch (info_it->second.type) {
            case ResourceType::Texture:
                if (textures_.count(handle)) {
                    renderer_->destroy_texture(textures_[handle].gpu_handle);
                    textures_.erase(handle);
                }
                break;
            // ... other types
        }

        path_to_handle_.erase(info_it->second.path);
        resource_info_.erase(handle);
    }

    void unload_all() {
        for (auto& [handle, info] : resource_info_) {
            unload(handle);
        }
    }

    // Info
    ResourceState get_state(ResourceHandle handle) const {
        auto it = resource_info_.find(handle);
        return (it != resource_info_.end()) ? it->second.state : ResourceState::NotLoaded;
    }

private:
    ResourceType get_type_from_extension(const std::string& path) {
        size_t dot = path.rfind('.');
        if (dot == std::string::npos) return ResourceType::Data;

        std::string ext = path.substr(dot + 1);
        // Convert to lowercase
        for (char& c : ext) c = std::tolower(c);

        if (ext == "png" || ext == "bmp") return ResourceType::Texture;
        if (ext == "wav" || ext == "ogg") return ResourceType::Audio;
        if (ext == "fnt") return ResourceType::Font;
        if (ext == "json") return ResourceType::Data;
        if (ext == "msl" || ext == "glsl") return ResourceType::Shader;

        return ResourceType::Data;
    }

    bool load_texture(ResourceHandle handle, const std::string& path);
    bool load_audio(ResourceHandle handle, const std::string& path);
    bool load_font(ResourceHandle handle, const std::string& path);
    bool load_data(ResourceHandle handle, const std::string& path);
};

} // namespace cafe
```

## Texture Loading

```cpp
// engine/resource_texture.cpp

struct TextureData {
    TextureHandle gpu_handle;
    int width;
    int height;
};

bool ResourceManager::load_texture(ResourceHandle handle, const std::string& path) {
    // Read file bytes
    std::string full_path = std::string(platform_->get_assets_path()) + path;
    std::vector<uint8_t> file_data;
    if (!platform_->read_file(full_path.c_str(), &file_data)) {
        return false;
    }

    // Decode PNG/BMP to raw pixels
    // Using stb_image (single-header library, acceptable dependency)
    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(
        file_data.data(), file_data.size(),
        &width, &height, &channels, 4  // Force RGBA
    );

    if (!pixels) {
        return false;
    }

    // Upload to GPU
    TextureHandle gpu_handle = renderer_->create_texture(width, height, pixels);
    stbi_image_free(pixels);

    if (gpu_handle == INVALID_TEXTURE) {
        return false;
    }

    textures_[handle] = {gpu_handle, width, height};
    resource_info_[handle].memory_size = width * height * 4;

    return true;
}
```

## Sprite Sheet Helper

```cpp
// engine/sprite_sheet.h

namespace cafe {

struct SpriteFrame {
    std::string name;
    Rect source_rect;
    int anchor_x, anchor_y;  // Pivot point
};

class SpriteSheet {
private:
    ResourceHandle texture_handle_;
    TextureHandle gpu_texture_;
    std::unordered_map<std::string, SpriteFrame> frames_;
    std::unordered_map<std::string, Animation> animations_;

public:
    bool load(ResourceManager* resources, const std::string& image_path, const std::string& data_path) {
        // Load texture
        texture_handle_ = resources->load(image_path);
        gpu_texture_ = resources->get_texture(texture_handle_);

        // Load JSON data describing frames
        ResourceHandle data_handle = resources->load(data_path);
        const JsonData* data = resources->get_data(data_handle);

        if (!data) return false;

        // Parse frames
        // Expected format:
        // {
        //   "frames": {
        //     "player_idle_0": {"x": 0, "y": 0, "w": 32, "h": 48, "ax": 16, "ay": 48},
        //     ...
        //   },
        //   "animations": {
        //     "idle": {"frames": ["player_idle_0", "player_idle_1"], "fps": 2, "loop": true},
        //     ...
        //   }
        // }

        parse_frames(data);
        parse_animations(data);

        return true;
    }

    const SpriteFrame* get_frame(const std::string& name) const {
        auto it = frames_.find(name);
        return (it != frames_.end()) ? &it->second : nullptr;
    }

    const Animation* get_animation(const std::string& name) const {
        auto it = animations_.find(name);
        return (it != animations_.end()) ? &it->second : nullptr;
    }

    TextureHandle texture() const { return gpu_texture_; }

private:
    void parse_frames(const JsonData* data);
    void parse_animations(const JsonData* data);
};

} // namespace cafe
```

## JSON Parsing

We'll use a minimal JSON parser (or write our own simple one for learning):

```cpp
// engine/json.h

namespace cafe {

// Minimal JSON value representation
class JsonValue {
public:
    enum Type { Null, Bool, Number, String, Array, Object };

private:
    Type type_;
    union {
        bool bool_value_;
        double number_value_;
    };
    std::string string_value_;
    std::vector<JsonValue> array_value_;
    std::unordered_map<std::string, JsonValue> object_value_;

public:
    // Accessors
    bool is_null() const { return type_ == Null; }
    bool is_bool() const { return type_ == Bool; }
    bool is_number() const { return type_ == Number; }
    bool is_string() const { return type_ == String; }
    bool is_array() const { return type_ == Array; }
    bool is_object() const { return type_ == Object; }

    bool as_bool() const { return bool_value_; }
    double as_number() const { return number_value_; }
    int as_int() const { return static_cast<int>(number_value_); }
    const std::string& as_string() const { return string_value_; }

    const JsonValue& operator[](size_t index) const { return array_value_[index]; }
    const JsonValue& operator[](const std::string& key) const;

    size_t size() const;
    bool has(const std::string& key) const;

    // Iteration
    auto begin() const { return array_value_.begin(); }
    auto end() const { return array_value_.end(); }
    const auto& items() const { return object_value_; }
};

// Parse JSON from string
JsonValue parse_json(const std::string& json_string);

using JsonData = JsonValue;

} // namespace cafe
```

## Asset Pipeline (Future)

For development, we might add:

```
tools/asset_pipeline/
├── texture_packer.cpp    # Combine sprites into atlases
├── audio_converter.cpp   # Convert to runtime formats
└── data_validator.cpp    # Validate JSON schemas
```

## Memory Budget

For a cafe sim, rough asset budget:

| Category | Budget | Notes |
|----------|--------|-------|
| Textures | 64 MB | Sprites, tiles, UI |
| Audio | 32 MB | Music, sound effects |
| Data | 4 MB | Configs, levels |
| **Total** | ~100 MB | Comfortable for all platforms |

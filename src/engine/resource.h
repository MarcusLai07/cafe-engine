#ifndef CAFE_RESOURCE_H
#define CAFE_RESOURCE_H

#include "../renderer/renderer.h"
#include "image.h"
#include "sprite_sheet.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <typeindex>

namespace cafe {

// ============================================================================
// Resource Handle - Type-safe reference to a loaded resource
// ============================================================================

template<typename T>
class ResourceHandle {
public:
    ResourceHandle() = default;
    explicit ResourceHandle(const std::string& id) : id_(id) {}

    const std::string& id() const { return id_; }
    bool is_valid() const { return !id_.empty(); }
    explicit operator bool() const { return is_valid(); }

    bool operator==(const ResourceHandle& other) const { return id_ == other.id_; }
    bool operator!=(const ResourceHandle& other) const { return id_ != other.id_; }

private:
    std::string id_;
};

// Common handle types
using TextureResource = ResourceHandle<TextureHandle>;
using SpriteSheetResource = ResourceHandle<SpriteSheet>;

// ============================================================================
// Resource Manager - Loads, caches, and manages game assets
// ============================================================================

class ResourceManager {
public:
    ResourceManager() = default;
    ~ResourceManager();

    // Non-copyable
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    // Initialize with renderer (needed for GPU resources)
    void initialize(Renderer* renderer);

    // Shutdown and release all resources
    void shutdown();

    // ========================================================================
    // Texture Loading
    // ========================================================================

    // Load texture from file (cached by path)
    TextureResource load_texture(const std::string& path,
                                  TextureFilter filter = TextureFilter::Nearest);

    // Load texture with custom ID
    TextureResource load_texture(const std::string& id,
                                  const std::string& path,
                                  TextureFilter filter = TextureFilter::Nearest);

    // Create texture from image data
    TextureResource create_texture(const std::string& id,
                                    const Image& image,
                                    TextureFilter filter = TextureFilter::Nearest);

    // Get texture handle (returns INVALID_TEXTURE if not found)
    TextureHandle get_texture(const TextureResource& handle) const;
    TextureHandle get_texture(const std::string& id) const;

    // Get texture info
    TextureInfo get_texture_info(const TextureResource& handle) const;

    // ========================================================================
    // Sprite Sheet Loading
    // ========================================================================

    // Load sprite sheet from image file
    SpriteSheetResource load_sprite_sheet(const std::string& path,
                                           TextureFilter filter = TextureFilter::Nearest);

    // Load with custom ID
    SpriteSheetResource load_sprite_sheet(const std::string& id,
                                           const std::string& path,
                                           TextureFilter filter = TextureFilter::Nearest);

    // Get sprite sheet (returns nullptr if not found)
    SpriteSheet* get_sprite_sheet(const SpriteSheetResource& handle);
    const SpriteSheet* get_sprite_sheet(const SpriteSheetResource& handle) const;
    SpriteSheet* get_sprite_sheet(const std::string& id);

    // ========================================================================
    // Resource Management
    // ========================================================================

    // Check if resource exists
    bool has_texture(const std::string& id) const;
    bool has_sprite_sheet(const std::string& id) const;

    // Unload specific resources
    void unload_texture(const std::string& id);
    void unload_sprite_sheet(const std::string& id);

    // Unload all resources of a type
    void unload_all_textures();
    void unload_all_sprite_sheets();

    // Unload everything
    void unload_all();

    // Get statistics
    size_t texture_count() const { return textures_.size(); }
    size_t sprite_sheet_count() const { return sprite_sheets_.size(); }

    // Set base path for asset loading (e.g., "assets/")
    void set_base_path(const std::string& path);
    const std::string& base_path() const { return base_path_; }

private:
    Renderer* renderer_ = nullptr;
    std::string base_path_;

    // Texture storage
    struct TextureEntry {
        TextureHandle handle = INVALID_TEXTURE;
        TextureInfo info;
        std::string source_path;
    };
    std::unordered_map<std::string, TextureEntry> textures_;

    // Sprite sheet storage
    std::unordered_map<std::string, std::unique_ptr<SpriteSheet>> sprite_sheets_;

    // Helper to resolve path
    std::string resolve_path(const std::string& path) const;
};

// ============================================================================
// Global Resource Manager Access (optional convenience)
// ============================================================================

// Set/get global resource manager instance
void set_resource_manager(ResourceManager* manager);
ResourceManager* resource_manager();

} // namespace cafe

#endif // CAFE_RESOURCE_H

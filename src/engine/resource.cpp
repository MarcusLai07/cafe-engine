#include "resource.h"
#include <iostream>

namespace cafe {

// ============================================================================
// Global Resource Manager
// ============================================================================

static ResourceManager* g_resource_manager = nullptr;

void set_resource_manager(ResourceManager* manager) {
    g_resource_manager = manager;
}

ResourceManager* resource_manager() {
    return g_resource_manager;
}

// ============================================================================
// ResourceManager Implementation
// ============================================================================

ResourceManager::~ResourceManager() {
    shutdown();
}

void ResourceManager::initialize(Renderer* renderer) {
    renderer_ = renderer;
}

void ResourceManager::shutdown() {
    unload_all();
    renderer_ = nullptr;
}

void ResourceManager::set_base_path(const std::string& path) {
    base_path_ = path;
    // Ensure trailing slash
    if (!base_path_.empty() && base_path_.back() != '/') {
        base_path_ += '/';
    }
}

std::string ResourceManager::resolve_path(const std::string& path) const {
    // If path is absolute or base_path is empty, use as-is
    if (path.empty() || path[0] == '/' || base_path_.empty()) {
        return path;
    }
    return base_path_ + path;
}

// ============================================================================
// Texture Loading
// ============================================================================

TextureResource ResourceManager::load_texture(const std::string& path,
                                               TextureFilter filter) {
    // Use path as ID
    return load_texture(path, path, filter);
}

TextureResource ResourceManager::load_texture(const std::string& id,
                                               const std::string& path,
                                               TextureFilter filter) {
    // Check if already loaded
    if (has_texture(id)) {
        return TextureResource(id);
    }

    if (!renderer_) {
        std::cerr << "ResourceManager: No renderer set\n";
        return TextureResource();
    }

    // Load image
    std::string full_path = resolve_path(path);
    auto image = Image::load_from_file(full_path);
    if (!image) {
        std::cerr << "ResourceManager: Failed to load image: " << full_path << "\n";
        return TextureResource();
    }

    // Create texture
    TextureInfo info;
    info.width = image->width();
    info.height = image->height();
    info.filter = filter;
    info.wrap = TextureWrap::Clamp;

    TextureHandle handle = renderer_->create_texture(image->data(), info);
    if (handle == INVALID_TEXTURE) {
        std::cerr << "ResourceManager: Failed to create texture: " << id << "\n";
        return TextureResource();
    }

    // Store
    TextureEntry entry;
    entry.handle = handle;
    entry.info = info;
    entry.source_path = full_path;
    textures_[id] = entry;

    return TextureResource(id);
}

TextureResource ResourceManager::create_texture(const std::string& id,
                                                  const Image& image,
                                                  TextureFilter filter) {
    if (!renderer_) {
        std::cerr << "ResourceManager: No renderer set\n";
        return TextureResource();
    }

    // Unload existing if present
    if (has_texture(id)) {
        unload_texture(id);
    }

    TextureInfo info;
    info.width = image.width();
    info.height = image.height();
    info.filter = filter;
    info.wrap = TextureWrap::Clamp;

    TextureHandle handle = renderer_->create_texture(image.data(), info);
    if (handle == INVALID_TEXTURE) {
        std::cerr << "ResourceManager: Failed to create texture: " << id << "\n";
        return TextureResource();
    }

    TextureEntry entry;
    entry.handle = handle;
    entry.info = info;
    entry.source_path = "";  // Procedurally generated
    textures_[id] = entry;

    return TextureResource(id);
}

TextureHandle ResourceManager::get_texture(const TextureResource& handle) const {
    return get_texture(handle.id());
}

TextureHandle ResourceManager::get_texture(const std::string& id) const {
    auto it = textures_.find(id);
    if (it != textures_.end()) {
        return it->second.handle;
    }
    return INVALID_TEXTURE;
}

TextureInfo ResourceManager::get_texture_info(const TextureResource& handle) const {
    auto it = textures_.find(handle.id());
    if (it != textures_.end()) {
        return it->second.info;
    }
    return TextureInfo();
}

// ============================================================================
// Sprite Sheet Loading
// ============================================================================

SpriteSheetResource ResourceManager::load_sprite_sheet(const std::string& path,
                                                        TextureFilter filter) {
    return load_sprite_sheet(path, path, filter);
}

SpriteSheetResource ResourceManager::load_sprite_sheet(const std::string& id,
                                                        const std::string& path,
                                                        TextureFilter filter) {
    // Check if already loaded
    if (has_sprite_sheet(id)) {
        return SpriteSheetResource(id);
    }

    if (!renderer_) {
        std::cerr << "ResourceManager: No renderer set\n";
        return SpriteSheetResource();
    }

    // Create sprite sheet
    auto sheet = std::make_unique<SpriteSheet>();
    std::string full_path = resolve_path(path);

    if (!sheet->load(renderer_, full_path, filter)) {
        std::cerr << "ResourceManager: Failed to load sprite sheet: " << full_path << "\n";
        return SpriteSheetResource();
    }

    sprite_sheets_[id] = std::move(sheet);
    return SpriteSheetResource(id);
}

SpriteSheet* ResourceManager::get_sprite_sheet(const SpriteSheetResource& handle) {
    return get_sprite_sheet(handle.id());
}

const SpriteSheet* ResourceManager::get_sprite_sheet(const SpriteSheetResource& handle) const {
    auto it = sprite_sheets_.find(handle.id());
    if (it != sprite_sheets_.end()) {
        return it->second.get();
    }
    return nullptr;
}

SpriteSheet* ResourceManager::get_sprite_sheet(const std::string& id) {
    auto it = sprite_sheets_.find(id);
    if (it != sprite_sheets_.end()) {
        return it->second.get();
    }
    return nullptr;
}

// ============================================================================
// Resource Management
// ============================================================================

bool ResourceManager::has_texture(const std::string& id) const {
    return textures_.find(id) != textures_.end();
}

bool ResourceManager::has_sprite_sheet(const std::string& id) const {
    return sprite_sheets_.find(id) != sprite_sheets_.end();
}

void ResourceManager::unload_texture(const std::string& id) {
    auto it = textures_.find(id);
    if (it != textures_.end()) {
        if (renderer_ && it->second.handle != INVALID_TEXTURE) {
            renderer_->destroy_texture(it->second.handle);
        }
        textures_.erase(it);
    }
}

void ResourceManager::unload_sprite_sheet(const std::string& id) {
    auto it = sprite_sheets_.find(id);
    if (it != sprite_sheets_.end()) {
        if (renderer_) {
            it->second->unload(renderer_);
        }
        sprite_sheets_.erase(it);
    }
}

void ResourceManager::unload_all_textures() {
    if (renderer_) {
        for (auto& [id, entry] : textures_) {
            if (entry.handle != INVALID_TEXTURE) {
                renderer_->destroy_texture(entry.handle);
            }
        }
    }
    textures_.clear();
}

void ResourceManager::unload_all_sprite_sheets() {
    if (renderer_) {
        for (auto& [id, sheet] : sprite_sheets_) {
            sheet->unload(renderer_);
        }
    }
    sprite_sheets_.clear();
}

void ResourceManager::unload_all() {
    unload_all_sprite_sheets();
    unload_all_textures();
}

} // namespace cafe

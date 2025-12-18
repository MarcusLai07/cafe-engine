#include "isometric.h"
#include "sprite_sheet.h"
#include <algorithm>
#include <cmath>

namespace cafe {

// ============================================================================
// Isometric Static Members
// ============================================================================

float Isometric::tile_width_ = 64.0f;
float Isometric::tile_height_ = 32.0f;
float Isometric::camera_x_ = 0.0f;
float Isometric::camera_y_ = 0.0f;

void Isometric::set_tile_size(float tile_width, float tile_height) {
    tile_width_ = tile_width;
    tile_height_ = tile_height;
}

float Isometric::tile_width() { return tile_width_; }
float Isometric::tile_height() { return tile_height_; }

Vec2 Isometric::tile_to_screen(float tile_x, float tile_y) {
    // Classic isometric projection
    // Screen X = (tile_x - tile_y) * (tile_width / 2)
    // Screen Y = (tile_x + tile_y) * (tile_height / 2)
    float screen_x = (tile_x - tile_y) * (tile_width_ * 0.5f);
    float screen_y = (tile_x + tile_y) * (tile_height_ * 0.5f);

    // Apply camera offset
    screen_x -= camera_x_;
    screen_y -= camera_y_;

    return {screen_x, screen_y};
}

Vec2 Isometric::tile_to_screen(int tile_x, int tile_y) {
    return tile_to_screen(static_cast<float>(tile_x), static_cast<float>(tile_y));
}

Vec2 Isometric::screen_to_tile(float screen_x, float screen_y) {
    // Apply camera offset (reverse)
    screen_x += camera_x_;
    screen_y += camera_y_;

    // Inverse of tile_to_screen
    // tile_x = (screen_x / (tile_width/2) + screen_y / (tile_height/2)) / 2
    // tile_y = (screen_y / (tile_height/2) - screen_x / (tile_width/2)) / 2
    float half_width = tile_width_ * 0.5f;
    float half_height = tile_height_ * 0.5f;

    float tile_x = (screen_x / half_width + screen_y / half_height) * 0.5f;
    float tile_y = (screen_y / half_height - screen_x / half_width) * 0.5f;

    return {tile_x, tile_y};
}

void Isometric::screen_to_tile_int(float screen_x, float screen_y, int& tile_x, int& tile_y) {
    Vec2 tile = screen_to_tile(screen_x, screen_y);
    tile_x = static_cast<int>(std::floor(tile.x));
    tile_y = static_cast<int>(std::floor(tile.y));
}

int Isometric::tile_depth(int tile_x, int tile_y) {
    // Tiles with higher (x + y) are drawn later (on top)
    // This gives correct depth sorting for isometric view
    return tile_x + tile_y;
}

void Isometric::set_camera(float x, float y) {
    camera_x_ = x;
    camera_y_ = y;
}

Vec2 Isometric::camera() {
    return {camera_x_, camera_y_};
}

// ============================================================================
// TileMap Implementation
// ============================================================================

const Tile TileMap::empty_tile_ = {};

TileMap::TileMap(int width, int height) {
    resize(width, height);
}

void TileMap::resize(int width, int height) {
    width_ = width;
    height_ = height;
    tiles_.clear();
    tiles_.resize(static_cast<size_t>(width) * height);
}

Tile& TileMap::at(int x, int y) {
    if (!in_bounds(x, y)) {
        // Return empty tile for out-of-bounds access
        static Tile mutable_empty;
        mutable_empty = empty_tile_;
        return mutable_empty;
    }
    return tiles_[static_cast<size_t>(y) * width_ + x];
}

const Tile& TileMap::at(int x, int y) const {
    if (!in_bounds(x, y)) {
        return empty_tile_;
    }
    return tiles_[static_cast<size_t>(y) * width_ + x];
}

bool TileMap::in_bounds(int x, int y) const {
    return x >= 0 && x < width_ && y >= 0 && y < height_;
}

void TileMap::fill(const Tile& tile) {
    std::fill(tiles_.begin(), tiles_.end(), tile);
}

void TileMap::set_tileset(SpriteSheet* tileset) {
    tileset_ = tileset;
}

void TileMap::for_each_visible(const Rect& viewport, const TileCallback& callback) const {
    // viewport is in SCREEN coordinates (0,0 to width,height)
    // screen_to_tile converts screen coords to tile coords using camera

    // Calculate tile bounds from viewport corners
    Vec2 top_left = Isometric::screen_to_tile(viewport.x, viewport.y);
    Vec2 top_right = Isometric::screen_to_tile(viewport.x + viewport.width, viewport.y);
    Vec2 bottom_left = Isometric::screen_to_tile(viewport.x, viewport.y + viewport.height);
    Vec2 bottom_right = Isometric::screen_to_tile(viewport.x + viewport.width,
                                                   viewport.y + viewport.height);

    // Find bounding box of visible tiles (with margin for partially visible tiles)
    int min_x = static_cast<int>(std::floor(std::min({top_left.x, bottom_left.x, top_right.x, bottom_right.x}))) - 2;
    int max_x = static_cast<int>(std::ceil(std::max({top_left.x, bottom_left.x, top_right.x, bottom_right.x}))) + 2;
    int min_y = static_cast<int>(std::floor(std::min({top_left.y, bottom_left.y, top_right.y, bottom_right.y}))) - 2;
    int max_y = static_cast<int>(std::ceil(std::max({top_left.y, bottom_left.y, top_right.y, bottom_right.y}))) + 2;

    // Clamp to map bounds
    min_x = std::max(0, min_x);
    max_x = std::min(width_ - 1, max_x);
    min_y = std::max(0, min_y);
    max_y = std::min(height_ - 1, max_y);

    // Safety check - if bounds are invalid, nothing to render
    if (min_x > max_x || min_y > max_y) {
        return;
    }

    // Collect visible tiles for sorting
    struct TileEntry {
        int x, y;
        int depth;
        float screen_x, screen_y;
    };
    std::vector<TileEntry> visible_tiles;

    // Reserve reasonable amount (clamp to prevent overflow)
    size_t estimated = static_cast<size_t>(max_x - min_x + 1) * static_cast<size_t>(max_y - min_y + 1);
    visible_tiles.reserve(std::min(estimated, static_cast<size_t>(10000)));

    for (int ty = min_y; ty <= max_y; ++ty) {
        for (int tx = min_x; tx <= max_x; ++tx) {
            const Tile& tile = at(tx, ty);
            if (tile.is_empty()) continue;

            Vec2 screen = Isometric::tile_to_screen(tx, ty);

            // Check if actually visible on screen
            float margin_x = Isometric::tile_width();
            float margin_y = Isometric::tile_height() * 2;

            if (screen.x >= -margin_x &&
                screen.x <= viewport.width + margin_x &&
                screen.y >= -margin_y &&
                screen.y <= viewport.height + margin_y) {

                visible_tiles.push_back({
                    tx, ty,
                    Isometric::tile_depth(tx, ty) + tile.height * 1000,
                    screen.x, screen.y
                });
            }
        }
    }

    // Sort by depth (back to front)
    std::sort(visible_tiles.begin(), visible_tiles.end(),
              [](const TileEntry& a, const TileEntry& b) {
                  return a.depth < b.depth;
              });

    // Call callback for each visible tile
    for (const auto& entry : visible_tiles) {
        const Tile& tile = at(entry.x, entry.y);
        callback(entry.x, entry.y, tile, entry.screen_x, entry.screen_y);
    }
}

void TileMap::render(Renderer* renderer, const Rect& viewport) {
    if (!tileset_ || !renderer) return;

    renderer->begin_batch();

    for_each_visible(viewport, [&](int, int, const Tile& tile, float screen_x, float screen_y) {
        // Get sprite frame for this tile
        const SpriteFrame* frame = tileset_->frame(tile.tile_id - 1);  // tile_id 1-based
        if (!frame) return;

        // Adjust Y position for tile height (tiles are drawn from their base)
        float adjusted_y = screen_y - static_cast<float>(tile.height) * Isometric::tile_height();

        Sprite sprite;
        sprite.position = {screen_x, adjusted_y};
        sprite.size = {static_cast<float>(frame->width), static_cast<float>(frame->height)};
        sprite.region = frame->region;
        sprite.tint = Color::white();
        sprite.rotation = 0.0f;
        sprite.origin = {0.5f, 1.0f};  // Bottom-center origin for isometric tiles

        renderer->draw_sprite(sprite);
    });

    renderer->end_batch();
}

// ============================================================================
// TileMapRenderer Implementation
// ============================================================================

void TileMapRenderer::set_tilemap(const TileMap* map) {
    map_ = map;
}

void TileMapRenderer::set_renderer(Renderer* renderer) {
    renderer_ = renderer;
}

void TileMapRenderer::render(const Rect& viewport) {
    if (!map_ || !renderer_) return;

    tiles_rendered_ = 0;

    auto* tileset = map_->tileset();
    if (!tileset) return;

    renderer_->begin_batch();

    map_->for_each_visible(viewport, [&](int, int, const Tile& tile, float screen_x, float screen_y) {
        const SpriteFrame* frame = tileset->frame(tile.tile_id - 1);
        if (!frame) return;

        float adjusted_y = screen_y - static_cast<float>(tile.height) * Isometric::tile_height();

        Sprite sprite;
        sprite.position = {screen_x, adjusted_y};
        sprite.size = {static_cast<float>(frame->width), static_cast<float>(frame->height)};
        sprite.region = frame->region;
        sprite.tint = Color::white();
        sprite.rotation = 0.0f;
        sprite.origin = {0.5f, 1.0f};

        renderer_->draw_sprite(sprite);
        ++tiles_rendered_;
    });

    renderer_->end_batch();
}

} // namespace cafe

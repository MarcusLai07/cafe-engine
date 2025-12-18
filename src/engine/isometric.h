#ifndef CAFE_ISOMETRIC_H
#define CAFE_ISOMETRIC_H

#include "../renderer/renderer.h"
#include <vector>
#include <functional>

namespace cafe {

// Forward declarations
class SpriteSheet;
struct SpriteFrame;

// ============================================================================
// Isometric Coordinate System
// ============================================================================
//
// Isometric projection converts 3D tile coordinates (tile_x, tile_y) to
// 2D screen coordinates. The classic isometric ratio is 2:1 (width:height).
//
//     Screen Y
//        ^
//        |     (0,0)
//        |    /     \
//        |   (1,0)   (0,1)
//        |  /     \ /     \
//        | (2,0)  (1,1)   (0,2)
//        |
//        +-----------------------> Screen X
//
// Tile coordinates: (x, y) where x increases right-down, y increases left-down
// Screen coordinates: standard 2D pixel position
// ============================================================================

// Isometric coordinate conversion utilities
class Isometric {
public:
    // Set tile dimensions (width and height of a tile in pixels)
    // For classic 2:1 isometric: tile_width = 64, tile_height = 32
    static void set_tile_size(float tile_width, float tile_height);

    // Get current tile dimensions
    static float tile_width();
    static float tile_height();

    // Convert tile (grid) coordinates to screen (pixel) coordinates
    // Returns the center of the tile on screen
    static Vec2 tile_to_screen(float tile_x, float tile_y);
    static Vec2 tile_to_screen(int tile_x, int tile_y);

    // Convert screen coordinates to tile coordinates
    // Returns floating-point tile position (fractional = within tile)
    static Vec2 screen_to_tile(float screen_x, float screen_y);

    // Get tile under screen point (integer tile coordinates)
    static void screen_to_tile_int(float screen_x, float screen_y, int& tile_x, int& tile_y);

    // Calculate draw order (depth) for a tile
    // Higher values should be drawn later (on top)
    static int tile_depth(int tile_x, int tile_y);

    // Set camera offset (world scroll position)
    static void set_camera(float x, float y);
    static Vec2 camera();

private:
    static float tile_width_;
    static float tile_height_;
    static float camera_x_;
    static float camera_y_;
};

// ============================================================================
// Tile Data
// ============================================================================

struct Tile {
    int tile_id = 0;           // Index into tileset (0 = empty/no tile)
    int height = 0;            // Height level (for stacking tiles vertically)
    uint8_t flags = 0;         // Custom flags (walkable, etc.)

    bool is_empty() const { return tile_id == 0; }
};

// ============================================================================
// TileMap - 2D grid of tiles with isometric rendering
// ============================================================================

class TileMap {
public:
    TileMap() = default;
    TileMap(int width, int height);

    // Resize the map (clears existing data)
    void resize(int width, int height);

    // Dimensions
    int width() const { return width_; }
    int height() const { return height_; }

    // Tile access
    Tile& at(int x, int y);
    const Tile& at(int x, int y) const;

    // Check bounds
    bool in_bounds(int x, int y) const;

    // Fill entire map with a tile
    void fill(const Tile& tile);

    // Set tileset (sprite sheet containing tile graphics)
    void set_tileset(SpriteSheet* tileset);
    SpriteSheet* tileset() const { return tileset_; }

    // Render all visible tiles
    // Handles depth sorting automatically
    void render(Renderer* renderer, const Rect& viewport);

    // Get tiles in draw order (sorted by depth)
    // Callback receives: tile_x, tile_y, tile reference, screen_x, screen_y
    using TileCallback = std::function<void(int, int, const Tile&, float, float)>;
    void for_each_visible(const Rect& viewport, const TileCallback& callback) const;

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<Tile> tiles_;
    SpriteSheet* tileset_ = nullptr;

    static const Tile empty_tile_;
};

// ============================================================================
// TileMapRenderer - Efficient batched tile rendering
// ============================================================================

class TileMapRenderer {
public:
    TileMapRenderer() = default;

    // Set the tilemap to render
    void set_tilemap(const TileMap* map);

    // Set the renderer to use
    void set_renderer(Renderer* renderer);

    // Render visible portion of the tilemap
    // viewport: screen rectangle showing visible area
    void render(const Rect& viewport);

    // Statistics
    int tiles_rendered() const { return tiles_rendered_; }

private:
    const TileMap* map_ = nullptr;
    Renderer* renderer_ = nullptr;
    int tiles_rendered_ = 0;
};

} // namespace cafe

#endif // CAFE_ISOMETRIC_H

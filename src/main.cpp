#include "platform/platform.h"
#include "renderer/renderer.h"
#include "engine/game_loop.h"
#include "engine/image.h"
#include "engine/sprite_sheet.h"
#include "engine/isometric.h"
#include <iostream>
#include <cmath>

// ============================================================================
// Phase 3: Renderer Abstraction - Isometric Demo
// ============================================================================
//
// This demonstrates:
// - Abstracted renderer interface (Task 3.1-3.2)
// - Texture and sprite systems (Task 3.5)
// - Isometric tile rendering with depth sorting (Task 3.6)
//
// Controls:
// - WASD or Arrow Keys: Pan the camera
// - Escape: Close window
// ============================================================================

// Generate a simple isometric tileset (4 tile types)
std::unique_ptr<cafe::Image> create_isometric_tileset() {
    // 128x64 image: 4 tiles of 32x32 (but we draw them as 64x32 iso diamonds)
    // Actually for isometric, tiles are diamond-shaped: 64 wide, 32 tall
    // We'll make a 256x32 strip with 4 tiles (64x32 each)
    const int tile_w = 64;
    const int tile_h = 32;
    const int num_tiles = 4;

    auto image = cafe::Image::create(tile_w * num_tiles, tile_h, 4);
    if (!image) return nullptr;

    // Tile colors
    struct TileColor { uint8_t r, g, b; const char* name; };
    TileColor colors[4] = {
        {100, 180, 100, "grass"},    // Tile 1: Green (grass)
        {180, 150, 100, "dirt"},     // Tile 2: Brown (dirt)
        {80, 120, 180, "water"},     // Tile 3: Blue (water)
        {150, 150, 160, "stone"},    // Tile 4: Gray (stone)
    };

    // Draw each tile as an isometric diamond
    for (int tile = 0; tile < num_tiles; ++tile) {
        int offset_x = tile * tile_w;
        auto& c = colors[tile];

        for (int y = 0; y < tile_h; ++y) {
            for (int x = 0; x < tile_w; ++x) {
                // Diamond shape: points at (32,0), (64,16), (32,32), (0,16)
                // Check if pixel is inside the diamond
                int cx = x - tile_w / 2;  // Center x (-32 to 32)
                int cy = y - tile_h / 2;  // Center y (-16 to 16)

                // Diamond equation: |x|/32 + |y|/16 <= 1
                float dx = std::abs(cx) / 32.0f;
                float dy = std::abs(cy) / 16.0f;

                if (dx + dy <= 1.0f) {
                    // Add some shading variation based on position
                    float shade = 1.0f - dy * 0.3f;  // Darker at top/bottom
                    float edge_factor = std::max(0.0f, 1.0f - (dx + dy) * 1.1f);

                    uint8_t r = static_cast<uint8_t>(std::min(255.0f, c.r * shade));
                    uint8_t g = static_cast<uint8_t>(std::min(255.0f, c.g * shade));
                    uint8_t b = static_cast<uint8_t>(std::min(255.0f, c.b * shade));

                    // Lighter in center
                    r = static_cast<uint8_t>(std::min(255.0f, r + edge_factor * 30.0f));
                    g = static_cast<uint8_t>(std::min(255.0f, g + edge_factor * 30.0f));
                    b = static_cast<uint8_t>(std::min(255.0f, b + edge_factor * 30.0f));

                    // Edge highlight
                    if (dx + dy > 0.85f) {
                        r = static_cast<uint8_t>(r * 0.7f);
                        g = static_cast<uint8_t>(g * 0.7f);
                        b = static_cast<uint8_t>(b * 0.7f);
                    }

                    image->set_pixel(offset_x + x, y, r, g, b, 255);
                } else {
                    image->set_pixel(offset_x + x, y, 0, 0, 0, 0);  // Transparent
                }
            }
        }
    }

    return image;
}

// Generate a simple character sprite
std::unique_ptr<cafe::Image> create_character_sprite() {
    // 16x24 simple character
    auto image = cafe::Image::create(16, 24, 4);
    if (!image) return nullptr;

    // Draw a simple pixel art character
    for (int y = 0; y < 24; ++y) {
        for (int x = 0; x < 16; ++x) {
            bool draw = false;
            uint8_t r = 255, g = 200, b = 150;  // Skin color

            // Head (circle-ish, top)
            if (y >= 0 && y <= 7 && x >= 4 && x <= 11) {
                float dx = x - 7.5f;
                float dy = y - 3.5f;
                if (dx * dx / 16.0f + dy * dy / 16.0f <= 1.0f) {
                    draw = true;
                }
            }

            // Body (shirt)
            if (y >= 8 && y <= 15 && x >= 3 && x <= 12) {
                draw = true;
                r = 80; g = 120; b = 200;  // Blue shirt
            }

            // Legs (pants)
            if (y >= 16 && y <= 23) {
                if ((x >= 4 && x <= 7) || (x >= 8 && x <= 11)) {
                    draw = true;
                    r = 60; g = 60; b = 80;  // Dark pants
                }
            }

            if (draw) {
                image->set_pixel(x, y, r, g, b, 255);
            } else {
                image->set_pixel(x, y, 0, 0, 0, 0);
            }
        }
    }

    return image;
}

// Game state
struct GameState {
    float camera_x = 0.0f;
    float camera_y = 0.0f;
    float player_tile_x = 5.0f;
    float player_tile_y = 5.0f;
    const float camera_speed = 300.0f;  // Pixels per second
    const float player_speed = 3.0f;    // Tiles per second
};

int main() {
    std::cout << "Cafe Engine - Phase 3: Isometric Demo\n";
    std::cout << "=======================================\n\n";

    // Create platform
    auto platform = cafe::create_platform();
    std::cout << "Platform: " << platform->name() << "\n";

    // Create window
    cafe::WindowConfig config;
    config.title = "Cafe Engine - Isometric Tiles";
    config.width = 1280;
    config.height = 720;

    auto window = platform->create_window(config);
    std::cout << "Window: " << window->width() << "x" << window->height() << "\n";

    // Create renderer
    auto renderer = cafe::create_renderer();
    if (!renderer->initialize(window.get())) {
        std::cerr << "Failed to initialize renderer!\n";
        return 1;
    }
    std::cout << "Renderer: " << renderer->backend_name() << "\n";

    // Set up orthographic projection (pixel coordinates, Y-down)
    float width = static_cast<float>(window->width());
    float height = static_cast<float>(window->height());
    renderer->set_viewport(0, 0, window->width(), window->height());
    renderer->set_projection(0, width, height, 0);

    // Set up isometric tile size (classic 2:1 ratio)
    cafe::Isometric::set_tile_size(64.0f, 32.0f);

    // Create tileset texture
    std::cout << "\nCreating isometric tileset...\n";
    auto tileset_image = create_isometric_tileset();
    cafe::TextureInfo tileset_info{256, 32, cafe::TextureFilter::Nearest, cafe::TextureWrap::Clamp};
    auto tileset_tex = renderer->create_texture(tileset_image->data(), tileset_info);

    // Create sprite sheet for tileset
    cafe::SpriteSheet tileset;
    tileset.set_texture(tileset_tex, 256, 32);
    tileset.define_frame("grass", 0, 0, 64, 32);
    tileset.define_frame("dirt", 64, 0, 64, 32);
    tileset.define_frame("water", 128, 0, 64, 32);
    tileset.define_frame("stone", 192, 0, 64, 32);

    // Create character sprite
    auto char_image = create_character_sprite();
    cafe::TextureInfo char_info{16, 24, cafe::TextureFilter::Nearest, cafe::TextureWrap::Clamp};
    auto char_tex = renderer->create_texture(char_image->data(), char_info);
    cafe::TextureRegion char_region(char_tex);

    // Create tile map (15x15 tiles)
    const int map_size = 15;
    cafe::TileMap tilemap(map_size, map_size);
    tilemap.set_tileset(&tileset);

    // Fill map with a pattern
    for (int y = 0; y < map_size; ++y) {
        for (int x = 0; x < map_size; ++x) {
            cafe::Tile& tile = tilemap.at(x, y);

            // Create an interesting pattern
            // Water in center
            if (x >= 5 && x <= 9 && y >= 5 && y <= 9) {
                tile.tile_id = 3;  // water
            }
            // Stone path
            else if (x == 7 || y == 7) {
                tile.tile_id = 4;  // stone
            }
            // Dirt around edges
            else if (x == 0 || x == map_size - 1 || y == 0 || y == map_size - 1) {
                tile.tile_id = 2;  // dirt
            }
            // Grass everywhere else
            else {
                tile.tile_id = 1;  // grass
            }
        }
    }

    std::cout << "  Created " << map_size << "x" << map_size << " tile map\n";

    // Game state
    GameState state;

    // Center camera on map
    // Calculate world position of map center (without camera offset)
    // Formula: screen_x = (tile_x - tile_y) * (tile_width / 2)
    //          screen_y = (tile_x + tile_y) * (tile_height / 2)
    float center_tile = static_cast<float>(map_size) / 2.0f;
    float world_center_x = (center_tile - center_tile) * (64.0f / 2.0f);  // = 0 for center
    float world_center_y = (center_tile + center_tile) * (32.0f / 2.0f);  // = center_tile * 32

    // Camera position: offset so map center appears at screen center
    state.camera_x = world_center_x - width / 2.0f;
    state.camera_y = world_center_y - height / 2.0f;
    cafe::Isometric::set_camera(state.camera_x, state.camera_y);

    // Create game loop
    cafe::GameLoop loop(platform.get(), window.get());
    loop.set_target_fps(60);

    std::cout << "\nControls:\n";
    std::cout << "  WASD/Arrows: Pan camera\n";
    std::cout << "  Escape: Quit\n\n";

    // Update callback
    loop.set_update_callback([&](float dt) {
        if (window->is_key_down(cafe::Key::Escape)) {
            loop.stop();
            return;
        }

        // Camera movement
        float move = state.camera_speed * dt;
        if (window->is_key_down(cafe::Key::W) || window->is_key_down(cafe::Key::Up)) {
            state.camera_y -= move;
        }
        if (window->is_key_down(cafe::Key::S) || window->is_key_down(cafe::Key::Down)) {
            state.camera_y += move;
        }
        if (window->is_key_down(cafe::Key::A) || window->is_key_down(cafe::Key::Left)) {
            state.camera_x -= move;
        }
        if (window->is_key_down(cafe::Key::D) || window->is_key_down(cafe::Key::Right)) {
            state.camera_x += move;
        }

        // Update isometric camera
        cafe::Isometric::set_camera(state.camera_x, state.camera_y);
    });

    // Render callback
    loop.set_render_callback([&](float) {
        // Dark background
        renderer->set_clear_color({0.15f, 0.18f, 0.25f, 1.0f});

        renderer->begin_frame();
        renderer->clear();

        // Viewport in screen coordinates (0,0 to width,height)
        // The camera offset is handled internally by Isometric functions
        cafe::Rect viewport{0, 0, width, height};

        // Render tilemap
        tilemap.render(renderer.get(), viewport);

        // Draw player at their tile position
        renderer->begin_batch();
        {
            // tile_to_screen converts tile coords to screen coords (applies camera)
            cafe::Vec2 player_screen = cafe::Isometric::tile_to_screen(
                state.player_tile_x, state.player_tile_y);

            cafe::Sprite player;
            player.position = {
                player_screen.x,
                player_screen.y - 12.0f  // Offset up from tile base
            };
            player.size = {32.0f, 48.0f};  // 2x scale
            player.region = char_region;
            player.tint = cafe::Color::white();
            player.origin = {0.5f, 1.0f};  // Bottom center

            renderer->draw_sprite(player);
        }
        renderer->end_batch();

        // Draw UI overlay
        renderer->begin_batch();
        {
            cafe::Sprite indicator;
            indicator.position = {40.0f, 40.0f};
            indicator.size = {20.0f, 20.0f};
            indicator.region = tileset.frame("grass")->region;
            indicator.tint = {1.0f, 1.0f, 1.0f, 0.5f};
            renderer->draw_sprite(indicator);
        }
        renderer->end_batch();

        renderer->end_frame();
    });

    // FPS callback
    loop.set_frame_callback([&](int fps, float) {
        std::string title = "Cafe Engine - Isometric [" + std::to_string(fps) + " FPS]";
        window->set_title(title);
    });

    // Run game loop
    loop.run();

    // Cleanup
    tileset.unload(renderer.get());
    renderer->destroy_texture(char_tex);
    renderer->shutdown();

    std::cout << "\nWindow closed. Goodbye!\n";
    return 0;
}

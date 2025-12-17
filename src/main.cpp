#include "platform/platform.h"
#include "renderer/renderer.h"
#include "engine/game_loop.h"
#include <iostream>
#include <cmath>

// ============================================================================
// Phase 2: Platform Layer + Metal Rendering Demo
// ============================================================================
//
// This demonstrates:
// - macOS window creation (Task 2.1)
// - Metal GPU initialization (Task 2.2)
// - Basic rendering with shaders (Task 2.3)
// - Input handling (Task 2.4)
// - Fixed timestep game loop (Task 2.5)
//
// Controls:
// - WASD or Arrow Keys: Move the quad
// - Escape: Close window
// ============================================================================

// Game state
struct GameState {
    float quad_x = 0.0f;
    float quad_y = 0.0f;
    float prev_quad_x = 0.0f;  // Previous position for interpolation
    float prev_quad_y = 0.0f;
    float color_time = 0.0f;
    const float move_speed = 1.5f;  // Units per second
};

int main() {
    std::cout << "Cafe Engine - Phase 2: Platform + Metal\n";
    std::cout << "========================================\n\n";

    // Create platform
    auto platform = cafe::create_platform();
    std::cout << "Platform: " << platform->name() << "\n";

    // Create window
    cafe::WindowConfig config;
    config.title = "Cafe Engine - Metal Rendering";
    config.width = 1280;
    config.height = 720;

    auto window = platform->create_window(config);
    std::cout << "Window: " << window->width() << "x" << window->height();
    std::cout << " @ " << window->scale_factor() << "x scale\n";

    // Create renderer
    auto renderer = cafe::create_renderer();
    if (!renderer->initialize(window.get())) {
        std::cerr << "Failed to initialize renderer!\n";
        return 1;
    }
    std::cout << "Renderer: " << renderer->backend_name() << "\n";

    // Create game loop (60 Hz fixed update)
    cafe::GameLoop loop(platform.get(), window.get());
    loop.set_target_fps(60);

    // Game state
    GameState state;

    std::cout << "\nControls: WASD/Arrows to move, Escape to quit\n";
    std::cout << "Running at 60 Hz fixed timestep...\n\n";

    // Update callback - runs at fixed 60 Hz
    loop.set_update_callback([&](float dt) {
        // Store previous position for interpolation
        state.prev_quad_x = state.quad_x;
        state.prev_quad_y = state.quad_y;

        // Handle input
        if (window->is_key_down(cafe::Key::Escape)) {
            loop.stop();
            return;
        }

        // Movement (WASD + Arrow keys)
        float move = state.move_speed * dt;
        if (window->is_key_down(cafe::Key::W) || window->is_key_down(cafe::Key::Up)) {
            state.quad_y += move;
        }
        if (window->is_key_down(cafe::Key::S) || window->is_key_down(cafe::Key::Down)) {
            state.quad_y -= move;
        }
        if (window->is_key_down(cafe::Key::A) || window->is_key_down(cafe::Key::Left)) {
            state.quad_x -= move;
        }
        if (window->is_key_down(cafe::Key::D) || window->is_key_down(cafe::Key::Right)) {
            state.quad_x += move;
        }

        // Clamp position to screen bounds
        state.quad_x = std::max(-0.8f, std::min(0.8f, state.quad_x));
        state.quad_y = std::max(-0.8f, std::min(0.8f, state.quad_y));

        // Update color animation time
        state.color_time += dt;
    });

    // Render callback - runs every frame with interpolation
    loop.set_render_callback([&](float alpha) {
        // Interpolate position for smooth rendering
        float render_x = state.prev_quad_x + (state.quad_x - state.prev_quad_x) * alpha;
        float render_y = state.prev_quad_y + (state.quad_y - state.prev_quad_y) * alpha;

        // Animate clear color
        float r = std::sin(state.color_time * 0.5f) * 0.5f + 0.5f;
        float g = std::sin(state.color_time * 0.7f + 2.0f) * 0.5f + 0.5f;
        float b = std::sin(state.color_time * 0.9f + 4.0f) * 0.5f + 0.5f;
        renderer->set_clear_color({r, g, b, 1.0f});

        // Render frame
        renderer->begin_frame();
        renderer->clear();

        // Draw the player-controlled quad at interpolated position
        renderer->draw_quad({render_x, render_y}, {0.3f, 0.3f}, cafe::Color::white());

        renderer->end_frame();
    });

    // FPS callback - runs once per second
    loop.set_frame_callback([&](int fps, float avg_frame_time) {
        (void)avg_frame_time;  // Unused for now
        std::string title = "Cafe Engine - Metal [" + std::to_string(fps) + " FPS]";
        window->set_title(title);
    });

    // Run the game loop (blocks until window closes or loop.stop() is called)
    loop.run();

    // Cleanup
    renderer->shutdown();

    std::cout << "\nWindow closed. Goodbye!\n";
    return 0;
}

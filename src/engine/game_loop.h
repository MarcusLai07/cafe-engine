#ifndef CAFE_GAME_LOOP_H
#define CAFE_GAME_LOOP_H

#include <functional>

namespace cafe {

// Forward declarations
class Platform;
class Window;

// ============================================================================
// Fixed Timestep Game Loop
// ============================================================================
//
// Provides deterministic game updates at a fixed rate (default 60 Hz) while
// rendering as fast as possible. This ensures consistent game behavior
// regardless of frame rate.
//
// Usage:
//   GameLoop loop(platform.get(), window.get());
//   loop.set_update_callback([&](float dt) {
//       // Update game logic with fixed dt (e.g., 1/60 second)
//   });
//   loop.set_render_callback([&](float alpha) {
//       // Render with interpolation alpha (0.0 to 1.0)
//   });
//   loop.run();
//
// ============================================================================

class GameLoop {
public:
    // Callback types
    using UpdateCallback = std::function<void(float dt)>;       // Fixed timestep update
    using RenderCallback = std::function<void(float alpha)>;    // Render with interpolation
    using FrameCallback = std::function<void(int fps, float frame_time)>;  // FPS reporting

    GameLoop(Platform* platform, Window* window);

    // Configuration
    void set_target_fps(int fps);           // Target update rate (default: 60)
    void set_max_frame_skip(int frames);    // Max updates per frame (default: 5)

    // Callbacks
    void set_update_callback(UpdateCallback callback);
    void set_render_callback(RenderCallback callback);
    void set_frame_callback(FrameCallback callback);  // Called once per second

    // Run the loop (blocks until window closes)
    void run();

    // Loop control
    void stop();
    bool is_running() const { return running_; }

    // Timing info
    float fixed_delta_time() const { return fixed_dt_; }
    float frame_time() const { return frame_time_; }
    int current_fps() const { return current_fps_; }

private:
    Platform* platform_;
    Window* window_;

    // Callbacks
    UpdateCallback update_callback_;
    RenderCallback render_callback_;
    FrameCallback frame_callback_;

    // Configuration
    float fixed_dt_ = 1.0f / 60.0f;  // Fixed timestep (60 Hz)
    int max_frame_skip_ = 5;          // Prevent spiral of death

    // State
    bool running_ = false;
    float frame_time_ = 0.0f;
    int current_fps_ = 0;
};

} // namespace cafe

#endif // CAFE_GAME_LOOP_H

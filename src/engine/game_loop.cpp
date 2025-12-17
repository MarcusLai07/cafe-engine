#include "game_loop.h"
#include "../platform/platform.h"

namespace cafe {

GameLoop::GameLoop(Platform* platform, Window* window)
    : platform_(platform)
    , window_(window)
{
}

void GameLoop::set_target_fps(int fps) {
    if (fps > 0) {
        fixed_dt_ = 1.0f / static_cast<float>(fps);
    }
}

void GameLoop::set_max_frame_skip(int frames) {
    if (frames > 0) {
        max_frame_skip_ = frames;
    }
}

void GameLoop::set_update_callback(UpdateCallback callback) {
    update_callback_ = std::move(callback);
}

void GameLoop::set_render_callback(RenderCallback callback) {
    render_callback_ = std::move(callback);
}

void GameLoop::set_frame_callback(FrameCallback callback) {
    frame_callback_ = std::move(callback);
}

void GameLoop::run() {
    running_ = true;

    double previous_time = platform_->get_time();
    double accumulator = 0.0;

    // FPS tracking
    double fps_timer = 0.0;
    int frame_count = 0;

    while (running_ && window_->is_open()) {
        // Poll platform events
        platform_->poll_events();

        // Check if window was closed during event processing
        if (!window_->is_open()) {
            break;
        }

        // Calculate frame time
        double current_time = platform_->get_time();
        double frame_dt = current_time - previous_time;
        previous_time = current_time;

        // Clamp frame time to avoid spiral of death
        // (e.g., if debugger pauses execution)
        if (frame_dt > fixed_dt_ * max_frame_skip_) {
            frame_dt = fixed_dt_ * max_frame_skip_;
        }

        frame_time_ = static_cast<float>(frame_dt);
        accumulator += frame_dt;

        // Fixed timestep updates
        int updates = 0;
        while (accumulator >= fixed_dt_ && updates < max_frame_skip_) {
            if (update_callback_) {
                update_callback_(fixed_dt_);
            }
            // Update input state after each fixed update
            window_->update_input();
            accumulator -= fixed_dt_;
            updates++;
        }

        // Calculate interpolation alpha for smooth rendering
        float alpha = static_cast<float>(accumulator / fixed_dt_);

        // Render
        if (render_callback_) {
            render_callback_(alpha);
        }

        // FPS tracking
        frame_count++;
        fps_timer += frame_dt;
        if (fps_timer >= 1.0) {
            current_fps_ = frame_count;
            if (frame_callback_) {
                frame_callback_(current_fps_, static_cast<float>(fps_timer / frame_count));
            }
            frame_count = 0;
            fps_timer = 0.0;
        }
    }

    running_ = false;
}

void GameLoop::stop() {
    running_ = false;
}

} // namespace cafe

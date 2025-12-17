# Game Loop Architecture

The game loop is the heartbeat of the engine - it drives updates and rendering at a consistent rate.

## Fixed Timestep Design

We use a fixed timestep for game logic with variable rendering:

```
┌─────────────────────────────────────────────────────────────────┐
│                         FRAME                                   │
├─────────────────────────────────────────────────────────────────┤
│  Input  │  Update (fixed)  │  Update (fixed)  │     Render     │
│  Poll   │    16.67ms       │    16.67ms       │   (variable)   │
└─────────────────────────────────────────────────────────────────┘
          │◀─── accumulator ───▶│
```

**Why fixed timestep?**
- Physics and game logic are deterministic
- Same behavior regardless of frame rate
- Easier to reason about and debug
- Replays and networking are simpler

## Implementation

```cpp
// engine/game_loop.h

namespace cafe {

class GameLoop {
public:
    static constexpr double FIXED_DT = 1.0 / 60.0;  // 60 updates per second
    static constexpr int MAX_UPDATES_PER_FRAME = 5; // Prevent spiral of death

private:
    double accumulator_ = 0.0;
    double previous_time_ = 0.0;
    double game_time_ = 0.0;
    bool running_ = false;

    Platform* platform_;
    Game* game_;
    Renderer* renderer_;

public:
    void run() {
        running_ = true;
        previous_time_ = platform_->get_time();

        while (running_ && !platform_->should_quit()) {
            double current_time = platform_->get_time();
            double frame_time = current_time - previous_time_;
            previous_time_ = current_time;

            // Clamp frame time to prevent spiral of death
            if (frame_time > MAX_UPDATES_PER_FRAME * FIXED_DT) {
                frame_time = MAX_UPDATES_PER_FRAME * FIXED_DT;
            }

            accumulator_ += frame_time;

            // Process input once per frame
            platform_->poll_events();
            process_input();

            // Fixed timestep updates
            while (accumulator_ >= FIXED_DT) {
                game_->update(FIXED_DT);
                game_time_ += FIXED_DT;
                accumulator_ -= FIXED_DT;
            }

            // Render with interpolation
            double alpha = accumulator_ / FIXED_DT;
            render(alpha);
        }
    }

private:
    void process_input() {
        InputEvent event;
        while (platform_->pop_event(&event)) {
            game_->handle_input(event);
        }
    }

    void render(double alpha) {
        renderer_->begin_frame();
        game_->render(renderer_, alpha);
        renderer_->end_frame();
        renderer_->present();
    }
};

} // namespace cafe
```

## Game Interface

```cpp
// engine/game.h

namespace cafe {

class Game {
public:
    virtual ~Game() = default;

    // Called once at startup
    virtual bool initialize() = 0;

    // Called once at shutdown
    virtual void shutdown() = 0;

    // Called for each input event
    virtual void handle_input(const InputEvent& event) = 0;

    // Called at fixed timestep (60 times per second)
    virtual void update(double dt) = 0;

    // Called each frame, alpha is interpolation factor (0.0 to 1.0)
    virtual void render(Renderer* renderer, double alpha) = 0;
};

} // namespace cafe
```

## Interpolation for Smooth Rendering

Since update and render rates differ, interpolate positions for smooth visuals:

```cpp
// Without interpolation: choppy when update rate != render rate
// With interpolation: smooth at any frame rate

struct Transform {
    float x, y;              // Current position (after update)
    float prev_x, prev_y;    // Previous position (before update)

    // Call at start of each update
    void save_previous() {
        prev_x = x;
        prev_y = y;
    }

    // Get interpolated position for rendering
    float render_x(float alpha) const {
        return prev_x + (x - prev_x) * alpha;
    }

    float render_y(float alpha) const {
        return prev_y + (y - prev_y) * alpha;
    }
};

// In entity update:
void Entity::update(double dt) {
    transform_.save_previous();
    // ... update transform_.x, transform_.y
}

// In entity render:
void Entity::render(Renderer* r, double alpha) {
    float render_x = transform_.render_x(alpha);
    float render_y = transform_.render_y(alpha);
    // Draw at interpolated position
}
```

## Time Management

```cpp
// engine/time.h

namespace cafe {

class Time {
private:
    double game_time_ = 0.0;      // Total game time (paused time excluded)
    double real_time_ = 0.0;      // Real elapsed time
    double time_scale_ = 1.0;     // Speed multiplier (0.5 = half speed, 2.0 = double)
    bool paused_ = false;

public:
    void update(double dt) {
        real_time_ += dt;
        if (!paused_) {
            game_time_ += dt * time_scale_;
        }
    }

    double game_time() const { return game_time_; }
    double real_time() const { return real_time_; }
    double time_scale() const { return time_scale_; }
    bool is_paused() const { return paused_; }

    void set_time_scale(double scale) { time_scale_ = scale; }
    void pause() { paused_ = true; }
    void resume() { paused_ = false; }
    void toggle_pause() { paused_ = !paused_; }
};

// For the cafe game, we'll also have:
// - In-game day/night time
// - Speed controls (1x, 2x, 3x)
// - Pause for menus

} // namespace cafe
```

## Web Platform Considerations

On web, we can't use a traditional while loop. Instead, use `requestAnimationFrame`:

```cpp
// platform/web/web_game_loop.cpp

static GameLoop* g_loop = nullptr;

void web_frame() {
    g_loop->tick();  // Single frame update + render
    emscripten_request_animation_frame(web_frame_callback, nullptr);
}

EM_BOOL web_frame_callback(double time, void* data) {
    web_frame();
    return EM_TRUE;
}

void GameLoop::run_web() {
    g_loop = this;
    previous_time_ = platform_->get_time();
    emscripten_request_animation_frame(web_frame_callback, nullptr);
    // Control returns immediately - browser calls web_frame each frame
}

// tick() does one iteration of what run() does in the while loop
void GameLoop::tick() {
    // Same logic as inside the while loop
}
```

## Performance Monitoring

```cpp
// engine/profiler.h

struct FrameStats {
    double update_time_ms;
    double render_time_ms;
    double frame_time_ms;
    int update_count;  // Updates this frame
    int draw_calls;
    int sprite_count;
};

class Profiler {
private:
    std::array<FrameStats, 60> history_;
    int index_ = 0;

public:
    void record_frame(const FrameStats& stats) {
        history_[index_] = stats;
        index_ = (index_ + 1) % 60;
    }

    double average_fps() const {
        double total = 0;
        for (const auto& s : history_) {
            total += s.frame_time_ms;
        }
        return 1000.0 / (total / 60.0);
    }

    // Render debug overlay with stats
    void render_debug(Renderer* r);
};
```

## Main Entry Point

```cpp
// main.cpp (macOS)

int main(int argc, char* argv[]) {
    auto platform = create_platform();
    platform->initialize({
        .window_title = "Cafe Engine",
        .window_width = 1280,
        .window_height = 720,
        .fullscreen = false,
        .resizable = true
    });

    auto renderer = create_renderer(GraphicsAPI::Metal);
    renderer->initialize(platform->get_native_handle(), {
        .render_width = 320,   // Internal resolution
        .render_height = 180,
        .window_width = 1280,
        .window_height = 720,
        .pixel_perfect = true
    });

    auto game = std::make_unique<CafeGame>();
    game->initialize();

    GameLoop loop(platform.get(), game.get(), renderer.get());
    loop.run();

    game->shutdown();
    renderer->shutdown();
    platform->shutdown();

    return 0;
}
```

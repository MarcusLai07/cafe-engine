# UI System Architecture

The UI system handles menus, HUD elements, buttons, and text for the game interface.

## Design Goals

1. **Immediate mode inspired** - Simple API, minimal state management
2. **Resolution independent** - UI scales properly across devices
3. **Pixel art friendly** - Crisp rendering, 9-slice for panels
4. **Accessible** - Keyboard navigation, clear visual feedback

## UI Coordinate System

```
┌─────────────────────────────────────────┐
│ (0,0)                          (320,0)  │
│                                         │
│              GAME VIEW                  │
│            (pixel coords)               │
│                                         │
│ (0,180)                      (320,180)  │
└─────────────────────────────────────────┘

UI renders in game resolution (320x180) and scales with the game.
```

## Core Types

```cpp
// framework/ui/ui.h

namespace cafe {
namespace ui {

struct Rect {
    float x, y, width, height;

    bool contains(float px, float py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
};

struct Padding {
    float left, right, top, bottom;
};

enum class Anchor {
    TopLeft, TopCenter, TopRight,
    MiddleLeft, Center, MiddleRight,
    BottomLeft, BottomCenter, BottomRight
};

// Calculate position from anchor
Rect anchor_rect(Anchor anchor, float width, float height, float screen_w, float screen_h, float margin = 0) {
    Rect r = {0, 0, width, height};

    switch (anchor) {
        case Anchor::TopLeft:      r.x = margin; r.y = margin; break;
        case Anchor::TopCenter:    r.x = (screen_w - width) / 2; r.y = margin; break;
        case Anchor::TopRight:     r.x = screen_w - width - margin; r.y = margin; break;
        case Anchor::MiddleLeft:   r.x = margin; r.y = (screen_h - height) / 2; break;
        case Anchor::Center:       r.x = (screen_w - width) / 2; r.y = (screen_h - height) / 2; break;
        case Anchor::MiddleRight:  r.x = screen_w - width - margin; r.y = (screen_h - height) / 2; break;
        case Anchor::BottomLeft:   r.x = margin; r.y = screen_h - height - margin; break;
        case Anchor::BottomCenter: r.x = (screen_w - width) / 2; r.y = screen_h - height - margin; break;
        case Anchor::BottomRight:  r.x = screen_w - width - margin; r.y = screen_h - height - margin; break;
    }

    return r;
}

} // namespace ui
} // namespace cafe
```

## UI Context

```cpp
// framework/ui/ui_context.h

namespace cafe {
namespace ui {

// Tracks UI state for the current frame
class UIContext {
private:
    Renderer* renderer_;
    Input* input_;
    SoundManager* audio_;

    // Mouse state in UI coordinates
    float mouse_x_, mouse_y_;
    bool mouse_down_, mouse_pressed_, mouse_released_;

    // Hot/Active tracking
    uint32_t hot_id_ = 0;      // Widget mouse is over
    uint32_t active_id_ = 0;   // Widget being interacted with

    // ID generation
    uint32_t current_id_ = 0;

    // Layout state
    float cursor_x_ = 0, cursor_y_ = 0;
    float line_height_ = 0;

    // Styling
    TextureHandle skin_texture_ = INVALID_TEXTURE;
    struct Style {
        Color text_color = {1, 1, 1, 1};
        Color button_color = {0.3f, 0.3f, 0.4f, 1};
        Color button_hover_color = {0.4f, 0.4f, 0.5f, 1};
        Color button_active_color = {0.2f, 0.2f, 0.3f, 1};
        Color panel_color = {0.2f, 0.2f, 0.25f, 0.9f};
    } style_;

public:
    UIContext(Renderer* renderer, Input* input, SoundManager* audio)
        : renderer_(renderer), input_(input), audio_(audio) {}

    void begin_frame() {
        mouse_x_ = input_->mouse_x();
        mouse_y_ = input_->mouse_y();
        mouse_down_ = input_->mouse_down(MouseButton::Left);
        mouse_pressed_ = input_->mouse_pressed(MouseButton::Left);
        mouse_released_ = input_->mouse_released(MouseButton::Left);

        current_id_ = 1;
        hot_id_ = 0;
    }

    void end_frame() {
        if (!mouse_down_) {
            active_id_ = 0;
        }
    }

    // Generate unique ID for widget
    uint32_t next_id() { return current_id_++; }

    // Check if mouse is over rect
    bool is_hot(uint32_t id, const Rect& bounds) {
        if (bounds.contains(mouse_x_, mouse_y_)) {
            hot_id_ = id;
            return true;
        }
        return false;
    }

    // Check if widget is being clicked
    bool is_active(uint32_t id) const { return active_id_ == id; }

    void set_active(uint32_t id) { active_id_ = id; }

    // Accessors
    Renderer* renderer() { return renderer_; }
    float mouse_x() const { return mouse_x_; }
    float mouse_y() const { return mouse_y_; }
    bool mouse_pressed() const { return mouse_pressed_; }
    bool mouse_released() const { return mouse_released_; }

    const Style& style() const { return style_; }
    void set_style(const Style& s) { style_ = s; }
};

} // namespace ui
} // namespace cafe
```

## Widget Functions

```cpp
// framework/ui/widgets.h

namespace cafe {
namespace ui {

// Button - returns true if clicked
bool button(UIContext& ctx, const Rect& bounds, const std::string& text) {
    uint32_t id = ctx.next_id();
    bool hot = ctx.is_hot(id, bounds);
    bool clicked = false;

    // Handle interaction
    if (hot) {
        if (ctx.mouse_pressed()) {
            ctx.set_active(id);
        }
    }

    if (ctx.is_active(id) && ctx.mouse_released()) {
        if (hot) {
            clicked = true;
            // Play click sound
        }
    }

    // Determine visual state
    Color color = ctx.style().button_color;
    if (ctx.is_active(id)) {
        color = ctx.style().button_active_color;
    } else if (hot) {
        color = ctx.style().button_hover_color;
    }

    // Draw button background
    ctx.renderer()->draw_rect(bounds, color);

    // Draw text centered
    // (text rendering covered in font system)
    // draw_text_centered(ctx, bounds, text);

    return clicked;
}

// Label - just draws text
void label(UIContext& ctx, float x, float y, const std::string& text, Color color = {1,1,1,1}) {
    // draw_text(ctx, x, y, text, color);
}

// Panel - 9-slice background
void panel(UIContext& ctx, const Rect& bounds) {
    // Draw 9-slice panel using skin texture
    // Corners stay fixed size, edges stretch, center fills
    draw_9slice(ctx, bounds, /* panel slice coords */);
}

// Checkbox - returns new checked state
bool checkbox(UIContext& ctx, const Rect& bounds, const std::string& label, bool checked) {
    uint32_t id = ctx.next_id();
    bool hot = ctx.is_hot(id, bounds);

    if (hot && ctx.mouse_pressed()) {
        ctx.set_active(id);
    }

    if (ctx.is_active(id) && ctx.mouse_released() && hot) {
        checked = !checked;
    }

    // Draw checkbox box
    Rect box = {bounds.x, bounds.y, 12, 12};
    ctx.renderer()->draw_rect(box, hot ? ctx.style().button_hover_color : ctx.style().button_color);

    if (checked) {
        // Draw checkmark
        Rect inner = {box.x + 2, box.y + 2, 8, 8};
        ctx.renderer()->draw_rect(inner, ctx.style().text_color);
    }

    // Draw label
    // draw_text(ctx, bounds.x + 16, bounds.y, label);

    return checked;
}

// Slider - returns new value
float slider(UIContext& ctx, const Rect& bounds, float value, float min_val, float max_val) {
    uint32_t id = ctx.next_id();
    bool hot = ctx.is_hot(id, bounds);

    if (hot && ctx.mouse_pressed()) {
        ctx.set_active(id);
    }

    if (ctx.is_active(id)) {
        // Update value based on mouse position
        float t = (ctx.mouse_x() - bounds.x) / bounds.width;
        t = std::clamp(t, 0.0f, 1.0f);
        value = min_val + t * (max_val - min_val);
    }

    // Draw track
    ctx.renderer()->draw_rect(bounds, ctx.style().button_color);

    // Draw fill
    float t = (value - min_val) / (max_val - min_val);
    Rect fill = {bounds.x, bounds.y, bounds.width * t, bounds.height};
    ctx.renderer()->draw_rect(fill, ctx.style().button_hover_color);

    // Draw handle
    float handle_x = bounds.x + bounds.width * t - 4;
    Rect handle = {handle_x, bounds.y - 2, 8, bounds.height + 4};
    ctx.renderer()->draw_rect(handle, ctx.style().text_color);

    return value;
}

// Progress bar - display only
void progress_bar(UIContext& ctx, const Rect& bounds, float progress, Color fill_color) {
    // Background
    ctx.renderer()->draw_rect(bounds, ctx.style().button_color);

    // Fill
    Rect fill = {bounds.x, bounds.y, bounds.width * std::clamp(progress, 0.0f, 1.0f), bounds.height};
    ctx.renderer()->draw_rect(fill, fill_color);
}

} // namespace ui
} // namespace cafe
```

## 9-Slice Rendering

For stretchy panels that maintain crisp corners:

```cpp
// framework/ui/nine_slice.h

struct NineSliceInfo {
    Rect texture_rect;  // Full region in texture
    int border_left, border_right, border_top, border_bottom;
};

void draw_9slice(UIContext& ctx, const Rect& bounds, const NineSliceInfo& info) {
    TextureHandle tex = ctx.skin_texture();
    auto& tr = info.texture_rect;

    float bl = info.border_left, br = info.border_right;
    float bt = info.border_top, bb = info.border_bottom;

    // Source rects (9 regions of texture)
    Rect src[9] = {
        {tr.x, tr.y, bl, bt},                              // Top-left
        {tr.x + bl, tr.y, tr.width - bl - br, bt},         // Top
        {tr.x + tr.width - br, tr.y, br, bt},              // Top-right
        {tr.x, tr.y + bt, bl, tr.height - bt - bb},        // Left
        {tr.x + bl, tr.y + bt, tr.width - bl - br, tr.height - bt - bb}, // Center
        {tr.x + tr.width - br, tr.y + bt, br, tr.height - bt - bb},      // Right
        {tr.x, tr.y + tr.height - bb, bl, bb},             // Bottom-left
        {tr.x + bl, tr.y + tr.height - bb, tr.width - bl - br, bb},      // Bottom
        {tr.x + tr.width - br, tr.y + tr.height - bb, br, bb}            // Bottom-right
    };

    // Destination rects (scaled to bounds)
    float cw = bounds.width - bl - br;  // Center width
    float ch = bounds.height - bt - bb; // Center height

    Rect dst[9] = {
        {bounds.x, bounds.y, bl, bt},
        {bounds.x + bl, bounds.y, cw, bt},
        {bounds.x + bounds.width - br, bounds.y, br, bt},
        {bounds.x, bounds.y + bt, bl, ch},
        {bounds.x + bl, bounds.y + bt, cw, ch},
        {bounds.x + bounds.width - br, bounds.y + bt, br, ch},
        {bounds.x, bounds.y + bounds.height - bb, bl, bb},
        {bounds.x + bl, bounds.y + bounds.height - bb, cw, bb},
        {bounds.x + bounds.width - br, bounds.y + bounds.height - bb, br, bb}
    };

    for (int i = 0; i < 9; i++) {
        ctx.renderer()->draw_sprite(tex, src[i], dst[i]);
    }
}
```

## Game HUD Example

```cpp
// game/ui/hud.cpp

void CafeGame::render_hud(UIContext& ctx) {
    float screen_w = 320, screen_h = 180;

    // Money display (top-right)
    {
        Rect panel_rect = anchor_rect(Anchor::TopRight, 80, 20, screen_w, screen_h, 4);
        ui::panel(ctx, panel_rect);
        ui::label(ctx, panel_rect.x + 4, panel_rect.y + 4,
                  "$ " + std::to_string(economy_->money()));
    }

    // Time display (top-center)
    {
        Rect panel_rect = anchor_rect(Anchor::TopCenter, 60, 20, screen_w, screen_h, 4);
        ui::panel(ctx, panel_rect);
        ui::label(ctx, panel_rect.x + 4, panel_rect.y + 4,
                  time_system_->formatted_time());  // "Day 3 14:30"
    }

    // Speed controls (top-left)
    {
        Rect panel_rect = anchor_rect(Anchor::TopLeft, 50, 20, screen_w, screen_h, 4);
        ui::panel(ctx, panel_rect);

        if (ui::button(ctx, {panel_rect.x + 2, panel_rect.y + 2, 14, 16}, "||")) {
            toggle_pause();
        }
        if (ui::button(ctx, {panel_rect.x + 18, panel_rect.y + 2, 14, 16}, ">")) {
            set_speed(1);
        }
        if (ui::button(ctx, {panel_rect.x + 34, panel_rect.y + 2, 14, 16}, ">>")) {
            set_speed(2);
        }
    }
}
```

## Modal Dialogs

```cpp
// framework/ui/modal.h

class ModalDialog {
protected:
    bool visible_ = false;
    Rect bounds_;

public:
    virtual void show() { visible_ = true; }
    virtual void hide() { visible_ = false; }
    bool visible() const { return visible_; }

    virtual void update(UIContext& ctx) = 0;
    virtual void render(UIContext& ctx) = 0;
};

// Example: Pause menu
class PauseMenu : public ModalDialog {
public:
    enum Result { None, Resume, Settings, Quit };
    Result result = None;

    void update(UIContext& ctx) override {
        result = None;
        if (!visible_) return;

        bounds_ = anchor_rect(Anchor::Center, 100, 80, 320, 180);
    }

    void render(UIContext& ctx) override {
        if (!visible_) return;

        ui::panel(ctx, bounds_);

        ui::label(ctx, bounds_.x + 30, bounds_.y + 8, "PAUSED");

        if (ui::button(ctx, {bounds_.x + 10, bounds_.y + 24, 80, 14}, "Resume")) {
            result = Resume;
            hide();
        }
        if (ui::button(ctx, {bounds_.x + 10, bounds_.y + 42, 80, 14}, "Settings")) {
            result = Settings;
        }
        if (ui::button(ctx, {bounds_.x + 10, bounds_.y + 60, 80, 14}, "Quit")) {
            result = Quit;
        }
    }
};
```

# Phase 5: Game Framework

**Duration**: Months 12-14
**Goal**: Build higher-level systems for game logic
**Deliverable**: Interactive isometric world with UI and save/load

## Overview

This phase adds game-ready systems:
- UI system (menus, HUD)
- Save/load system
- State machines for entities
- A* pathfinding
- Camera controls

After this phase, you have a complete framework for building the cafe game.

## Tasks

### 5.1 UI System

**Goals:**
- Draw buttons, panels, labels
- Handle mouse interaction
- Support pixel art scaling

**Approach:** Immediate-mode inspired (simple, minimal state)

```cpp
// Usage example
void render_hud(UIContext& ctx) {
    // Money display
    Rect panel = {10, 10, 80, 24};
    ui::panel(ctx, panel);
    ui::label(ctx, panel.x + 8, panel.y + 6, "$" + std::to_string(money_));

    // Pause button
    if (ui::button(ctx, {screen_w - 30, 10, 20, 20}, "||")) {
        toggle_pause();
    }
}
```

**9-Slice Rendering:**
For panels that scale without stretching corners:
```
┌───┬───────┬───┐
│ 1 │   2   │ 3 │  <- Corners stay fixed
├───┼───────┼───┤
│ 4 │   5   │ 6 │  <- Edges stretch one direction
├───┼───────┼───┤
│ 7 │   8   │ 9 │  <- Center fills
└───┴───────┴───┘
```

**Checklist:**
- [ ] UIContext tracks hot/active widgets
- [ ] Button with hover/click states
- [ ] Panel (9-slice background)
- [ ] Label (text rendering)
- [ ] Checkbox
- [ ] Slider
- [ ] Modal dialogs

---

### 5.2 Save/Load System

**Goals:**
- Persist game state to JSON
- Atomic writes (no corruption)
- Version migration support

**Save Data Structure:**
```cpp
struct SaveData {
    int version;
    int day, hour, minute;
    int money, lifetime_earnings;
    std::vector<std::string> unlocked_items;
    std::vector<PlacedItem> placed_items;
    float music_volume, sfx_volume;
};
```

**Platform Storage:**
- macOS: `~/Library/Application Support/CafeEngine/`
- Web: localStorage
- iOS: Documents directory
- Windows: `%APPDATA%/CafeEngine/`

**Implementation:**
```cpp
class SaveManager {
public:
    bool save(const SaveData& data);
    bool load(SaveData* data);
    bool has_save();
    bool delete_save();
};

// Atomic save pattern
bool SaveManager::save(const SaveData& data) {
    std::string json = serialize(data);

    // Write to temp file first
    std::string temp_path = save_path_ + ".tmp";
    if (!write_file(temp_path, json)) return false;

    // Rename (atomic on most filesystems)
    return rename_file(temp_path, save_path_);
}
```

**Checklist:**
- [ ] Serialize game state to JSON
- [ ] Deserialize JSON to game state
- [ ] Atomic file writes
- [ ] Version field for migrations
- [ ] Works on Mac and Web

---

### 5.3 State Machines

**Goals:**
- Clean state management for entities
- Visual state tracking (idle, walking, etc.)
- Game state management (playing, paused, menu)

**Entity State Machine:**
```cpp
class StateMachine {
public:
    void add_state(const std::string& name, std::unique_ptr<State> state);
    void transition(const std::string& name);
    void update(double dt);

    const std::string& current_state() const;
};

// State base class
class State {
public:
    virtual void enter(Entity* entity) {}
    virtual void update(Entity* entity, double dt) {}
    virtual void exit(Entity* entity) {}
};

// Example: Customer states
class CustomerIdleState : public State {
    void enter(Entity* entity) override {
        entity->get_component<AnimationComponent>()->play("idle");
    }
    void update(Entity* entity, double dt) override {
        // Check for triggers
        if (order_ready) {
            entity->get_component<StateMachine>()->transition("walking_to_counter");
        }
    }
};
```

**Game State Machine:**
```cpp
enum class GameState {
    MainMenu,
    Playing,
    Paused,
    Settings,
    GameOver
};

class GameStateMachine {
    GameState current_ = GameState::MainMenu;

public:
    void transition(GameState new_state);
    void update(double dt);
    void render(Renderer* r);
};
```

**Checklist:**
- [ ] State base class
- [ ] StateMachine component
- [ ] Enter/exit callbacks
- [ ] Game state machine
- [ ] State transitions working

---

### 5.4 Pathfinding (A*)

**Goals:**
- Entities navigate around obstacles
- Works on isometric grid
- Efficient for small maps

**A* Implementation:**
```cpp
struct PathNode {
    int x, y;
    float g_cost;  // Cost from start
    float h_cost;  // Heuristic (estimated cost to goal)
    float f_cost() const { return g_cost + h_cost; }
    PathNode* parent;
};

class Pathfinder {
    TileMap* tilemap_;

public:
    std::vector<Vec2i> find_path(int start_x, int start_y, int goal_x, int goal_y) {
        std::priority_queue<PathNode*, std::vector<PathNode*>, CompareF> open;
        std::unordered_set<int> closed;

        // A* algorithm
        // 1. Add start to open list
        // 2. While open list not empty:
        //    a. Get node with lowest f_cost
        //    b. If goal, reconstruct path
        //    c. For each neighbor:
        //       - Skip if not walkable or in closed
        //       - Calculate g, h, f costs
        //       - Add to open if not there or better path found

        return reconstruct_path(goal_node);
    }

private:
    float heuristic(int x1, int y1, int x2, int y2) {
        // Manhattan distance for grid
        return std::abs(x2 - x1) + std::abs(y2 - y1);
    }

    std::vector<Vec2i> get_neighbors(int x, int y) {
        // 4-directional or 8-directional
        return {{x+1,y}, {x-1,y}, {x,y+1}, {x,y-1}};
    }
};
```

**Path Following Component:**
```cpp
class PathFollowerComponent : public Component {
    std::vector<Vec2i> path_;
    int path_index_ = 0;
    float speed_ = 50.0f;

public:
    void set_destination(int tile_x, int tile_y) {
        path_ = pathfinder_->find_path(current_x, current_y, tile_x, tile_y);
        path_index_ = 0;
    }

    void update(double dt) override {
        if (path_index_ >= path_.size()) return;

        Vec2i target = path_[path_index_];
        Vec2 target_world = tile_to_world(target);

        // Move toward target
        Vec2 dir = normalize(target_world - owner_->position());
        owner_->move(dir * speed_ * dt);

        // Check if reached waypoint
        if (distance(owner_->position(), target_world) < 2.0f) {
            path_index_++;
        }
    }
};
```

**Checklist:**
- [ ] A* algorithm implemented
- [ ] Works on isometric grid
- [ ] PathFollowerComponent
- [ ] Entities navigate around obstacles
- [ ] Smooth movement along path

---

### 5.5 Camera System

**Goals:**
- Pan camera with mouse/touch
- Zoom in/out
- Smooth following
- Screen shake for feedback

**Implementation:**
```cpp
class Camera {
    float x_, y_;           // Camera position
    float target_x_, target_y_;  // For smooth following
    float zoom_ = 1.0f;
    float shake_intensity_ = 0;
    float shake_duration_ = 0;

public:
    void set_position(float x, float y) {
        target_x_ = x;
        target_y_ = y;
    }

    void pan(float dx, float dy) {
        target_x_ += dx / zoom_;
        target_y_ += dy / zoom_;
    }

    void zoom_to(float level) {
        zoom_ = std::clamp(level, 0.5f, 4.0f);
    }

    void shake(float intensity, float duration) {
        shake_intensity_ = intensity;
        shake_duration_ = duration;
    }

    void update(double dt) {
        // Smooth follow
        float lerp_speed = 5.0f;
        x_ += (target_x_ - x_) * lerp_speed * dt;
        y_ += (target_y_ - y_) * lerp_speed * dt;

        // Update shake
        if (shake_duration_ > 0) {
            shake_duration_ -= dt;
        }
    }

    // Get camera position with shake applied
    Vec2 get_render_position() const {
        float shake_x = 0, shake_y = 0;
        if (shake_duration_ > 0) {
            shake_x = random_range(-shake_intensity_, shake_intensity_);
            shake_y = random_range(-shake_intensity_, shake_intensity_);
        }
        return {x_ + shake_x, y_ + shake_y};
    }

    // Convert screen coords to world coords
    Vec2 screen_to_world(float screen_x, float screen_y) const {
        return {
            (screen_x - screen_w/2) / zoom_ + x_,
            (screen_y - screen_h/2) / zoom_ + y_
        };
    }
};
```

**Checklist:**
- [ ] Camera position and zoom
- [ ] Pan with mouse drag
- [ ] Zoom with scroll wheel
- [ ] Smooth following
- [ ] Screen shake effect
- [ ] Screen-to-world coordinate conversion

---

## Milestone: Interactive World Demo

Create a demo that:
- Shows isometric cafe interior
- Click to place/remove furniture
- NPCs spawn and pathfind to destinations
- HUD shows money, time
- Pause menu with settings
- Save and load game state

This demo proves all framework systems work together.

---

## Checklist

- [ ] UI system with buttons, panels, sliders
- [ ] Save/load working on all platforms
- [ ] State machines for entities and game
- [ ] A* pathfinding working
- [ ] Camera pan, zoom, shake
- [ ] Interactive world demo complete

## Next Phase

Proceed to [Phase 6: Cafe Game - Core Loop](../phase-6-cafe-core-loop/overview.md) to implement game mechanics.

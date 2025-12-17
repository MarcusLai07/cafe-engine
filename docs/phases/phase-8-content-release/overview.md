# Phase 8: Content & Release

**Duration**: Months 21-24
**Goal**: Complete the game with content and polish for release
**Deliverable**: Shippable game on app stores and web

## Overview

The final phase focuses on:
- Completing remaining features (staff, building)
- Creating content (sprites, audio)
- Balancing and playtesting
- App store preparation

## Tasks

### 8.1 Staff Management System

**Features:**
- Hire staff members
- Assign to roles (barista, server, chef)
- Staff skill levels affect performance
- Wages reduce daily profit

**Implementation:**
```cpp
struct Staff {
    std::string name;
    StaffRole role;
    int skill_level;  // 1-5
    int wage;         // Daily cost
    float efficiency; // Based on skill
};

enum class StaffRole {
    Barista,   // Makes drinks faster
    Server,    // Delivers orders faster
    Chef       // Prepares food faster
};

class StaffSystem {
    std::vector<Staff> staff_;
    int max_staff_ = 5;

public:
    void hire(const Staff& staff) {
        if (staff_.size() < max_staff_) {
            staff_.push_back(staff);
        }
    }

    void fire(int index) {
        staff_.erase(staff_.begin() + index);
    }

    float get_prep_speed_bonus() const {
        // Calculate bonus based on staff skills
        float bonus = 1.0f;
        for (const auto& s : staff_) {
            if (s.role == StaffRole::Barista || s.role == StaffRole::Chef) {
                bonus += s.skill_level * 0.1f;
            }
        }
        return bonus;
    }

    int daily_wages() const {
        int total = 0;
        for (const auto& s : staff_) {
            total += s.wage;
        }
        return total;
    }
};
```

**UI:**
- Staff list with hire/fire buttons
- Role assignment dropdown
- Skill upgrade option (costs money)

**Checklist:**
- [ ] Staff data structure
- [ ] Hire/fire functionality
- [ ] Staff affects game mechanics
- [ ] Daily wage deduction
- [ ] Staff management UI

---

### 8.2 Building/Decoration System

**Features:**
- Place furniture (tables, chairs, decorations)
- Grid-based placement
- Furniture affects capacity/ambiance
- Cost to purchase items

**Implementation:**
```cpp
struct Furniture {
    std::string id;
    std::string name;
    int width, height;      // Tiles
    int cost;
    int capacity_bonus;     // Extra seating
    int ambiance_bonus;     // Affects customer satisfaction
    TextureHandle texture;
};

class BuildingSystem {
    TileMap* tilemap_;
    std::vector<PlacedFurniture> placed_;

    enum class Mode {
        Normal,
        Placing,
        Removing
    };
    Mode mode_ = Mode::Normal;
    std::string placing_id_;

public:
    void enter_build_mode(const std::string& furniture_id) {
        mode_ = Mode::Placing;
        placing_id_ = furniture_id;
    }

    void handle_click(int tile_x, int tile_y) {
        if (mode_ == Mode::Placing) {
            if (can_place(placing_id_, tile_x, tile_y)) {
                place(placing_id_, tile_x, tile_y);
            }
        } else if (mode_ == Mode::Removing) {
            remove_at(tile_x, tile_y);
        }
    }

    bool can_place(const std::string& id, int x, int y) {
        Furniture* f = get_furniture_def(id);
        // Check all tiles are empty and walkable
        for (int dy = 0; dy < f->height; dy++) {
            for (int dx = 0; dx < f->width; dx++) {
                if (is_occupied(x + dx, y + dy)) return false;
            }
        }
        return true;
    }

    void render_ghost(Renderer* r, int mouse_x, int mouse_y) {
        // Show semi-transparent preview while placing
        if (mode_ == Mode::Placing) {
            // Convert mouse to tile coords
            // Draw furniture sprite with alpha
        }
    }
};
```

**Furniture Data:**
```json
{
    "furniture": [
        {
            "id": "table_small",
            "name": "Small Table",
            "size": [1, 1],
            "cost": 50,
            "capacity": 2,
            "ambiance": 1,
            "sprite": "furniture/table_small.png"
        },
        {
            "id": "plant",
            "name": "Potted Plant",
            "size": [1, 1],
            "cost": 30,
            "capacity": 0,
            "ambiance": 5,
            "sprite": "furniture/plant.png"
        }
    ]
}
```

**Checklist:**
- [ ] Build mode toggle
- [ ] Grid-based placement preview
- [ ] Place/remove furniture
- [ ] Furniture affects gameplay
- [ ] Save placed furniture

---

### 8.3 Content Creation

**Required Assets:**

**Sprites (Pixel Art):**
```
assets/sprites/
├── characters/
│   ├── customer_1.png    # Customer sprite sheet
│   ├── customer_2.png
│   ├── customer_3.png
│   └── staff.png         # Staff sprite sheet
├── furniture/
│   ├── table_small.png
│   ├── table_large.png
│   ├── chair.png
│   ├── counter.png
│   └── decorations.png
├── items/
│   ├── coffee.png
│   ├── latte.png
│   └── food.png
├── tiles/
│   └── cafe_tileset.png  # Floor, walls
└── ui/
    ├── buttons.png
    ├── panels.png
    └── icons.png
```

**Animation Frames:**
Each character needs:
- Idle (2-4 frames)
- Walk (4-8 frames, 4 directions for isometric)
- Action (making drinks, eating, etc.)

**Audio:**
```
assets/audio/
├── music/
│   ├── menu_theme.ogg
│   └── cafe_ambient.ogg
└── sfx/
    ├── cash_register.wav
    ├── coffee_pour.wav
    ├── door_bell.wav
    ├── order_ready.wav
    └── ui_click.wav
```

**Tools for Content:**
- Aseprite (pixel art, animation)
- Tiled (tile map editor)
- Audacity (sound editing)
- BFXR/SFXR (retro sound effects)

**Checklist:**
- [ ] Customer sprites (3+ variants)
- [ ] Staff sprites
- [ ] Furniture sprites
- [ ] Tileset for cafe interior
- [ ] UI skin
- [ ] Background music
- [ ] Sound effects

---

### 8.4 Balancing & Playtesting

**Key Metrics to Balance:**

| Metric | Target | How to Adjust |
|--------|--------|---------------|
| Session length | 15-30 min | Adjust day length |
| Progression pace | New unlock every 10-15 min | Adjust XP requirements |
| Difficulty curve | Gradual increase | Customer patience, spawn rate |
| Economy | Always a goal to save for | Item prices, unlock costs |

**Playtesting Checklist:**
- [ ] Is the core loop satisfying?
- [ ] Is early game too easy/hard?
- [ ] Is progression meaningful?
- [ ] Are there frustrating moments?
- [ ] Is UI clear and usable?
- [ ] Any crashes or bugs?

**Analytics (Optional):**
```cpp
class Analytics {
public:
    void track_event(const std::string& name, const JsonValue& params);

    void on_level_up(int level) {
        track_event("level_up", {{"level", level}});
    }

    void on_session_end(int duration_sec) {
        track_event("session_end", {{"duration", duration_sec}});
    }
};
```

**Checklist:**
- [ ] Internal playtesting complete
- [ ] External playtesters (friends/family)
- [ ] Bug fixes from testing
- [ ] Balance adjustments made
- [ ] Final polish pass

---

### 8.5 Release Preparation

**App Store Requirements:**

**iOS (App Store):**
- App icons (various sizes)
- Screenshots (6.5", 5.5" iPhones, iPad)
- App description, keywords
- Privacy policy URL
- Age rating questionnaire
- TestFlight beta testing

**Android (Google Play):**
- App icons and feature graphic
- Screenshots (phone and tablet)
- Store listing description
- Content rating questionnaire
- Signed APK/AAB

**Web:**
- Hosting (itch.io, own domain)
- OpenGraph meta tags for sharing
- Loading screen

**Desktop (Steam/itch.io):**
- Store page assets
- Capsule images
- Trailer (optional but recommended)

**Release Checklist:**
- [ ] Version number set
- [ ] All placeholder content replaced
- [ ] Credits screen complete
- [ ] Privacy policy (if needed)
- [ ] App icons for all platforms
- [ ] Store screenshots
- [ ] Store descriptions
- [ ] Beta testing complete
- [ ] Build and submit to stores

---

## Final Checklist

### Game Complete
- [ ] Core loop is fun and polished
- [ ] All planned features implemented
- [ ] Content is complete (art, audio)
- [ ] No known critical bugs
- [ ] Save/load working perfectly
- [ ] Performance acceptable on all platforms

### Release Ready
- [ ] App store assets prepared
- [ ] Store listings written
- [ ] Beta tested
- [ ] Privacy policy (if applicable)
- [ ] Builds submitted to stores

---

## Post-Launch

**Consider for future updates:**
- More menu items
- Special events (holidays)
- Customer types with unique behaviors
- Cafe upgrades (kitchen, outdoor seating)
- Multiple cafe locations
- Achievements
- Leaderboards

---

## Congratulations!

If you've completed all 8 phases, you have:

1. **Learned C++ deeply** - Memory management, modern patterns
2. **Built a game engine from scratch** - Platform layer, renderer, audio
3. **Shipped a cross-platform game** - Mac, iOS, Android, Windows, Web
4. **Completed a full game** - Design, implementation, content, release

This is a significant achievement. The knowledge you've gained applies to any game development work in the future.

**What's Next?**
- Make another game with your engine
- Improve the engine based on lessons learned
- Open source the engine to help others learn
- Move to 3D? Networking? VR?

The skills you've built transfer to any of these directions. Good luck!

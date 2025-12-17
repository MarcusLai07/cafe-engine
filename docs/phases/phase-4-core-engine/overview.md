# Phase 4: Core Engine Systems

**Duration**: Months 9-11
**Goal**: Build foundational engine systems needed for any game
**Deliverable**: Engine can load a scene with sprites, audio, and entities

## Overview

This phase builds the core infrastructure:
- Resource/asset management
- Entity-Component system
- Scene management
- Audio system
- Input mapping

These systems form the foundation that game-specific code builds upon.

## Tasks

### 4.1 Resource Management

**Goals:**
- Unified asset loading across platforms
- Caching to prevent duplicate loads
- Handle-based access (not raw pointers)

**Implementation:**
```cpp
class ResourceManager {
public:
    // Load with auto type detection
    ResourceHandle load(const std::string& path);

    // Typed getters
    TextureHandle get_texture(ResourceHandle h);
    SoundHandle get_audio(ResourceHandle h);
    const JsonValue* get_data(ResourceHandle h);

    // Lifecycle
    void unload(ResourceHandle h);
    void unload_all();
};
```

**Image Loading (using stb_image):**
```cpp
// Single-header library - just include it
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool load_texture(const std::string& path, TextureData* out) {
    int w, h, channels;
    unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!pixels) return false;

    out->width = w;
    out->height = h;
    out->pixels.assign(pixels, pixels + w * h * 4);
    stbi_image_free(pixels);
    return true;
}
```

**Checklist:**
- [ ] Load PNG textures
- [ ] Load WAV audio
- [ ] Load JSON data files
- [ ] Resource caching working
- [ ] Unload unused resources

---

### 4.2 Entity-Component System

**Goals:**
- Flexible game object representation
- Easy to add behaviors to entities
- Efficient iteration over entities

**Implementation Approach:**
Simple Entity-Component (not full ECS):
- Entities own components directly
- Components have both data and behavior
- Simpler than cache-friendly ECS, sufficient for our scale

```cpp
// Entity owns components
class Entity {
    std::unordered_map<std::string, std::unique_ptr<Component>> components_;
public:
    template<typename T, typename... Args>
    T* add_component(Args&&... args);

    template<typename T>
    T* get_component();

    void update(double dt);
    void render(Renderer* r, double alpha);
};

// Component base class
class Component {
protected:
    Entity* owner_;
public:
    virtual void update(double dt) {}
    virtual void render(Renderer* r, double alpha) {}
};

// Example component
class SpriteComponent : public Component {
    TextureHandle texture_;
    Rect source_rect_;
public:
    void render(Renderer* r, double alpha) override {
        float x = owner_->render_x(alpha);
        float y = owner_->render_y(alpha);
        r->draw_sprite(texture_, source_rect_, {x, y, w, h});
    }
};
```

**Checklist:**
- [ ] Entity creation/destruction
- [ ] Component add/get/remove
- [ ] SpriteComponent working
- [ ] AnimationComponent working
- [ ] Entity queries (find by name, find with component)

---

### 4.3 Scene Management

**Goals:**
- Load/save scenes from data files
- Manage scene transitions
- Handle entity lifecycle within scenes

**Scene File Format (JSON):**
```json
{
    "name": "cafe_interior",
    "tilemap": "maps/cafe.tmj",
    "entities": [
        {
            "name": "counter",
            "position": [100, 50],
            "components": [
                {"type": "Sprite", "texture": "furniture/counter.png"}
            ]
        },
        {
            "name": "spawn_point",
            "position": [20, 100],
            "components": [
                {"type": "CustomerSpawner", "rate": 30}
            ]
        }
    ]
}
```

**Scene Class:**
```cpp
class Scene {
    std::string name_;
    EntityManager entities_;
    TileMap tilemap_;

public:
    bool load(const std::string& path, ResourceManager* resources);
    void unload();

    void update(double dt);
    void render(Renderer* r, double alpha);

    EntityManager& entities() { return entities_; }
};

class SceneManager {
    std::unique_ptr<Scene> current_scene_;
    std::unique_ptr<Scene> next_scene_;  // For transitions

public:
    void load_scene(const std::string& path);
    void update(double dt);
    void render(Renderer* r, double alpha);
};
```

**Checklist:**
- [ ] Load scene from JSON
- [ ] Create entities from scene data
- [ ] Scene transitions working
- [ ] Tilemap loads with scene

---

### 4.4 Audio System

**Goals:**
- Play music and sound effects
- Volume control
- Positional audio (stereo panning)

**Platform Implementations:**
- macOS: CoreAudio / AVFoundation
- Web: Web Audio API

**High-Level API:**
```cpp
class SoundManager {
public:
    void preload(const std::string& name, const std::string& path);

    // Sound effects (one-shot)
    void play_sfx(const std::string& name, float volume = 1.0f);
    void play_sfx_at(const std::string& name, float world_x, float camera_x);

    // Music (looping)
    void play_music(const std::string& name, float volume = 0.5f);
    void stop_music();
    void set_music_volume(float volume);
};

// Usage
sound_manager->preload("order_complete", "audio/sfx/ding.wav");
sound_manager->preload("bgm_cafe", "audio/music/cafe_theme.ogg");

sound_manager->play_music("bgm_cafe");
sound_manager->play_sfx("order_complete");
```

**Checklist:**
- [ ] WAV loading working
- [ ] Sound playback on Mac
- [ ] Sound playback on Web
- [ ] Music looping
- [ ] Volume control
- [ ] Stereo panning

---

### 4.5 Input Mapping

**Goals:**
- Map physical inputs to game actions
- Support rebinding
- Handle multiple input types (keyboard, mouse, touch)

**Implementation:**
```cpp
class InputMap {
public:
    ActionID register_action(const std::string& name);
    void bind(const std::string& action, Key key);
    void bind(const std::string& action, MouseButton button);

    bool is_action_pressed(const std::string& action);
    bool is_action_down(const std::string& action);
    bool is_action_released(const std::string& action);

    void save_bindings(const std::string& path);
    void load_bindings(const std::string& path);
};

// Setup
input_map->register_action("select");
input_map->bind("select", MouseButton::Left);
input_map->bind("select", Key::Space);

// In game
if (input_map->is_action_pressed("select")) {
    handle_selection();
}
```

**Checklist:**
- [ ] Action registration
- [ ] Keyboard bindings
- [ ] Mouse bindings
- [ ] Query action state
- [ ] Save/load bindings

---

## Milestone: Scene Demo

Create a demo that:
- Loads a scene from JSON file
- Shows isometric tilemap
- Has entities with sprites and animations
- Plays background music
- Has sound effects on interaction
- Uses action-based input

**Test:** Click on tiles to place/remove sprites, hear sound effects.

---

## Project Structure After Phase 4

```
cafe-engine/
├── src/
│   ├── platform/
│   │   ├── macos/
│   │   └── web/
│   ├── renderer/
│   │   ├── metal/
│   │   └── webgl/
│   ├── engine/
│   │   ├── resource_manager.cpp
│   │   ├── entity.cpp
│   │   ├── entity_manager.cpp
│   │   ├── scene.cpp
│   │   ├── scene_manager.cpp
│   │   ├── audio.cpp
│   │   └── input_map.cpp
│   └── components/
│       ├── sprite_component.cpp
│       └── animation_component.cpp
├── assets/
│   ├── textures/
│   ├── audio/
│   └── scenes/
└── docs/
```

---

## Checklist

- [ ] ResourceManager loads textures, audio, JSON
- [ ] Entity system working with components
- [ ] Scenes load from JSON
- [ ] Audio plays on both platforms
- [ ] Input mapping system working
- [ ] Scene demo complete

## Next Phase

Proceed to [Phase 5: Game Framework](../phase-5-game-framework/overview.md) to build UI, save system, and pathfinding.

# Entity System Architecture

The entity system manages game objects (customers, furniture, UI elements) in a flexible, performant way.

## Design Choice: Simple Entity-Component

We'll use a straightforward Entity-Component model rather than a full ECS (Entity-Component-System) architecture.

**Why not full ECS?**
- Overkill for a 2D cafe sim
- More complex to implement and understand
- Cache benefits matter less with our entity counts (<1000)

**Our approach:**
- Entities own their components directly
- Components contain data AND behavior
- Simple and intuitive for this game's scope

## Core Types

```cpp
// engine/entity.h

namespace cafe {

using EntityID = uint32_t;
constexpr EntityID INVALID_ENTITY = 0;

// Forward declarations
class Entity;
class Component;
class EntityManager;

// Base component class
class Component {
protected:
    Entity* owner_ = nullptr;

public:
    virtual ~Component() = default;

    virtual void initialize() {}
    virtual void update(double dt) {}
    virtual void render(Renderer* r, double alpha) {}

    Entity* owner() const { return owner_; }
    void set_owner(Entity* owner) { owner_ = owner; }

    // Type identification
    virtual const char* type_name() const = 0;
};

// Macro to reduce boilerplate
#define COMPONENT_TYPE(name) \
    const char* type_name() const override { return #name; } \
    static const char* static_type_name() { return #name; }

} // namespace cafe
```

## Entity Class

```cpp
// engine/entity.h (continued)

namespace cafe {

class Entity {
private:
    EntityID id_;
    std::string name_;
    bool active_ = true;
    bool marked_for_destroy_ = false;

    // Transform (all entities have position)
    float x_ = 0, y_ = 0, z_ = 0;
    float prev_x_ = 0, prev_y_ = 0, prev_z_ = 0;

    // Components stored by type
    std::unordered_map<std::string, std::unique_ptr<Component>> components_;

    // Parent/child hierarchy
    Entity* parent_ = nullptr;
    std::vector<Entity*> children_;

public:
    Entity(EntityID id, const std::string& name)
        : id_(id), name_(name) {}

    // Transform
    void set_position(float x, float y, float z = 0) {
        x_ = x; y_ = y; z_ = z;
    }

    float x() const { return x_; }
    float y() const { return y_; }
    float z() const { return z_; }

    // Interpolated position for rendering
    float render_x(float alpha) const { return prev_x_ + (x_ - prev_x_) * alpha; }
    float render_y(float alpha) const { return prev_y_ + (y_ - prev_y_) * alpha; }

    void save_previous_transform() {
        prev_x_ = x_; prev_y_ = y_; prev_z_ = z_;
    }

    // Component management
    template<typename T, typename... Args>
    T* add_component(Args&&... args) {
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = component.get();
        ptr->set_owner(this);
        components_[T::static_type_name()] = std::move(component);
        ptr->initialize();
        return ptr;
    }

    template<typename T>
    T* get_component() {
        auto it = components_.find(T::static_type_name());
        if (it != components_.end()) {
            return static_cast<T*>(it->second.get());
        }
        return nullptr;
    }

    template<typename T>
    bool has_component() const {
        return components_.find(T::static_type_name()) != components_.end();
    }

    // Lifecycle
    void update(double dt) {
        save_previous_transform();
        for (auto& [name, component] : components_) {
            component->update(dt);
        }
    }

    void render(Renderer* r, double alpha) {
        for (auto& [name, component] : components_) {
            component->render(r, alpha);
        }
    }

    // Hierarchy
    void add_child(Entity* child) {
        child->parent_ = this;
        children_.push_back(child);
    }

    void remove_child(Entity* child) {
        child->parent_ = nullptr;
        children_.erase(
            std::remove(children_.begin(), children_.end(), child),
            children_.end()
        );
    }

    // Getters
    EntityID id() const { return id_; }
    const std::string& name() const { return name_; }
    bool is_active() const { return active_; }
    void set_active(bool active) { active_ = active; }
    void destroy() { marked_for_destroy_ = true; }
    bool marked_for_destroy() const { return marked_for_destroy_; }
};

} // namespace cafe
```

## Entity Manager

```cpp
// engine/entity_manager.h

namespace cafe {

class EntityManager {
private:
    std::unordered_map<EntityID, std::unique_ptr<Entity>> entities_;
    EntityID next_id_ = 1;

    // Entities to add/remove (deferred to prevent iteration issues)
    std::vector<std::unique_ptr<Entity>> pending_add_;
    std::vector<EntityID> pending_remove_;

public:
    Entity* create_entity(const std::string& name = "") {
        auto entity = std::make_unique<Entity>(next_id_++, name);
        Entity* ptr = entity.get();
        pending_add_.push_back(std::move(entity));
        return ptr;
    }

    void destroy_entity(EntityID id) {
        pending_remove_.push_back(id);
    }

    Entity* get_entity(EntityID id) {
        auto it = entities_.find(id);
        return (it != entities_.end()) ? it->second.get() : nullptr;
    }

    void update(double dt) {
        // Process pending additions
        for (auto& entity : pending_add_) {
            entities_[entity->id()] = std::move(entity);
        }
        pending_add_.clear();

        // Update all active entities
        for (auto& [id, entity] : entities_) {
            if (entity->is_active()) {
                entity->update(dt);
            }
        }

        // Process pending removals and destroy-marked entities
        for (auto& [id, entity] : entities_) {
            if (entity->marked_for_destroy()) {
                pending_remove_.push_back(id);
            }
        }

        for (EntityID id : pending_remove_) {
            entities_.erase(id);
        }
        pending_remove_.clear();
    }

    void render(Renderer* r, double alpha) {
        // Collect and sort by depth for isometric rendering
        std::vector<Entity*> render_list;
        for (auto& [id, entity] : entities_) {
            if (entity->is_active()) {
                render_list.push_back(entity.get());
            }
        }

        std::sort(render_list.begin(), render_list.end(),
            [](Entity* a, Entity* b) {
                // Isometric depth: higher Y draws first, then by Z
                float depth_a = a->y() - a->z() * 0.5f;
                float depth_b = b->y() - b->z() * 0.5f;
                return depth_a < depth_b;
            });

        for (Entity* entity : render_list) {
            entity->render(r, alpha);
        }
    }

    // Query helpers
    template<typename T>
    std::vector<Entity*> find_with_component() {
        std::vector<Entity*> result;
        for (auto& [id, entity] : entities_) {
            if (entity->has_component<T>()) {
                result.push_back(entity.get());
            }
        }
        return result;
    }

    Entity* find_by_name(const std::string& name) {
        for (auto& [id, entity] : entities_) {
            if (entity->name() == name) {
                return entity.get();
            }
        }
        return nullptr;
    }
};

} // namespace cafe
```

## Common Components

```cpp
// engine/components/sprite_component.h

class SpriteComponent : public Component {
    COMPONENT_TYPE(SpriteComponent)

private:
    TextureHandle texture_ = INVALID_TEXTURE;
    Rect source_rect_;     // Region of texture to draw
    int width_, height_;   // Display size
    Color tint_ = Color::white();
    bool flip_x_ = false;

public:
    void set_texture(TextureHandle tex, int w, int h) {
        texture_ = tex;
        width_ = w;
        height_ = h;
        source_rect_ = {0, 0, (float)w, (float)h};
    }

    void set_source_rect(const Rect& rect) {
        source_rect_ = rect;
    }

    void render(Renderer* r, double alpha) override {
        float x = owner()->render_x(alpha);
        float y = owner()->render_y(alpha);

        Rect dst = {
            x - width_ / 2.0f,
            y - height_ / 2.0f,
            (float)width_,
            (float)height_
        };

        if (flip_x_) {
            dst.x += dst.width;
            dst.width = -dst.width;
        }

        r->draw_sprite(texture_, source_rect_, dst, tint_);
    }
};
```

```cpp
// engine/components/animation_component.h

struct AnimationFrame {
    Rect source_rect;
    float duration;  // Seconds
};

struct Animation {
    std::string name;
    std::vector<AnimationFrame> frames;
    bool looping = true;
};

class AnimationComponent : public Component {
    COMPONENT_TYPE(AnimationComponent)

private:
    SpriteComponent* sprite_ = nullptr;
    std::unordered_map<std::string, Animation> animations_;
    std::string current_animation_;
    int current_frame_ = 0;
    float frame_timer_ = 0;
    bool playing_ = false;

public:
    void initialize() override {
        sprite_ = owner()->get_component<SpriteComponent>();
    }

    void add_animation(const Animation& anim) {
        animations_[anim.name] = anim;
    }

    void play(const std::string& name, bool restart = false) {
        if (current_animation_ == name && !restart) return;
        current_animation_ = name;
        current_frame_ = 0;
        frame_timer_ = 0;
        playing_ = true;
        update_sprite();
    }

    void update(double dt) override {
        if (!playing_ || animations_.empty()) return;

        auto& anim = animations_[current_animation_];
        frame_timer_ += dt;

        while (frame_timer_ >= anim.frames[current_frame_].duration) {
            frame_timer_ -= anim.frames[current_frame_].duration;
            current_frame_++;

            if (current_frame_ >= anim.frames.size()) {
                if (anim.looping) {
                    current_frame_ = 0;
                } else {
                    current_frame_ = anim.frames.size() - 1;
                    playing_ = false;
                    break;
                }
            }
        }

        update_sprite();
    }

private:
    void update_sprite() {
        if (sprite_ && !animations_.empty()) {
            auto& anim = animations_[current_animation_];
            sprite_->set_source_rect(anim.frames[current_frame_].source_rect);
        }
    }
};
```

## Usage Example

```cpp
// Creating a customer entity

Entity* customer = entity_manager.create_entity("Customer");
customer->set_position(100, 200);

auto* sprite = customer->add_component<SpriteComponent>();
sprite->set_texture(customer_texture, 32, 48);

auto* anim = customer->add_component<AnimationComponent>();
anim->add_animation({
    .name = "idle",
    .frames = {
        {{0, 0, 32, 48}, 0.5f},
        {{32, 0, 32, 48}, 0.5f}
    },
    .looping = true
});
anim->add_animation({
    .name = "walk",
    .frames = {
        {{0, 48, 32, 48}, 0.1f},
        {{32, 48, 32, 48}, 0.1f},
        {{64, 48, 32, 48}, 0.1f},
        {{96, 48, 32, 48}, 0.1f}
    },
    .looping = true
});
anim->play("idle");

auto* behavior = customer->add_component<CustomerBehavior>();
// CustomerBehavior is game-specific, defined in game/ folder
```

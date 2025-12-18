#ifndef CAFE_ENTITY_H
#define CAFE_ENTITY_H

#include "../renderer/renderer.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <functional>

namespace cafe {

// Forward declarations
class Entity;
class EntityManager;

// ============================================================================
// Entity ID
// ============================================================================

using EntityID = uint32_t;
constexpr EntityID INVALID_ENTITY = 0;

// ============================================================================
// Component - Base class for all components
// ============================================================================

class Component {
    friend class Entity;  // Allow Entity to set owner_ during move operations

public:
    virtual ~Component() = default;

    // Called when component is added to entity
    virtual void on_attach(Entity* entity) { owner_ = entity; }

    // Called when component is removed from entity
    virtual void on_detach() { owner_ = nullptr; }

    // Get owning entity
    Entity* owner() const { return owner_; }

    // Enable/disable component
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }

protected:
    Entity* owner_ = nullptr;
    bool enabled_ = true;
};

// ============================================================================
// Common Components
// ============================================================================

// Transform - Position, rotation, scale
struct Transform : public Component {
    Vec2 position = {0, 0};
    Vec2 scale = {1, 1};
    float rotation = 0.0f;  // Radians

    // Helpers
    void translate(float dx, float dy) { position.x += dx; position.y += dy; }
    void translate(Vec2 delta) { position = position + delta; }
};

// SpriteRenderer - Draws a sprite
struct SpriteRenderer : public Component {
    TextureRegion region;
    Color tint = Color::white();
    Vec2 size = {32, 32};
    Vec2 origin = {0.5f, 0.5f};  // Center by default
    int layer = 0;               // Draw order (higher = on top)
    bool flip_x = false;
    bool flip_y = false;
};

// Animator - Plays sprite animations
class Animator : public Component {
public:
    void set_sprite_sheet(class SpriteSheet* sheet) { sheet_ = sheet; }
    void play(const std::string& animation, bool force = false);
    void update(float dt);
    void pause() { playing_ = false; }
    void resume() { playing_ = true; }
    void stop();

    bool is_playing() const { return playing_; }
    bool is_finished() const;
    const std::string& current_animation() const { return current_anim_; }
    TextureRegion current_region() const;

    float speed = 1.0f;

private:
    class SpriteSheet* sheet_ = nullptr;
    std::string current_anim_;
    float elapsed_ = 0.0f;
    bool playing_ = false;
};

// BoxCollider - Simple AABB collision
struct BoxCollider : public Component {
    Vec2 offset = {0, 0};   // Offset from transform position
    Vec2 size = {32, 32};   // Collision box size
    bool is_trigger = false; // Triggers don't block movement

    // Get world-space bounds
    Rect get_bounds() const;
};

// Tag - Simple string tag for identification
struct Tag : public Component {
    std::string value;

    Tag() = default;
    explicit Tag(const std::string& tag) : value(tag) {}
};

// ============================================================================
// Entity - Game object with components
// ============================================================================

class Entity {
public:
    explicit Entity(EntityID id, EntityManager* manager = nullptr);
    ~Entity();

    // Non-copyable, movable
    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;
    Entity(Entity&&) noexcept;
    Entity& operator=(Entity&&) noexcept;

    // Identity
    EntityID id() const { return id_; }
    const std::string& name() const { return name_; }
    void set_name(const std::string& name) { name_ = name; }

    // Enable/disable entire entity
    void set_active(bool active) { active_ = active; }
    bool is_active() const { return active_; }

    // Component management
    template<typename T, typename... Args>
    T* add_component(Args&&... args);

    template<typename T>
    T* get_component();

    template<typename T>
    const T* get_component() const;

    template<typename T>
    bool has_component() const;

    template<typename T>
    void remove_component();

    // Quick access to common components
    Transform* transform() { return get_component<Transform>(); }
    const Transform* transform() const { return get_component<Transform>(); }

    // Get all components
    const std::vector<std::unique_ptr<Component>>& components() const { return components_; }

    // Manager access
    EntityManager* manager() const { return manager_; }

private:
    EntityID id_ = INVALID_ENTITY;
    std::string name_;
    bool active_ = true;
    EntityManager* manager_ = nullptr;

    std::vector<std::unique_ptr<Component>> components_;
    std::unordered_map<std::type_index, Component*> component_map_;
};

// ============================================================================
// Entity Template Implementation
// ============================================================================

template<typename T, typename... Args>
T* Entity::add_component(Args&&... args) {
    static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

    std::type_index type = typeid(T);

    // Check if already has component
    if (component_map_.find(type) != component_map_.end()) {
        return static_cast<T*>(component_map_[type]);
    }

    // Create and add component
    auto component = std::make_unique<T>(std::forward<Args>(args)...);
    T* ptr = component.get();

    component_map_[type] = ptr;
    components_.push_back(std::move(component));

    ptr->on_attach(this);
    return ptr;
}

template<typename T>
T* Entity::get_component() {
    static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

    auto it = component_map_.find(typeid(T));
    if (it != component_map_.end()) {
        return static_cast<T*>(it->second);
    }
    return nullptr;
}

template<typename T>
const T* Entity::get_component() const {
    static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

    auto it = component_map_.find(typeid(T));
    if (it != component_map_.end()) {
        return static_cast<const T*>(it->second);
    }
    return nullptr;
}

template<typename T>
bool Entity::has_component() const {
    static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
    return component_map_.find(typeid(T)) != component_map_.end();
}

template<typename T>
void Entity::remove_component() {
    static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

    auto it = component_map_.find(typeid(T));
    if (it != component_map_.end()) {
        Component* comp = it->second;
        comp->on_detach();

        // Remove from vector
        components_.erase(
            std::remove_if(components_.begin(), components_.end(),
                [comp](const std::unique_ptr<Component>& c) {
                    return c.get() == comp;
                }),
            components_.end());

        component_map_.erase(it);
    }
}

// ============================================================================
// EntityManager - Creates and manages entities
// ============================================================================

class EntityManager {
public:
    EntityManager() = default;
    ~EntityManager() = default;

    // Create a new entity
    Entity* create_entity(const std::string& name = "");

    // Destroy an entity
    void destroy_entity(EntityID id);
    void destroy_entity(Entity* entity);

    // Get entity by ID
    Entity* get_entity(EntityID id);

    // Find entity by name (returns first match)
    Entity* find_entity(const std::string& name);

    // Find all entities with a specific component
    template<typename T>
    std::vector<Entity*> find_entities_with();

    // Find all entities with a specific tag
    std::vector<Entity*> find_entities_with_tag(const std::string& tag);

    // Iterate all entities
    void for_each(const std::function<void(Entity*)>& callback);

    // Iterate entities with specific component
    template<typename T>
    void for_each(const std::function<void(Entity*, T*)>& callback);

    // Entity count
    size_t entity_count() const { return entities_.size(); }

    // Clear all entities
    void clear();

    // Process pending destroys (call at end of frame)
    void process_pending_destroys();

private:
    EntityID next_id_ = 1;
    std::unordered_map<EntityID, std::unique_ptr<Entity>> entities_;
    std::vector<EntityID> pending_destroy_;
};

// ============================================================================
// EntityManager Template Implementation
// ============================================================================

template<typename T>
std::vector<Entity*> EntityManager::find_entities_with() {
    std::vector<Entity*> result;
    for (auto& [id, entity] : entities_) {
        if (entity->has_component<T>()) {
            result.push_back(entity.get());
        }
    }
    return result;
}

template<typename T>
void EntityManager::for_each(const std::function<void(Entity*, T*)>& callback) {
    for (auto& [id, entity] : entities_) {
        if (entity->is_active()) {
            T* component = entity->get_component<T>();
            if (component && component->is_enabled()) {
                callback(entity.get(), component);
            }
        }
    }
}

} // namespace cafe

#endif // CAFE_ENTITY_H

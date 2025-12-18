#ifndef CAFE_SCENE_H
#define CAFE_SCENE_H

#include "entity.h"
#include "resource.h"
#include "../renderer/renderer.h"
#include <string>
#include <memory>
#include <vector>
#include <functional>

namespace cafe {

// Forward declarations
class Scene;
class SceneManager;

// ============================================================================
// Scene - A game scene (level, menu, etc.)
// ============================================================================

class Scene {
public:
    explicit Scene(const std::string& name = "Scene");
    virtual ~Scene() = default;

    // Non-copyable
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    // Scene name
    const std::string& name() const { return name_; }

    // Lifecycle callbacks (override in subclasses)
    virtual void on_enter() {}   // Called when scene becomes active
    virtual void on_exit() {}    // Called when scene is deactivated
    virtual void on_pause() {}   // Called when scene is pushed down (overlay on top)
    virtual void on_resume() {}  // Called when overlay is popped

    // Update and render (override in subclasses)
    virtual void update(float dt);
    virtual void render(Renderer* renderer);

    // Entity management
    EntityManager& entities() { return entities_; }
    const EntityManager& entities() const { return entities_; }

    // Create entity helper
    Entity* create_entity(const std::string& name = "");

    // Find entities
    Entity* find_entity(const std::string& name);
    std::vector<Entity*> find_entities_with_tag(const std::string& tag);

    // Scene manager access (set by SceneManager)
    SceneManager* scene_manager() const { return scene_manager_; }

    // Check if scene is currently active
    bool is_active() const { return is_active_; }

protected:
    // Render all sprite renderers (called by default render())
    void render_sprites(Renderer* renderer);

    // Update all animators (called by default update())
    void update_animators(float dt);

    std::string name_;
    EntityManager entities_;
    SceneManager* scene_manager_ = nullptr;
    bool is_active_ = false;

    friend class SceneManager;
};

// ============================================================================
// SceneManager - Manages scene stack and transitions
// ============================================================================

class SceneManager {
public:
    SceneManager() = default;
    ~SceneManager() = default;

    // Non-copyable
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    // Initialize with renderer and resource manager
    void initialize(Renderer* renderer, ResourceManager* resources);

    // Access renderer and resources from scenes
    Renderer* renderer() const { return renderer_; }
    ResourceManager* resources() const { return resources_; }

    // ========================================================================
    // Scene Stack Operations
    // ========================================================================

    // Push a new scene onto the stack (becomes active, previous is paused)
    template<typename T, typename... Args>
    T* push_scene(Args&&... args);

    // Pop the current scene (returns to previous, or nullptr if empty)
    void pop_scene();

    // Replace current scene with a new one
    template<typename T, typename... Args>
    T* replace_scene(Args&&... args);

    // Clear all scenes
    void clear_scenes();

    // Get current active scene
    Scene* current_scene();
    const Scene* current_scene() const;

    // Get scene by index (0 = bottom, size-1 = top)
    Scene* scene_at(size_t index);

    // Scene count
    size_t scene_count() const { return scenes_.size(); }
    bool has_scenes() const { return !scenes_.empty(); }

    // ========================================================================
    // Update and Render
    // ========================================================================

    // Update current scene
    void update(float dt);

    // Render all visible scenes (from bottom to top)
    void render();

    // Process pending scene changes (call at end of frame)
    void process_pending();

    // ========================================================================
    // Scene Transitions
    // ========================================================================

    // Set transition callback (called during scene changes)
    using TransitionCallback = std::function<void(Scene* from, Scene* to)>;
    void set_transition_callback(TransitionCallback callback);

private:
    // Internal push without template
    void push_scene_internal(std::unique_ptr<Scene> scene);
    void replace_scene_internal(std::unique_ptr<Scene> scene);

    Renderer* renderer_ = nullptr;
    ResourceManager* resources_ = nullptr;

    std::vector<std::unique_ptr<Scene>> scenes_;
    TransitionCallback transition_callback_;

    // Pending operations (processed at end of frame)
    enum class PendingOp { None, Push, Pop, Replace, Clear };
    PendingOp pending_op_ = PendingOp::None;
    std::unique_ptr<Scene> pending_scene_;
};

// ============================================================================
// SceneManager Template Implementation
// ============================================================================

template<typename T, typename... Args>
T* SceneManager::push_scene(Args&&... args) {
    static_assert(std::is_base_of<Scene, T>::value, "T must derive from Scene");

    auto scene = std::make_unique<T>(std::forward<Args>(args)...);
    T* ptr = scene.get();
    pending_scene_ = std::move(scene);
    pending_op_ = PendingOp::Push;
    return ptr;
}

template<typename T, typename... Args>
T* SceneManager::replace_scene(Args&&... args) {
    static_assert(std::is_base_of<Scene, T>::value, "T must derive from Scene");

    auto scene = std::make_unique<T>(std::forward<Args>(args)...);
    T* ptr = scene.get();
    pending_scene_ = std::move(scene);
    pending_op_ = PendingOp::Replace;
    return ptr;
}

} // namespace cafe

#endif // CAFE_SCENE_H

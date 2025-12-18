#include "scene.h"
#include <algorithm>

namespace cafe {

// ============================================================================
// Scene Implementation
// ============================================================================

Scene::Scene(const std::string& name)
    : name_(name) {
}

Entity* Scene::create_entity(const std::string& name) {
    return entities_.create_entity(name);
}

Entity* Scene::find_entity(const std::string& name) {
    return entities_.find_entity(name);
}

std::vector<Entity*> Scene::find_entities_with_tag(const std::string& tag) {
    return entities_.find_entities_with_tag(tag);
}

void Scene::update(float dt) {
    // Update animators
    update_animators(dt);

    // Process pending entity destroys
    entities_.process_pending_destroys();
}

void Scene::render(Renderer* renderer) {
    render_sprites(renderer);
}

void Scene::update_animators(float dt) {
    entities_.for_each<Animator>([dt](Entity*, Animator* animator) {
        animator->update(dt);
    });
}

void Scene::render_sprites(Renderer* renderer) {
    if (!renderer) return;

    // Collect all sprites with their transforms
    struct SpriteEntry {
        const Transform* transform;
        const SpriteRenderer* sprite;
        int layer;
    };
    std::vector<SpriteEntry> sprites;

    entities_.for_each([&sprites](Entity* entity) {
        const Transform* transform = entity->transform();
        const SpriteRenderer* sprite = entity->get_component<SpriteRenderer>();

        if (transform && sprite && sprite->is_enabled()) {
            sprites.push_back({transform, sprite, sprite->layer});
        }

        // Also check for Animator to update sprite region
        Animator* animator = entity->get_component<Animator>();
        if (animator && animator->is_playing()) {
            // Note: Animator updates the SpriteRenderer's region
            // This connection would need to be managed by the game
        }
    });

    // Sort by layer (lower layers first)
    std::sort(sprites.begin(), sprites.end(),
        [](const SpriteEntry& a, const SpriteEntry& b) {
            return a.layer < b.layer;
        });

    // Render sprites
    renderer->begin_batch();

    for (const auto& entry : sprites) {
        Sprite s;
        s.position = entry.transform->position;
        s.size = entry.sprite->size;
        s.region = entry.sprite->region;
        s.tint = entry.sprite->tint;
        s.rotation = entry.transform->rotation;
        s.origin = entry.sprite->origin;

        // Handle flipping via UV coordinates
        if (entry.sprite->flip_x) {
            std::swap(s.region.u0, s.region.u1);
        }
        if (entry.sprite->flip_y) {
            std::swap(s.region.v0, s.region.v1);
        }

        // Apply scale
        s.size.x *= entry.transform->scale.x;
        s.size.y *= entry.transform->scale.y;

        renderer->draw_sprite(s);
    }

    renderer->end_batch();
}

// ============================================================================
// SceneManager Implementation
// ============================================================================

void SceneManager::initialize(Renderer* renderer, ResourceManager* resources) {
    renderer_ = renderer;
    resources_ = resources;
}

Scene* SceneManager::current_scene() {
    if (scenes_.empty()) return nullptr;
    return scenes_.back().get();
}

const Scene* SceneManager::current_scene() const {
    if (scenes_.empty()) return nullptr;
    return scenes_.back().get();
}

Scene* SceneManager::scene_at(size_t index) {
    if (index >= scenes_.size()) return nullptr;
    return scenes_[index].get();
}

void SceneManager::push_scene_internal(std::unique_ptr<Scene> scene) {
    // Pause current scene
    if (!scenes_.empty()) {
        scenes_.back()->is_active_ = false;
        scenes_.back()->on_pause();
    }

    // Set up new scene
    scene->scene_manager_ = this;
    scene->is_active_ = true;

    Scene* new_scene = scene.get();
    Scene* old_scene = scenes_.empty() ? nullptr : scenes_.back().get();

    // Add to stack
    scenes_.push_back(std::move(scene));

    // Transition callback
    if (transition_callback_) {
        transition_callback_(old_scene, new_scene);
    }

    // Enter new scene
    new_scene->on_enter();
}

void SceneManager::replace_scene_internal(std::unique_ptr<Scene> scene) {
    Scene* old_scene = nullptr;

    // Exit and remove current scene
    if (!scenes_.empty()) {
        old_scene = scenes_.back().get();
        old_scene->is_active_ = false;
        old_scene->on_exit();
        scenes_.pop_back();
    }

    // Set up new scene
    scene->scene_manager_ = this;
    scene->is_active_ = true;

    Scene* new_scene = scene.get();

    // Add to stack
    scenes_.push_back(std::move(scene));

    // Transition callback
    if (transition_callback_) {
        transition_callback_(old_scene, new_scene);
    }

    // Enter new scene
    new_scene->on_enter();
}

void SceneManager::pop_scene() {
    pending_op_ = PendingOp::Pop;
}

void SceneManager::clear_scenes() {
    pending_op_ = PendingOp::Clear;
}

void SceneManager::update(float dt) {
    Scene* scene = current_scene();
    if (scene && scene->is_active_) {
        scene->update(dt);
    }
}

void SceneManager::render() {
    if (!renderer_) return;

    // Render all scenes from bottom to top
    // (useful for overlays/menus that don't fully cover screen)
    for (auto& scene : scenes_) {
        scene->render(renderer_);
    }
}

void SceneManager::process_pending() {
    switch (pending_op_) {
        case PendingOp::Push:
            if (pending_scene_) {
                push_scene_internal(std::move(pending_scene_));
            }
            break;

        case PendingOp::Pop:
            if (!scenes_.empty()) {
                Scene* old_scene = scenes_.back().get();
                old_scene->is_active_ = false;
                old_scene->on_exit();

                scenes_.pop_back();

                // Resume previous scene
                if (!scenes_.empty()) {
                    Scene* new_scene = scenes_.back().get();
                    new_scene->is_active_ = true;
                    new_scene->on_resume();

                    if (transition_callback_) {
                        transition_callback_(old_scene, new_scene);
                    }
                }
            }
            break;

        case PendingOp::Replace:
            if (pending_scene_) {
                replace_scene_internal(std::move(pending_scene_));
            }
            break;

        case PendingOp::Clear:
            // Exit all scenes
            while (!scenes_.empty()) {
                Scene* scene = scenes_.back().get();
                scene->is_active_ = false;
                scene->on_exit();
                scenes_.pop_back();
            }
            break;

        case PendingOp::None:
        default:
            break;
    }

    pending_op_ = PendingOp::None;
    pending_scene_.reset();
}

void SceneManager::set_transition_callback(TransitionCallback callback) {
    transition_callback_ = std::move(callback);
}

} // namespace cafe

#include "entity.h"
#include "sprite_sheet.h"

namespace cafe {

// ============================================================================
// Animator Implementation
// ============================================================================

void Animator::play(const std::string& animation, bool force) {
    if (!force && playing_ && current_anim_ == animation) {
        return;
    }
    current_anim_ = animation;
    elapsed_ = 0.0f;
    playing_ = true;
}

void Animator::update(float dt) {
    if (!playing_ || !sheet_) return;

    elapsed_ += dt * speed;

    // Check if non-looping animation finished
    const Animation* anim = sheet_->animation(current_anim_);
    if (anim && !anim->looping) {
        if (elapsed_ >= anim->total_duration()) {
            playing_ = false;
        }
    }
}

void Animator::stop() {
    playing_ = false;
    current_anim_.clear();
    elapsed_ = 0.0f;
}

bool Animator::is_finished() const {
    if (!sheet_) return true;

    const Animation* anim = sheet_->animation(current_anim_);
    if (!anim) return true;
    if (anim->looping) return false;

    return elapsed_ >= anim->total_duration();
}

TextureRegion Animator::current_region() const {
    if (!sheet_) return TextureRegion();
    return sheet_->animation_frame(current_anim_, elapsed_);
}

// ============================================================================
// BoxCollider Implementation
// ============================================================================

Rect BoxCollider::get_bounds() const {
    if (!owner_) return Rect();

    const Transform* transform = owner_->transform();
    if (!transform) return Rect(offset.x, offset.y, size.x, size.y);

    return Rect(
        transform->position.x + offset.x - size.x / 2,
        transform->position.y + offset.y - size.y / 2,
        size.x,
        size.y
    );
}

// ============================================================================
// Entity Implementation
// ============================================================================

Entity::Entity(EntityID id, EntityManager* manager)
    : id_(id), manager_(manager) {
    // All entities have a Transform by default
    add_component<Transform>();
}

Entity::~Entity() {
    // Detach all components
    for (auto& comp : components_) {
        comp->on_detach();
    }
}

Entity::Entity(Entity&& other) noexcept
    : id_(other.id_)
    , name_(std::move(other.name_))
    , active_(other.active_)
    , manager_(other.manager_)
    , components_(std::move(other.components_))
    , component_map_(std::move(other.component_map_)) {
    other.id_ = INVALID_ENTITY;
    other.manager_ = nullptr;

    // Update owner pointers
    for (auto& comp : components_) {
        comp->owner_ = this;
    }
}

Entity& Entity::operator=(Entity&& other) noexcept {
    if (this != &other) {
        id_ = other.id_;
        name_ = std::move(other.name_);
        active_ = other.active_;
        manager_ = other.manager_;
        components_ = std::move(other.components_);
        component_map_ = std::move(other.component_map_);

        other.id_ = INVALID_ENTITY;
        other.manager_ = nullptr;

        for (auto& comp : components_) {
            comp->owner_ = this;
        }
    }
    return *this;
}

// ============================================================================
// EntityManager Implementation
// ============================================================================

Entity* EntityManager::create_entity(const std::string& name) {
    EntityID id = next_id_++;
    auto entity = std::make_unique<Entity>(id, this);
    entity->set_name(name.empty() ? "Entity_" + std::to_string(id) : name);

    Entity* ptr = entity.get();
    entities_[id] = std::move(entity);
    return ptr;
}

void EntityManager::destroy_entity(EntityID id) {
    // Queue for destruction (avoid issues during iteration)
    pending_destroy_.push_back(id);
}

void EntityManager::destroy_entity(Entity* entity) {
    if (entity) {
        destroy_entity(entity->id());
    }
}

Entity* EntityManager::get_entity(EntityID id) {
    auto it = entities_.find(id);
    if (it != entities_.end()) {
        return it->second.get();
    }
    return nullptr;
}

Entity* EntityManager::find_entity(const std::string& name) {
    for (auto& [id, entity] : entities_) {
        if (entity->name() == name) {
            return entity.get();
        }
    }
    return nullptr;
}

std::vector<Entity*> EntityManager::find_entities_with_tag(const std::string& tag) {
    std::vector<Entity*> result;
    for (auto& [id, entity] : entities_) {
        Tag* tag_comp = entity->get_component<Tag>();
        if (tag_comp && tag_comp->value == tag) {
            result.push_back(entity.get());
        }
    }
    return result;
}

void EntityManager::for_each(const std::function<void(Entity*)>& callback) {
    for (auto& [id, entity] : entities_) {
        if (entity->is_active()) {
            callback(entity.get());
        }
    }
}

void EntityManager::clear() {
    entities_.clear();
    pending_destroy_.clear();
}

void EntityManager::process_pending_destroys() {
    for (EntityID id : pending_destroy_) {
        entities_.erase(id);
    }
    pending_destroy_.clear();
}

} // namespace cafe

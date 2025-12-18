#ifndef CAFE_SPRITE_SHEET_H
#define CAFE_SPRITE_SHEET_H

#include "../renderer/renderer.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace cafe {

// ============================================================================
// SpriteFrame - A single frame within a sprite sheet
// ============================================================================

struct SpriteFrame {
    std::string name;
    TextureRegion region;
    int width = 0;
    int height = 0;
};

// ============================================================================
// Animation - Sequence of frames with timing
// ============================================================================

struct Animation {
    std::string name;
    std::vector<int> frame_indices;  // Indices into SpriteSheet's frames
    float frame_duration = 0.1f;     // Seconds per frame
    bool looping = true;

    int frame_count() const { return static_cast<int>(frame_indices.size()); }
    float total_duration() const { return frame_count() * frame_duration; }
};

// ============================================================================
// SpriteSheet - Manages sprite atlas with named frames and animations
// ============================================================================

class SpriteSheet {
public:
    SpriteSheet() = default;
    ~SpriteSheet() = default;

    // Non-copyable, movable
    SpriteSheet(const SpriteSheet&) = delete;
    SpriteSheet& operator=(const SpriteSheet&) = delete;
    SpriteSheet(SpriteSheet&&) = default;
    SpriteSheet& operator=(SpriteSheet&&) = default;

    // Load sprite sheet from image file
    // Returns true on success
    bool load(Renderer* renderer, const std::string& image_path,
              TextureFilter filter = TextureFilter::Nearest);

    // Create from existing texture
    void set_texture(TextureHandle texture, int width, int height);

    // Define frames
    // Regular grid: splits texture into uniform cells
    void define_grid(int cell_width, int cell_height,
                     int columns = -1, int rows = -1,
                     int padding = 0, int margin = 0);

    // Named frame at specific position
    void define_frame(const std::string& name, int x, int y, int width, int height);

    // Define animation from frame range
    void define_animation(const std::string& name,
                          int start_frame, int end_frame,
                          float frame_duration = 0.1f,
                          bool looping = true);

    // Define animation from frame names
    void define_animation(const std::string& name,
                          const std::vector<std::string>& frame_names,
                          float frame_duration = 0.1f,
                          bool looping = true);

    // Accessors
    TextureHandle texture() const { return texture_; }
    int texture_width() const { return texture_width_; }
    int texture_height() const { return texture_height_; }

    // Get frame by index
    const SpriteFrame* frame(int index) const;
    int frame_count() const { return static_cast<int>(frames_.size()); }

    // Get frame by name
    const SpriteFrame* frame(const std::string& name) const;
    int frame_index(const std::string& name) const;

    // Get animation
    const Animation* animation(const std::string& name) const;

    // Get texture region for animation at specific time
    TextureRegion animation_frame(const std::string& anim_name, float time) const;
    int animation_frame_index(const std::string& anim_name, float time) const;

    // Check validity
    bool is_valid() const { return texture_ != INVALID_TEXTURE; }

    // Cleanup (call before destroying renderer)
    void unload(Renderer* renderer);

private:
    TextureHandle texture_ = INVALID_TEXTURE;
    int texture_width_ = 0;
    int texture_height_ = 0;

    std::vector<SpriteFrame> frames_;
    std::unordered_map<std::string, int> frame_by_name_;
    std::unordered_map<std::string, Animation> animations_;
};

// ============================================================================
// AnimationPlayer - Plays animations with state tracking
// ============================================================================

class AnimationPlayer {
public:
    AnimationPlayer() = default;
    explicit AnimationPlayer(const SpriteSheet* sheet);

    void set_sprite_sheet(const SpriteSheet* sheet);

    // Play animation (does nothing if already playing same animation unless force=true)
    void play(const std::string& animation_name, bool force = false);

    // Update animation time
    void update(float delta_time);

    // Get current frame region
    TextureRegion current_region() const;
    int current_frame_index() const;

    // State
    bool is_playing() const { return is_playing_; }
    bool is_finished() const;  // True if non-looping animation completed
    const std::string& current_animation() const { return current_animation_; }
    float elapsed_time() const { return elapsed_time_; }

    // Control
    void pause() { is_playing_ = false; }
    void resume() { is_playing_ = true; }
    void stop();
    void set_speed(float speed) { speed_ = speed; }

private:
    const SpriteSheet* sheet_ = nullptr;
    std::string current_animation_;
    float elapsed_time_ = 0.0f;
    float speed_ = 1.0f;
    bool is_playing_ = false;
};

} // namespace cafe

#endif // CAFE_SPRITE_SHEET_H

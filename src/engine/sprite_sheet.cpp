#include "sprite_sheet.h"
#include "image.h"
#include <cmath>

namespace cafe {

// ============================================================================
// SpriteSheet Implementation
// ============================================================================

bool SpriteSheet::load(Renderer* renderer, const std::string& image_path,
                       TextureFilter filter) {
    auto image = Image::load_from_file(image_path);
    if (!image) {
        return false;
    }

    TextureInfo info;
    info.width = image->width();
    info.height = image->height();
    info.filter = filter;
    info.wrap = TextureWrap::Clamp;

    texture_ = renderer->create_texture(image->data(), info);
    if (texture_ == INVALID_TEXTURE) {
        return false;
    }

    texture_width_ = image->width();
    texture_height_ = image->height();

    return true;
}

void SpriteSheet::set_texture(TextureHandle texture, int width, int height) {
    texture_ = texture;
    texture_width_ = width;
    texture_height_ = height;
}

void SpriteSheet::define_grid(int cell_width, int cell_height,
                               int columns, int rows,
                               int padding, int margin) {
    if (texture_ == INVALID_TEXTURE) return;

    // Calculate columns/rows if not specified
    int usable_width = texture_width_ - 2 * margin;
    int usable_height = texture_height_ - 2 * margin;

    if (columns < 0) {
        columns = (usable_width + padding) / (cell_width + padding);
    }
    if (rows < 0) {
        rows = (usable_height + padding) / (cell_height + padding);
    }

    frames_.clear();
    frame_by_name_.clear();

    int frame_index = 0;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {
            int x = margin + col * (cell_width + padding);
            int y = margin + row * (cell_height + padding);

            SpriteFrame frame;
            frame.name = "frame_" + std::to_string(frame_index);
            frame.width = cell_width;
            frame.height = cell_height;
            frame.region = TextureRegion::from_pixels(
                texture_, texture_width_, texture_height_,
                x, y, cell_width, cell_height);

            frame_by_name_[frame.name] = static_cast<int>(frames_.size());
            frames_.push_back(frame);
            ++frame_index;
        }
    }
}

void SpriteSheet::define_frame(const std::string& name, int x, int y, int width, int height) {
    if (texture_ == INVALID_TEXTURE) return;

    SpriteFrame frame;
    frame.name = name;
    frame.width = width;
    frame.height = height;
    frame.region = TextureRegion::from_pixels(
        texture_, texture_width_, texture_height_,
        x, y, width, height);

    frame_by_name_[name] = static_cast<int>(frames_.size());
    frames_.push_back(frame);
}

void SpriteSheet::define_animation(const std::string& name,
                                    int start_frame, int end_frame,
                                    float frame_duration,
                                    bool looping) {
    Animation anim;
    anim.name = name;
    anim.frame_duration = frame_duration;
    anim.looping = looping;

    for (int i = start_frame; i <= end_frame; ++i) {
        if (i >= 0 && i < static_cast<int>(frames_.size())) {
            anim.frame_indices.push_back(i);
        }
    }

    animations_[name] = anim;
}

void SpriteSheet::define_animation(const std::string& name,
                                    const std::vector<std::string>& frame_names,
                                    float frame_duration,
                                    bool looping) {
    Animation anim;
    anim.name = name;
    anim.frame_duration = frame_duration;
    anim.looping = looping;

    for (const auto& fname : frame_names) {
        int idx = frame_index(fname);
        if (idx >= 0) {
            anim.frame_indices.push_back(idx);
        }
    }

    animations_[name] = anim;
}

const SpriteFrame* SpriteSheet::frame(int index) const {
    if (index < 0 || index >= static_cast<int>(frames_.size())) {
        return nullptr;
    }
    return &frames_[index];
}

const SpriteFrame* SpriteSheet::frame(const std::string& name) const {
    auto it = frame_by_name_.find(name);
    if (it == frame_by_name_.end()) {
        return nullptr;
    }
    return &frames_[it->second];
}

int SpriteSheet::frame_index(const std::string& name) const {
    auto it = frame_by_name_.find(name);
    if (it == frame_by_name_.end()) {
        return -1;
    }
    return it->second;
}

const Animation* SpriteSheet::animation(const std::string& name) const {
    auto it = animations_.find(name);
    if (it == animations_.end()) {
        return nullptr;
    }
    return &it->second;
}

int SpriteSheet::animation_frame_index(const std::string& anim_name, float time) const {
    const Animation* anim = animation(anim_name);
    if (!anim || anim->frame_indices.empty()) {
        return 0;
    }

    float total_duration = anim->total_duration();
    if (total_duration <= 0.0f) {
        return anim->frame_indices[0];
    }

    float t = time;
    if (anim->looping) {
        t = std::fmod(time, total_duration);
        if (t < 0.0f) t += total_duration;
    } else {
        t = std::min(time, total_duration - 0.0001f);
        t = std::max(t, 0.0f);
    }

    int frame_num = static_cast<int>(t / anim->frame_duration);
    frame_num = std::min(frame_num, static_cast<int>(anim->frame_indices.size()) - 1);
    frame_num = std::max(frame_num, 0);

    return anim->frame_indices[frame_num];
}

TextureRegion SpriteSheet::animation_frame(const std::string& anim_name, float time) const {
    int idx = animation_frame_index(anim_name, time);
    const SpriteFrame* f = frame(idx);
    if (f) {
        return f->region;
    }
    return TextureRegion(texture_);
}

void SpriteSheet::unload(Renderer* renderer) {
    if (texture_ != INVALID_TEXTURE && renderer) {
        renderer->destroy_texture(texture_);
        texture_ = INVALID_TEXTURE;
    }
    frames_.clear();
    frame_by_name_.clear();
    animations_.clear();
    texture_width_ = 0;
    texture_height_ = 0;
}

// ============================================================================
// AnimationPlayer Implementation
// ============================================================================

AnimationPlayer::AnimationPlayer(const SpriteSheet* sheet)
    : sheet_(sheet) {
}

void AnimationPlayer::set_sprite_sheet(const SpriteSheet* sheet) {
    sheet_ = sheet;
    stop();
}

void AnimationPlayer::play(const std::string& animation_name, bool force) {
    if (!force && is_playing_ && current_animation_ == animation_name) {
        return;
    }

    current_animation_ = animation_name;
    elapsed_time_ = 0.0f;
    is_playing_ = true;
}

void AnimationPlayer::update(float delta_time) {
    if (!is_playing_ || !sheet_) return;

    elapsed_time_ += delta_time * speed_;

    // Check if non-looping animation finished
    const Animation* anim = sheet_->animation(current_animation_);
    if (anim && !anim->looping) {
        if (elapsed_time_ >= anim->total_duration()) {
            is_playing_ = false;
        }
    }
}

TextureRegion AnimationPlayer::current_region() const {
    if (!sheet_) {
        return TextureRegion();
    }
    return sheet_->animation_frame(current_animation_, elapsed_time_);
}

int AnimationPlayer::current_frame_index() const {
    if (!sheet_) {
        return 0;
    }
    return sheet_->animation_frame_index(current_animation_, elapsed_time_);
}

bool AnimationPlayer::is_finished() const {
    if (!sheet_) return true;

    const Animation* anim = sheet_->animation(current_animation_);
    if (!anim) return true;
    if (anim->looping) return false;

    return elapsed_time_ >= anim->total_duration();
}

void AnimationPlayer::stop() {
    is_playing_ = false;
    current_animation_.clear();
    elapsed_time_ = 0.0f;
}

} // namespace cafe

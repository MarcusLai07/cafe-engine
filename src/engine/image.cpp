#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../../third_party/stb/stb_image.h"

#include <cstring>

namespace cafe {

Image::~Image() {
    if (data_ && owned_) {
        stbi_image_free(data_);
    }
}

Image::Image(Image&& other) noexcept
    : data_(other.data_)
    , width_(other.width_)
    , height_(other.height_)
    , channels_(other.channels_)
    , owned_(other.owned_) {
    other.data_ = nullptr;
    other.width_ = 0;
    other.height_ = 0;
    other.channels_ = 0;
    other.owned_ = false;
}

Image& Image::operator=(Image&& other) noexcept {
    if (this != &other) {
        if (data_ && owned_) {
            stbi_image_free(data_);
        }
        data_ = other.data_;
        width_ = other.width_;
        height_ = other.height_;
        channels_ = other.channels_;
        owned_ = other.owned_;
        other.data_ = nullptr;
        other.width_ = 0;
        other.height_ = 0;
        other.channels_ = 0;
        other.owned_ = false;
    }
    return *this;
}

std::unique_ptr<Image> Image::load_from_file(const std::string& path) {
    auto image = std::make_unique<Image>();

    // Force 4 channels (RGBA) for consistency
    image->data_ = stbi_load(path.c_str(), &image->width_, &image->height_, &image->channels_, 4);
    image->channels_ = 4;  // We forced 4 channels
    image->owned_ = true;

    if (!image->data_) {
        return nullptr;
    }

    return image;
}

std::unique_ptr<Image> Image::load_from_memory(const uint8_t* data, size_t size) {
    auto image = std::make_unique<Image>();

    image->data_ = stbi_load_from_memory(data, static_cast<int>(size),
                                          &image->width_, &image->height_,
                                          &image->channels_, 4);
    image->channels_ = 4;
    image->owned_ = true;

    if (!image->data_) {
        return nullptr;
    }

    return image;
}

std::unique_ptr<Image> Image::create(int width, int height, int channels) {
    auto image = std::make_unique<Image>();

    image->width_ = width;
    image->height_ = height;
    image->channels_ = channels;
    image->owned_ = true;

    size_t size = static_cast<size_t>(width) * height * channels;
    image->data_ = static_cast<uint8_t*>(malloc(size));

    if (!image->data_) {
        return nullptr;
    }

    // Initialize to transparent black
    std::memset(image->data_, 0, size);

    return image;
}

uint8_t* Image::pixel_at(int x, int y) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_ || !data_) {
        return nullptr;
    }
    return data_ + (y * width_ + x) * channels_;
}

const uint8_t* Image::pixel_at(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_ || !data_) {
        return nullptr;
    }
    return data_ + (y * width_ + x) * channels_;
}

void Image::set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    uint8_t* p = pixel_at(x, y);
    if (p && channels_ >= 3) {
        p[0] = r;
        p[1] = g;
        p[2] = b;
        if (channels_ >= 4) {
            p[3] = a;
        }
    }
}

} // namespace cafe

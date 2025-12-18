#ifndef CAFE_IMAGE_H
#define CAFE_IMAGE_H

#include <cstdint>
#include <memory>
#include <string>

namespace cafe {

// ============================================================================
// Image - Loaded image data in CPU memory
// ============================================================================

class Image {
public:
    Image() = default;
    ~Image();

    // Non-copyable, movable
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;

    // Load from file
    static std::unique_ptr<Image> load_from_file(const std::string& path);

    // Load from memory
    static std::unique_ptr<Image> load_from_memory(const uint8_t* data, size_t size);

    // Create empty image (useful for procedural generation)
    static std::unique_ptr<Image> create(int width, int height, int channels = 4);

    // Accessors
    int width() const { return width_; }
    int height() const { return height_; }
    int channels() const { return channels_; }
    const uint8_t* data() const { return data_; }
    uint8_t* data() { return data_; }

    // Get pixel at position (returns nullptr if out of bounds)
    uint8_t* pixel_at(int x, int y);
    const uint8_t* pixel_at(int x, int y) const;

    // Set pixel at position
    void set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    // Check if valid
    bool is_valid() const { return data_ != nullptr; }

private:
    uint8_t* data_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    int channels_ = 0;
    bool owned_ = false;  // Whether we need to free the data
};

} // namespace cafe

#endif // CAFE_IMAGE_H

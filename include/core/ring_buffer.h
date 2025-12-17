#ifndef CAFE_RING_BUFFER_H
#define CAFE_RING_BUFFER_H

#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>

namespace cafe {

// RingBuffer: A fixed-size circular buffer (FIFO queue).
//
// Key C++ concepts demonstrated:
// - Circular indexing with modulo
// - std::optional for safe empty-state handling
// - constexpr for compile-time constants
// - Efficient memory usage (no allocations after construction)
//
// Use cases in games:
// - Audio sample buffers
// - Input event history (for combos, replays)
// - Network packet queues
// - Rolling averages (FPS, ping)
// - Command history / undo buffer
//
// Properties:
// - O(1) push and pop
// - Fixed memory footprint
// - When full, oldest items are overwritten (configurable)
//
template<typename T, size_t N>
class RingBuffer {
    static_assert(N > 0, "RingBuffer capacity must be greater than 0");

private:
    std::array<T, N> data_;
    size_t head_ = 0;  // Next write position
    size_t tail_ = 0;  // Next read position
    size_t size_ = 0;  // Current number of elements

public:
    RingBuffer() = default;

    // Push an item to the back
    // Returns false if buffer is full (item not added)
    bool push(const T& value) {
        if (full()) {
            return false;
        }

        data_[head_] = value;
        head_ = (head_ + 1) % N;
        ++size_;
        return true;
    }

    bool push(T&& value) {
        if (full()) {
            return false;
        }

        data_[head_] = std::move(value);
        head_ = (head_ + 1) % N;
        ++size_;
        return true;
    }

    // Push, overwriting oldest if full
    void push_overwrite(const T& value) {
        if (full()) {
            // Overwrite oldest (advance tail)
            tail_ = (tail_ + 1) % N;
            --size_;
        }
        push(value);
    }

    void push_overwrite(T&& value) {
        if (full()) {
            tail_ = (tail_ + 1) % N;
            --size_;
        }
        push(std::move(value));
    }

    // Pop an item from the front
    std::optional<T> pop() {
        if (empty()) {
            return std::nullopt;
        }

        T value = std::move(data_[tail_]);
        tail_ = (tail_ + 1) % N;
        --size_;
        return value;
    }

    // Peek at front item without removing
    std::optional<T> peek() const {
        if (empty()) {
            return std::nullopt;
        }
        return data_[tail_];
    }

    // Peek at back item (most recently added)
    std::optional<T> peek_back() const {
        if (empty()) {
            return std::nullopt;
        }
        size_t back_index = (head_ + N - 1) % N;
        return data_[back_index];
    }

    // Access by index (0 = oldest, size-1 = newest)
    T& at(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("RingBuffer index out of range");
        }
        return data_[(tail_ + index) % N];
    }

    const T& at(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("RingBuffer index out of range");
        }
        return data_[(tail_ + index) % N];
    }

    T& operator[](size_t index) {
        return data_[(tail_ + index) % N];
    }

    const T& operator[](size_t index) const {
        return data_[(tail_ + index) % N];
    }

    // Clear all items
    void clear() {
        head_ = 0;
        tail_ = 0;
        size_ = 0;
    }

    // State queries
    bool empty() const { return size_ == 0; }
    bool full() const { return size_ == N; }
    size_t size() const { return size_; }
    static constexpr size_t capacity() { return N; }

    // Iterator support for range-based for loops
    class Iterator {
    private:
        const RingBuffer* buffer_;
        size_t index_;

    public:
        Iterator(const RingBuffer* buffer, size_t index)
            : buffer_(buffer), index_(index) {}

        const T& operator*() const {
            return buffer_->at(index_);
        }

        Iterator& operator++() {
            ++index_;
            return *this;
        }

        bool operator!=(const Iterator& other) const {
            return index_ != other.index_;
        }
    };

    Iterator begin() const { return Iterator(this, 0); }
    Iterator end() const { return Iterator(this, size_); }

    // Compute average (for numeric types)
    // Useful for FPS counters, ping averages, etc.
    template<typename U = T>
    U average() const {
        if (empty()) return U{};

        U sum{};
        for (size_t i = 0; i < size_; ++i) {
            sum += at(i);
        }
        return sum / static_cast<U>(size_);
    }
};

} // namespace cafe

#endif // CAFE_RING_BUFFER_H

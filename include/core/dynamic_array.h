#ifndef CAFE_DYNAMIC_ARRAY_H
#define CAFE_DYNAMIC_ARRAY_H

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace cafe {

// DynamicArray: A simple resizable array demonstrating manual memory management.
//
// Key C++ concepts demonstrated:
// - RAII: Memory is allocated in constructor, freed in destructor
// - Rule of Five: Custom destructor requires custom copy/move operations
// - Move semantics: Efficient transfer of ownership without copying
// - Templates: Generic container that works with any type
//
template<typename T>
class DynamicArray {
private:
    T* data_;           // Raw pointer to heap-allocated array
    size_t size_;       // Number of elements currently stored
    size_t capacity_;   // Total allocated space

    // Helper to grow the array when full
    void grow() {
        size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2;
        T* new_data = new T[new_capacity];

        // Move existing elements to new storage
        for (size_t i = 0; i < size_; ++i) {
            new_data[i] = std::move(data_[i]);
        }

        delete[] data_;  // Free old storage
        data_ = new_data;
        capacity_ = new_capacity;
    }

public:
    // Default constructor - starts empty
    DynamicArray()
        : data_(nullptr)
        , size_(0)
        , capacity_(0)
    {
    }

    // Destructor - RAII: automatically frees memory
    // This is called when the object goes out of scope
    ~DynamicArray() {
        delete[] data_;  // Safe to call on nullptr
    }

    // Copy constructor - deleted to keep example simple
    // In production, you'd implement deep copy
    DynamicArray(const DynamicArray&) = delete;
    DynamicArray& operator=(const DynamicArray&) = delete;

    // Move constructor - transfers ownership without copying
    // The 'other' object is left in a valid but empty state
    DynamicArray(DynamicArray&& other) noexcept
        : data_(other.data_)
        , size_(other.size_)
        , capacity_(other.capacity_)
    {
        // Leave 'other' in valid empty state
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    // Move assignment - similar to move constructor
    DynamicArray& operator=(DynamicArray&& other) noexcept {
        if (this != &other) {
            // Free our current data
            delete[] data_;

            // Take ownership of other's data
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;

            // Leave other in valid empty state
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    // Add element to end
    void push_back(const T& value) {
        if (size_ >= capacity_) {
            grow();
        }
        data_[size_] = value;
        ++size_;
    }

    // Move version of push_back for efficiency
    void push_back(T&& value) {
        if (size_ >= capacity_) {
            grow();
        }
        data_[size_] = std::move(value);
        ++size_;
    }

    // Element access with bounds checking
    T& operator[](size_t index) {
        if (index >= size_) {
            throw std::out_of_range("DynamicArray index out of bounds");
        }
        return data_[index];
    }

    const T& operator[](size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("DynamicArray index out of bounds");
        }
        return data_[index];
    }

    // Accessors
    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }
    bool empty() const { return size_ == 0; }

    // Iterator support for range-based for loops
    T* begin() { return data_; }
    T* end() { return data_ + size_; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + size_; }
};

} // namespace cafe

#endif // CAFE_DYNAMIC_ARRAY_H

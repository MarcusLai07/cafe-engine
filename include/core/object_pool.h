#ifndef CAFE_OBJECT_POOL_H
#define CAFE_OBJECT_POOL_H

#include <array>
#include <bitset>
#include <cstddef>
#include <stdexcept>
#include <new>

namespace cafe {

// ObjectPool: A fixed-size pool for fast allocation/deallocation of objects.
//
// Key C++ concepts demonstrated:
// - Placement new (construct object in pre-allocated memory)
// - Manual destructor calls
// - Bitset for tracking used slots
// - constexpr template parameters
//
// Use cases in games:
// - Particle systems (thousands of short-lived particles)
// - Bullets/projectiles
// - Sound effect instances
// - Any frequently created/destroyed objects
//
// Benefits over new/delete:
// - No heap fragmentation
// - O(1) allocation (no malloc overhead)
// - Cache-friendly (contiguous memory)
// - Predictable memory usage
//
template<typename T, size_t N>
class ObjectPool {
private:
    // Storage for objects (uninitialized memory)
    alignas(T) std::array<unsigned char[sizeof(T)], N> storage_;

    // Track which slots are in use
    std::bitset<N> used_;

    // Hint for next free slot (optimization)
    size_t next_free_ = 0;

    T* slot_ptr(size_t index) {
        return reinterpret_cast<T*>(&storage_[index]);
    }

    const T* slot_ptr(size_t index) const {
        return reinterpret_cast<const T*>(&storage_[index]);
    }

public:
    ObjectPool() = default;

    ~ObjectPool() {
        // Destroy all active objects
        for (size_t i = 0; i < N; ++i) {
            if (used_[i]) {
                slot_ptr(i)->~T();
            }
        }
    }

    // Non-copyable (objects have pointers into pool)
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    // Acquire an object from the pool
    // Returns pointer to constructed object, or nullptr if pool is full
    template<typename... Args>
    T* acquire(Args&&... args) {
        // Find a free slot starting from hint
        for (size_t i = 0; i < N; ++i) {
            size_t index = (next_free_ + i) % N;
            if (!used_[index]) {
                used_[index] = true;
                next_free_ = (index + 1) % N;

                // Placement new: construct object in pre-allocated memory
                return new (slot_ptr(index)) T(std::forward<Args>(args)...);
            }
        }
        return nullptr;  // Pool is full
    }

    // Release an object back to the pool
    void release(T* obj) {
        if (!obj) return;

        // Calculate index from pointer
        auto* base = reinterpret_cast<unsigned char*>(&storage_[0]);
        auto* ptr = reinterpret_cast<unsigned char*>(obj);

        if (ptr < base || ptr >= base + sizeof(storage_)) {
            throw std::invalid_argument("Object does not belong to this pool");
        }

        size_t index = static_cast<size_t>(ptr - base) / sizeof(T);

        if (!used_[index]) {
            throw std::logic_error("Double-release of pooled object");
        }

        // Manually call destructor
        obj->~T();
        used_[index] = false;
        next_free_ = index;  // Hint for next allocation
    }

    // Check if an object belongs to this pool
    bool owns(const T* obj) const {
        if (!obj) return false;

        auto* base = reinterpret_cast<const unsigned char*>(&storage_[0]);
        auto* ptr = reinterpret_cast<const unsigned char*>(obj);

        return ptr >= base && ptr < base + sizeof(storage_);
    }

    // Get number of active objects
    size_t active_count() const {
        return used_.count();
    }

    // Get number of available slots
    size_t available() const {
        return N - used_.count();
    }

    // Check if pool is full
    bool full() const {
        return used_.count() == N;
    }

    // Check if pool is empty
    bool empty() const {
        return used_.count() == 0;
    }

    // Total capacity
    static constexpr size_t capacity() {
        return N;
    }

    // Iterate over active objects
    template<typename Func>
    void for_each(Func&& func) {
        for (size_t i = 0; i < N; ++i) {
            if (used_[i]) {
                func(*slot_ptr(i));
            }
        }
    }

    template<typename Func>
    void for_each(Func&& func) const {
        for (size_t i = 0; i < N; ++i) {
            if (used_[i]) {
                func(*slot_ptr(i));
            }
        }
    }
};

} // namespace cafe

#endif // CAFE_OBJECT_POOL_H

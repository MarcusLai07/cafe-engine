#ifndef CAFE_HASH_MAP_H
#define CAFE_HASH_MAP_H

#include <cstddef>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace cafe {

// HashMap: A simple hash table using open addressing with linear probing.
//
// Key C++ concepts demonstrated:
// - Hash functions and collision resolution
// - std::optional for nullable returns
// - Template specialization (std::hash)
// - Iterator pattern for range-based for loops
//
// Use cases in games:
// - Entity lookup by ID or name
// - Resource caching (textures, sounds)
// - Fast string-to-value mapping
//
template<typename K, typename V, typename Hash = std::hash<K>>
class HashMap {
private:
    enum class SlotState { EMPTY, OCCUPIED, DELETED };

    struct Slot {
        K key;
        V value;
        SlotState state = SlotState::EMPTY;
    };

    std::vector<Slot> buckets_;
    size_t size_ = 0;
    size_t capacity_;
    Hash hasher_;

    static constexpr float MAX_LOAD_FACTOR = 0.7f;
    static constexpr size_t INITIAL_CAPACITY = 16;

    size_t hash_index(const K& key) const {
        return hasher_(key) % capacity_;
    }

    // Find slot for key (for insert or lookup)
    size_t find_slot(const K& key) const {
        size_t index = hash_index(key);
        size_t first_deleted = capacity_;  // Invalid sentinel

        for (size_t i = 0; i < capacity_; ++i) {
            size_t probe = (index + i) % capacity_;
            const Slot& slot = buckets_[probe];

            if (slot.state == SlotState::EMPTY) {
                // Key not found; return first deleted or this empty slot
                return (first_deleted < capacity_) ? first_deleted : probe;
            }

            if (slot.state == SlotState::DELETED && first_deleted == capacity_) {
                first_deleted = probe;  // Remember first deleted for insertion
            }

            if (slot.state == SlotState::OCCUPIED && slot.key == key) {
                return probe;  // Found the key
            }
        }

        // Table is full of deleted slots
        return (first_deleted < capacity_) ? first_deleted : 0;
    }

    void rehash(size_t new_capacity) {
        std::vector<Slot> old_buckets = std::move(buckets_);
        buckets_.clear();
        buckets_.resize(new_capacity);
        capacity_ = new_capacity;
        size_ = 0;

        for (auto& slot : old_buckets) {
            if (slot.state == SlotState::OCCUPIED) {
                insert(std::move(slot.key), std::move(slot.value));
            }
        }
    }

    void grow_if_needed() {
        float load = static_cast<float>(size_ + 1) / static_cast<float>(capacity_);
        if (load > MAX_LOAD_FACTOR) {
            rehash(capacity_ * 2);
        }
    }

public:
    HashMap(size_t initial_capacity = INITIAL_CAPACITY)
        : capacity_(initial_capacity)
    {
        buckets_.resize(capacity_);
    }

    // Insert or update a key-value pair
    void insert(const K& key, const V& value) {
        grow_if_needed();
        size_t index = find_slot(key);
        Slot& slot = buckets_[index];

        if (slot.state != SlotState::OCCUPIED) {
            ++size_;
        }

        slot.key = key;
        slot.value = value;
        slot.state = SlotState::OCCUPIED;
    }

    void insert(K&& key, V&& value) {
        grow_if_needed();
        size_t index = find_slot(key);
        Slot& slot = buckets_[index];

        if (slot.state != SlotState::OCCUPIED) {
            ++size_;
        }

        slot.key = std::move(key);
        slot.value = std::move(value);
        slot.state = SlotState::OCCUPIED;
    }

    // Look up a value by key
    std::optional<V> get(const K& key) const {
        size_t index = find_slot(key);
        const Slot& slot = buckets_[index];

        if (slot.state == SlotState::OCCUPIED && slot.key == key) {
            return slot.value;
        }
        return std::nullopt;
    }

    // Get reference to value, throws if not found
    V& at(const K& key) {
        size_t index = find_slot(key);
        Slot& slot = buckets_[index];

        if (slot.state == SlotState::OCCUPIED && slot.key == key) {
            return slot.value;
        }
        throw std::out_of_range("Key not found in HashMap");
    }

    const V& at(const K& key) const {
        size_t index = find_slot(key);
        const Slot& slot = buckets_[index];

        if (slot.state == SlotState::OCCUPIED && slot.key == key) {
            return slot.value;
        }
        throw std::out_of_range("Key not found in HashMap");
    }

    // Bracket operator for convenient access/insert
    V& operator[](const K& key) {
        grow_if_needed();
        size_t index = find_slot(key);
        Slot& slot = buckets_[index];

        if (slot.state != SlotState::OCCUPIED) {
            slot.key = key;
            slot.value = V{};
            slot.state = SlotState::OCCUPIED;
            ++size_;
        }

        return slot.value;
    }

    // Check if key exists
    bool contains(const K& key) const {
        size_t index = find_slot(key);
        const Slot& slot = buckets_[index];
        return slot.state == SlotState::OCCUPIED && slot.key == key;
    }

    // Remove a key
    bool remove(const K& key) {
        size_t index = find_slot(key);
        Slot& slot = buckets_[index];

        if (slot.state == SlotState::OCCUPIED && slot.key == key) {
            slot.state = SlotState::DELETED;
            --size_;
            return true;
        }
        return false;
    }

    // Clear all entries
    void clear() {
        for (auto& slot : buckets_) {
            slot.state = SlotState::EMPTY;
        }
        size_ = 0;
    }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    size_t capacity() const { return capacity_; }

    // Iterator for range-based for loops
    class Iterator {
    private:
        const HashMap* map_;
        size_t index_;

        void advance_to_valid() {
            while (index_ < map_->capacity_ &&
                   map_->buckets_[index_].state != SlotState::OCCUPIED) {
                ++index_;
            }
        }

    public:
        Iterator(const HashMap* map, size_t index)
            : map_(map), index_(index)
        {
            advance_to_valid();
        }

        std::pair<const K&, const V&> operator*() const {
            const Slot& slot = map_->buckets_[index_];
            return {slot.key, slot.value};
        }

        Iterator& operator++() {
            ++index_;
            advance_to_valid();
            return *this;
        }

        bool operator!=(const Iterator& other) const {
            return index_ != other.index_;
        }
    };

    Iterator begin() const { return Iterator(this, 0); }
    Iterator end() const { return Iterator(this, capacity_); }
};

} // namespace cafe

#endif // CAFE_HASH_MAP_H

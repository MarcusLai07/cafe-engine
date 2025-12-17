# Phase 1: C++ Foundations

**Duration**: Months 1-2
**Goal**: Build solid C++ fundamentals before touching engine code
**Deliverable**: A text-based cafe prototype demonstrating core C++ concepts

## Why This Phase Matters

Jumping straight into engine code without C++ fundamentals will result in:
- Memory leaks and crashes
- Unreadable/unmaintainable code
- Frustration from fighting the language instead of building

This phase ensures you're comfortable with C++ before tackling complex systems.

## Learning Path

### 1.1 Development Environment Setup

**Tasks:**
- [ ] Install Xcode (includes Clang compiler)
- [ ] Install CMake via Homebrew: `brew install cmake`
- [ ] Set up a code editor (VS Code with C++ extension, or CLion)
- [ ] Create a "Hello World" project with CMake
- [ ] Understand compile → link → run cycle

**Mini-Project:** Build and run a simple program that prints "Cafe Engine Starting..."

**Resources:**
- [CMake Tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/index.html)
- [Xcode Command Line Tools](https://developer.apple.com/xcode/resources/)

---

### 1.2 Memory Management & RAII

**Concepts to Learn:**
- Stack vs heap allocation
- Pointers and references
- `new`/`delete` and why to avoid them
- RAII (Resource Acquisition Is Initialization)
- Smart pointers: `unique_ptr`, `shared_ptr`
- Move semantics (lvalues vs rvalues)

**Exercises:**

```cpp
// Exercise 1: Implement a simple dynamic array
template<typename T>
class DynamicArray {
    T* data_;
    size_t size_;
    size_t capacity_;
public:
    DynamicArray();
    ~DynamicArray();
    DynamicArray(DynamicArray&& other);  // Move constructor
    DynamicArray& operator=(DynamicArray&& other);  // Move assignment

    void push_back(T value);
    T& operator[](size_t index);
    size_t size() const;
};

// Exercise 2: Implement a file reader with RAII
class FileReader {
    FILE* file_;
public:
    FileReader(const char* path);
    ~FileReader();  // Automatically closes file
    std::string read_all();
};
```

**Key Insight:** In C++, destructors are your friends. They guarantee cleanup happens, even if exceptions are thrown.

---

### 1.3 Build Systems (CMake)

**Concepts to Learn:**
- What CMake does (generate build files)
- CMakeLists.txt structure
- Targets, properties, dependencies
- Debug vs Release builds
- Platform detection

**Create a CMakeLists.txt:**

```cmake
cmake_minimum_required(VERSION 3.20)
project(CafeEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Main executable
add_executable(cafe_prototype
    src/main.cpp
    src/cafe.cpp
)

# Include directories
target_include_directories(cafe_prototype PRIVATE include)

# Compiler warnings
target_compile_options(cafe_prototype PRIVATE
    -Wall -Wextra -Wpedantic
)
```

**Build Commands:**
```bash
mkdir build && cd build
cmake ..
cmake --build .
./cafe_prototype
```

---

### 1.4 Data Structures & Algorithms

**Structures to Implement:**

```cpp
// 1. Hash Map (for entity lookup, resource caching)
template<typename K, typename V>
class HashMap {
    // Implement with chaining or open addressing
};

// 2. Object Pool (for frequently created/destroyed objects)
template<typename T, size_t N>
class ObjectPool {
    std::array<T, N> objects_;
    std::bitset<N> used_;
public:
    T* acquire();
    void release(T* obj);
};

// 3. Ring Buffer (for audio, input history)
template<typename T, size_t N>
class RingBuffer {
    std::array<T, N> data_;
    size_t head_ = 0, tail_ = 0;
public:
    void push(T value);
    T pop();
    bool empty() const;
    bool full() const;
};
```

**Algorithms to Know:**
- Sorting (std::sort, custom comparators for depth sorting)
- Searching (binary search for sorted data)
- A* pathfinding (will implement in Phase 5)

---

### 1.5 Modern C++ Patterns

**Concepts to Learn:**
- Templates (function and class templates)
- Lambdas and std::function
- std::optional, std::variant
- Structured bindings
- Range-based for loops
- constexpr

**Code Examples:**

```cpp
// Lambdas for callbacks
void for_each_entity(std::function<void(Entity&)> callback) {
    for (auto& entity : entities_) {
        callback(entity);
    }
}

// Usage
for_each_entity([](Entity& e) {
    e.update(dt);
});

// std::optional for nullable returns
std::optional<Entity*> find_entity(const std::string& name) {
    auto it = entities_.find(name);
    if (it != entities_.end()) {
        return it->second;
    }
    return std::nullopt;
}

// Usage
if (auto entity = find_entity("player")) {
    (*entity)->set_position(100, 200);
}

// Structured bindings
for (const auto& [id, entity] : entities_) {
    entity->update(dt);
}
```

---

## Milestone Project: Text-Based Cafe Prototype

Build a command-line cafe simulator that demonstrates all learned concepts.

**Features:**
- Customers arrive (random intervals)
- Menu with items and prices
- Order processing (customer waits, gets served)
- Money tracking
- Save/load game state (JSON file)
- Simple progression (unlock new items)

**Code Structure:**
```
text_cafe/
├── CMakeLists.txt
├── include/
│   ├── cafe.h
│   ├── customer.h
│   ├── menu.h
│   ├── economy.h
│   └── save.h
└── src/
    ├── main.cpp
    ├── cafe.cpp
    ├── customer.cpp
    ├── menu.cpp
    ├── economy.cpp
    └── save.cpp
```

**Sample Output:**
```
=== Day 1 - 08:00 ===
Money: $1000

[1] Wait for customer
[2] View menu
[3] View stats
[4] Save & Quit

> 1

Customer "Alice" arrived!
Alice wants: Latte ($4.50)

[1] Serve order (-$1.50 cost, +$4.50 revenue)
[2] Refuse order

> 1

Served Alice! Profit: $3.00
Money: $1003

Customer satisfaction: Happy!
```

---

## Checklist

- [ ] Development environment fully working
- [ ] Can create and build CMake projects
- [ ] Understand stack vs heap, when to use each
- [ ] Comfortable with smart pointers
- [ ] Can implement basic data structures
- [ ] Understand templates at basic level
- [ ] Text-based cafe prototype complete and working

## Next Phase

Once comfortable with these fundamentals, proceed to [Phase 2: Platform Layer - Mac](../phase-2-platform-mac/overview.md).

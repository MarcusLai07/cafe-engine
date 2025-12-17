# C++ Fundamentals for Game Development

A focused guide on the C++ concepts most relevant to building the Cafe Engine.

## Memory Management

### Stack vs Heap

**Stack:** Automatic, fast, limited size
```cpp
void example() {
    int x = 5;           // Stack allocated
    float arr[100];      // Stack allocated array
}  // Automatically freed when function exits
```

**Heap:** Manual, slower, unlimited size
```cpp
void example() {
    int* x = new int(5);      // Heap allocated
    float* arr = new float[100];  // Heap allocated array

    delete x;        // Must manually free
    delete[] arr;    // Use delete[] for arrays
}
```

### Smart Pointers (Prefer These!)

**unique_ptr:** Single owner, automatic cleanup
```cpp
#include <memory>

void example() {
    auto texture = std::make_unique<Texture>("sprite.png");
    texture->bind();  // Use like regular pointer

}  // Automatically deleted here

// Transfer ownership
auto texture2 = std::move(texture);  // texture is now nullptr
```

**shared_ptr:** Multiple owners, reference counted
```cpp
auto resource = std::make_shared<Resource>();
auto copy = resource;  // Both point to same object

// Deleted when ALL shared_ptrs are destroyed
```

**When to use:**
- `unique_ptr`: Default choice (90% of cases)
- `shared_ptr`: When multiple owners truly needed
- Raw pointer: Only for non-owning references

### RAII (Resource Acquisition Is Initialization)

Tie resource lifetime to object lifetime:
```cpp
class FileHandle {
    FILE* file_;
public:
    FileHandle(const char* path) {
        file_ = fopen(path, "r");
    }

    ~FileHandle() {
        if (file_) fclose(file_);  // Always closes!
    }

    // Prevent copying
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;
};

void example() {
    FileHandle f("data.txt");
    // Use file...
}  // File automatically closed, even if exception thrown
```

---

## Modern C++ Features

### Auto Type Deduction
```cpp
auto x = 5;                    // int
auto y = 3.14f;                // float
auto str = std::string("hi"); // std::string

// Especially useful for iterators
for (auto& entity : entities_) {
    entity.update(dt);
}
```

### Range-Based For Loops
```cpp
std::vector<Entity> entities;

// Old way
for (size_t i = 0; i < entities.size(); i++) {
    entities[i].update(dt);
}

// Modern way (preferred)
for (auto& entity : entities) {
    entity.update(dt);
}

// Const reference if not modifying
for (const auto& entity : entities) {
    entity.render();
}
```

### Lambdas
```cpp
// Basic lambda
auto add = [](int a, int b) { return a + b; };
int result = add(2, 3);  // 5

// Capture variables
int multiplier = 2;
auto scale = [multiplier](int x) { return x * multiplier; };

// Capture by reference
int counter = 0;
auto increment = [&counter]() { counter++; };

// Common use: sorting
std::sort(entities.begin(), entities.end(),
    [](const Entity& a, const Entity& b) {
        return a.depth() < b.depth();
    });
```

### std::optional
```cpp
#include <optional>

std::optional<Entity*> find_entity(const std::string& name) {
    auto it = entities_.find(name);
    if (it != entities_.end()) {
        return it->second;
    }
    return std::nullopt;  // No value
}

// Usage
if (auto entity = find_entity("player")) {
    (*entity)->update(dt);  // Dereference optional
}

// Or with value_or
Entity* e = find_entity("player").value_or(nullptr);
```

### Structured Bindings
```cpp
std::map<std::string, int> scores;

// Old way
for (auto it = scores.begin(); it != scores.end(); ++it) {
    std::cout << it->first << ": " << it->second << "\n";
}

// Modern way
for (const auto& [name, score] : scores) {
    std::cout << name << ": " << score << "\n";
}
```

---

## Templates

### Function Templates
```cpp
template<typename T>
T max(T a, T b) {
    return (a > b) ? a : b;
}

int x = max(5, 3);         // T = int
float y = max(2.5f, 1.3f); // T = float
```

### Class Templates
```cpp
template<typename T>
class DynamicArray {
    T* data_;
    size_t size_;
    size_t capacity_;

public:
    void push_back(const T& value);
    T& operator[](size_t index);
    size_t size() const { return size_; }
};

DynamicArray<int> numbers;
DynamicArray<Entity*> entities;
```

### Variadic Templates
```cpp
// Accept any number of arguments
template<typename... Args>
void log(const char* format, Args... args) {
    printf(format, args...);
}

log("Player at (%d, %d) with %f health\n", x, y, health);
```

---

## Data Structures

### std::vector (Dynamic Array)
```cpp
#include <vector>

std::vector<int> numbers;
numbers.push_back(1);
numbers.push_back(2);
numbers.emplace_back(3);  // Construct in-place

numbers[0] = 10;  // Access by index
numbers.size();   // 3
numbers.clear();  // Remove all
```

### std::unordered_map (Hash Map)
```cpp
#include <unordered_map>

std::unordered_map<std::string, int> scores;
scores["Alice"] = 100;
scores["Bob"] = 85;

if (scores.count("Alice")) {
    std::cout << scores["Alice"] << "\n";
}

// Iteration
for (const auto& [name, score] : scores) {
    std::cout << name << ": " << score << "\n";
}
```

### std::array (Fixed Size Array)
```cpp
#include <array>

std::array<int, 5> numbers = {1, 2, 3, 4, 5};
numbers[0] = 10;
numbers.size();  // 5 (compile-time constant)
```

---

## Classes and Inheritance

### Basic Class
```cpp
class Entity {
private:
    float x_, y_;
    std::string name_;

public:
    // Constructor
    Entity(const std::string& name, float x, float y)
        : name_(name), x_(x), y_(y) {}  // Initializer list

    // Methods
    void move(float dx, float dy) {
        x_ += dx;
        y_ += dy;
    }

    // Const method (doesn't modify object)
    float x() const { return x_; }
    float y() const { return y_; }
};
```

### Inheritance
```cpp
class Entity {
public:
    virtual void update(double dt) {}  // Virtual = can override
    virtual void render() = 0;          // Pure virtual = must override
    virtual ~Entity() = default;        // Virtual destructor!
};

class Player : public Entity {
public:
    void update(double dt) override {
        // Player-specific update
    }

    void render() override {
        // Player rendering
    }
};
```

### Rule of Five
If you define any of these, define all five:
```cpp
class Resource {
public:
    Resource();                                  // Constructor
    ~Resource();                                 // Destructor
    Resource(const Resource& other);            // Copy constructor
    Resource& operator=(const Resource& other); // Copy assignment
    Resource(Resource&& other);                 // Move constructor
    Resource& operator=(Resource&& other);      // Move assignment
};

// Or disable copying:
class Texture {
public:
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) = default;
    Texture& operator=(Texture&&) = default;
};
```

---

## Error Handling

### Return Codes (Preferred for Performance)
```cpp
enum class LoadResult {
    Success,
    FileNotFound,
    InvalidFormat,
    OutOfMemory
};

LoadResult load_texture(const char* path, Texture* out) {
    FILE* f = fopen(path, "rb");
    if (!f) return LoadResult::FileNotFound;

    // Load...
    return LoadResult::Success;
}

// Usage
Texture tex;
if (load_texture("sprite.png", &tex) != LoadResult::Success) {
    // Handle error
}
```

### Exceptions (Use Sparingly in Games)
```cpp
// Throwing
if (!file) {
    throw std::runtime_error("Failed to open file");
}

// Catching
try {
    load_game();
} catch (const std::exception& e) {
    log_error(e.what());
}
```

---

## Performance Tips

### Avoid Unnecessary Copies
```cpp
// Bad: copies entire vector
void process(std::vector<Entity> entities) { }

// Good: reference, no copy
void process(const std::vector<Entity>& entities) { }
```

### Reserve Vector Capacity
```cpp
std::vector<Sprite> sprites;
sprites.reserve(1000);  // Allocate once

for (int i = 0; i < 1000; i++) {
    sprites.push_back(create_sprite());  // No reallocation
}
```

### Use emplace_back
```cpp
std::vector<Entity> entities;

// push_back: creates temporary, then moves
entities.push_back(Entity("player", 0, 0));

// emplace_back: constructs in-place (faster)
entities.emplace_back("player", 0, 0);
```

### Cache-Friendly Iteration
```cpp
// Bad: pointer chasing
std::vector<Entity*> entities;
for (auto* e : entities) {
    e->update(dt);  // Cache miss likely
}

// Better: contiguous memory
std::vector<Entity> entities;
for (auto& e : entities) {
    e.update(dt);  // Cache-friendly
}
```

---

## Recommended Resources

**Books:**
- "A Tour of C++" by Bjarne Stroustrup (quick overview)
- "Effective Modern C++" by Scott Meyers (best practices)

**Online:**
- https://en.cppreference.com (reference)
- https://isocpp.org/faq (FAQ)
- https://www.learncpp.com (tutorials)

**Practice:**
- Build the text-based cafe prototype
- Implement basic data structures
- Write small programs using each concept

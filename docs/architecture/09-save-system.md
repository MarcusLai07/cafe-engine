# Save System Architecture

The save system handles serialization of game state for persistence across sessions.

## Design Goals

1. **Simple format** - Human-readable JSON for debugging
2. **Versioned** - Handle save file format changes gracefully
3. **Atomic writes** - No corruption on crash during save
4. **Cross-platform** - Works on all target platforms

## Save Data Structure

```cpp
// framework/save.h

namespace cafe {

// Version for handling format changes
constexpr int SAVE_VERSION = 1;

// Data that gets saved
struct SaveData {
    int version = SAVE_VERSION;

    // Game progress
    int day = 1;
    int hour = 8;
    int minute = 0;

    // Economy
    int money = 1000;
    int lifetime_earnings = 0;
    int customers_served = 0;

    // Unlocks
    std::vector<std::string> unlocked_items;
    std::vector<std::string> unlocked_recipes;

    // Cafe state
    struct PlacedItem {
        std::string item_id;
        int x, y;
        int rotation;
    };
    std::vector<PlacedItem> placed_items;

    // Staff (future)
    struct StaffMember {
        std::string name;
        std::string role;
        int skill_level;
    };
    std::vector<StaffMember> staff;

    // Settings (saved with game for convenience)
    float music_volume = 0.5f;
    float sfx_volume = 1.0f;
    std::string language = "en";
};

} // namespace cafe
```

## Save Manager

```cpp
// framework/save_manager.h

namespace cafe {

class SaveManager {
private:
    Platform* platform_;
    std::string save_path_;

public:
    SaveManager(Platform* platform) : platform_(platform) {
        save_path_ = std::string(platform_->get_save_path()) + "save.json";
    }

    bool save(const SaveData& data) {
        // Serialize to JSON
        std::string json = serialize(data);

        // Write to temp file first (atomic save)
        std::string temp_path = save_path_ + ".tmp";
        if (!platform_->write_file(temp_path.c_str(), json.data(), json.size())) {
            return false;
        }

        // Rename temp to actual save file
        // (On most platforms, rename is atomic)
        return rename_file(temp_path, save_path_);
    }

    bool load(SaveData* data) {
        std::vector<uint8_t> file_data;
        if (!platform_->read_file(save_path_.c_str(), &file_data)) {
            return false;  // No save file exists
        }

        std::string json(file_data.begin(), file_data.end());
        return deserialize(json, data);
    }

    bool has_save() {
        std::vector<uint8_t> dummy;
        return platform_->read_file(save_path_.c_str(), &dummy);
    }

    bool delete_save() {
        return delete_file(save_path_);
    }

private:
    std::string serialize(const SaveData& data) {
        // Build JSON string
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"version\": " << data.version << ",\n";
        ss << "  \"day\": " << data.day << ",\n";
        ss << "  \"hour\": " << data.hour << ",\n";
        ss << "  \"minute\": " << data.minute << ",\n";
        ss << "  \"money\": " << data.money << ",\n";
        ss << "  \"lifetime_earnings\": " << data.lifetime_earnings << ",\n";
        ss << "  \"customers_served\": " << data.customers_served << ",\n";

        // Unlocked items
        ss << "  \"unlocked_items\": [";
        for (size_t i = 0; i < data.unlocked_items.size(); i++) {
            if (i > 0) ss << ", ";
            ss << "\"" << data.unlocked_items[i] << "\"";
        }
        ss << "],\n";

        // Unlocked recipes
        ss << "  \"unlocked_recipes\": [";
        for (size_t i = 0; i < data.unlocked_recipes.size(); i++) {
            if (i > 0) ss << ", ";
            ss << "\"" << data.unlocked_recipes[i] << "\"";
        }
        ss << "],\n";

        // Placed items
        ss << "  \"placed_items\": [\n";
        for (size_t i = 0; i < data.placed_items.size(); i++) {
            const auto& item = data.placed_items[i];
            ss << "    {\"id\": \"" << item.item_id << "\", "
               << "\"x\": " << item.x << ", "
               << "\"y\": " << item.y << ", "
               << "\"rotation\": " << item.rotation << "}";
            if (i < data.placed_items.size() - 1) ss << ",";
            ss << "\n";
        }
        ss << "  ],\n";

        // Settings
        ss << "  \"music_volume\": " << data.music_volume << ",\n";
        ss << "  \"sfx_volume\": " << data.sfx_volume << ",\n";
        ss << "  \"language\": \"" << data.language << "\"\n";

        ss << "}\n";
        return ss.str();
    }

    bool deserialize(const std::string& json, SaveData* data) {
        JsonValue root = parse_json(json);
        if (root.is_null()) return false;

        // Check version
        int version = root["version"].as_int();
        if (version > SAVE_VERSION) {
            // Save from newer game version
            return false;
        }

        // Load data (with defaults for missing fields)
        data->version = version;
        data->day = root.has("day") ? root["day"].as_int() : 1;
        data->hour = root.has("hour") ? root["hour"].as_int() : 8;
        data->minute = root.has("minute") ? root["minute"].as_int() : 0;
        data->money = root.has("money") ? root["money"].as_int() : 1000;
        data->lifetime_earnings = root.has("lifetime_earnings") ? root["lifetime_earnings"].as_int() : 0;
        data->customers_served = root.has("customers_served") ? root["customers_served"].as_int() : 0;

        // Arrays
        if (root.has("unlocked_items")) {
            for (const auto& item : root["unlocked_items"]) {
                data->unlocked_items.push_back(item.as_string());
            }
        }

        if (root.has("unlocked_recipes")) {
            for (const auto& item : root["unlocked_recipes"]) {
                data->unlocked_recipes.push_back(item.as_string());
            }
        }

        if (root.has("placed_items")) {
            for (const auto& item : root["placed_items"]) {
                data->placed_items.push_back({
                    item["id"].as_string(),
                    item["x"].as_int(),
                    item["y"].as_int(),
                    item["rotation"].as_int()
                });
            }
        }

        // Settings
        data->music_volume = root.has("music_volume") ? (float)root["music_volume"].as_number() : 0.5f;
        data->sfx_volume = root.has("sfx_volume") ? (float)root["sfx_volume"].as_number() : 1.0f;
        data->language = root.has("language") ? root["language"].as_string() : "en";

        // Handle version migration
        if (version < SAVE_VERSION) {
            migrate_save(data, version);
        }

        return true;
    }

    void migrate_save(SaveData* data, int from_version) {
        // Handle save format changes between versions
        // Example: if (from_version < 2) { /* add new field defaults */ }
    }
};

} // namespace cafe
```

## Auto-Save

```cpp
// game/autosave.cpp

class AutoSave {
private:
    SaveManager* save_manager_;
    double last_save_time_ = 0;
    static constexpr double AUTOSAVE_INTERVAL = 60.0;  // Every minute

public:
    void update(double game_time, const SaveData& current_state) {
        if (game_time - last_save_time_ >= AUTOSAVE_INTERVAL) {
            save_manager_->save(current_state);
            last_save_time_ = game_time;
        }
    }

    void force_save(const SaveData& current_state) {
        save_manager_->save(current_state);
        last_save_time_ = current_state.day * 24 * 60 + current_state.hour * 60 + current_state.minute;
    }
};
```

## Web Platform Storage

For web, use localStorage or IndexedDB:

```cpp
// platform/web/web_save.cpp

bool WebPlatform::write_file(const char* path, const void* data, size_t size) {
    if (strncmp(path, "save", 4) == 0) {
        // Use localStorage for save files
        EM_ASM({
            var path = UTF8ToString($0);
            var data = UTF8ToString($1);
            localStorage.setItem('cafe_' + path, data);
        }, path, static_cast<const char*>(data));
        return true;
    }

    // Regular files not supported on web
    return false;
}

bool WebPlatform::read_file(const char* path, std::vector<uint8_t>* data) {
    if (strncmp(path, "save", 4) == 0) {
        // Read from localStorage
        char* result = (char*)EM_ASM_PTR({
            var path = UTF8ToString($0);
            var data = localStorage.getItem('cafe_' + path);
            if (!data) return 0;

            var len = lengthBytesUTF8(data) + 1;
            var buf = _malloc(len);
            stringToUTF8(data, buf, len);
            return buf;
        }, path);

        if (result) {
            std::string str(result);
            free(result);
            data->assign(str.begin(), str.end());
            return true;
        }
        return false;
    }

    // For asset files, use preloaded virtual filesystem
    // ...
}
```

## Save Slots (Optional)

```cpp
// framework/save_slots.h

class SaveSlotManager {
private:
    Platform* platform_;
    static constexpr int MAX_SLOTS = 3;

public:
    struct SlotInfo {
        bool exists;
        int day;
        int money;
        std::string last_played;  // ISO timestamp
    };

    std::array<SlotInfo, MAX_SLOTS> get_slot_info() {
        std::array<SlotInfo, MAX_SLOTS> info;

        for (int i = 0; i < MAX_SLOTS; i++) {
            std::string path = get_slot_path(i);
            SaveData data;

            if (load_from_path(path, &data)) {
                info[i] = {true, data.day, data.money, /* timestamp */};
            } else {
                info[i] = {false, 0, 0, ""};
            }
        }

        return info;
    }

    bool save_to_slot(int slot, const SaveData& data) {
        if (slot < 0 || slot >= MAX_SLOTS) return false;
        return save_to_path(get_slot_path(slot), data);
    }

    bool load_from_slot(int slot, SaveData* data) {
        if (slot < 0 || slot >= MAX_SLOTS) return false;
        return load_from_path(get_slot_path(slot), data);
    }

    bool delete_slot(int slot) {
        if (slot < 0 || slot >= MAX_SLOTS) return false;
        return delete_file(get_slot_path(slot));
    }

private:
    std::string get_slot_path(int slot) {
        return std::string(platform_->get_save_path()) + "save_" + std::to_string(slot) + ".json";
    }
};
```

## Integration with Game

```cpp
// game/cafe_game.cpp

class CafeGame : public Game {
private:
    SaveManager save_manager_;
    SaveData current_save_;

public:
    bool initialize() override {
        // Try to load existing save
        if (save_manager_.has_save()) {
            if (save_manager_.load(&current_save_)) {
                // Restore game state from save
                restore_from_save(current_save_);
            }
        } else {
            // New game
            current_save_ = SaveData{};  // Defaults
        }
    }

    void shutdown() override {
        // Save on exit
        gather_save_data(&current_save_);
        save_manager_.save(current_save_);
    }

private:
    void restore_from_save(const SaveData& save) {
        time_system_->set_day(save.day);
        time_system_->set_time(save.hour, save.minute);
        economy_->set_money(save.money);
        // ... restore other state
    }

    void gather_save_data(SaveData* save) {
        save->day = time_system_->day();
        save->hour = time_system_->hour();
        save->minute = time_system_->minute();
        save->money = economy_->money();
        // ... gather other state
    }
};
```

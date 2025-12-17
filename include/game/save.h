#ifndef CAFE_SAVE_H
#define CAFE_SAVE_H

#include <string>
#include <fstream>
#include <sstream>
#include <optional>
#include "core/file_reader.h"

namespace cafe {

// Simple JSON-like save format (no external dependencies)
// Format:
// {
//   "money": 150.50,
//   "xp": 75,
//   "level": 2,
//   "day": 3,
//   "customers_served": 15,
//   "customers_lost": 2,
//   "total_revenue": 200.00,
//   "total_costs": 50.00,
//   "unlocked_items": ["espresso", "latte", "mocha"]
// }

struct SaveData {
    float money = 100.0f;
    int xp = 0;
    int level = 1;
    int day = 1;
    int hour = 8;
    int customers_served = 0;
    int customers_lost = 0;
    float total_revenue = 0.0f;
    float total_costs = 0.0f;
    std::vector<std::string> unlocked_items;

    bool valid = false;
};

class SaveSystem {
private:
    std::string save_path_;

    // Simple parser helpers
    static std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\n\r");
        return s.substr(start, end - start + 1);
    }

    static std::optional<float> parse_float(const std::string& s) {
        try {
            return std::stof(trim(s));
        } catch (...) {
            return std::nullopt;
        }
    }

    static std::optional<int> parse_int(const std::string& s) {
        try {
            return std::stoi(trim(s));
        } catch (...) {
            return std::nullopt;
        }
    }

public:
    explicit SaveSystem(std::string path = "cafe_save.json")
        : save_path_(std::move(path))
    {}

    const std::string& path() const { return save_path_; }

    bool save(const SaveData& data) {
        std::ofstream file(save_path_);
        if (!file.is_open()) {
            return false;
        }

        file << "{\n";
        file << "  \"money\": " << data.money << ",\n";
        file << "  \"xp\": " << data.xp << ",\n";
        file << "  \"level\": " << data.level << ",\n";
        file << "  \"day\": " << data.day << ",\n";
        file << "  \"hour\": " << data.hour << ",\n";
        file << "  \"customers_served\": " << data.customers_served << ",\n";
        file << "  \"customers_lost\": " << data.customers_lost << ",\n";
        file << "  \"total_revenue\": " << data.total_revenue << ",\n";
        file << "  \"total_costs\": " << data.total_costs << ",\n";
        file << "  \"unlocked_items\": [";

        for (size_t i = 0; i < data.unlocked_items.size(); ++i) {
            if (i > 0) file << ", ";
            file << "\"" << data.unlocked_items[i] << "\"";
        }

        file << "]\n";
        file << "}\n";

        return file.good();
    }

    std::optional<SaveData> load() {
        std::ifstream file(save_path_);
        if (!file.is_open()) {
            return std::nullopt;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();

        SaveData data;

        // Very simple parser - find key-value pairs
        auto find_value = [&content](const std::string& key) -> std::string {
            std::string search = "\"" + key + "\":";
            size_t pos = content.find(search);
            if (pos == std::string::npos) return "";

            pos += search.length();
            size_t end = content.find_first_of(",}\n]", pos);
            if (end == std::string::npos) return "";

            return content.substr(pos, end - pos);
        };

        // Parse simple values
        if (auto v = parse_float(find_value("money"))) data.money = *v;
        if (auto v = parse_int(find_value("xp"))) data.xp = *v;
        if (auto v = parse_int(find_value("level"))) data.level = *v;
        if (auto v = parse_int(find_value("day"))) data.day = *v;
        if (auto v = parse_int(find_value("hour"))) data.hour = *v;
        if (auto v = parse_int(find_value("customers_served"))) data.customers_served = *v;
        if (auto v = parse_int(find_value("customers_lost"))) data.customers_lost = *v;
        if (auto v = parse_float(find_value("total_revenue"))) data.total_revenue = *v;
        if (auto v = parse_float(find_value("total_costs"))) data.total_costs = *v;

        // Parse unlocked_items array
        size_t arr_start = content.find("\"unlocked_items\":");
        if (arr_start != std::string::npos) {
            size_t bracket_start = content.find('[', arr_start);
            size_t bracket_end = content.find(']', bracket_start);
            if (bracket_start != std::string::npos && bracket_end != std::string::npos) {
                std::string arr_content = content.substr(bracket_start + 1,
                                                         bracket_end - bracket_start - 1);
                // Find quoted strings
                size_t pos = 0;
                while ((pos = arr_content.find('"', pos)) != std::string::npos) {
                    size_t end = arr_content.find('"', pos + 1);
                    if (end != std::string::npos) {
                        data.unlocked_items.push_back(
                            arr_content.substr(pos + 1, end - pos - 1));
                        pos = end + 1;
                    } else {
                        break;
                    }
                }
            }
        }

        data.valid = true;
        return data;
    }

    bool exists() const {
        std::ifstream file(save_path_);
        return file.good();
    }

    bool delete_save() {
        return std::remove(save_path_.c_str()) == 0;
    }
};

} // namespace cafe

#endif // CAFE_SAVE_H

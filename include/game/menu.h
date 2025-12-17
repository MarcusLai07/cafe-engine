#ifndef CAFE_MENU_H
#define CAFE_MENU_H

#include <string>
#include <vector>
#include <optional>
#include "core/hash_map.h"

namespace cafe {

// Represents a single menu item
struct MenuItem {
    std::string id;           // Unique identifier
    std::string name;         // Display name
    float sell_price;         // What customer pays
    float cost;               // What it costs to make
    int prep_time_seconds;    // Time to prepare
    int unlock_level;         // Level required to unlock
    bool unlocked;            // Currently available

    float profit() const { return sell_price - cost; }
};

// Manages the cafe's menu
class Menu {
private:
    HashMap<std::string, MenuItem> items_;
    std::vector<std::string> item_order_;  // For display ordering

public:
    Menu() {
        // Initialize default menu items
        add_item({"espresso", "Espresso", 2.50f, 0.50f, 2, 1, true});
        add_item({"latte", "Latte", 4.50f, 1.00f, 3, 1, true});
        add_item({"cappuccino", "Cappuccino", 4.00f, 0.90f, 3, 1, true});
        add_item({"mocha", "Mocha", 5.00f, 1.50f, 4, 2, false});
        add_item({"croissant", "Croissant", 3.50f, 1.00f, 1, 1, true});
        add_item({"muffin", "Muffin", 3.00f, 0.80f, 1, 1, true});
        add_item({"sandwich", "Sandwich", 6.50f, 2.00f, 5, 2, false});
        add_item({"cake_slice", "Cake Slice", 5.50f, 1.80f, 2, 3, false});
        add_item({"iced_coffee", "Iced Coffee", 4.00f, 0.70f, 3, 2, false});
        add_item({"tea", "Tea", 2.00f, 0.30f, 2, 1, true});
    }

    void add_item(MenuItem item) {
        std::string id = item.id;
        item_order_.push_back(id);
        items_.insert(std::move(id), std::move(item));
    }

    std::optional<MenuItem> get_item(const std::string& id) const {
        return items_.get(id);
    }

    MenuItem& get_item_ref(const std::string& id) {
        return items_.at(id);
    }

    // Get all unlocked items
    std::vector<const MenuItem*> get_available_items() const {
        std::vector<const MenuItem*> available;
        for (const auto& id : item_order_) {
            if (auto item = items_.get(id)) {
                if (item->unlocked) {
                    available.push_back(&items_.at(id));
                }
            }
        }
        return available;
    }

    // Get all items (for save/load and display)
    std::vector<const MenuItem*> get_all_items() const {
        std::vector<const MenuItem*> all;
        for (const auto& id : item_order_) {
            all.push_back(&items_.at(id));
        }
        return all;
    }

    // Unlock items based on level
    void unlock_for_level(int level) {
        for (const auto& id : item_order_) {
            MenuItem& item = items_.at(id);
            if (item.unlock_level <= level && !item.unlocked) {
                item.unlocked = true;
            }
        }
    }

    // Get a random available item ID (for customer orders)
    std::string get_random_item_id() const {
        auto available = get_available_items();
        if (available.empty()) return "";
        size_t index = static_cast<size_t>(std::rand()) % available.size();
        return available[index]->id;
    }

    size_t available_count() const {
        size_t count = 0;
        for (const auto& id : item_order_) {
            if (auto item = items_.get(id)) {
                if (item->unlocked) ++count;
            }
        }
        return count;
    }

    size_t total_count() const {
        return item_order_.size();
    }
};

} // namespace cafe

#endif // CAFE_MENU_H

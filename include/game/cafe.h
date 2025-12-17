#ifndef CAFE_CAFE_H
#define CAFE_CAFE_H

#include <iostream>
#include <string>
#include <iomanip>
#include <limits>

#include "game/menu.h"
#include "game/customer.h"
#include "game/economy.h"
#include "game/save.h"
#include "core/ring_buffer.h"

namespace cafe {

// Game event for the log
struct GameEvent {
    std::string message;
    int day;
    int hour;
};

// Main game state
class CafeGame {
private:
    Menu menu_;
    CustomerManager customers_;
    Economy economy_;
    SaveSystem save_system_;
    RingBuffer<GameEvent, 10> event_log_;

    int day_ = 1;
    int hour_ = 8;  // 8 AM start
    bool running_ = true;
    bool day_ended_ = false;

    // Business hours
    static constexpr int OPEN_HOUR = 8;
    static constexpr int CLOSE_HOUR = 20;  // 8 PM

    void log_event(const std::string& msg) {
        event_log_.push_overwrite({msg, day_, hour_});
    }

    void clear_screen() {
        // Simple "clear" - print newlines
        std::cout << "\n\n";
    }

    void print_header() {
        std::cout << "============================================\n";
        std::cout << "          CAFE SIMULATOR\n";
        std::cout << "============================================\n";
        std::cout << "Day " << day_ << " - ";
        if (hour_ < 12) {
            std::cout << hour_ << ":00 AM\n";
        } else if (hour_ == 12) {
            std::cout << "12:00 PM\n";
        } else {
            std::cout << (hour_ - 12) << ":00 PM\n";
        }
        std::cout << "--------------------------------------------\n";
    }

    void print_status() {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Money: $" << economy_.money();
        std::cout << "  |  Level: " << economy_.level();
        std::cout << "  |  XP: " << economy_.xp()
                  << "/" << xp_for_level(economy_.level() + 1) << "\n";
        std::cout << "Customers waiting: " << customers_.waiting_count() << "\n";
        std::cout << "--------------------------------------------\n";
    }

    void print_menu_options() {
        std::cout << "\nWhat would you like to do?\n";
        std::cout << "[1] Wait for customers\n";
        std::cout << "[2] Serve next customer\n";
        std::cout << "[3] View menu\n";
        std::cout << "[4] View stats\n";
        std::cout << "[5] View event log\n";
        std::cout << "[6] End day\n";
        std::cout << "[7] Save & Quit\n";
        std::cout << "\n> ";
    }

    int get_input() {
        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return -1;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return choice;
    }

    void wait_for_enter() {
        std::cout << "\nPress Enter to continue...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    void wait_for_customers() {
        // Advance time by 1 hour
        ++hour_;

        // Chance for customers to arrive (more likely during peak hours)
        float spawn_chance = 0.3f;
        if (hour_ >= 11 && hour_ <= 13) spawn_chance = 0.6f;  // Lunch rush
        if (hour_ >= 17 && hour_ <= 19) spawn_chance = 0.5f;  // Dinner rush

        int new_customers = 0;
        while (customers_.has_space() && customers_.generator().chance(spawn_chance)) {
            std::string order_id = menu_.get_random_item_id();
            if (!order_id.empty()) {
                Customer* c = customers_.spawn_customer(order_id);
                if (c) {
                    auto item = menu_.get_item(order_id);
                    std::cout << "\n" << c->name << " arrived and wants a "
                              << item->name << " ($" << std::fixed
                              << std::setprecision(2) << item->sell_price << ")\n";
                    log_event(c->name + " arrived, wants " + item->name);
                    ++new_customers;
                }
            }
            spawn_chance *= 0.5f;  // Decrease chance for multiple arrivals
        }

        if (new_customers == 0) {
            std::cout << "\nNo new customers arrived this hour.\n";
        }

        // Update waiting customers' patience
        for (auto* c : customers_.get_all_customers()) {
            if (!c->served && !c->left) {
                c->wait_time += 10.0f;  // Each hour = 10 seconds of patience used
                c->update_satisfaction();

                if (c->wait_time >= c->patience) {
                    std::cout << c->name << " got tired of waiting and left! "
                              << c->satisfaction_emoji() << "\n";
                    log_event(c->name + " left angry (waited too long)");
                    c->left = true;
                    economy_.record_lost();
                }
            }
        }

        // Check for end of day
        if (hour_ >= CLOSE_HOUR) {
            std::cout << "\nIt's " << CLOSE_HOUR - 12 << " PM - closing time!\n";
            end_day();
        }
    }

    void serve_customer() {
        Customer* c = customers_.get_next_customer();
        if (!c) {
            std::cout << "\nNo customers waiting to be served.\n";
            return;
        }

        auto item_opt = menu_.get_item(c->order_item_id);
        if (!item_opt) {
            std::cout << "\nError: Unknown menu item.\n";
            return;
        }

        const MenuItem& item = *item_opt;

        // Check if we can afford the cost
        if (!economy_.can_afford(item.cost)) {
            std::cout << "\nYou can't afford to make a " << item.name
                      << " (cost: $" << std::fixed << std::setprecision(2)
                      << item.cost << ")\n";
            return;
        }

        // Process the order
        economy_.spend_money(item.cost);
        c->update_satisfaction();

        int sat_level = static_cast<int>(c->satisfaction);
        float tip = Economy::calculate_tip(item.sell_price, sat_level);
        int xp = Economy::calculate_xp(sat_level);

        float total_earned = item.sell_price + tip;
        economy_.add_money(total_earned);
        economy_.add_xp(xp);
        economy_.record_served();

        c->served = true;

        // Display result
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n--------------------------------------------\n";
        std::cout << "Served " << c->name << " a " << item.name << "!\n";
        std::cout << "  Revenue:  $" << item.sell_price << "\n";
        std::cout << "  Cost:     $" << item.cost << "\n";
        std::cout << "  Profit:   $" << item.profit() << "\n";
        if (tip > 0) {
            std::cout << "  Tip:      $" << tip << " " << c->satisfaction_emoji() << "\n";
        }
        std::cout << "  XP:       +" << xp << "\n";
        std::cout << "  Customer: " << c->satisfaction_str() << " " << c->satisfaction_emoji() << "\n";
        std::cout << "--------------------------------------------\n";

        log_event("Served " + c->name + " (" + c->satisfaction_str() + ")");

        // Check for level up
        int old_level = economy_.level();
        // XP already added, check if level changed
        if (economy_.level() > old_level) {
            std::cout << "\n*** LEVEL UP! You are now level " << economy_.level() << "! ***\n";
            menu_.unlock_for_level(economy_.level());
            std::cout << "New menu items may have been unlocked!\n";
            log_event("Reached level " + std::to_string(economy_.level()));
        }

        // Remove served customer after a delay
        customers_.remove_customer(c);
    }

    void view_menu() {
        clear_screen();
        std::cout << "============================================\n";
        std::cout << "               MENU\n";
        std::cout << "============================================\n";
        std::cout << std::fixed << std::setprecision(2);

        auto items = menu_.get_all_items();
        for (const auto* item : items) {
            if (item->unlocked) {
                std::cout << "  " << std::left << std::setw(15) << item->name
                          << " $" << std::setw(5) << item->sell_price
                          << " (cost: $" << item->cost
                          << ", profit: $" << item->profit() << ")\n";
            } else {
                std::cout << "  " << std::left << std::setw(15) << "[LOCKED]"
                          << " Unlocks at level " << item->unlock_level << "\n";
            }
        }

        std::cout << "\nAvailable items: " << menu_.available_count()
                  << "/" << menu_.total_count() << "\n";
    }

    void view_stats() {
        clear_screen();
        std::cout << "============================================\n";
        std::cout << "             STATISTICS\n";
        std::cout << "============================================\n";
        std::cout << std::fixed << std::setprecision(2);

        std::cout << "Current Money:     $" << economy_.money() << "\n";
        std::cout << "Total Revenue:     $" << economy_.total_revenue() << "\n";
        std::cout << "Total Costs:       $" << economy_.total_costs() << "\n";
        std::cout << "Total Profit:      $" << economy_.total_profit() << "\n";
        std::cout << "\n";
        std::cout << "Level:             " << economy_.level() << "\n";
        std::cout << "XP:                " << economy_.xp() << "/"
                  << xp_for_level(economy_.level() + 1) << "\n";
        std::cout << "Progress:          ";
        int bar_width = 20;
        int filled = static_cast<int>(economy_.level_progress() * bar_width);
        std::cout << "[";
        for (int i = 0; i < bar_width; ++i) {
            std::cout << (i < filled ? "=" : " ");
        }
        std::cout << "]\n";
        std::cout << "\n";
        std::cout << "Day:               " << day_ << "\n";
        std::cout << "Customers Served:  " << economy_.customers_served() << "\n";
        std::cout << "Customers Lost:    " << economy_.customers_lost() << "\n";

        if (economy_.customers_served() + economy_.customers_lost() > 0) {
            float rate = static_cast<float>(economy_.customers_served()) /
                         static_cast<float>(economy_.customers_served() + economy_.customers_lost());
            std::cout << "Satisfaction Rate: " << static_cast<int>(rate * 100) << "%\n";
        }
    }

    void view_event_log() {
        clear_screen();
        std::cout << "============================================\n";
        std::cout << "            RECENT EVENTS\n";
        std::cout << "============================================\n";

        if (event_log_.empty()) {
            std::cout << "No events yet.\n";
        } else {
            for (size_t i = 0; i < event_log_.size(); ++i) {
                const auto& e = event_log_.at(i);
                std::cout << "[Day " << e.day << ", ";
                if (e.hour < 12) {
                    std::cout << e.hour << "AM";
                } else if (e.hour == 12) {
                    std::cout << "12PM";
                } else {
                    std::cout << (e.hour - 12) << "PM";
                }
                std::cout << "] " << e.message << "\n";
            }
        }
    }

    void end_day() {
        day_ended_ = true;

        // Clear remaining customers
        auto& all = customers_.get_all_customers();
        for (auto* c : all) {
            if (!c->served && !c->left) {
                std::cout << c->name << " left as the cafe closed.\n";
            }
        }
        while (!all.empty()) {
            customers_.remove_customer(all.back());
        }

        std::cout << "\n============================================\n";
        std::cout << "           END OF DAY " << day_ << "\n";
        std::cout << "============================================\n";
        view_stats();

        log_event("Day " + std::to_string(day_) + " ended");

        // Ask to continue
        std::cout << "\nStart Day " << (day_ + 1) << "? [y/n]: ";
        char choice;
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice == 'y' || choice == 'Y') {
            ++day_;
            hour_ = OPEN_HOUR;
            day_ended_ = false;
            log_event("Day " + std::to_string(day_) + " started");
        } else {
            save_and_quit();
        }
    }

    void save_and_quit() {
        SaveData data;
        data.money = economy_.money();
        data.xp = economy_.xp();
        data.level = economy_.level();
        data.day = day_;
        data.hour = hour_;
        data.customers_served = economy_.customers_served();
        data.customers_lost = economy_.customers_lost();
        data.total_revenue = economy_.total_revenue();
        data.total_costs = economy_.total_costs();

        for (const auto* item : menu_.get_all_items()) {
            if (item->unlocked) {
                data.unlocked_items.push_back(item->id);
            }
        }

        if (save_system_.save(data)) {
            std::cout << "\nGame saved to " << save_system_.path() << "\n";
        } else {
            std::cout << "\nFailed to save game!\n";
        }

        running_ = false;
    }

    void load_game() {
        auto data = save_system_.load();
        if (!data || !data->valid) {
            std::cout << "No save file found. Starting new game.\n";
            return;
        }

        economy_.set_state(
            data->money,
            data->xp,
            data->level,
            data->customers_served,
            data->customers_lost,
            data->total_revenue,
            data->total_costs
        );

        day_ = data->day;
        hour_ = data->hour;

        // Restore unlocked items
        menu_.unlock_for_level(data->level);
        for (const auto& id : data->unlocked_items) {
            if (auto item = menu_.get_item(id)) {
                menu_.get_item_ref(id).unlocked = true;
            }
        }

        std::cout << "Game loaded! Day " << day_ << ", $"
                  << std::fixed << std::setprecision(2) << economy_.money() << "\n";
        log_event("Game loaded");
    }

public:
    CafeGame() = default;

    void run() {
        clear_screen();
        std::cout << "============================================\n";
        std::cout << "       WELCOME TO CAFE SIMULATOR!\n";
        std::cout << "============================================\n";
        std::cout << "\n[1] New Game\n";
        std::cout << "[2] Continue\n";
        std::cout << "\n> ";

        int choice = get_input();
        if (choice == 2) {
            load_game();
        } else {
            log_event("New game started");
        }

        // Unlock items for current level
        menu_.unlock_for_level(economy_.level());

        while (running_) {
            clear_screen();
            print_header();
            print_status();
            print_menu_options();

            choice = get_input();

            switch (choice) {
                case 1: wait_for_customers(); break;
                case 2: serve_customer(); break;
                case 3: view_menu(); break;
                case 4: view_stats(); break;
                case 5: view_event_log(); break;
                case 6: end_day(); break;
                case 7: save_and_quit(); break;
                default:
                    std::cout << "\nInvalid choice. Try again.\n";
                    break;
            }

            if (running_ && !day_ended_) {
                wait_for_enter();
            }
        }

        std::cout << "\nThanks for playing!\n";
    }
};

} // namespace cafe

#endif // CAFE_CAFE_H

#ifndef CAFE_ECONOMY_H
#define CAFE_ECONOMY_H

#include <cmath>

namespace cafe {

// XP required for each level
constexpr int xp_for_level(int level) {
    // Level 1: 0 XP, Level 2: 100 XP, Level 3: 250 XP, etc.
    if (level <= 1) return 0;
    return 50 * level * level;
}

// Tracks money, XP, and progression
class Economy {
private:
    float money_;
    int xp_;
    int level_;
    int customers_served_;
    int customers_lost_;
    float total_revenue_;
    float total_costs_;

public:
    Economy()
        : money_(100.0f)      // Starting money
        , xp_(0)
        , level_(1)
        , customers_served_(0)
        , customers_lost_(0)
        , total_revenue_(0.0f)
        , total_costs_(0.0f)
    {}

    // Money operations
    float money() const { return money_; }

    bool can_afford(float amount) const {
        return money_ >= amount;
    }

    void add_money(float amount) {
        money_ += amount;
        if (amount > 0) {
            total_revenue_ += amount;
        }
    }

    bool spend_money(float amount) {
        if (!can_afford(amount)) return false;
        money_ -= amount;
        total_costs_ += amount;
        return true;
    }

    // XP and leveling
    int xp() const { return xp_; }
    int level() const { return level_; }

    void add_xp(int amount) {
        xp_ += amount;
        // Check for level up
        while (xp_ >= xp_for_level(level_ + 1)) {
            ++level_;
        }
    }

    int xp_to_next_level() const {
        return xp_for_level(level_ + 1) - xp_;
    }

    float level_progress() const {
        int current_threshold = xp_for_level(level_);
        int next_threshold = xp_for_level(level_ + 1);
        int range = next_threshold - current_threshold;
        if (range <= 0) return 1.0f;
        return static_cast<float>(xp_ - current_threshold) / static_cast<float>(range);
    }

    // Statistics
    int customers_served() const { return customers_served_; }
    int customers_lost() const { return customers_lost_; }
    float total_revenue() const { return total_revenue_; }
    float total_costs() const { return total_costs_; }
    float total_profit() const { return total_revenue_ - total_costs_; }

    void record_served() { ++customers_served_; }
    void record_lost() { ++customers_lost_; }

    // Calculate tip based on satisfaction
    static float calculate_tip(float base_price, int satisfaction_level) {
        // satisfaction_level: 0=angry, 1=unhappy, 2=neutral, 3=happy, 4=delighted
        switch (satisfaction_level) {
            case 4: return base_price * 0.25f;  // 25% tip
            case 3: return base_price * 0.15f;  // 15% tip
            case 2: return base_price * 0.05f;  // 5% tip
            default: return 0.0f;               // No tip
        }
    }

    // XP earned based on satisfaction
    static int calculate_xp(int satisfaction_level) {
        switch (satisfaction_level) {
            case 4: return 15;  // Delighted
            case 3: return 10;  // Happy
            case 2: return 5;   // Neutral
            case 1: return 2;   // Unhappy
            default: return 0;  // Angry
        }
    }

    // For save/load
    void set_state(float money, int xp, int level, int served, int lost,
                   float revenue, float costs) {
        money_ = money;
        xp_ = xp;
        level_ = level;
        customers_served_ = served;
        customers_lost_ = lost;
        total_revenue_ = revenue;
        total_costs_ = costs;
    }
};

} // namespace cafe

#endif // CAFE_ECONOMY_H

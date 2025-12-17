# Phase 6: Cafe Game - Core Loop

**Duration**: Months 15-17
**Goal**: Implement the actual cafe simulation gameplay
**Deliverable**: Playable cafe sim MVP with core mechanics

## Overview

This is where you build the actual game! Using the engine and framework from previous phases, implement:
- Economy system
- Time/day system
- Customer simulation
- Menu and ordering
- Progression and unlocks

## Core Game Loop

```
┌─────────────────────────────────────────────────────────────────┐
│                        CORE LOOP                                │
│                                                                 │
│   Time Passes → Customers Arrive → Orders Placed → Money Earned │
│        ↑                                                   │    │
│        └──────────── Unlock New Items ←────────────────────┘    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## Tasks

### 6.1 Economy System

**Components:**
- Money balance
- Costs (ingredients, overhead)
- Revenue (menu prices)
- Profit tracking

**Implementation:**
```cpp
class EconomySystem {
    int money_ = 1000;        // Starting money
    int lifetime_earnings_ = 0;
    int today_revenue_ = 0;
    int today_costs_ = 0;

public:
    bool can_afford(int amount) const { return money_ >= amount; }

    void spend(int amount, const std::string& reason) {
        money_ -= amount;
        today_costs_ += amount;
        // Log transaction
    }

    void earn(int amount, const std::string& source) {
        money_ += amount;
        today_revenue_ += amount;
        lifetime_earnings_ += amount;
    }

    int profit_today() const { return today_revenue_ - today_costs_; }

    void end_of_day() {
        // Daily summary
        // Reset daily counters
        today_revenue_ = 0;
        today_costs_ = 0;
    }
};
```

**Menu Item Economics:**
```cpp
struct MenuItem {
    std::string id;
    std::string name;
    int sell_price;      // What customer pays
    int cost;            // Ingredient cost
    int prep_time_sec;   // How long to prepare
    bool unlocked = false;
};

// Profit per item = sell_price - cost
// Example: Latte sells for $5, costs $2 = $3 profit
```

**Checklist:**
- [ ] Money tracking
- [ ] Earn money from sales
- [ ] Spend money on costs
- [ ] Daily profit calculation
- [ ] Transaction history

---

### 6.2 Time System

**Components:**
- In-game clock (hours, minutes)
- Day counter
- Speed controls (pause, 1x, 2x, 3x)
- Business hours

**Implementation:**
```cpp
class TimeSystem {
    int day_ = 1;
    int hour_ = 8;      // 24-hour format
    int minute_ = 0;

    float time_scale_ = 1.0f;  // 1.0 = normal, 2.0 = double speed
    bool paused_ = false;

    // 1 real second = 1 game minute at 1x speed
    float seconds_per_game_minute_ = 1.0f;
    float accumulator_ = 0;

    int open_hour_ = 8;
    int close_hour_ = 20;

public:
    void update(double dt) {
        if (paused_) return;

        accumulator_ += dt * time_scale_;

        while (accumulator_ >= seconds_per_game_minute_) {
            accumulator_ -= seconds_per_game_minute_;
            advance_minute();
        }
    }

    void advance_minute() {
        minute_++;
        if (minute_ >= 60) {
            minute_ = 0;
            hour_++;
            if (hour_ >= 24) {
                hour_ = 0;
                day_++;
                on_new_day();
            }
        }
        // Fire time events
        events_.fire("minute_tick");
    }

    bool is_open() const {
        return hour_ >= open_hour_ && hour_ < close_hour_;
    }

    std::string formatted_time() const {
        // "Day 3 14:30"
        return "Day " + std::to_string(day_) + " " +
               std::to_string(hour_) + ":" +
               (minute_ < 10 ? "0" : "") + std::to_string(minute_);
    }

    void set_speed(int level) {
        // 0 = pause, 1 = 1x, 2 = 2x, 3 = 3x
        if (level == 0) {
            paused_ = true;
        } else {
            paused_ = false;
            time_scale_ = (float)level;
        }
    }
};
```

**Checklist:**
- [ ] Time advances automatically
- [ ] Speed controls (pause, 1x, 2x, 3x)
- [ ] Day/night cycle
- [ ] Business hours
- [ ] End-of-day events

---

### 6.3 Customer Simulation

**Customer Lifecycle:**
```
Spawn → Walk to Seat → Browse Menu → Order → Wait → Receive → Eat → Pay → Leave
```

**Customer States:**
```cpp
enum class CustomerState {
    Entering,           // Walking to seat
    Browsing,           // Looking at menu
    WaitingToOrder,     // Ready to order
    WaitingForFood,     // Order placed
    Eating,             // Consuming order
    Paying,             // At counter
    Leaving             // Walking out
};

class CustomerComponent : public Component {
    CustomerState state_ = CustomerState::Entering;
    MenuItem* current_order_ = nullptr;
    float state_timer_ = 0;
    float patience_ = 100.0f;  // Decreases while waiting

public:
    void update(double dt) override {
        state_timer_ += dt;

        switch (state_) {
            case CustomerState::Entering:
                // Pathfind to available seat
                if (reached_seat()) {
                    transition(CustomerState::Browsing);
                }
                break;

            case CustomerState::Browsing:
                // Wait a few seconds, then order
                if (state_timer_ > 3.0f) {
                    current_order_ = pick_random_menu_item();
                    transition(CustomerState::WaitingToOrder);
                }
                break;

            case CustomerState::WaitingForFood:
                patience_ -= dt * 5;  // Lose patience over time
                if (patience_ <= 0) {
                    // Customer leaves unhappy
                    transition(CustomerState::Leaving);
                }
                break;

            // ... other states
        }
    }

    void receive_order() {
        if (state_ == CustomerState::WaitingForFood) {
            transition(CustomerState::Eating);
        }
    }
};
```

**Customer Spawning:**
```cpp
class CustomerSpawner {
    float spawn_timer_ = 0;
    float spawn_interval_ = 30.0f;  // Seconds between spawns

public:
    void update(double dt, TimeSystem* time) {
        if (!time->is_open()) return;

        spawn_timer_ += dt;
        if (spawn_timer_ >= spawn_interval_) {
            spawn_timer_ = 0;
            spawn_customer();

            // Adjust spawn rate based on time of day
            // Busier at lunch/dinner
        }
    }

    void spawn_customer() {
        Entity* customer = entity_manager_->create_entity("Customer");
        // Add components, set spawn position
        // Customer will pathfind to available seat
    }
};
```

**Checklist:**
- [ ] Customer spawning
- [ ] Customer state machine
- [ ] Pathfinding to seats
- [ ] Ordering behavior
- [ ] Patience system
- [ ] Customer leaves after eating/angry

---

### 6.4 Menu & Ordering Flow

**Menu Data:**
```json
{
    "menu_items": [
        {
            "id": "coffee",
            "name": "Coffee",
            "price": 3,
            "cost": 1,
            "prep_time": 5,
            "unlock_level": 1
        },
        {
            "id": "latte",
            "name": "Latte",
            "price": 5,
            "cost": 2,
            "prep_time": 10,
            "unlock_level": 2
        },
        {
            "id": "croissant",
            "name": "Croissant",
            "price": 4,
            "cost": 1,
            "prep_time": 3,
            "unlock_level": 1
        }
    ]
}
```

**Order Queue:**
```cpp
struct Order {
    EntityID customer_id;
    std::string item_id;
    float prep_time_remaining;
    OrderStatus status;  // Pending, Preparing, Ready
};

class OrderSystem {
    std::vector<Order> orders_;
    int max_concurrent_orders_ = 3;

public:
    void place_order(EntityID customer, const std::string& item_id) {
        MenuItem* item = menu_->get_item(item_id);

        Order order;
        order.customer_id = customer;
        order.item_id = item_id;
        order.prep_time_remaining = item->prep_time;
        order.status = OrderStatus::Pending;

        orders_.push_back(order);
        economy_->spend(item->cost, "Ingredients: " + item->name);
    }

    void update(double dt) {
        int preparing = 0;
        for (auto& order : orders_) {
            if (order.status == OrderStatus::Pending && preparing < max_concurrent_orders_) {
                order.status = OrderStatus::Preparing;
            }

            if (order.status == OrderStatus::Preparing) {
                preparing++;
                order.prep_time_remaining -= dt;
                if (order.prep_time_remaining <= 0) {
                    order.status = OrderStatus::Ready;
                    // Notify customer, collect payment
                    complete_order(order);
                }
            }
        }

        // Remove completed orders
        orders_.erase(
            std::remove_if(orders_.begin(), orders_.end(),
                [](const Order& o) { return o.status == OrderStatus::Completed; }),
            orders_.end()
        );
    }

    void complete_order(Order& order) {
        MenuItem* item = menu_->get_item(order.item_id);
        economy_->earn(item->price, "Sale: " + item->name);

        Entity* customer = entities_->get(order.customer_id);
        if (customer) {
            customer->get_component<CustomerComponent>()->receive_order();
        }

        order.status = OrderStatus::Completed;
        stats_->increment_served();
    }
};
```

**Checklist:**
- [ ] Menu loads from JSON
- [ ] Customers place orders
- [ ] Order queue system
- [ ] Prep time countdown
- [ ] Payment on completion

---

### 6.5 Progression & Unlocks

**XP and Levels:**
```cpp
class ProgressionSystem {
    int xp_ = 0;
    int level_ = 1;

    // XP required for each level
    int xp_for_level(int level) {
        return 100 * level * level;  // 100, 400, 900, 1600...
    }

public:
    void add_xp(int amount) {
        xp_ += amount;

        while (xp_ >= xp_for_level(level_)) {
            level_++;
            on_level_up();
        }
    }

    void on_level_up() {
        // Unlock new items
        for (auto& item : menu_->items()) {
            if (item.unlock_level == level_ && !item.unlocked) {
                item.unlocked = true;
                show_unlock_notification(item);
            }
        }
    }
};
```

**Unlock Types:**
- Menu items (new drinks, food)
- Furniture (more seats, decorations)
- Upgrades (faster prep, more capacity)

**Checklist:**
- [ ] XP system
- [ ] Level progression
- [ ] Unlock new menu items
- [ ] Unlock notifications
- [ ] Save/load progression

---

## Milestone: Playable MVP

Your MVP cafe sim should have:

1. **Start a new game** or continue saved game
2. **Time passes** automatically (with speed controls)
3. **Customers arrive** during business hours
4. **Orders are placed** and prepared
5. **Money is earned** from sales
6. **XP is earned** leading to **unlocks**
7. **Day ends** with summary
8. **Game can be saved** and resumed

**Gameplay Session:**
```
Day 1 starts at 8:00
First customer arrives, orders coffee
Coffee prepared, served, $3 earned
More customers arrive throughout the day
8:00 PM - cafe closes
Day summary: $47 profit, 12 customers served
Level up! Latte unlocked!
Day 2 begins...
```

---

## Checklist

- [ ] Economy system tracking money
- [ ] Time system with speed controls
- [ ] Customers spawn and behave correctly
- [ ] Orders placed and fulfilled
- [ ] Progression system with unlocks
- [ ] MVP is fun to play!

## Next Phase

Proceed to [Phase 7: Additional Platforms](../phase-7-additional-platforms/overview.md) to add iOS, Windows, and Android.

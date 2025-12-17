#ifndef CAFE_CUSTOMER_H
#define CAFE_CUSTOMER_H

#include <string>
#include <vector>
#include <random>
#include "core/object_pool.h"

namespace cafe {

// Customer satisfaction levels
enum class Satisfaction {
    ANGRY,      // Waited too long, left without ordering
    UNHAPPY,    // Slow service
    NEUTRAL,    // Normal service
    HAPPY,      // Fast service
    DELIGHTED   // Very fast service, might tip
};

// Represents a customer in the cafe
struct Customer {
    std::string name;
    std::string order_item_id;
    float patience;           // Seconds willing to wait
    float wait_time;          // How long they've waited
    Satisfaction satisfaction;
    bool served;
    bool left;

    Customer()
        : name("")
        , order_item_id("")
        , patience(30.0f)
        , wait_time(0.0f)
        , satisfaction(Satisfaction::NEUTRAL)
        , served(false)
        , left(false)
    {}

    Customer(std::string n, std::string order, float pat)
        : name(std::move(n))
        , order_item_id(std::move(order))
        , patience(pat)
        , wait_time(0.0f)
        , satisfaction(Satisfaction::NEUTRAL)
        , served(false)
        , left(false)
    {}

    // Update satisfaction based on wait time
    void update_satisfaction() {
        float ratio = wait_time / patience;
        if (ratio < 0.3f) {
            satisfaction = Satisfaction::DELIGHTED;
        } else if (ratio < 0.5f) {
            satisfaction = Satisfaction::HAPPY;
        } else if (ratio < 0.8f) {
            satisfaction = Satisfaction::NEUTRAL;
        } else if (ratio < 1.0f) {
            satisfaction = Satisfaction::UNHAPPY;
        } else {
            satisfaction = Satisfaction::ANGRY;
        }
    }

    const char* satisfaction_str() const {
        switch (satisfaction) {
            case Satisfaction::DELIGHTED: return "Delighted!";
            case Satisfaction::HAPPY: return "Happy";
            case Satisfaction::NEUTRAL: return "Okay";
            case Satisfaction::UNHAPPY: return "Unhappy";
            case Satisfaction::ANGRY: return "Angry!";
        }
        return "Unknown";
    }

    const char* satisfaction_emoji() const {
        switch (satisfaction) {
            case Satisfaction::DELIGHTED: return ":D";
            case Satisfaction::HAPPY: return ":)";
            case Satisfaction::NEUTRAL: return ":|";
            case Satisfaction::UNHAPPY: return ":(";
            case Satisfaction::ANGRY: return ">:(";
        }
        return "?";
    }
};

// Generates random customers
class CustomerGenerator {
private:
    std::vector<std::string> first_names_ = {
        "Alice", "Bob", "Charlie", "Diana", "Eve", "Frank",
        "Grace", "Henry", "Ivy", "Jack", "Kate", "Leo",
        "Mia", "Noah", "Olivia", "Pete", "Quinn", "Rose",
        "Sam", "Tina", "Uma", "Victor", "Wendy", "Xavier"
    };

    std::mt19937 rng_;

public:
    CustomerGenerator() : rng_(std::random_device{}()) {}

    // Seed for reproducible testing
    void seed(unsigned int s) { rng_.seed(s); }

    std::string random_name() {
        std::uniform_int_distribution<size_t> dist(0, first_names_.size() - 1);
        return first_names_[dist(rng_)];
    }

    float random_patience() {
        // 20-60 seconds of patience
        std::uniform_real_distribution<float> dist(20.0f, 60.0f);
        return dist(rng_);
    }

    // Returns true with given probability (0.0 to 1.0)
    bool chance(float probability) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(rng_) < probability;
    }

    // Random int in range [min, max]
    int random_int(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(rng_);
    }
};

// Manages active customers using object pool
class CustomerManager {
private:
    ObjectPool<Customer, 10> pool_;
    std::vector<Customer*> active_customers_;
    CustomerGenerator generator_;

public:
    Customer* spawn_customer(const std::string& order_item_id) {
        Customer* c = pool_.acquire(
            generator_.random_name(),
            order_item_id,
            generator_.random_patience()
        );

        if (c) {
            active_customers_.push_back(c);
        }
        return c;
    }

    void remove_customer(Customer* c) {
        auto it = std::find(active_customers_.begin(), active_customers_.end(), c);
        if (it != active_customers_.end()) {
            active_customers_.erase(it);
        }
        pool_.release(c);
    }

    // Get the first waiting customer (FIFO)
    Customer* get_next_customer() {
        for (auto* c : active_customers_) {
            if (!c->served && !c->left) {
                return c;
            }
        }
        return nullptr;
    }

    std::vector<Customer*>& get_all_customers() {
        return active_customers_;
    }

    size_t waiting_count() const {
        size_t count = 0;
        for (const auto* c : active_customers_) {
            if (!c->served && !c->left) ++count;
        }
        return count;
    }

    size_t total_count() const {
        return active_customers_.size();
    }

    bool has_space() const {
        return !pool_.full();
    }

    CustomerGenerator& generator() { return generator_; }
};

} // namespace cafe

#endif // CAFE_CUSTOMER_H

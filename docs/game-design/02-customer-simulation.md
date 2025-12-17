# Customer Simulation Design

## Overview

Customers are the heart of the cafe sim. Their behavior creates the gameplay loop and emotional engagement.

## Customer Lifecycle

```
[Spawn] → [Enter] → [Find Seat] → [Browse] → [Order] → [Wait] → [Eat] → [Pay] → [Leave]
```

### States

| State | Duration | Player Action |
|-------|----------|---------------|
| Entering | 2-3s | None |
| Finding Seat | 1-2s | None |
| Browsing Menu | 3-5s | None |
| Waiting to Order | Until acknowledged | Click to take order |
| Waiting for Food | Until served | Prepare order |
| Eating | 5-10s | None |
| Paying | 1-2s | Automatic |
| Leaving | 2-3s | None |

## Customer Types

### Regular Customer
- Most common (70%)
- Average patience
- Orders any available item
- Tips occasionally

### Coffee Enthusiast
- Uncommon (15%)
- Only orders coffee drinks
- Higher patience
- Tips well for specialty drinks

### Food Lover
- Uncommon (10%)
- Only orders food items
- Lower patience
- Orders multiple items

### VIP (Future)
- Rare (5%)
- Orders expensive items
- Impatient
- Tips generously if satisfied

## Patience System

Customers have a patience meter that depletes while waiting.

```cpp
struct CustomerPatience {
    float max_patience = 100.0f;
    float current = 100.0f;
    float decay_rate = 2.0f;  // Per second while waiting

    // Modifiers
    float browsing_decay = 0.5f;   // Slower while browsing
    float waiting_decay = 1.0f;    // Normal while waiting for food
    float crowded_decay = 1.5f;    // Faster when cafe is crowded
};
```

**Patience Outcomes:**

| Patience | Outcome |
|----------|---------|
| 100-70% | Happy - Full price + tip chance |
| 70-40% | Satisfied - Full price |
| 40-10% | Annoyed - Full price, no tip |
| <10% | Leaves - No payment, negative review |

## Customer Spawning

### Spawn Rate

Based on time of day:

```
Hour:     8  9  10 11 12 13 14 15 16 17 18 19
Rate:    Low Med Med Med High High Med Med Med Med High Med
```

**Peak Hours:**
- Lunch: 12:00-13:00
- Dinner: 18:00-19:00

### Spawn Formula

```cpp
float base_interval = 30.0f;  // Seconds between spawns
float time_multiplier = get_time_multiplier(current_hour);
float capacity_factor = 1.0f - (seated_customers / max_capacity);

float spawn_interval = base_interval / time_multiplier * (0.5f + capacity_factor * 0.5f);
```

### Customer Cap

Maximum simultaneous customers = available seats

## Order Behavior

### Menu Selection

```cpp
MenuItem* Customer::choose_order() {
    // Filter by type preference
    auto available = menu->get_available_items();

    if (type == CustomerType::CoffeeEnthusiast) {
        available = filter(available, is_coffee);
    }

    // Weight by price (prefer affordable)
    // Weight by popularity (slight random factor)

    return weighted_random(available);
}
```

### Multiple Orders (Future)

Food customers may order:
- Drink + Food item
- Multiple food items for groups

## Visual Indicators

### Patience Indicator
- Small icon above customer
- Green → Yellow → Red as patience depletes

### Order Bubble
- Speech bubble showing what they ordered
- "!" icon when ready to order
- "..." while waiting

### Emotion Indicators
- Happy face when served quickly
- Neutral when satisfied
- Angry face when leaving dissatisfied

## Pathfinding Behavior

### Entry
1. Spawn at door
2. Pathfind to nearest available seat
3. Sit down

### Exit
1. Stand up
2. Pathfind to counter (to pay)
3. Pathfind to door
4. Despawn

### Avoidance
- Avoid other customers
- Avoid furniture
- Recalculate if path blocked

## Customer Memory (Future)

Repeat customers remember:
- Their favorite items
- Service quality
- Return more often if happy

## Tuning Parameters

```cpp
namespace CustomerConfig {
    // Spawning
    constexpr float BASE_SPAWN_INTERVAL = 30.0f;
    constexpr int MAX_CUSTOMERS = 20;

    // Patience
    constexpr float BASE_PATIENCE = 100.0f;
    constexpr float PATIENCE_DECAY_RATE = 2.0f;
    constexpr float LEAVE_THRESHOLD = 10.0f;

    // Timing
    constexpr float BROWSE_TIME_MIN = 3.0f;
    constexpr float BROWSE_TIME_MAX = 5.0f;
    constexpr float EAT_TIME_MIN = 5.0f;
    constexpr float EAT_TIME_MAX = 10.0f;

    // Type distribution
    constexpr float REGULAR_CHANCE = 0.70f;
    constexpr float COFFEE_FAN_CHANCE = 0.15f;
    constexpr float FOOD_LOVER_CHANCE = 0.10f;
    constexpr float VIP_CHANCE = 0.05f;
}
```

## Customer Sprite Variations

To avoid repetition, customers should have visual variety:

**Variation Sources:**
- 3+ base character sprites
- Hair color variations (palette swap)
- Clothing color variations
- Accessories (glasses, hats)

**Implementation:**
```cpp
struct CustomerAppearance {
    int base_sprite;      // 0-2
    int hair_palette;     // 0-4
    int clothes_palette;  // 0-4
    int accessory;        // 0-3 (none, glasses, hat, both)
};

// 3 * 5 * 5 * 4 = 300 unique combinations
```

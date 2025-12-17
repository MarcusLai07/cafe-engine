# Economy System Design

## Overview

The economy system manages money flow, pricing, costs, and player wealth.

## Currency

**Primary Currency:** Dollars ($)
- Earned from customer sales
- Spent on ingredients, staff, furniture

## Income Sources

### Sales Revenue
Each menu item has a sell price paid by customers.

| Item | Sell Price | Cost | Profit |
|------|------------|------|--------|
| Coffee | $3 | $1 | $2 |
| Latte | $5 | $2 | $3 |
| Cappuccino | $5 | $2 | $3 |
| Espresso | $4 | $1 | $3 |
| Tea | $3 | $1 | $2 |
| Croissant | $4 | $1 | $3 |
| Muffin | $3 | $1 | $2 |
| Sandwich | $7 | $3 | $4 |
| Cake Slice | $6 | $2 | $4 |

### Tips (Future)
- Happy customers may leave tips
- Based on service speed and satisfaction

## Expenses

### Ingredient Costs
- Deducted when order is placed (not when served)
- Ensures players feel the cost commitment

### Staff Wages
| Role | Daily Wage |
|------|------------|
| Barista | $50 |
| Server | $40 |
| Chef | $60 |

### Furniture Costs
One-time purchase:
| Item | Cost |
|------|------|
| Small Table | $50 |
| Large Table | $100 |
| Chair | $25 |
| Counter | $200 |
| Decoration | $30-100 |

### Overhead (Future)
- Rent: Daily fixed cost
- Utilities: Based on size

## Starting Conditions

**New Game Start:**
- Money: $1,000
- Menu: Coffee, Tea, Muffin
- Furniture: Basic counter, 2 tables, 4 chairs
- Staff: None (player handles everything)

## Economic Balance

### Target Metrics

| Metric | Target |
|--------|--------|
| Daily Revenue (Early) | $50-100 |
| Daily Revenue (Mid) | $200-400 |
| Daily Revenue (Late) | $500+ |
| Profit Margin | 50-70% |
| Days to first unlock | 2-3 |
| Days to max level | 30-50 |

### Scaling

Revenue increases through:
1. More customers (capacity increase)
2. Higher-priced items (unlocks)
3. Staff efficiency (faster service)
4. Furniture bonuses (ambiance)

Costs increase through:
1. Staff wages
2. Premium ingredient items
3. Overhead (future)

### Anti-Snowball Mechanics

To prevent early advantages from compounding:
- Unlock costs scale with level
- Staff wages provide diminishing returns
- Customer patience doesn't scale infinitely

## Economic Events (Future)

### Positive Events
- Rush hour (2x customers)
- Review boost (better tips)
- Sale day (draw more customers)

### Negative Events
- Slow day (fewer customers)
- Supply shortage (some items unavailable)
- Equipment breakdown (slower service)

## Save Data

```json
{
    "money": 1523,
    "lifetime_earnings": 8420,
    "customers_served": 156,
    "best_day_profit": 245
}
```

## UI Display

**HUD (Always Visible):**
- Current money: "$1,523"

**End of Day Summary:**
```
=== Day 5 Summary ===
Revenue:    $234
Costs:      -$89
Wages:      -$50
-----------------
Profit:     $95

Customers:  18
Best Tip:   $5
```

## Tuning Parameters

These values should be easily adjustable for balancing:

```cpp
namespace EconomyConfig {
    constexpr int STARTING_MONEY = 1000;
    constexpr float PROFIT_MARGIN_TARGET = 0.6f;
    constexpr int BASE_CUSTOMER_VALUE = 5;
    constexpr float TIP_CHANCE = 0.2f;
    constexpr float TIP_MULTIPLIER = 0.15f;
}
```

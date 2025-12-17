# Time System Design

## Overview

The time system drives the game's pacing, creating the rhythm of cafe days and progression.

## Time Scale

### Real Time to Game Time

| Speed | Real:Game Ratio | Description |
|-------|-----------------|-------------|
| Paused | 0:0 | Game frozen |
| 1x | 1s = 1m | Normal speed |
| 2x | 1s = 2m | Double speed |
| 3x | 1s = 3m | Triple speed |

### Day Length

At 1x speed:
- 1 game hour = 60 real seconds = 1 real minute
- 1 game day (8AM-8PM) = 12 game hours = 12 real minutes
- Full day including closed hours = 24 game hours = 24 real minutes

**Target session:** 2-4 game days = 24-48 real minutes

## Business Hours

### Default Hours
- Open: 8:00 AM
- Close: 8:00 PM (20:00)
- Duration: 12 hours

### Closed Hours (8PM - 8AM)
- No customers
- Time passes faster (optional skip)
- Daily summary displayed
- Wages deducted
- New day preparations

## Daily Cycle

```
8:00  - Cafe opens
       - Morning rush begins
       - Coffee orders dominate

12:00 - Lunch rush
       - Food orders increase
       - Peak customer count

15:00 - Afternoon lull
       - Slower period
       - Time to catch up

18:00 - Dinner rush
       - Second peak
       - Mixed orders

20:00 - Cafe closes
       - Final customers leave
       - Day summary shown

20:00-8:00 - Night (skipped or fast-forward)
```

## Time Events

### Hourly Events
```cpp
void TimeSystem::on_hour_change(int new_hour) {
    // Update spawn rates
    customer_spawner_->set_rate_multiplier(get_hour_multiplier(new_hour));

    // Visual changes
    lighting_->set_time_of_day(new_hour);

    // Special hour events
    if (new_hour == 12) events_->fire("lunch_rush_start");
    if (new_hour == 18) events_->fire("dinner_rush_start");
}
```

### End of Day
```cpp
void TimeSystem::on_day_end() {
    // Stop spawning
    customer_spawner_->set_enabled(false);

    // Wait for remaining customers
    // Show day summary
    // Deduct wages
    // Save game

    // Advance to next day
    day_++;
    hour_ = 8;
    minute_ = 0;
}
```

## Speed Controls UI

```
[ || ]  [ > ]  [ >> ]  [ >>> ]
Pause   1x     2x      3x

Current: Day 3, 14:35  [2x]
```

### Speed Behavior

| Speed | When to Use |
|-------|-------------|
| Pause | Reading menus, planning, AFK |
| 1x | Active gameplay, managing rush |
| 2x | Moderate activity, catching up |
| 3x | Slow periods, end of day |

### Auto-Speed (Optional Future Feature)
- Automatically slow down during rush
- Speed up during quiet periods
- Player can override

## Day/Night Visual Effects

### Lighting Changes

| Time | Lighting |
|------|----------|
| 8:00-10:00 | Morning (warm, golden) |
| 10:00-16:00 | Day (bright, neutral) |
| 16:00-18:00 | Afternoon (warm, orange tint) |
| 18:00-20:00 | Evening (dim, purple tint) |

### Implementation
```cpp
Color TimeSystem::get_ambient_color(int hour, int minute) {
    // Interpolate between time-of-day colors
    float t = (hour * 60 + minute) / (24.0f * 60.0f);

    // Morning: warm gold
    // Midday: neutral white
    // Evening: warm orange/purple

    return lerp_color(morning_color, evening_color, t);
}
```

## Calendar (Future)

### Days of Week
- Weekdays: Normal activity
- Weekend: 1.5x customer rate

### Seasons
- Spring: Flower decorations
- Summer: Cold drinks popular
- Fall: Warm drinks popular
- Winter: Holiday events

### Special Days
- Holidays: Special customers, themed items
- Anniversary: Bonus XP
- Random events: Health inspector, critic visit

## Time Display

### HUD Format
```
Day 3 | 14:35 | [>>]
```

### Detailed View
```
Monday, Day 3
2:35 PM
Business Hours: 8 AM - 8 PM
Time until close: 5h 25m
```

## Save/Load Time

```json
{
    "day": 3,
    "hour": 14,
    "minute": 35,
    "total_minutes_played": 4567
}
```

## Tuning Parameters

```cpp
namespace TimeConfig {
    // Core timing
    constexpr float SECONDS_PER_GAME_MINUTE = 1.0f;
    constexpr int OPEN_HOUR = 8;
    constexpr int CLOSE_HOUR = 20;

    // Speed settings
    constexpr float SPEED_1X = 1.0f;
    constexpr float SPEED_2X = 2.0f;
    constexpr float SPEED_3X = 3.0f;

    // Rush hour multipliers
    constexpr float MORNING_RUSH_MULT = 1.3f;
    constexpr float LUNCH_RUSH_MULT = 1.8f;
    constexpr float AFTERNOON_MULT = 0.7f;
    constexpr float DINNER_RUSH_MULT = 1.6f;
}
```

## Technical Considerations

### Frame Independence
```cpp
void TimeSystem::update(double real_dt) {
    if (paused_) return;

    // Scale by game speed
    double game_dt = real_dt * speed_multiplier_;

    // Accumulate time
    accumulated_seconds_ += game_dt;

    // Convert to game minutes
    while (accumulated_seconds_ >= SECONDS_PER_GAME_MINUTE) {
        accumulated_seconds_ -= SECONDS_PER_GAME_MINUTE;
        advance_minute();
    }
}
```

### Determinism
For potential replay/sync features, time should be deterministic:
- Based on tick count, not wall clock
- Same inputs = same time progression

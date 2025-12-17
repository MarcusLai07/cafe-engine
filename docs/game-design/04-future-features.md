# Future Features

This document captures ideas for features beyond the MVP, prioritized by value and complexity.

## Priority Matrix

| Priority | Value | Complexity |
|----------|-------|------------|
| P1 | High value, Low complexity | Do first |
| P2 | High value, Medium complexity | Do after core |
| P3 | Medium value, Any complexity | Nice to have |
| P4 | Low value or High complexity | Maybe later |

---

## P1 - High Priority Post-MVP

### More Menu Items
**Value:** Keeps progression interesting
**Complexity:** Low (data only)

Additional items to unlock:
- Drinks: Mocha, Hot Chocolate, Smoothies, Iced Coffee
- Food: Bagels, Salads, Pasta, Soup
- Desserts: Cheesecake, Ice Cream, Cookies

### Customer Variety
**Value:** Visual interest, replayability
**Complexity:** Low (sprites + palette swaps)

- More base character designs
- Randomized accessories
- Seasonal outfits

### Sound Effects Polish
**Value:** Game feel
**Complexity:** Low

- More varied customer sounds
- Environmental ambiance
- Music variations by time of day

---

## P2 - Medium Priority

### Staff Management
**Value:** Adds strategic depth
**Complexity:** Medium

**Features:**
- Hire/fire staff
- Assign roles (barista, server, chef)
- Staff skill progression
- Wage management

**Gameplay Impact:**
- More staff = faster service
- Skilled staff = better quality
- Wages affect profit margin

### Cafe Decoration
**Value:** Player expression, goals
**Complexity:** Medium

**Features:**
- Place furniture freely
- Decorative items (plants, art)
- Furniture affects ambiance
- Ambiance affects customer happiness

### Customer Types
**Value:** Gameplay variety
**Complexity:** Medium

**Types:**
- Regular - Standard behavior
- Critic - Affects reputation
- Blogger - Shares on social media
- Tourist - Orders specialty items
- Group - Multiple orders

### Weather System
**Value:** Visual variety, gameplay impact
**Complexity:** Medium

**Weather Types:**
- Sunny - Normal
- Rainy - More customers (shelter)
- Snowy - Hot drinks popular
- Hot - Cold drinks popular

---

## P3 - Nice to Have

### Reputation System
**Value:** Long-term goals
**Complexity:** Medium

- Stars (1-5) based on service
- Affects customer rate
- Unlock new customers at higher rep
- Can decrease from poor service

### Daily Challenges
**Value:** Session variety
**Complexity:** Medium

**Examples:**
- "Serve 20 coffees today"
- "Earn $100 in tips"
- "No unhappy customers"

**Rewards:**
- Bonus XP
- Bonus money
- Cosmetic unlocks

### Seasonal Events
**Value:** Retention, freshness
**Complexity:** Medium-High

**Events:**
- Halloween - Spooky decorations, themed items
- Christmas - Holiday specials, gifts
- Valentine's - Couple customers, heart items
- Summer - Beach theme, cold drinks

### Multiple Locations
**Value:** End-game content
**Complexity:** High

- Unlock second cafe location
- Different themes (beach cafe, downtown)
- Manage multiple locations
- Staff transfers between locations

---

## P4 - Maybe Later

### Achievements
**Value:** Completionist goals
**Complexity:** Low

**Examples:**
- "Serve 1000 customers"
- "Earn $10,000 lifetime"
- "Reach level 20"
- "Go a week without angry customers"

### Leaderboards
**Value:** Competition
**Complexity:** Medium (requires server)

- Daily earnings ranking
- Customers served ranking
- Weekly challenges

### Mini-Games
**Value:** Variety
**Complexity:** High

- Latte art drawing
- Ingredient matching
- Rush hour rhythm game

### Story Mode
**Value:** Narrative engagement
**Complexity:** High

- Characters with storylines
- Dialogue system
- Story unlocks
- Multiple endings

### Multiplayer (Co-op)
**Value:** Social play
**Complexity:** Very High

- Manage cafe together
- Split roles
- Online or local

---

## Technical Debt to Address

### Before Major Features
- Performance profiling
- Memory usage optimization
- Save file migration system
- Analytics integration
- Crash reporting

### Code Quality
- Unit tests for systems
- Integration tests
- Automated builds
- Code documentation

---

## Feature Dependencies

```
MVP Complete
    │
    ├─► Staff Management
    │       └─► Staff Progression
    │
    ├─► Cafe Decoration
    │       └─► Furniture Effects
    │
    ├─► Customer Types
    │       ├─► Reputation System
    │       └─► Customer Memory
    │
    └─► More Content
            ├─► Seasonal Events
            └─► Multiple Locations
```

---

## User Feedback Priorities

Features to gauge interest through feedback:
1. What content do players want more of?
2. Is the game too easy/hard?
3. What features feel missing?
4. What's frustrating?

---

## Monetization Considerations (if applicable)

### Free-to-Play Options
- Cosmetic purchases (furniture skins)
- Time skip (avoid waiting)
- Ad removal

### Premium Options
- One-time purchase
- Expansion packs (new locations)
- Season passes

### What to Avoid
- Pay-to-win mechanics
- Predatory timers
- Required purchases to progress

---

## Version Roadmap

### v1.0 - MVP Release
- Core gameplay loop
- 10+ menu items
- Basic progression
- All 5 platforms

### v1.1 - Content Update
- More menu items
- Customer variety
- Bug fixes

### v1.2 - Staff Update
- Staff management
- Staff progression

### v1.3 - Decoration Update
- Furniture placement
- Decorative items

### v2.0 - Major Expansion
- Multiple locations
- Seasonal events
- Achievement system

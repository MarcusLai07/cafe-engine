# Changelog

Track what's been built, decisions made, and lessons learned.

## Documentation Created

**Date:** Initial Setup

### Completed
- [x] Project structure created
- [x] README.md - Project overview
- [x] ROADMAP.md - Full 8-phase development plan
- [x] Architecture documentation (9 documents)
- [x] Phase documentation (8 phases)
- [x] Game design documentation (5 documents)
- [x] Development guides (4 guides)

### Key Decisions Made

1. **Language:** C++ (for maximum learning and control)
2. **Graphics API:** Metal first, with abstraction for WebGL
3. **Platform priority:** Mac → Web → iOS → Windows → Android
4. **Art style:** Isometric pixel art
5. **Approach:** Write platform abstractions from scratch (bare metal learning)

### Documents Structure

```
docs/
├── README.md
├── ROADMAP.md
├── architecture/
│   ├── 00-overview.md
│   ├── 01-platform-layer.md
│   ├── 02-rendering-system.md
│   ├── 03-game-loop.md
│   ├── 04-entity-system.md
│   ├── 05-resource-management.md
│   ├── 06-audio-system.md
│   ├── 07-input-system.md
│   ├── 08-ui-system.md
│   └── 09-save-system.md
├── phases/
│   ├── phase-1-cpp-foundations/
│   ├── phase-2-platform-mac/
│   ├── phase-3-renderer-abstraction/
│   ├── phase-4-core-engine/
│   ├── phase-5-game-framework/
│   ├── phase-6-cafe-core-loop/
│   ├── phase-7-additional-platforms/
│   └── phase-8-content-release/
├── game-design/
│   ├── 00-game-overview.md
│   ├── 01-economy-system.md
│   ├── 02-customer-simulation.md
│   ├── 03-time-system.md
│   └── 04-future-features.md
├── guides/
│   ├── cpp-fundamentals.md
│   ├── macos-dev-setup.md
│   ├── emscripten-setup.md
│   └── debugging-guide.md
└── progress/
    └── changelog.md
```

---

## Development Log

### [Not Started] Phase 1: C++ Foundations

_This section will be updated as development progresses._

**Planned:**
- Development environment setup
- Memory management exercises
- CMake project setup
- Text-based cafe prototype

---

_Add entries as you complete milestones:_

```markdown
### [Date] Phase X.X: Task Name

**What was done:**
- Bullet points of work completed

**Decisions made:**
- Any architectural or design decisions

**Lessons learned:**
- What went well / what was challenging

**Next steps:**
- What to work on next
```

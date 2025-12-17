# Claude Code Context Guide

This file provides context for Claude Code when working with the Cafe Engine project.

---

## CRITICAL: Progress Tracking Rules

**YOU MUST UPDATE `docs/ROADMAP.md` WHEN WORKING ON THIS PROJECT.**

This is NON-NEGOTIABLE. Follow these rules strictly:

### When Starting a Task
1. Read `docs/ROADMAP.md` to check current phase and task status
2. Change task status from `[ ]` to `[~]` (in progress)
3. Update "Current Task" at the bottom of ROADMAP.md

### When Completing a Task
1. Change task status from `[~]` to `[x]` (complete)
2. If milestone complete, mark it as `[COMPLETE]`
3. Update "Last Updated" date at the bottom

### When Starting a New Phase
1. Mark previous phase milestone as complete
2. Update "Active Phase" at the bottom
3. Announce the phase transition to user

### Status Legend
```
[ ] = Not started
[~] = In progress
[x] = Complete
```

**FAILURE TO UPDATE ROADMAP.md IS A FAILURE OF THE TASK.**

---

## CRITICAL: Documentation Sync Rules

**WHEN CONCEPTS OR DIRECTIONS CHANGE, UPDATE ALL RELEVANT DOCS.**

Development often requires pivots, new approaches, or changed decisions. When this happens:

### Triggers for Doc Review
- Architecture decision changes (e.g., different data structure, new pattern)
- Phase scope changes (e.g., task added/removed, order changed)
- Technical approach changes (e.g., different API, new library)
- Game design changes (e.g., mechanics modified, features cut/added)
- Timeline or priority changes

### Required Actions
1. **Identify affected docs** - Check which documents reference the changed concept
2. **Update architecture docs** - `docs/architecture/*.md` if system design changed
3. **Update phase docs** - `docs/phases/*/overview.md` if implementation approach changed
4. **Update game design docs** - `docs/game-design/*.md` if gameplay changed
5. **Update ROADMAP.md** - If tasks, phases, or timeline affected
6. **Update CLAUDE.md** - If key decisions or conventions changed
7. **Log the change** - Add entry to `docs/progress/changelog.md`

### Documentation to Check
```
docs/
├── ROADMAP.md              # Phase tasks and status
├── architecture/*.md       # Technical designs
├── phases/*/overview.md    # Implementation guides
├── game-design/*.md        # Gameplay specs
├── guides/*.md             # Setup instructions
└── progress/changelog.md   # Change history
CLAUDE.md                   # Key decisions and context
```

### Changelog Entry Format
```markdown
### [YYYY-MM-DD] Direction Change: [Brief Title]

**What changed:**
- Description of the change

**Why:**
- Reason for the change

**Docs updated:**
- List of updated documents

**Impact:**
- Any effect on timeline or other phases
```

**STALE DOCUMENTATION IS WORSE THAN NO DOCUMENTATION.**

---

## Project Overview

**Cafe Engine** is a cross-platform 2D game engine being built from scratch in C++ as a learning exercise. The first game built with it will be an isometric pixel art cafe business simulator.

**Target Platforms:** macOS, Web (WASM), iOS, Windows, Android

**Timeline:** 18-24 months at 15-20 hours/week

**Current Phase:** Phase 2 - Platform Layer Mac (Phase 1 Complete)

## Key Decisions Already Made

These decisions were made during initial design. Do not suggest alternatives unless asked:

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Language | C++ | Maximum learning, industry standard |
| Graphics | Metal first, then WebGL | Modern API, good abstraction practice |
| Approach | Bare metal (write platform layer) | Deep learning goal |
| First Platform | macOS | Developer's machine |
| Second Platform | Web | Easy sharing/distribution |
| Art Style | Isometric pixel art | Achievable without art skills |
| Architecture | Simple Entity-Component | Sufficient for 2D cafe sim scale |

## Documentation Structure

Always check these docs before implementing:

```
docs/
├── ROADMAP.md              # Current phase and progress tracking
├── architecture/           # Technical system designs
│   ├── 00-overview.md      # Start here for architecture
│   └── [01-09]-*.md        # Specific systems
├── phases/                 # Implementation guides per phase
│   └── phase-N-*/overview.md
├── game-design/            # Gameplay specifications
└── guides/                 # Setup and reference guides
```

## When Asked to Implement Something

**BEFORE writing any code:**
1. Read `docs/ROADMAP.md` - check current phase and task status
2. Mark the task as `[~]` in progress in ROADMAP.md
3. Read relevant architecture doc in `docs/architecture/`
4. Follow the phase guide in `docs/phases/phase-N-*/overview.md`

**AFTER completing implementation:**
5. Mark the task as `[x]` complete in ROADMAP.md
6. Update "Last Updated" timestamp
7. If phase milestone complete, update "Active Phase"

**This tracking is MANDATORY - do not skip these steps.**

## Code Conventions

### File Organization
```
src/
├── platform/           # Platform abstraction (one folder per platform)
│   ├── platform.h      # Abstract interface
│   ├── macos/          # macOS implementation (.mm files)
│   └── web/            # Emscripten implementation
├── renderer/           # Graphics abstraction
│   ├── renderer.h      # Abstract interface
│   ├── metal/          # Metal backend
│   └── webgl/          # WebGL backend
├── engine/             # Core systems (entities, resources, etc.)
├── framework/          # Higher-level systems (UI, save, pathfinding)
└── core/               # Shared utilities (math, types, containers)

game/                   # Game-specific code (cafe sim logic)
assets/                 # Game assets (sprites, audio, data)
```

### Naming Conventions
- **Files:** `snake_case.cpp`, `snake_case.h`
- **Classes:** `PascalCase`
- **Functions:** `snake_case()` or `camelCase()` (be consistent within file)
- **Members:** `snake_case_` (trailing underscore)
- **Constants:** `SCREAMING_SNAKE_CASE`
- **Namespaces:** `cafe::`

### Code Style
```cpp
namespace cafe {

class ExampleClass {
private:
    int member_variable_;

public:
    void do_something(int parameter) {
        // Implementation
    }
};

} // namespace cafe
```

### Platform-Specific Code
```cpp
#ifdef PLATFORM_MACOS
    // macOS-specific code
#endif

#ifdef PLATFORM_WEB
    // Emscripten/WebGL code
#endif
```

## Building

### macOS
```bash
cmake -B build -G Ninja
cmake --build build
./build/cafe_engine
```

### Web (Emscripten)
```bash
emcmake cmake -B build-web
emmake make -C build-web
# Serve build-web/ with local web server
```

## Important Technical Context

### Memory Management
- Use `std::unique_ptr` for owned resources
- Use raw pointers only for non-owning references
- Follow RAII patterns for all resources

### Graphics Architecture
- Renderer interface abstracts Metal/WebGL differences
- Sprite batching minimizes draw calls
- Isometric depth sorting: higher Y draws first

### Game Loop
- Fixed timestep (60 updates/second) for game logic
- Variable timestep for rendering
- Interpolation for smooth visuals

### Platform Layer
- Abstract interface in `platform/platform.h`
- Each platform implements: window, input, time, file I/O
- Graphics context passed to renderer

## What NOT to Do

- Don't suggest Unity/Godot/other engines (learning from scratch is the goal)
- Don't over-engineer early phases (YAGNI - add complexity when needed)
- Don't skip phases (each builds on previous)
- Don't change architecture decisions without explicit discussion
- Don't add features not in current phase scope

## Helpful Commands

```bash
# Check project status
cat docs/ROADMAP.md

# Find architecture for a system
cat docs/architecture/00-overview.md

# Read current phase guide
cat docs/phases/phase-1-cpp-foundations/overview.md

# Check game design specs
ls docs/game-design/
```

## When User Asks About Progress

Reference `docs/ROADMAP.md` for:
- Current phase and status
- Completed vs pending tasks
- Next steps

## When User Seems Stuck

1. Check if they've completed prerequisites from earlier phases
2. Point to relevant guide in `docs/guides/`
3. Suggest smaller incremental steps
4. Offer to break down the problem

## Phase Quick Reference

| Phase | Focus | Key Deliverable |
|-------|-------|-----------------|
| 1 | C++ Fundamentals | Text-based cafe prototype |
| 2 | macOS Platform | Window + Metal + Input |
| 3 | Renderer Abstraction | Mac + Web running same code |
| 4 | Core Engine | Entities, Resources, Audio |
| 5 | Game Framework | UI, Save, Pathfinding |
| 6 | Cafe Game | Playable MVP |
| 7 | More Platforms | iOS, Windows, Android |
| 8 | Polish & Release | Shippable game |

## Project Location

```
/Users/ctg/Documents/GitHub/cafe-engine/
```

---

*Last Updated: 2025-12-17 - Added strict progress tracking and documentation sync rules. Phase 1 complete, Phase 2 in progress.*

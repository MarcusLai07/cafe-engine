# Cafe Engine Roadmap

This document tracks the development phases, milestones, and current progress of the Cafe Engine project.

## Progress Legend

- [ ] Not started
- [~] In progress
- [x] Completed

---

## Phase 1: C++ Foundations (Months 1-2)

**Goal**: Build solid C++ fundamentals before touching engine code.

**Deliverable**: A text-based cafe prototype demonstrating core C++ concepts.

| Task | Status | Description |
|------|--------|-------------|
| 1.1 Development environment setup | [x] | Xcode, CMake, compiler toolchain |
| 1.2 Memory management & RAII | [x] | Pointers, smart pointers, ownership |
| 1.3 Build systems (CMake) | [x] | Project configuration, dependencies |
| 1.4 Data structures & algorithms | [x] | Vectors, hashmaps, trees for games |
| 1.5 Modern C++ patterns | [x] | Move semantics, templates, lambdas |

**Milestone**: Text-based cafe game in terminal [COMPLETE]

[Detailed Phase 1 Documentation](./phases/phase-1-cpp-foundations/overview.md)

---

## Phase 2: Platform Layer - Mac (Months 3-5)

**Goal**: Create native macOS window with Metal rendering.

**Deliverable**: Native window displaying animated graphics with input handling.

| Task | Status | Description |
|------|--------|-------------|
| 2.1 Cocoa window creation | [x] | Objective-C++ interop, NSWindow |
| 2.2 Metal initialization | [x] | Device, command queue, render pipeline |
| 2.3 Basic rendering | [x] | Shaders, vertex buffers, draw a quad |
| 2.4 Input handling | [x] | Keyboard, mouse events via Cocoa |
| 2.5 Game loop | [x] | Fixed timestep update/render cycle |

**Milestone**: Window with spinning colored square, responds to keyboard [COMPLETE]

[Detailed Phase 2 Documentation](./phases/phase-2-platform-mac/overview.md)

---

## Phase 3: Renderer Abstraction (Months 6-8)

**Goal**: Abstract rendering so same code runs on Metal and WebGL.

**Deliverable**: Demo running on both Mac native and web browser.

| Task | Status | Description |
|------|--------|-------------|
| 3.1 Design renderer interface | [x] | Abstract GPU operations |
| 3.2 Metal backend | [x] | Implement interface for Metal |
| 3.3 Emscripten setup | [x] | WebAssembly toolchain configuration |
| 3.4 WebGL backend | [x] | Implement interface for WebGL |
| 3.5 Sprite & texture system | [x] | Load images, batch sprite rendering |
| 3.6 Isometric tile rendering | [x] | Tile maps, depth sorting |

**Milestone**: Isometric tile demo runs on Mac + Web [COMPLETE]

[Detailed Phase 3 Documentation](./phases/phase-3-renderer-abstraction/overview.md)

---

## Phase 4: Core Engine Systems (Months 9-11)

**Goal**: Build foundational engine systems needed for any game.

**Deliverable**: Engine can load a scene with sprites, audio, and entities.

| Task | Status | Description |
|------|--------|-------------|
| 4.1 Resource management | [ ] | Asset loading, caching, memory management |
| 4.2 Entity-Component system | [ ] | Flexible game object architecture |
| 4.3 Scene management | [ ] | Scene loading, transitions, lifecycle |
| 4.4 Audio system | [ ] | Sound playback abstraction (CoreAudio, Web Audio) |
| 4.5 Input mapping | [ ] | Action-based input, rebindable controls |

**Milestone**: Load scene file with positioned sprites and background music

[Detailed Phase 4 Documentation](./phases/phase-4-core-engine/overview.md)

---

## Phase 5: Game Framework (Months 12-14)

**Goal**: Build higher-level systems for game logic.

**Deliverable**: Interactive isometric world with UI and save/load.

| Task | Status | Description |
|------|--------|-------------|
| 5.1 UI system | [ ] | Menus, buttons, HUD elements |
| 5.2 Save/load system | [ ] | Serialization, game state persistence |
| 5.3 State machines | [ ] | Entity behavior, game states |
| 5.4 Pathfinding | [ ] | A* algorithm for NPC movement |
| 5.5 Camera system | [ ] | Pan, zoom, follow, screen shake |

**Milestone**: Navigable isometric world with working UI and saves

[Detailed Phase 5 Documentation](./phases/phase-5-game-framework/overview.md)

---

## Phase 6: Cafe Game - Core Loop (Months 15-17)

**Goal**: Implement the actual cafe simulation gameplay.

**Deliverable**: Playable cafe sim MVP with core mechanics.

| Task | Status | Description |
|------|--------|-------------|
| 6.1 Economy system | [ ] | Money, pricing, costs, profit tracking |
| 6.2 Time system | [ ] | Day/night cycle, business hours, speed control |
| 6.3 Customer simulation | [ ] | Spawn, enter, order, wait, leave behaviors |
| 6.4 Menu & ordering | [ ] | Food items, preparation, serving flow |
| 6.5 Progression & unlocks | [ ] | XP, levels, new items, upgrades |

**Milestone**: Playable cafe where customers come, order, pay, leave

[Detailed Phase 6 Documentation](./phases/phase-6-cafe-core-loop/overview.md)

---

## Phase 7: Additional Platforms (Months 18-20)

**Goal**: Extend engine to all target platforms.

**Deliverable**: Game runs on Mac, Web, iOS, Windows, Android.

| Task | Status | Description |
|------|--------|-------------|
| 7.1 iOS platform layer | [ ] | UIKit, Metal (shared with Mac), touch input |
| 7.2 Windows platform layer | [ ] | Win32, DirectX 11 or OpenGL |
| 7.3 Android platform layer | [ ] | NDK, OpenGL ES, JNI bridge |
| 7.4 Touch input abstraction | [ ] | Unified touch/mouse handling |
| 7.5 Platform builds | [ ] | CI/CD, platform-specific packaging |

**Milestone**: Same game binary/build runs on 5 platforms

[Detailed Phase 7 Documentation](./phases/phase-7-additional-platforms/overview.md)

---

## Phase 8: Content & Release (Months 21-24)

**Goal**: Complete the game with content and polish for release.

**Deliverable**: Shippable game on app stores and web.

| Task | Status | Description |
|------|--------|-------------|
| 8.1 Staff management | [ ] | Hire, assign, upgrade workers |
| 8.2 Building/decoration | [ ] | Place furniture, customize cafe |
| 8.3 Content creation | [ ] | Sprites, animations, sounds, music |
| 8.4 Balancing | [ ] | Economy tuning, progression pacing |
| 8.5 Release preparation | [ ] | App store assets, builds, submission |

**Milestone**: Game released on App Store, Play Store, Web, Steam

[Detailed Phase 8 Documentation](./phases/phase-8-content-release/overview.md)

---

## Summary Timeline

```
Month:  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24
        |-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|
Phase:  [ P1  ][   P2    ][   P3    ][   P4    ][   P5    ][   P6    ][  P7   ][ P8  ]
        C++    Mac/Metal  Renderer   Engine     Framework  Cafe Game  Platforms Release
```

## Current Focus

**Active Phase**: Phase 3 - COMPLETE

**Next Phase**: Phase 4 - Core Engine Systems

**Last Updated**: 2025-12-17 - Phase 3 complete! Added: WebGL renderer, web platform layer, image loading with stb_image, sprite sheets with animation, isometric tile system with depth sorting. Ready for web testing with Emscripten.

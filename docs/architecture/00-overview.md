# Engine Architecture Overview

This document describes the high-level architecture of Cafe Engine.

## Design Principles

1. **Platform abstraction** - Core engine code never touches platform APIs directly
2. **Data-driven** - Game content defined in data files, not hardcoded
3. **Minimal dependencies** - Build everything we can, minimize third-party code
4. **Clear ownership** - Every resource has one owner responsible for its lifetime

## Layer Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         GAME LAYER                              │
│  Cafe Simulator: Economy, Customers, Time, Progression          │
├─────────────────────────────────────────────────────────────────┤
│                      FRAMEWORK LAYER                            │
│  UI System | Save/Load | State Machines | Pathfinding | Camera  │
├─────────────────────────────────────────────────────────────────┤
│                        ENGINE LAYER                             │
│  Entities | Resources | Scenes | Audio | Input Mapping          │
├─────────────────────────────────────────────────────────────────┤
│                      RENDERER LAYER                             │
│  Sprites | Tiles | Text | Batching | Shaders                    │
├─────────────────────────────────────────────────────────────────┤
│                      PLATFORM LAYER                             │
│  Window | Graphics API | Input | Audio | File I/O | Time        │
├────────────────┬────────────────┬───────────────────────────────┤
│     macOS      │      Web       │    iOS    │ Windows │ Android │
│  Cocoa/Metal   │ Emscripten/    │   UIKit/  │ Win32/  │  NDK/   │
│                │    WebGL       │   Metal   │  D3D11  │ OpenGL  │
└────────────────┴────────────────┴───────────┴─────────┴─────────┘
```

## Code Organization

```
cafe-engine/
├── src/
│   ├── platform/           # Platform abstraction layer
│   │   ├── platform.h      # Abstract interface
│   │   ├── macos/          # macOS implementation
│   │   ├── web/            # Emscripten/WebGL implementation
│   │   ├── ios/            # iOS implementation
│   │   ├── windows/        # Windows implementation
│   │   └── android/        # Android implementation
│   │
│   ├── renderer/           # Rendering abstraction
│   │   ├── renderer.h      # Abstract interface
│   │   ├── metal/          # Metal backend
│   │   ├── webgl/          # WebGL backend
│   │   ├── d3d11/          # DirectX 11 backend
│   │   └── opengl/         # OpenGL/ES backend
│   │
│   ├── engine/             # Core engine systems
│   │   ├── entity.h        # Entity-Component system
│   │   ├── resource.h      # Asset management
│   │   ├── scene.h         # Scene management
│   │   ├── audio.h         # Audio playback
│   │   └── input.h         # Input mapping
│   │
│   ├── framework/          # Game framework layer
│   │   ├── ui/             # UI system
│   │   ├── save.h          # Save/load system
│   │   ├── fsm.h           # Finite state machines
│   │   ├── pathfinding.h   # A* and navigation
│   │   └── camera.h        # Camera controls
│   │
│   └── core/               # Shared utilities
│       ├── types.h         # Common type definitions
│       ├── math.h          # Vector, matrix math
│       ├── memory.h        # Memory utilities
│       └── containers.h    # Custom containers
│
├── game/                   # Cafe game specific code
│   ├── economy/            # Money, pricing, costs
│   ├── customer/           # Customer behavior
│   ├── time/               # Day/night, scheduling
│   └── progression/        # Unlocks, upgrades
│
├── assets/                 # Game assets
│   ├── sprites/
│   ├── audio/
│   └── data/
│
└── tools/                  # Development tools
    ├── asset_pipeline/     # Asset processing
    └── editor/             # Level editor (future)
```

## System Communication

Systems communicate through:

1. **Direct calls** - For synchronous, immediate operations
2. **Event queue** - For decoupled, asynchronous notifications
3. **Shared state** - For data that multiple systems read (e.g., game time)

```
┌──────────┐     events      ┌──────────┐
│ Customer │ ───────────────▶│ Economy  │
│  System  │                 │  System  │
└──────────┘                 └──────────┘
      │                            │
      │        ┌──────────┐        │
      └───────▶│  Event   │◀───────┘
               │  Queue   │
               └──────────┘
                    │
      ┌─────────────┼─────────────┐
      ▼             ▼             ▼
┌──────────┐  ┌──────────┐  ┌──────────┐
│   UI     │  │  Audio   │  │   Save   │
│  System  │  │  System  │  │  System  │
└──────────┘  └──────────┘  └──────────┘
```

## Memory Model

- **Stack allocation** preferred for temporary/local data
- **Arena allocators** for per-frame temporary allocations
- **Pool allocators** for frequently created/destroyed objects (entities, particles)
- **RAII** for resource management (textures, buffers, file handles)

## Threading Model (Future)

Initial implementation is single-threaded. Future phases may add:
- Render thread separate from game logic
- Asset loading on background thread
- Audio mixing on dedicated thread

## Related Documents

- [Platform Layer](./01-platform-layer.md)
- [Rendering System](./02-rendering-system.md)
- [Game Loop](./03-game-loop.md)
- [Entity System](./04-entity-system.md)
- [Resource Management](./05-resource-management.md)

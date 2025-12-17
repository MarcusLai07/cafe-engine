# Cafe Engine

A cross-platform 2D game engine built from scratch in C++, designed as a learning exercise in low-level game development. The engine targets Mac, Windows, iOS, Android, and Web (WebAssembly).

## First Game: Cafe Simulator

An isometric pixel art business simulation game where players manage a cafe, serve customers, and build a thriving business through progression and unlocks.

## Project Goals

1. **Deep learning** - Understand every layer from platform APIs to game logic
2. **Cross-platform** - Single codebase, multiple platform backends
3. **Reusable foundation** - Engine architecture extensible for future games
4. **Ship something real** - Not just a tech demo, an actual playable game

## Tech Stack

- **Language**: C++17/20
- **Graphics**: Metal (Mac/iOS), WebGL (Web), DirectX/OpenGL (Windows), OpenGL ES (Android)
- **Build**: CMake + platform-specific toolchains
- **Platforms**: macOS, iOS, Windows, Android, Web (WASM)

## Documentation

| Document | Description |
|----------|-------------|
| [ROADMAP.md](./ROADMAP.md) | Development phases and current progress |
| [Architecture](./architecture/) | Engine system designs |
| [Game Design](./game-design/) | Cafe simulator specifications |
| [Phases](./phases/) | Detailed phase breakdowns |
| [Guides](./guides/) | Setup and development guides |

## Current Status

**Phase 1: C++ Foundations** - Not started

See [ROADMAP.md](./ROADMAP.md) for detailed progress tracking.

## Timeline

- **Estimated duration**: 18-24 months
- **Time commitment**: 15-20 hours/week
- **Start date**: TBD

## Platform Priority

1. **macOS** (primary development platform)
2. **Web** (easy sharing/distribution)
3. **iOS** (mobile, shares Metal with Mac)
4. **Windows** (largest gaming platform)
5. **Android** (mobile reach)

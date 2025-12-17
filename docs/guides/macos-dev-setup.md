# macOS Development Setup

Complete guide to setting up your development environment for Cafe Engine on macOS.

## Prerequisites

- macOS 12 (Monterey) or later
- Admin access to install software
- ~10GB free disk space

## 1. Install Xcode

Xcode provides the compiler, Metal framework, and iOS development tools.

### Option A: Full Xcode (Recommended)
```bash
# Install from App Store
# Or download from https://developer.apple.com/xcode/

# After installation, accept license:
sudo xcodebuild -license accept

# Install additional components:
xcode-select --install
```

### Option B: Command Line Tools Only
```bash
xcode-select --install
```

**Note:** Full Xcode required for iOS development.

### Verify Installation
```bash
clang --version
# Should show Apple clang version 14+

xcrun --sdk macosx --show-sdk-path
# Should show SDK path
```

## 2. Install Homebrew

Package manager for development tools.

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Add to PATH (for Apple Silicon Macs)
echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
eval "$(/opt/homebrew/bin/brew shellenv)"

# Verify
brew --version
```

## 3. Install CMake

Build system generator.

```bash
brew install cmake

# Verify
cmake --version
# Should be 3.20+
```

## 4. Install Code Editor

### Option A: Visual Studio Code (Recommended)
```bash
brew install --cask visual-studio-code

# Install C++ extensions:
# - C/C++ (Microsoft)
# - CMake Tools
# - clangd
```

**VS Code settings for C++ (`.vscode/settings.json`):**
```json
{
    "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json",
    "C_Cpp.default.cppStandard": "c++20",
    "cmake.buildDirectory": "${workspaceFolder}/build"
}
```

### Option B: CLion
```bash
brew install --cask clion
```

Professional IDE with excellent CMake integration. Free for students.

### Option C: Xcode
Already installed. Good for debugging, less ideal for CMake projects.

## 5. Additional Tools

```bash
# Git (usually pre-installed)
git --version

# Ninja (faster builds)
brew install ninja

# ccache (faster recompilation)
brew install ccache
```

## 6. Create Project Directory

```bash
cd ~/Documents
mkdir -p cafe-engine
cd cafe-engine

# Initialize git
git init

# Create basic structure
mkdir -p src include assets docs build
```

## 7. Create CMakeLists.txt

```bash
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.20)
project(CafeEngine VERSION 0.1.0 LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Source files
file(GLOB_RECURSE SOURCES
    src/*.cpp
    src/*.mm
)

add_executable(cafe_engine ${SOURCES})

target_include_directories(cafe_engine PRIVATE include)

# macOS frameworks
target_link_libraries(cafe_engine PRIVATE
    "-framework Cocoa"
    "-framework Metal"
    "-framework MetalKit"
    "-framework QuartzCore"
)

# Compiler warnings
target_compile_options(cafe_engine PRIVATE
    -Wall -Wextra -Wpedantic
)
EOF
```

## 8. Create Hello World

```bash
cat > src/main.mm << 'EOF'
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow* window;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    NSRect frame = NSMakeRect(0, 0, 1280, 720);

    self.window = [[NSWindow alloc]
        initWithContentRect:frame
        styleMask:NSWindowStyleMaskTitled |
                  NSWindowStyleMaskClosable |
                  NSWindowStyleMaskResizable
        backing:NSBackingStoreBuffered
        defer:NO];

    [self.window setTitle:@"Cafe Engine"];
    [self.window center];
    [self.window makeKeyAndOrderFront:nil];

    // Check Metal support
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device) {
        NSLog(@"Metal device: %@", device.name);
    } else {
        NSLog(@"Metal not supported!");
    }
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    return YES;
}

@end

int main(int argc, const char* argv[]) {
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        AppDelegate* delegate = [[AppDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
}
EOF
```

## 9. Build and Run

```bash
cd ~/Documents/cafe-engine

# Configure (first time)
cmake -B build -G Ninja

# Build
cmake --build build

# Run
./build/cafe_engine
```

You should see a window titled "Cafe Engine" with Metal device logged.

## 10. Development Workflow

### Daily Commands
```bash
# Build
cmake --build build

# Clean build
rm -rf build && cmake -B build -G Ninja && cmake --build build

# Run
./build/cafe_engine
```

### VS Code Tasks (`.vscode/tasks.json`)
```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build",
            "type": "shell",
            "command": "cmake --build build",
            "group": {"kind": "build", "isDefault": true},
            "problemMatcher": "$gcc"
        },
        {
            "label": "Run",
            "type": "shell",
            "command": "./build/cafe_engine",
            "dependsOn": "Build"
        }
    ]
}
```

## Troubleshooting

### "xcrun: error: invalid active developer path"
```bash
xcode-select --install
# Or
sudo xcode-select --reset
```

### CMake can't find compiler
```bash
sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
```

### Metal not available
- Make sure you have macOS 10.14+
- Check System Preferences → Displays → GPU settings

### Build errors with frameworks
```bash
# Verify SDK
xcrun --sdk macosx --show-sdk-path

# If wrong, set explicitly
export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
```

### Permission denied
```bash
chmod +x ./build/cafe_engine
```

## Recommended Aliases

Add to `~/.zshrc`:
```bash
alias cb='cmake --build build'
alias cr='cmake --build build && ./build/cafe_engine'
alias cc='rm -rf build && cmake -B build -G Ninja'
```

## Next Steps

1. Complete the Hello World and verify window appears
2. Proceed to [Phase 1: C++ Foundations](../phases/phase-1-cpp-foundations/overview.md)
3. When ready for Metal rendering, see [Phase 2: Platform Layer - Mac](../phases/phase-2-platform-mac/overview.md)

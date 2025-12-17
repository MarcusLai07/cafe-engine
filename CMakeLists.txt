cmake_minimum_required(VERSION 3.20)
project(CafeEngine VERSION 0.1.0 LANGUAGES CXX OBJCXX)

# C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Objective-C++ uses same standard
set(CMAKE_OBJCXX_STANDARD 20)
set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)

# macOS: Fix C++ standard library include path
if(APPLE)
    execute_process(
        COMMAND xcrun --show-sdk-path
        OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    include_directories(SYSTEM "${CMAKE_OSX_SYSROOT}/usr/include/c++/v1")
endif()

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Collect source files
set(CAFE_SOURCES
    src/main.cpp
    src/engine/game_loop.cpp
)

# Platform-specific sources
if(APPLE)
    list(APPEND CAFE_SOURCES
        src/platform/macos/macos_platform.mm
        src/renderer/metal/metal_renderer.mm
    )
endif()

# Main executable
add_executable(cafe_engine ${CAFE_SOURCES})

# Include directories
target_include_directories(cafe_engine PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Compiler warnings (strict for learning)
target_compile_options(cafe_engine PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

# macOS frameworks
if(APPLE)
    target_link_libraries(cafe_engine PRIVATE
        "-framework Cocoa"
        "-framework QuartzCore"
        "-framework Metal"
        "-framework MetalKit"
    )
endif()

# Debug/Release configurations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(cafe_engine PRIVATE DEBUG_BUILD)
endif()

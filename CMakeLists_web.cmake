# ============================================================================
# Cafe Engine - Web/Emscripten Build Configuration
# ============================================================================
#
# Usage:
#   emcmake cmake -B build-web -DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
#   cmake --build build-web
#
# Or use the helper script: ./scripts/build-web.sh
# ============================================================================

cmake_minimum_required(VERSION 3.20)
project(CafeEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Emscripten-specific settings
set(CMAKE_EXECUTABLE_SUFFIX ".html")

# Source files for web build
set(CAFE_WEB_SOURCES
    src/main.cpp
    src/engine/game_loop.cpp
    src/platform/web/web_platform.cpp
    src/renderer/webgl/webgl_renderer.cpp
)

add_executable(cafe_engine ${CAFE_WEB_SOURCES})

target_include_directories(cafe_engine PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Emscripten compile flags
target_compile_options(cafe_engine PRIVATE
    -Wall
    -Wextra
)

# Emscripten link flags
set_target_properties(cafe_engine PROPERTIES
    LINK_FLAGS "\
        -s USE_WEBGL2=1 \
        -s FULL_ES3=1 \
        -s ALLOW_MEMORY_GROWTH=1 \
        -s WASM=1 \
        -s NO_EXIT_RUNTIME=1 \
        --shell-file ${CMAKE_SOURCE_DIR}/web/shell.html \
    "
)

# Copy assets to build directory
# add_custom_command(TARGET cafe_engine POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#     ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:cafe_engine>/assets
# )

#!/bin/bash
# ============================================================================
# Cafe Engine - Web Build Script
# ============================================================================
# Requirements:
#   - Emscripten SDK installed and sourced (source emsdk_env.sh)
#
# Usage:
#   ./scripts/build-web.sh         # Build and serve
#   ./scripts/build-web.sh build   # Build only
#   ./scripts/build-web.sh serve   # Serve only (assumes already built)
#   ./scripts/build-web.sh clean   # Clean build directory
# ============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build-web"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${GREEN}[BUILD]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_emscripten() {
    if ! command -v emcc &> /dev/null; then
        print_error "Emscripten not found!"
        echo ""
        echo "Please install Emscripten SDK:"
        echo "  git clone https://github.com/emscripten-core/emsdk.git"
        echo "  cd emsdk"
        echo "  ./emsdk install latest"
        echo "  ./emsdk activate latest"
        echo "  source ./emsdk_env.sh"
        echo ""
        exit 1
    fi
    print_status "Emscripten found: $(emcc --version | head -n1)"
}

do_clean() {
    print_status "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    print_status "Clean complete."
}

do_build() {
    check_emscripten

    print_status "Configuring web build..."

    if [ ! -d "$BUILD_DIR" ]; then
        mkdir -p "$BUILD_DIR"
    fi

    cd "$BUILD_DIR"

    emcmake cmake "$PROJECT_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        -G "Unix Makefiles" \
        -C "$PROJECT_DIR/CMakeLists_web.cmake"

    print_status "Building..."
    emmake make -j$(sysctl -n hw.ncpu 2>/dev/null || nproc)

    print_status "Build complete!"
    echo ""
    echo "Output files in: $BUILD_DIR"
    ls -la "$BUILD_DIR"/*.html "$BUILD_DIR"/*.js "$BUILD_DIR"/*.wasm 2>/dev/null || true
}

do_serve() {
    if [ ! -f "$BUILD_DIR/cafe_engine.html" ]; then
        print_error "Build not found. Run './scripts/build-web.sh build' first."
        exit 1
    fi

    print_status "Starting local server..."
    echo ""
    echo "Open in browser: http://localhost:8080/cafe_engine.html"
    echo "Press Ctrl+C to stop"
    echo ""

    cd "$BUILD_DIR"

    # Try python3 first, then python
    if command -v python3 &> /dev/null; then
        python3 -m http.server 8080
    elif command -v python &> /dev/null; then
        python -m http.server 8080
    else
        print_error "Python not found. Install Python to serve files."
        exit 1
    fi
}

# Main
case "${1:-all}" in
    clean)
        do_clean
        ;;
    build)
        do_build
        ;;
    serve)
        do_serve
        ;;
    all|"")
        do_build
        do_serve
        ;;
    *)
        echo "Usage: $0 {build|serve|clean|all}"
        exit 1
        ;;
esac

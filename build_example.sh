#!/usr/bin/env bash
set -e

# Ensures ROOT_DIR is always the repository root regardless of where the script is invoked
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$SCRIPT_DIR"

EXAMPLE_DIR="$ROOT_DIR/example-app"
ENGINE_DIR="$ROOT_DIR/engine"
APP_NAME="${1:-app}"
ROOT_DIST_DIR="$ROOT_DIR/dist"

mkdir -p "$ROOT_DIST_DIR"

# 1. Compiles the Example App to VSI binary via CMake
echo "=== [1/3] Compiling Example App -> VSI ==="
cmake -B "$EXAMPLE_DIR/build" -S "$EXAMPLE_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$EXAMPLE_DIR/build" --parallel

# 2. Compiles Engine Library & Installs Toolchain into engine/dist/
echo "=== [2/3] Building Engine Library & Toolchain ==="
cmake -B "$ENGINE_DIR/build" -S "$ENGINE_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$ENGINE_DIR/build" --parallel
cmake --install "$ENGINE_DIR/build" --prefix "$ENGINE_DIR/dist"

# 3. Invokes compiled vsir driver to generate final executable in ./dist/app
echo "=== [3/3] Compiling App via vsir CLI driver ==="
(
    cd "$ENGINE_DIR/dist"
    export PATH="$PWD:$PATH"
    ./vsir "$EXAMPLE_DIR/build/app.vsi" -o "$ROOT_DIST_DIR/$APP_NAME" -v
)

echo ""
echo "========================================================="
echo " SUCCESS! Application generated: ./dist/${APP_NAME}"
echo "========================================================="

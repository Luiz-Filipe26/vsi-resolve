#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$SCRIPT_DIR"

EXAMPLE_DIR="$ROOT_DIR/example-app"
HOST_DIR="$ROOT_DIR/host"
VSIR_DIR="$ROOT_DIR/tools/vsir/common_desktop"
APP_NAME="${1:-app}"
ROOT_DIST_DIR="$ROOT_DIR/dist"

mkdir -p "$ROOT_DIST_DIR"

# 1. Compiles Example App -> VSI (WASM64)
echo "=== [1/4] Compiling Example App -> VSI ==="
cmake -B "$EXAMPLE_DIR/build" -S "$EXAMPLE_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$EXAMPLE_DIR/build" --parallel

# 2. Compiles Host Library
echo "=== [2/4] Building Host Library ==="
cmake -B "$HOST_DIR/build" -S "$HOST_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$HOST_DIR/build" --parallel
cmake --install "$HOST_DIR/build" --prefix "$ROOT_DIST_DIR/staging"

# 3. Compiles VSIR Toolchain
echo "=== [3/4] Building VSIR Toolchain ==="
cmake -B "$VSIR_DIR/build" -S "$VSIR_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$VSIR_DIR/build" --parallel
cmake --install "$VSIR_DIR/build" --prefix "$ROOT_DIST_DIR/staging"

# 4. Compiles App via vsir CLI driver
echo "=== [4/4] Compiling App via vsir CLI driver ==="
(
    cd "$ROOT_DIST_DIR/staging"
    export PATH="$PWD:$PATH"
    ./vsir "$EXAMPLE_DIR/build/app.vsi" -o "$ROOT_DIST_DIR/$APP_NAME" -v
)

echo ""
echo "========================================================="
echo " SUCCESS! Application generated: ./dist/${APP_NAME}"
echo "========================================================="

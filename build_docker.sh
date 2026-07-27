#!/usr/bin/env bash
set -e

# Ensure execution from repository root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

IMAGE_NAME="vsi-builder:local"
DIST_DIR="$SCRIPT_DIR/dist"
ARCHIVE_NAME="vsi-toolchain-linux-x86_64.tar.gz"

mkdir -p "$DIST_DIR" "engine/build/_deps"

USER_ID="${UID:-$(id -u 2>/dev/null || echo 1000)}"
GROUP_ID="$(id -g 2>/dev/null || echo 1000)"

echo "=== [1/2] Building Environment Docker Image ==="
docker build -t "$IMAGE_NAME" -f Dockerfile .

echo "=== [2/2] Compiling and Packaging Toolchain via Container ==="
docker run --rm \
    --user "$USER_ID:$GROUP_ID" \
    -v "$SCRIPT_DIR:/app" \
    -w /app \
    "$IMAGE_NAME" \
    bash -c "
        cmake -B engine/build -S engine -G Ninja -DCMAKE_BUILD_TYPE=Release && \
        cmake --build engine/build --parallel && \
        cmake --install engine/build --prefix engine/dist && \
        tar -czf /app/dist/$ARCHIVE_NAME -C /app/engine/dist .
    "

echo ""
echo "========================================================="
echo " SUCCESS! Hermetic toolchain release created:"
echo " ./dist/${ARCHIVE_NAME}"
echo "========================================================="

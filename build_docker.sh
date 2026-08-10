#!/usr/bin/env bash
set -e

# Ensure execution from repository root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

DIST_DIR="$SCRIPT_DIR/dist"
ARCHIVE_NAME="vsi-toolchain-linux-x86_64.tar.gz"

# Create build directories for decoupled projects
mkdir -p "$DIST_DIR" "host/build" "tools/vsir/common_desktop/build"

USER_ID="${UID:-$(id -u 2>/dev/null || echo 1000)}"
GROUP_ID="$(id -g 2>/dev/null || echo 1000)"

# 1. Build Host Static Library inside isolated container
echo "=== [1/3] Building Host Library Container ==="
docker build -t "vsi-host-builder:local" -f host/Dockerfile host/
docker run --rm \
    --user "$USER_ID:$GROUP_ID" \
    -v "$SCRIPT_DIR:/app" \
    -w /app/host \
    "vsi-host-builder:local" \
    bash -c "
        cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release && \
        cmake --build build --parallel && \
        cmake --install build --prefix dist
    "

# 2. Build VSIR Toolchain Driver inside isolated container
echo "=== [2/3] Building VSIR Toolchain Container ==="
docker build -t "vsir-builder:local" -f tools/vsir/common_desktop/Dockerfile tools/vsir/common_desktop/
docker run --rm \
    --user "$USER_ID:$GROUP_ID" \
    -v "$SCRIPT_DIR:/app" \
    -w /app/tools/vsir/common_desktop \
    "vsir-builder:local" \
    bash -c "
        cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release && \
        cmake --build build --parallel && \
        cmake --install build --prefix dist
    "

# 3. Merge isolated build artifacts and create hermetic release package
echo "=== [3/3] Packaging Release Bundle ==="
STAGING_DIR="$(mktemp -d)"
cp -r "$SCRIPT_DIR/host/dist/"* "$STAGING_DIR/"
cp -r "$SCRIPT_DIR/tools/vsir/common_desktop/dist/"* "$STAGING_DIR/"
tar -czf "$DIST_DIR/$ARCHIVE_NAME" -C "$STAGING_DIR" .
rm -rf "$STAGING_DIR"

echo ""
echo "========================================================="
echo " SUCCESS! Hermetic toolchain release created:"
echo " ./dist/${ARCHIVE_NAME}"
echo "========================================================="

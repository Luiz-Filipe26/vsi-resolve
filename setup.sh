#!/usr/bin/env bash
set -e

echo "=== [1/2] Generating Engine compilation database ==="
cmake -B engine/build -S engine -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "=== [2/2] Generating Example App compilation database ==="
cmake -B example-app/build -S example-app -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "Environment setup complete! You can now open your code editor."

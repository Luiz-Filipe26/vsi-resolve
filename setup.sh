#!/usr/bin/env bash
set -e

# Generate compilation databases (compile_commands.json) for language server (clangd)
echo "=== [1/3] Generating Host compilation database ==="
cmake -B host/build -S host -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "=== [2/3] Generating VSIR Toolchain compilation database ==="
cmake -B tools/vsir/common_desktop/build -S tools/vsir/common_desktop -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "=== [3/3] Generating Example App compilation database ==="
cmake -B example-app/build -S example-app -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "Environment setup complete! You can now open your code editor."

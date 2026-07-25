#!/usr/bin/env bash
set -e

# Configuração de Diretórios
APP_DIR="app"
ENGINE_DIR="engine"
SDK_INC_DIR="sdk/include"
BUILD_DIR="build"
BIN_NAME="app_nativo"

mkdir -p "$BUILD_DIR"

echo "========================================================="
echo " INICIANDO BUILD - VSI RUNTIME "
echo "========================================================="

# 1. Compila C++ do Desenvolvedor -> WASM
echo "[1/4] Compilando ${APP_DIR}/app.cpp -> WASM (${BUILD_DIR}/app.wasm)..."
clang --target=wasm64 -O2 -nostdlib \
    -I"$SDK_INC_DIR" \
    -Wl,--no-entry \
    -Wl,--export=app_main \
    -Wl,--allow-undefined \
    "$APP_DIR/app.cpp" -o "$BUILD_DIR/app.wasm"

# 2. Transpila WASM -> C via wasm2c
echo "[2/4] Transpilando WASM -> C via wasm2c..."
wasm2c "$BUILD_DIR/app.wasm" --enable-memory64 --module-name=app -o "$BUILD_DIR/app_transpiled.c"

# 3. Detecta Runtime do WABT
echo "[3/4] Detectando runtime do WABT..."
WASM_RT_DIR=""
if [ -f "/usr/share/wabt/wasm2c/wasm-rt-impl.c" ]; then
    WASM_RT_DIR="/usr/share/wabt/wasm2c"
elif [ -f "/usr/src/wasm2c/wasm-rt-impl.c" ]; then
    WASM_RT_DIR="/usr/src/wasm2c"
elif [ -f "/usr/share/wabt/wasm-rt-impl.c" ]; then
    WASM_RT_DIR="/usr/share/wabt"
elif [ -f "/usr/include/wasm-rt-impl.c" ]; then
    WASM_RT_DIR="/usr/include"
else
    echo "ERRO: wasm-rt-impl.c não encontrado no sistema!"
    exit 1
fi

MEM_IMPL=""
if [ -f "$WASM_RT_DIR/wasm-rt-mem-1-page.c" ]; then
    MEM_IMPL="$WASM_RT_DIR/wasm-rt-mem-1-page.c"
fi

# 4. Compilação dos Arquivos C++ da Engine
echo "[4/4] Compilando objetos intermediários..."

for src in "$ENGINE_DIR"/*.cpp; do
    filename=$(basename "$src" .cpp)
    echo "  -> Compilando $src..."
    clang++ -O3 -std=c++17 -DVSI_BUILDING_ENGINE=1 \
        -I"$WASM_RT_DIR" -I"$ENGINE_DIR" -I"$SDK_INC_DIR" -I"$BUILD_DIR" \
        -c "$src" -o "$BUILD_DIR/$filename.o"
done

# 5. Linkagem Final de tudo que estiver em build/*.o
echo "[LINK] Gerando executável ${BUILD_DIR}/${BIN_NAME}..."
clang++ -O3 "$BUILD_DIR"/*.o -o "$BUILD_DIR/$BIN_NAME" -lSDL2

echo ""
echo "========================================================="
echo " SUCESSO! Executável gerado: ./${BUILD_DIR}/${BIN_NAME}"
echo "========================================================="

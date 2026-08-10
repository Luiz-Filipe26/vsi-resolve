# Linux Platform Configuration

# _GNU_SOURCE is required by glibc to expose POSIX extensions
add_compile_definitions(_GNU_SOURCE)

# Detect processor architecture tag for Clang release asset
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
    set(VSI_ARCH_TAG "X64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64|ARM64)$")
    set(VSI_ARCH_TAG "ARM64")
else()
    message(FATAL_ERROR "Unsupported architecture for Linux: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

set(VSI_CLANG_ASSET "LLVM-${VSI_CLANG_VERSION}-Linux-${VSI_ARCH_TAG}.tar.xz")

# Setup patchelf and dependency staging for Linux hermetic runtime
function(vsi_setup_platform_hermeticism)
    find_program(PATCHELF_PATH patchelf)
    if(PATCHELF_PATH)
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/stage_hermetic.sh
"#!/usr/bin/env bash
set -e
mkdir -p \"${CMAKE_CURRENT_BINARY_DIR}/toolchain_deps\"
CLANG_BIN=\"${VSI_CLANG_BIN}\"
KNOWN_DEPS=\"${VSI_KNOWN_CLANG_DEPS}\"
DEPS_DIR=\"${CMAKE_CURRENT_BINARY_DIR}/toolchain_deps\"

DEPS=\$(ldd \"\$CLANG_BIN\" | awk '{print \$1}' | grep -v -E '^(linux-vdso|libc\\.so|libm\\.so|libpthread\\.so|libdl\\.so|librt\\.so|libgcc_s\\.so)|ld-linux')

for dep in \$DEPS; do
    if ! echo \"\$KNOWN_DEPS\" | grep -qw \"\$dep\"; then
        echo \"ERROR: unknown dynamic dependency detected: \$dep\" >&2
        exit 1
    fi
done

ldd \"\$CLANG_BIN\" | grep -E '(libtinfo|libzstd|libxml2|libstdc\\+\\+|libz)\\.so' | awk '{print \$3}' | xargs -I{} cp -n {} \"\$DEPS_DIR/\"
${PATCHELF_PATH} --set-rpath '\$ORIGIN/../lib' \"\$CLANG_BIN\"
"
        )
        add_custom_target(stage_clang_deps ALL
            COMMAND bash ${CMAKE_CURRENT_BINARY_DIR}/stage_hermetic.sh
            COMMENT "Staging hermetic Clang dynamic dependencies"
        )

        install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/toolchain_deps/ DESTINATION toolchain/lib)
    endif()
endfunction()

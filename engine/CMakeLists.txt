cmake_minimum_required(VERSION 3.24)

project(vsi_engine C CXX)

# Force generation of compile_commands.json for clangd/LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDK_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../sdk/include")
set(DIST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dist")
set(ENGINE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

set_property(DIRECTORY PROPERTY ADDITIONAL_CLEAN_FILES "${DIST_DIR}")

include(FetchContent)

# ==============================================================================
# 1. FETCH SDL3
# ==============================================================================
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.4.12
)
FetchContent_MakeAvailable(SDL3)

# ==============================================================================
# 2. FETCH WABT 1.0.41 (wasm2c)
# ==============================================================================
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_LIBWASM OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    wabt
    GIT_REPOSITORY https://github.com/WebAssembly/wabt.git
    GIT_TAG        1.0.41
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(wabt)

set(WASM_RT_DIR "${wabt_SOURCE_DIR}/wasm2c")
file(GLOB WASM_RT_SOURCES "${WASM_RT_DIR}/wasm-rt-*.c")

# ==============================================================================
# 3. FETCH HERMETIC LLVM / CLANG TOOLCHAIN
# ==============================================================================
set(VSI_CLANG_VERSION "22.1.8" CACHE STRING "Embedded LLVM/Clang version")
string(REGEX MATCH "^[0-9]+" VSI_CLANG_MAJOR "${VSI_CLANG_VERSION}")
set(VSI_KNOWN_CLANG_DEPS "libtinfo.so.6 libzstd.so.1 libxml2.so.2 libstdc++.so.6 libz.so.1" CACHE STRING
    "Known and accepted dynamic dependencies of the embedded Clang on Linux")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
    set(VSI_ARCH_TAG "X64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64|ARM64)$")
    set(VSI_ARCH_TAG "ARM64")
else()
    message(FATAL_ERROR "Unsupported architecture for embedded Clang toolchain: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(VSI_CLANG_ASSET "LLVM-${VSI_CLANG_VERSION}-Linux-${VSI_ARCH_TAG}.tar.xz")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(VSI_CLANG_ASSET "LLVM-${VSI_CLANG_VERSION}-macOS-${VSI_ARCH_TAG}.tar.xz")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(VSI_CLANG_ASSET "clang+llvm-${VSI_CLANG_VERSION}-x86_64-pc-windows-msvc.tar.xz")
endif()

message(STATUS "Fetching embedded LLVM/Clang toolchain (${VSI_CLANG_ASSET})...")
FetchContent_Declare(
    clang_release
    URL "https://github.com/llvm/llvm-project/releases/download/llvmorg-${VSI_CLANG_VERSION}/${VSI_CLANG_ASSET}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(clang_release)

find_program(ARCHIVER_PATH llvm-ar
    HINTS ${clang_release_SOURCE_DIR}/bin
    NO_DEFAULT_PATH
    REQUIRED
)
find_program(VSI_CLANG_BIN clang
    HINTS ${clang_release_SOURCE_DIR}/bin
    NO_DEFAULT_PATH
    REQUIRED
)

# ==============================================================================
# 4. INTERNAL ENGINE STATIC LIBRARY (vsi_internal)
# ==============================================================================
file(GLOB ENGINE_SOURCES "${ENGINE_SRC_DIR}/*.c")
list(REMOVE_ITEM ENGINE_SOURCES
    "${ENGINE_SRC_DIR}/vsi_shim.c"
    "${ENGINE_SRC_DIR}/vsir.c"
)

add_library(vsi_internal STATIC
    ${ENGINE_SOURCES}
    ${WASM_RT_SOURCES}
)

target_compile_definitions(vsi_internal PRIVATE
    WASM_RT_USE_MMAP=0
    VSI_BUILDING_ENGINE=1
)

target_include_directories(vsi_internal PUBLIC
    ${ENGINE_SRC_DIR}
    ${SDK_INC_DIR}
    ${WASM_RT_DIR}
)

target_link_libraries(vsi_internal PUBLIC SDL3::SDL3-static)

# ==============================================================================
# 5. VSIR CLI DRIVER TARGET
# ==============================================================================
add_executable(vsir "${ENGINE_SRC_DIR}/vsir.c")
target_include_directories(vsir PRIVATE ${SDK_INC_DIR})
set_target_properties(vsir PROPERTIES C_STANDARD 11)

# ==============================================================================
# 6. MONOLITHIC FAT ARCHIVE CREATION & TOOLCHAIN STAGING
# ==============================================================================
file(GENERATE
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/merge.mri
    CONTENT "CREATE ${CMAKE_CURRENT_BINARY_DIR}/libvsi.a
ADDLIB $<TARGET_FILE:vsi_internal>
ADDLIB $<TARGET_FILE:SDL3-static>
SAVE
END
"
)

# Linux hermeticity: strict validation and bundling of Clang shared library dependencies
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_program(PATCHELF_PATH patchelf)
    if(NOT PATCHELF_PATH)
        message(WARNING "patchelf not found. The embedded Clang will NOT be hermetic.")
    else()
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/stage_hermetic.sh
"#!/usr/bin/env bash
set -e
mkdir -p \"${CMAKE_CURRENT_BINARY_DIR}/toolchain_deps\"

CLANG_BIN=\"${VSI_CLANG_BIN}\"
KNOWN_DEPS=\"${VSI_KNOWN_CLANG_DEPS}\"
DEPS_DIR=\"${CMAKE_CURRENT_BINARY_DIR}/toolchain_deps\"

# Filter out core glibc/kernel libraries (including the dynamic loader /lib64/ld-linux...)
DEPS=\$(ldd \"\$CLANG_BIN\" | awk '{print \$1}' | grep -v -E '^(linux-vdso|libc\\.so|libm\\.so|libpthread\\.so|libdl\\.so|librt\\.so|libgcc_s\\.so)|ld-linux')

for dep in \$DEPS; do
    if ! echo \"\$KNOWN_DEPS\" | grep -qw \"\$dep\"; then
        echo \"ERROR: unknown dynamic dependency detected in embedded Clang: \$dep\" >&2
        echo \"Add it to VSI_KNOWN_CLANG_DEPS in CMakeLists.txt if it is expected, or investigate the issue.\" >&2
        exit 1
    fi
done

ldd \"\$CLANG_BIN\" | grep -E '(libtinfo|libzstd|libxml2|libstdc\\+\\+|libz)\\.so' | awk '{print \$3}' | xargs -I{} cp -n {} \"\$DEPS_DIR/\"
${PATCHELF_PATH} --set-rpath '\$ORIGIN/../lib' \"\$CLANG_BIN\"
"
        )

        set(LINUX_HERMETIC_COMMANDS
            COMMAND bash ${CMAKE_CURRENT_BINARY_DIR}/stage_hermetic.sh
        )
    endif()
endif()

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/run_merge.cmake
"execute_process(
    COMMAND \${ARCHIVER_PATH} -M
    INPUT_FILE \${MRI_SCRIPT}
    RESULT_VARIABLE MERGE_RESULT
)

if(NOT MERGE_RESULT EQUAL 0)
    message(FATAL_ERROR \"Failed to merge static libraries: exit code \${MERGE_RESULT}\")
endif()
"
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libvsi.a
    COMMAND ${CMAKE_COMMAND}
            -DARCHIVER_PATH=${ARCHIVER_PATH}
            -DMRI_SCRIPT=${CMAKE_CURRENT_BINARY_DIR}/merge.mri
            -P ${CMAKE_CURRENT_BINARY_DIR}/run_merge.cmake
    ${LINUX_HERMETIC_COMMANDS}
    DEPENDS vsi_internal SDL3-static ${CMAKE_CURRENT_BINARY_DIR}/merge.mri
    COMMENT "Merging static libraries and staging the hermetic Clang runtime"
)

add_custom_target(merge_libvsi ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libvsi.a)

# ==============================================================================
# 7. INSTALLATION / STAGING RULES
# ==============================================================================
install(TARGETS vsir wasm2c RUNTIME DESTINATION .)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libvsi.a DESTINATION .)
install(FILES ${ENGINE_SRC_DIR}/vsi_shim.c DESTINATION .)
install(DIRECTORY ${SDK_INC_DIR}/ DESTINATION include)
install(FILES ${WASM_RT_DIR}/wasm-rt.h ${WASM_RT_DIR}/wasm-rt-exceptions.h DESTINATION include)

# Install only the actual executable, the 'clang' symlink, and 'llvm-ar'
install(PROGRAMS
    "${clang_release_SOURCE_DIR}/bin/clang-${VSI_CLANG_MAJOR}"
    "${clang_release_SOURCE_DIR}/bin/llvm-ar"
    DESTINATION toolchain/bin
)

# Recreate the lightweight symlink 'clang -> clang-22' without duplicating the file on disk
install(CODE "
    execute_process(
        COMMAND \${CMAKE_COMMAND} -E create_symlink clang-${VSI_CLANG_MAJOR} \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/toolchain/bin/clang\"
    )
")

# Install the Resource Directory (intrinsic headers and runtime libraries)
install(DIRECTORY ${clang_release_SOURCE_DIR}/lib/clang
    DESTINATION toolchain/lib
)

# Install packaged Linux dynamic dependencies (libz, libstdc++, etc.)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND PATCHELF_PATH)
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/toolchain_deps/
        DESTINATION toolchain/lib
    )
endif()

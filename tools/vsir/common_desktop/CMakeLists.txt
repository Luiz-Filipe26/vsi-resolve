cmake_minimum_required(VERSION 3.24)

project(vsir_toolchain C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Default is ON (opt-out). Check environment variable for local overrides.
set(VSI_EMBEDDED_DEFAULT ON)
if(DEFINED ENV{VSI_USE_EMBEDDED_CLANG})
    if("$ENV{VSI_USE_EMBEDDED_CLANG}" MATCHES "^(0|OFF|off|FALSE|false)$")
        set(VSI_EMBEDDED_DEFAULT OFF)
    endif()
endif()

option(VSI_USE_EMBEDDED_CLANG "Fetch and stage hermetic LLVM/Clang toolchain" ${VSI_EMBEDDED_DEFAULT})

set(VSI_CLANG_VERSION "22.1.8" CACHE STRING "Embedded LLVM/Clang version")
string(REGEX MATCH "^[0-9]+" VSI_CLANG_MAJOR "${VSI_CLANG_VERSION}")
set(VSI_KNOWN_CLANG_DEPS "libtinfo.so.6 libzstd.so.1 libxml2.so.2 libstdc++.so.6 libz.so.1" CACHE STRING
    "Known dynamic dependencies of embedded Clang on Linux")

string(TOLOWER "${CMAKE_SYSTEM_NAME}" VSI_PLATFORM_NAME)
if(VSI_PLATFORM_NAME STREQUAL "darwin")
    set(VSI_PLATFORM_NAME "macos")
endif()
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/platform/${VSI_PLATFORM_NAME}.cmake")

include(FetchContent)

# 1. Fetch WABT (wasm2c)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_LIBWASM OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    wabt
    GIT_REPOSITORY https://github.com/WebAssembly/wabt.git
    GIT_TAG        1.0.41
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(wabt)

# 2. Configure LLVM / Clang (Opt-out logic)
if(VSI_USE_EMBEDDED_CLANG)
    message(STATUS "VSI: Staging hermetic LLVM/Clang bundle (Opt-out active)...")
    FetchContent_Declare(
        clang_release
        URL "https://github.com/llvm/llvm-project/releases/download/llvmorg-${VSI_CLANG_VERSION}/${VSI_CLANG_ASSET}"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(clang_release)

    find_program(VSI_CLANG_BIN clang HINTS ${clang_release_SOURCE_DIR}/bin NO_DEFAULT_PATH REQUIRED)
    find_program(VSI_AR_BIN llvm-ar HINTS ${clang_release_SOURCE_DIR}/bin NO_DEFAULT_PATH REQUIRED)
else()
    message(STATUS "VSI: Opt-out triggered. Using system LLVM/Clang for local dev.")
    find_program(VSI_CLANG_BIN clang REQUIRED)
    find_program(VSI_AR_BIN NAMES llvm-ar ar REQUIRED)
endif()

# 3. Build VSIR CLI driver
add_executable(vsir
    src/vsir.c
    "src/platform/${VSI_PLATFORM_NAME}.c"
)
target_include_directories(vsir PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 4. OS-specific hermetic staging (only executed if embedded Clang is enabled)
if(VSI_USE_EMBEDDED_CLANG)
    vsi_setup_platform_hermeticism()
endif()

# 5. Staging rules
install(TARGETS vsir wasm2c RUNTIME DESTINATION .)
install(FILES src/vsi_shim.c DESTINATION .)

if(VSI_USE_EMBEDDED_CLANG)
    install(PROGRAMS
        "${clang_release_SOURCE_DIR}/bin/clang-${VSI_CLANG_MAJOR}"
        "${clang_release_SOURCE_DIR}/bin/llvm-ar"
        DESTINATION toolchain/bin
    )
    install(CODE "
        execute_process(
            COMMAND \${CMAKE_COMMAND} -E create_symlink clang-${VSI_CLANG_MAJOR} \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/toolchain/bin/clang\"
        )
    ")
    install(DIRECTORY ${clang_release_SOURCE_DIR}/lib/clang DESTINATION toolchain/lib)
else()
    install(CODE "
        file(MAKE_DIRECTORY \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/toolchain/bin\")
        execute_process(COMMAND \${CMAKE_COMMAND} -E create_symlink \"${VSI_CLANG_BIN}\" \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/toolchain/bin/clang\")
        execute_process(COMMAND \${CMAKE_COMMAND} -E create_symlink \"${VSI_AR_BIN}\" \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/toolchain/bin/llvm-ar\")
    ")
endif()

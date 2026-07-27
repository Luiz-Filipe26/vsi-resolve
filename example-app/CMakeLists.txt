cmake_minimum_required(VERSION 3.20)

# -----------------------------------------------------------------------------
# Prefer Clang for WebAssembly builds if not explicitly overridden by the user.
# GCC does not support native '--target=wasm64'.
# -----------------------------------------------------------------------------
if(NOT DEFINED CMAKE_C_COMPILER)
    find_program(CLANG_C_BIN clang)
    if(CLANG_C_BIN)
        set(CMAKE_C_COMPILER "${CLANG_C_BIN}")
    endif()
endif()

if(NOT DEFINED CMAKE_CXX_COMPILER)
    find_program(CLANG_CPP_BIN clang++)
    if(CLANG_CPP_BIN)
        set(CMAKE_CXX_COMPILER "${CLANG_CPP_BIN}")
    endif()
endif()

project(example_app CXX C)

# Force generation of compile_commands.json for clangd/LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configure target architecture and optimization options
add_compile_options(
    --target=wasm64
    -O2
    -nostdlib
)

# Configure Linker flags for the VSI binary specification
add_link_options(
    --target=wasm64
    -nostdlib
    -Wl,--no-entry
    -Wl,--export=app_main
    -Wl,--allow-undefined
)

# Recursively find all C/C++ source files inside src/
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
)

if(NOT SOURCES)
    message(FATAL_ERROR "No C/C++ source files found inside src/")
endif()

# Define the binary artifact (app.vsi)
add_executable(app ${SOURCES})
set_target_properties(app PROPERTIES 
    OUTPUT_NAME "app.vsi"
    SUFFIX ""
)

# Include paths
target_include_directories(app PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/../sdk/include"
)

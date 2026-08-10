cmake_minimum_required(VERSION 3.24)

project(vsi_host C CXX)

# Force compilation database generation for clangd/LSP integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Load OS-specific host configuration
string(TOLOWER "${CMAKE_SYSTEM_NAME}" VSI_PLATFORM_NAME)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/platform/${VSI_PLATFORM_NAME}.cmake")

set(SDK_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../sdk/include")
set(DIST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dist")
set(HOST_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

set_property(DIRECTORY PROPERTY ADDITIONAL_CLEAN_FILES "${DIST_DIR}")

include(FetchContent)

# 1. Fetch SDL3 static library
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

# 2. Fetch WABT headers and sources for wasm2c runtime
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

find_program(ARCHIVER_PATH NAMES llvm-ar ar REQUIRED)

# 3. Build internal host static library
file(GLOB HOST_SOURCES "${HOST_SRC_DIR}/*.c")

add_library(vsi_internal STATIC
    ${HOST_SOURCES}
    ${WASM_RT_SOURCES}
)

target_compile_definitions(vsi_internal PRIVATE
    WASM_RT_USE_MMAP=0
    VSI_BUILDING_ENGINE=1
)

target_include_directories(vsi_internal PUBLIC
    ${HOST_SRC_DIR}
    ${SDK_INC_DIR}
    ${WASM_RT_DIR}
)

target_link_libraries(vsi_internal PUBLIC SDL3::SDL3-static)

# 4. Monolithic fat archive merge script generation (libvsi.a)
file(GENERATE
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/merge.mri
    CONTENT "CREATE ${CMAKE_CURRENT_BINARY_DIR}/libvsi.a
ADDLIB $<TARGET_FILE:vsi_internal>
ADDLIB $<TARGET_FILE:SDL3-static>
SAVE
END
"
)

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
    DEPENDS vsi_internal SDL3-static ${CMAKE_CURRENT_BINARY_DIR}/merge.mri
    COMMENT "Merging static host libraries into libvsi.a"
)

add_custom_target(merge_libvsi ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libvsi.a)

# 5. Host installation rules
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libvsi.a DESTINATION .)
install(FILES ${WASM_RT_DIR}/wasm-rt.h ${WASM_RT_DIR}/wasm-rt-exceptions.h DESTINATION include)

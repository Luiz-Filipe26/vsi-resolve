# macOS Platform Configuration

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
    set(VSI_ARCH_TAG "X64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64|ARM64)$")
    set(VSI_ARCH_TAG "ARM64")
else()
    message(FATAL_ERROR "Unsupported architecture for macOS: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

set(VSI_CLANG_ASSET "LLVM-${VSI_CLANG_VERSION}-macOS-${VSI_ARCH_TAG}.tar.xz")

function(vsi_setup_platform_hermeticism)
    # No dynamic ELF patching required on macOS
endfunction()

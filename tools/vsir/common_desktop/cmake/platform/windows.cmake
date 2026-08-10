# Windows Platform Configuration

add_compile_definitions(WIN32_LEAN_AND_MEAN)

set(VSI_CLANG_ASSET "clang+llvm-${VSI_CLANG_VERSION}-x86_64-pc-windows-msvc.tar.xz")

function(vsi_setup_platform_hermeticism)
    # No dynamic ELF patching required on Windows
endfunction()

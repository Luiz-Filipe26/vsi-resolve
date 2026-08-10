# Linux Host Platform Configuration

# _GNU_SOURCE is required by glibc to expose POSIX XSI extensions (sigaltstack, SA_ONSTACK, SIGSTKSZ)
add_compile_definitions(_GNU_SOURCE)

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#if defined(__linux__)
#include <unistd.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

#define VSIR_VERSION "0.1.0"
#define TEMP_C_FILE  "/tmp/app_transpiled.c"
#define TEMP_H_FILE  "/tmp/app_transpiled.h"

#if defined(_WIN32)
#define VSI_EXE_SUFFIX ".exe"
#else
#define VSI_EXE_SUFFIX ""
#endif

typedef struct {
    const char *input_vsi;
    const char *output_bin;
    bool verbose;
} cli_options_t;

static void get_own_dir(char *out, size_t out_size) {
    char path[PATH_MAX] = {0};

#if defined(__linux__)
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1) { out[0] = '\0'; return; }
    path[len] = '\0';
#elif defined(__APPLE__)
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) != 0) { out[0] = '\0'; return; }
#elif defined(_WIN32)
    if (GetModuleFileNameA(NULL, path, sizeof(path)) == 0) { out[0] = '\0'; return; }
#endif

    char *last_sep = strrchr(path, '/');
#if defined(_WIN32)
    char *last_sep_win = strrchr(path, '\\');
    if (last_sep_win > last_sep) last_sep = last_sep_win;
#endif
    if (last_sep) *last_sep = '\0';
    snprintf(out, out_size, "%s", path);
}

static void print_usage(const char *exec_name) {
    printf("VSI Runner & Compiler Driver v%s\n", VSIR_VERSION);
    printf("Usage: %s <input.vsi> [options]\n\n", exec_name);
    printf("Options:\n");
    printf("  -o <output>    Specify output binary name (default: 'app')\n");
    printf("  -v, --verbose  Print execution commands and retain temp C files\n");
    printf("  -h, --help     Show this help message\n");
}

static bool parse_cli_args(int argc, char **argv, cli_options_t *opts) {
    opts->input_vsi = NULL;
    opts->output_bin = "app";
    opts->verbose = false;

    if (argc < 2) {
        print_usage(argv[0]);
        return false;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            opts->verbose = true;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                opts->output_bin = argv[++i];
            } else {
                fprintf(stderr, "Error: '-o' option requires an output filename.\n");
                return false;
            }
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            return false;
        } else {
            if (opts->input_vsi != NULL) {
                fprintf(stderr, "Error: Multiple input files specified ('%s' and '%s')\n", opts->input_vsi, argv[i]);
                return false;
            }
            opts->input_vsi = argv[i];
        }
    }

    if (!opts->input_vsi) {
        fprintf(stderr, "Error: No input .vsi file provided.\n");
        return false;
    }

    return true;
}

static bool transpile_wasm_to_c(const cli_options_t *opts, const char *own_dir) {
    char command[4096];
    snprintf(command, sizeof(command),
             "\"%s/wasm2c" VSI_EXE_SUFFIX "\" \"%s\" --enable-memory64 --module-name=app -o \"%s\"",
             own_dir, opts->input_vsi, TEMP_C_FILE);

    if (opts->verbose) {
        printf("[VSIR] Transpiling WASM...\n  %s\n", command);
    }

    int status = system(command);
    if (status != 0) {
        fprintf(stderr, "\n[VSIR ERROR] Transpilation failed (wasm2c exited with code %d).\n", status);
        return false;
    }
    return true;
}

static bool compile_native_binary(const cli_options_t *opts, const char *own_dir) {
    char command[4096];
    snprintf(command, sizeof(command),
             "\"%s/toolchain/bin/clang" VSI_EXE_SUFFIX "\" -O3 \"%s\" \"%s/vsi_shim.c\" \"%s/libvsi.a\" -I\"%s/include\" -I/tmp -lm -lpthread -ldl -o \"%s\"",
             own_dir, TEMP_C_FILE, own_dir, own_dir, own_dir, opts->output_bin);

    if (opts->verbose) {
        printf("[VSIR] Compiling native binary...\n  %s\n", command);
    }

    int status = system(command);
    if (status != 0) {
        fprintf(stderr, "\n[VSIR ERROR] Native build failed (clang exited with code %d).\n", status);
        return false;
    }
    return true;
}

static void cleanup_temp_files(bool verbose) {
    if (!verbose) {
        remove(TEMP_C_FILE);
        remove(TEMP_H_FILE);
    }
}

int main(int argc, char **argv) {
    cli_options_t opts;
    if (!parse_cli_args(argc, argv, &opts)) {
        return 1;
    }

    char own_dir[PATH_MAX];
    get_own_dir(own_dir, sizeof(own_dir));

    if (!transpile_wasm_to_c(&opts, own_dir)) {
        return 1;
    }

    if (!compile_native_binary(&opts, own_dir)) {
        return 1;
    }

    cleanup_temp_files(opts.verbose);

    printf("Successfully built binary: %s\n", opts.output_bin);
    return 0;
}

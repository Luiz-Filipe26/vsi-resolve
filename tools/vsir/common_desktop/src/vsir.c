#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "platform/platform.h"

#define VSIR_VERSION "0.1.0"
#define PATH_MAX_BUF 4096

typedef struct {
    const char *input_vsi;
    const char *output_bin;
    bool verbose;
} cli_options_t;

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

    return opts->input_vsi != NULL;
}

static void log_cmd(char *const argv[]) {
    printf("[VSIR] Executing:");
    for (int i = 0; argv[i] != NULL; i++) {
        printf(" %s", argv[i]);
    }
    printf("\n");
}

static bool transpile_wasm_to_c(const cli_options_t *opts, const char *own_dir, const char *temp_dir, char *out_c_path) {
    const char *sep = platform_path_sep();
    const char *ext = platform_exe_suffix();

    char wasm2c_bin[PATH_MAX_BUF];
    snprintf(wasm2c_bin, sizeof(wasm2c_bin), "%s%swasm2c%s", own_dir, sep, ext);
    snprintf(out_c_path, PATH_MAX_BUF, "%s%sapp_transpiled.c", temp_dir, sep);

    char *argv[] = {
        wasm2c_bin,
        (char *)opts->input_vsi,
        "--enable-memory64",
        "--module-name=app",
        "-o",
        out_c_path,
        NULL
    };

    if (opts->verbose) log_cmd(argv);

    int exit_code = platform_run_cmd(argv);
    if (exit_code != 0) {
        fprintf(stderr, "\n[VSIR ERROR] Transpilation failed (wasm2c exited with code %d).\n", exit_code);
        return false;
    }
    return true;
}

static bool compile_native_binary(const cli_options_t *opts, const char *own_dir, const char *temp_dir, const char *c_file) {
    const char *sep = platform_path_sep();
    const char *ext = platform_exe_suffix();

    char clang_bin[PATH_MAX_BUF];
    char shim_c[PATH_MAX_BUF];
    char libvsi_a[PATH_MAX_BUF];
    char inc_temp_arg[PATH_MAX_BUF];
    char inc_own_arg[PATH_MAX_BUF];

    snprintf(clang_bin, sizeof(clang_bin), "%s%stoolchain%sbin%sclang%s", own_dir, sep, sep, sep, ext);
    snprintf(shim_c, sizeof(shim_c), "%s%svsi_shim.c", own_dir, sep);
    snprintf(libvsi_a, sizeof(libvsi_a), "%s%slibvsi.a", own_dir, sep);
    snprintf(inc_temp_arg, sizeof(inc_temp_arg), "-I%s", temp_dir);
    snprintf(inc_own_arg, sizeof(inc_own_arg), "-I%s%sinclude", own_dir, sep);

    platform_link_flags_t flags = platform_link_flags();

    char *argv[64];
    int argc = 0;

    argv[argc++] = clang_bin;
    argv[argc++] = "-O3";
    argv[argc++] = (char *)c_file;
    argv[argc++] = shim_c;
    argv[argc++] = libvsi_a;
    argv[argc++] = inc_temp_arg;
    argv[argc++] = inc_own_arg;

    for (int i = 0; i < flags.count; i++) {
        argv[argc++] = (char *)flags.items[i];
    }

    argv[argc++] = "-o";
    argv[argc++] = (char *)opts->output_bin;
    argv[argc] = NULL;

    if (opts->verbose) log_cmd(argv);

    int exit_code = platform_run_cmd(argv);
    if (exit_code != 0) {
        fprintf(stderr, "\n[VSIR ERROR] Native build failed (clang exited with code %d).\n", exit_code);
        return false;
    }
    return true;
}

static void cleanup_temp_files(const char *temp_dir, const char *c_file, bool verbose) {
    if (!verbose) {
        const char *sep = platform_path_sep();
        char h_file[PATH_MAX_BUF];
        snprintf(h_file, sizeof(h_file), "%s%sapp_transpiled.h", temp_dir, sep);
        
        remove(c_file);
        remove(h_file);
        rmdir(temp_dir);
    }
}

int main(int argc, char **argv) {
    cli_options_t opts;
    if (!parse_cli_args(argc, argv, &opts)) return 1;

    char own_dir[PATH_MAX_BUF];
    if (!platform_get_own_dir(own_dir, sizeof(own_dir))) {
        fprintf(stderr, "[VSIR ERROR] Failed to resolve executable location.\n");
        return 1;
    }

    char temp_dir[PATH_MAX_BUF];
    if (!platform_make_temp_dir(temp_dir, sizeof(temp_dir))) {
        fprintf(stderr, "[VSIR ERROR] Failed to create temporary directory.\n");
        return 1;
    }

    char c_file[PATH_MAX_BUF];
    if (!transpile_wasm_to_c(&opts, own_dir, temp_dir, c_file)) {
        cleanup_temp_files(temp_dir, c_file, opts.verbose);
        return 1;
    }

    if (!compile_native_binary(&opts, own_dir, temp_dir, c_file)) {
        cleanup_temp_files(temp_dir, c_file, opts.verbose);
        return 1;
    }

    cleanup_temp_files(temp_dir, c_file, opts.verbose);

    printf("Successfully built binary: %s\n", opts.output_bin);
    return 0;
}

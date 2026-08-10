#include "posix_common.h"
#include <limits.h>

bool platform_get_own_dir(char *out, size_t out_size) {
    char path[PATH_MAX] = {0};
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1) return false;
    
    char *last_sep = strrchr(path, '/');
    if (last_sep) *last_sep = '\0';
    snprintf(out, out_size, "%s", path);
    return true;
}

bool platform_make_temp_dir(char *out, size_t out_size) { return posix_make_temp_dir(out, out_size); }
int platform_run_cmd(char *const argv[]) { return posix_run_cmd(argv); }
const char *platform_exe_suffix(void) { return ""; }
const char *platform_path_sep(void) { return "/"; }

static const char *s_link_flags[] = {"-lm", "-lpthread", "-ldl"};
platform_link_flags_t platform_link_flags(void) {
    platform_link_flags_t flags = {
        .items = s_link_flags,
        .count = 3
    };
    return flags;
}

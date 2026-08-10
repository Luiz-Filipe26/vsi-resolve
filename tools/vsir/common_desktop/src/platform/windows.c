#include "platform.h"
#include <windows.h>
#include <process.h>
#include <direct.h>
#include <stdio.h>

bool platform_get_own_dir(char *out, size_t out_size) {
    char path[MAX_PATH] = {0};
    if (GetModuleFileNameA(NULL, path, sizeof(path)) == 0) return false;

    char *last_sep = strrchr(path, '\\');
    char *last_sep_slash = strrchr(path, '/');
    if (last_sep_slash > last_sep) last_sep = last_sep_slash;
    if (last_sep) *last_sep = '\0';
    snprintf(out, out_size, "%s", path);
    return true;
}

bool platform_make_temp_dir(char *out, size_t out_size) {
    char temp_base[MAX_PATH];
    DWORD len = GetTempPathA(sizeof(temp_base), temp_base);
    if (len == 0 || len > sizeof(temp_base)) return false;

    LARGE_INTEGER pc;
    QueryPerformanceCounter(&pc);
    unsigned long seed = (unsigned long)(pc.QuadPart ^ GetCurrentProcessId());

    for (int i = 0; i < 100; i++) {
        snprintf(out, out_size, "%svsir_%lu_%lx", temp_base, (unsigned long)GetCurrentProcessId(), seed + i);
        if (_mkdir(out) == 0) return true;
        if (GetLastError() != ERROR_ALREADY_EXISTS) return false;
    }
    return false;
}

int platform_run_cmd(char *const argv[]) {
    intptr_t status = _spawnv(_P_WAIT, argv[0], (const char * const *)argv);
    return (status == -1) ? -1 : (int)status;
}

const char *platform_exe_suffix(void) { return ".exe"; }
const char *platform_path_sep(void) { return "\\"; }

platform_link_flags_t platform_link_flags(void) {
    platform_link_flags_t flags = {
        .items = NULL,
        .count = 0
    };
    return flags;
}

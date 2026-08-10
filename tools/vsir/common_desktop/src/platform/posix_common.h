#ifndef VSI_POSIX_COMMON_H
#define VSI_POSIX_COMMON_H

#define _POSIX_C_SOURCE 200809L
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>

extern char **environ;

inline bool posix_make_temp_dir(char *out, size_t out_size) {
    snprintf(out, out_size, "/tmp/vsir_XXXXXX");
    return (mkdtemp(out) != NULL);
}

inline int posix_run_cmd(char *const argv[]) {
    pid_t pid;
    int status;
    int res = posix_spawn(&pid, argv[0], NULL, NULL, argv, environ);
    if (res != 0) return -1;
    if (waitpid(pid, &status, 0) == -1) return -1;
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

#endif

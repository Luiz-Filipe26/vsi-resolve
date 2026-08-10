#ifndef VSI_PLATFORM_H
#define VSI_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>

bool platform_get_own_dir(char *out, size_t out_size);
bool platform_make_temp_dir(char *out, size_t out_size);
int platform_run_cmd(char *const argv[]);
const char *platform_exe_suffix(void);
const char *platform_path_sep(void);

typedef struct {
    const char * const *items;
    int count;
} platform_link_flags_t;

platform_link_flags_t platform_link_flags(void);

#endif /* VSI_PLATFORM_H */

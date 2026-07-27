#include <stdlib.h>
#include <stdint.h>

/* When compiled via vsir, app_transpiled.h is injected into the build pipeline.
 * Otherwise, fallback to a stub for static analysis. */
#if __has_include("app_transpiled.h")
    #include "app_transpiled.h"
#else
    typedef struct w2c_app {
        struct {
            uint8_t* data;
        } w2c_memory;
    } w2c_app;
#endif

w2c_app* vsi_alloc_app(void) {
    return (w2c_app*)calloc(1, sizeof(w2c_app));
}

uint8_t* vsi_get_memory_base(w2c_app* app) {
    return app->w2c_memory.data;
}

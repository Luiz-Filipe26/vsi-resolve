#ifndef VSI_WASM_TYPES_H
#define VSI_WASM_TYPES_H

#include <stdint.h>

typedef uint32_t u32;
typedef uint64_t u64;

struct w2c_env {
    uint8_t* memory_base;
};

/* Opaque forward declaration: sizeof(w2c_app) is resolved at final build time
 * by vsi_shim.c */
typedef struct w2c_app w2c_app;

void wasm_rt_init(void);
void wasm_rt_free(void);
void wasm2c_app_instantiate(w2c_app*, struct w2c_env*);
void w2c_app_app_main(w2c_app*);
void wasm2c_app_free(w2c_app*);

#endif /* VSI_WASM_TYPES_H */

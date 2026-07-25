#ifndef VSI_WASM_TYPES_H
#define VSI_WASM_TYPES_H

#include <stdint.h>

/*
 * C++ compatibility fix for WABT's 'wasm-rt.h' header:
 * In C++, 'const' variables without an initializer produce a fatal error
 * (default_init_const).
 * We apply the '= {0}' initializer only while including the header and
 * immediately undefine the macro afterward to avoid polluting the rest
 * of the codebase.
 */
#ifdef __cplusplus
  #ifndef wasm_rt_funcref_null_value
    #define wasm_rt_funcref_null_value wasm_rt_funcref_null_value = {0}
    #define VSI_CLEANUP_FUNCREF_MACRO
  #endif
#endif

#include "wasm-rt.h"

#ifdef VSI_CLEANUP_FUNCREF_MACRO
  #undef wasm_rt_funcref_null_value
  #undef VSI_CLEANUP_FUNCREF_MACRO
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t u32;

struct w2c_env {
    uint8_t* memory_base;
};

#if __has_include("app_transpiled.h")
    #include "app_transpiled.h"
#elif defined(VSI_BUILDING_ENGINE)
    #error "FATAL: 'app_transpiled.h' was not found during the build! Run ./build.sh first."
#else
    /* Passive mocks EXCLUSIVELY for the LSP (clangd) to index before the first build */
    typedef struct { uint8_t* data; } w2c_memory_mock;
    typedef struct w2c_app { w2c_memory_mock w2c_memory; } w2c_app;

    void wasm_rt_init(void);
    void wasm_rt_free(void);
    void wasm2c_app_instantiate(w2c_app*, struct w2c_env*);
    void w2c_app_app_main(w2c_app*);
    void wasm2c_app_free(w2c_app*);
#endif

#ifdef __cplusplus
}
#endif

#endif /* VSI_WASM_TYPES_H */

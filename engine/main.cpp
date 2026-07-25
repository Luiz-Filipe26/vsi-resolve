#include "vsi_wasm_types.h"

int main() {
    wasm_rt_init();
    struct w2c_env env_instance;
    w2c_app app_instance;
    wasm2c_app_instantiate(&app_instance, &env_instance);
    env_instance.memory_base = app_instance.w2c_memory.data;
    w2c_app_app_main(&app_instance);
    wasm2c_app_free(&app_instance);
    wasm_rt_free();
    return 0;
}

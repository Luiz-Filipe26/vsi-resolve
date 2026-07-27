#include "vsi_wasm_types.h"

/* Forward declarations that will be resolved on .vsi instalation process */
extern w2c_app* vsi_alloc_app(void);
extern uint8_t* vsi_get_memory_base(w2c_app* app);

int main(void) {
    wasm_rt_init();
    struct w2c_env env_instance = {0};
    w2c_app* app_instance = vsi_alloc_app();
    wasm2c_app_instantiate(app_instance, &env_instance);
    env_instance.memory_base = vsi_get_memory_base(app_instance);
    w2c_app_app_main(app_instance);
    return 0;
}

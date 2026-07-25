#include "vsi/audio.h"
#include "vsi_wasm_types.h"

extern "C" {

uint32_t w2c_env_vsi_audio_init(struct w2c_env* instance, u32 rate, u32 ch) {
    return vsi_audio_init(rate, ch);
}

uint32_t w2c_env_vsi_audio_write(struct w2c_env* instance, u64 pcm_offset, u32 bytes) {
    if (!instance || !instance->memory_base) return 0;
    const void* real_ptr = (const void*)(instance->memory_base + pcm_offset);
    return vsi_audio_write(real_ptr, bytes);
}

uint32_t w2c_env_vsi_audio_pause(struct w2c_env* instance, u32 pause_on) {
    return vsi_audio_pause(pause_on);
}

uint32_t w2c_env_vsi_audio_status(struct w2c_env* instance) {
    return vsi_audio_status();
}

void w2c_env_vsi_audio_shutdown(struct w2c_env* instance) {
    vsi_audio_shutdown();
}

}

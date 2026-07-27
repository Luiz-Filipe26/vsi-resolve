#include "vsi/audio_out.h"
#include "vsi_wasm_types.h"

void w2c_env_vsi_audio_out_open(struct w2c_env* instance, u64 ret_ptr, u32 rate,
                                u32 ch, u32 fmt, u64 out_handle_offset) {
    if (!instance || !instance->memory_base || ret_ptr == 0) return;

    vsi_audio_out_result_t* ret_val =
        (vsi_audio_out_result_t*)(instance->memory_base + ret_ptr);

    if (out_handle_offset == 0) {
        ret_val->base = VSI_ERROR_INVALID_ARG;
        ret_val->code = VSI_AUDIO_OUT_ERR_NONE;
        return;
    }

    vsi_audio_out_handle_t* real_handle_ptr =
        (vsi_audio_out_handle_t*)(instance->memory_base + out_handle_offset);

    *ret_val = vsi_audio_out_open(rate, ch, (vsi_audio_out_format_t)fmt,
                                  real_handle_ptr);
}

void w2c_env_vsi_audio_out_write(struct w2c_env* instance, u64 ret_ptr,
                                 u64 handle_val, u64 pcm_data_offset,
                                 u32 bytes) {
    if (!instance || !instance->memory_base || ret_ptr == 0) return;

    vsi_audio_out_result_t* ret_val =
        (vsi_audio_out_result_t*)(instance->memory_base + ret_ptr);

    if (pcm_data_offset == 0) {
        ret_val->base = VSI_ERROR_INVALID_ARG;
        ret_val->code = VSI_AUDIO_OUT_ERR_NONE;
        return;
    }

    const void* real_pcm_ptr =
        (const void*)(instance->memory_base + pcm_data_offset);
    vsi_audio_out_handle_t handle =
        (vsi_audio_out_handle_t)(uintptr_t)handle_val;

    *ret_val = vsi_audio_out_write(handle, real_pcm_ptr, bytes);
}

void w2c_env_vsi_audio_out_pause(struct w2c_env* instance, u64 ret_ptr,
                                 u64 handle_val, u32 pause_on) {
    if (!instance || !instance->memory_base || ret_ptr == 0) return;

    vsi_audio_out_result_t* ret_val =
        (vsi_audio_out_result_t*)(instance->memory_base + ret_ptr);

    vsi_audio_out_handle_t handle =
        (vsi_audio_out_handle_t)(uintptr_t)handle_val;
    *ret_val = vsi_audio_out_pause(handle, pause_on);
}

void w2c_env_vsi_audio_out_get_queued_bytes(struct w2c_env* instance,
                                            u64 ret_ptr, u64 handle_val,
                                            u64 out_bytes_offset) {
    if (!instance || !instance->memory_base || ret_ptr == 0) return;

    vsi_audio_out_result_t* ret_val =
        (vsi_audio_out_result_t*)(instance->memory_base + ret_ptr);

    if (out_bytes_offset == 0) {
        ret_val->base = VSI_ERROR_INVALID_ARG;
        ret_val->code = VSI_AUDIO_OUT_ERR_NONE;
        return;
    }

    uint32_t* real_bytes_ptr =
        (uint32_t*)(instance->memory_base + out_bytes_offset);
    vsi_audio_out_handle_t handle =
        (vsi_audio_out_handle_t)(uintptr_t)handle_val;

    *ret_val = vsi_audio_out_get_queued_bytes(handle, real_bytes_ptr);
}

void w2c_env_vsi_audio_out_close(struct w2c_env* instance, u64 ret_ptr,
                                 u64 handle_val) {
    if (!instance || !instance->memory_base || ret_ptr == 0) return;

    vsi_audio_out_result_t* ret_val =
        (vsi_audio_out_result_t*)(instance->memory_base + ret_ptr);

    vsi_audio_out_handle_t handle =
        (vsi_audio_out_handle_t)(uintptr_t)handle_val;
    *ret_val = vsi_audio_out_close(handle);
}

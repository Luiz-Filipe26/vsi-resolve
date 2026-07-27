#ifndef VSI_AUDIO_OUT_H
#define VSI_AUDIO_OUT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vsi_audio_out_handle_T* vsi_audio_out_handle_t;
#define VSI_NULL_AUDIO_OUT_HANDLE ((vsi_audio_out_handle_t)0)

typedef enum {
    VSI_AUDIO_OUT_FMT_S16 = 1,
    VSI_AUDIO_OUT_FMT_F32 = 2
} vsi_audio_out_format_t;

typedef enum {
    VSI_AUDIO_OUT_ERR_NONE = 0,
    VSI_AUDIO_OUT_ERR_DEVICE_FAILURE = 1,
    VSI_AUDIO_OUT_ERR_FORMAT_UNSUPPORTED = 2,
} vsi_audio_out_error_t;

typedef struct {
    vsi_base_status_t base;
    vsi_audio_out_error_t code;
} vsi_audio_out_result_t;

vsi_audio_out_result_t vsi_audio_out_open(uint32_t sample_rate,
                                          uint32_t channels,
                                          vsi_audio_out_format_t format,
                                          vsi_audio_out_handle_t* out_handle);

vsi_audio_out_result_t vsi_audio_out_write(vsi_audio_out_handle_t handle,
                                           const void* pcm_data,
                                           uint32_t bytes);

vsi_audio_out_result_t vsi_audio_out_pause(vsi_audio_out_handle_t handle,
                                           uint32_t pause_on);

vsi_audio_out_result_t vsi_audio_out_get_queued_bytes(
    vsi_audio_out_handle_t handle, uint32_t* out_bytes);

vsi_audio_out_result_t vsi_audio_out_close(vsi_audio_out_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* VSI_AUDIO_OUT_H */

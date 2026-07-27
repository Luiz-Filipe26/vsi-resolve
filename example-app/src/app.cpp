#include "vsi/audio_out.h"
#include <stdint.h>

static int16_t g_pcm_buffer[44100 * 2];

extern "C" void app_main(void) {
    uint32_t sample_rate = 44100;
    uint32_t duration_sec = 2;
    uint32_t total_samples = sample_rate * duration_sec;
    
    int period = sample_rate / 440;
    for (uint32_t i = 0; i < total_samples; i++) {
        g_pcm_buffer[i] = ((i % period) < (period / 2)) ? 8000 : -8000;
    }

    vsi_audio_out_handle_t handle = VSI_NULL_AUDIO_OUT_HANDLE;
    vsi_audio_out_result_t res = vsi_audio_out_open(sample_rate, 
                                                     1, 
                                                     VSI_AUDIO_OUT_FMT_S16, 
                                                     &handle);

    if (res.base != VSI_OK) return;

    res = vsi_audio_out_write(handle, g_pcm_buffer, total_samples * sizeof(int16_t));
    if (res.base != VSI_OK) {
        vsi_audio_out_close(handle);
        return;
    }

    vsi_audio_out_pause(handle, 0);

    uint32_t queued = 0;
    do {
        res = vsi_audio_out_get_queued_bytes(handle, &queued);
    } while (res.base == VSI_OK && queued > 0);

    vsi_audio_out_close(handle);
}

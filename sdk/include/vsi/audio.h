#ifndef VSI_AUDIO_H
#define VSI_AUDIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t vsi_audio_init(uint32_t sample_rate, uint32_t channels);
uint32_t vsi_audio_write(const void* pcm_data, uint32_t bytes);
uint32_t vsi_audio_pause(uint32_t pause_on);
uint32_t vsi_audio_status(void);
void     vsi_audio_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif

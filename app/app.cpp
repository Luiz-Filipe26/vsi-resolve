#include "vsi/audio.h"
#include <stdint.h>

static int16_t g_pcm_buffer[44100 * 2];

extern "C" void app_main(void) {
    uint32_t sample_rate = 44100;
    uint32_t duration_sec = 2;
    uint32_t total_samples = sample_rate * duration_sec;
    
    // Generate a 440 Hz tone
    int period = sample_rate / 440;
    for (uint32_t i = 0; i < total_samples; i++) {
        g_pcm_buffer[i] = ((i % period) < (period / 2)) ? 8000 : -8000;
    }

    if (vsi_audio_init(sample_rate, 1) == 0) {
        vsi_audio_write(g_pcm_buffer, total_samples * sizeof(int16_t));
        vsi_audio_pause(0);
        
        while (vsi_audio_status() > 0) {
            // Wait for the queue to empty
        }
        
        vsi_audio_shutdown();
    }
}

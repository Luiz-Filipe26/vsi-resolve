#include <SDL2/SDL.h>
#include "vsi/audio.h"

static SDL_AudioDeviceID g_audio_device = 0;

extern "C" {

uint32_t vsi_audio_init(uint32_t sample_rate, uint32_t channels) {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) return 1;
    SDL_AudioSpec desired;
    SDL_zero(desired);
    desired.freq = sample_rate;
    desired.format = AUDIO_S16SYS;
    desired.channels = channels;
    desired.samples = 2048;
    g_audio_device = SDL_OpenAudioDevice(NULL, 0, &desired, NULL, 0);
    return (g_audio_device == 0) ? 1 : 0;
}

uint32_t vsi_audio_write(const void* pcm_data, uint32_t bytes) {
    if (g_audio_device == 0) return 1;
    SDL_QueueAudio(g_audio_device, pcm_data, bytes);
    return 0;
}

uint32_t vsi_audio_pause(uint32_t pause_on) {
    if (g_audio_device == 0) return 1;
    SDL_PauseAudioDevice(g_audio_device, pause_on ? 1 : 0);
    return 0;
}

uint32_t vsi_audio_status(void) {
    if (g_audio_device == 0) return 0;
    SDL_Delay(10);
    return SDL_GetQueuedAudioSize(g_audio_device);
}

void vsi_audio_shutdown(void) {
    if (g_audio_device != 0) SDL_CloseAudioDevice(g_audio_device);
    SDL_Quit();
}

}

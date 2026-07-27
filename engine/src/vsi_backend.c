#include <SDL3/SDL.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "vsi/audio_out.h"

typedef struct {
    SDL_AudioStream* stream;
    uint32_t generation;
    bool active;
    SDL_AudioSpec spec;
} AudioOutSlot;

static AudioOutSlot* g_slots = NULL;
static size_t g_slots_capacity = 0;
static size_t g_slots_count = 0;
static pthread_mutex_t g_audio_mutex = PTHREAD_MUTEX_INITIALIZER;

// Internal helper for consistent construction of vsi_audio_out_result_t
static inline vsi_audio_out_result_t make_res(
    vsi_base_status_t base,
    vsi_audio_out_error_t code) {
    vsi_audio_out_result_t res;
    res.base = base;
    res.code = code;
    return res;
}

// Validates the handle and retrieves the corresponding slot in O(1)
static AudioOutSlot* get_slot_locked(vsi_audio_out_handle_t handle) {
    uint64_t handle_val = (uint64_t)(uintptr_t)handle;
    if (handle_val == 0) return NULL;

    uint32_t slot_idx = (uint32_t)(handle_val & 0xFFFFFFFF) - 1;
    uint32_t generation = (uint32_t)(handle_val >> 32);

    if (slot_idx >= g_slots_count) return NULL;

    AudioOutSlot* slot = &g_slots[slot_idx];
    if (!slot->active || slot->generation != generation) {
        return NULL;  // Invalid or closed (stale) handle
    }

    return slot;
}

vsi_audio_out_result_t vsi_audio_out_open(uint32_t sample_rate,
                                           uint32_t channels,
                                           vsi_audio_out_format_t format,
                                           vsi_audio_out_handle_t* out_handle) {
    if (!out_handle || sample_rate == 0 || channels == 0) {
        return make_res(VSI_ERROR_INVALID_ARG, VSI_AUDIO_OUT_ERR_NONE);
    }

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        return make_res(VSI_MODULE_ERROR, VSI_AUDIO_OUT_ERR_DEVICE_FAILURE);
    }

    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.freq = (int)sample_rate;
    spec.channels = (int)channels;

    if (format == VSI_AUDIO_OUT_FMT_S16) {
        spec.format = SDL_AUDIO_S16;
    } else if (format == VSI_AUDIO_OUT_FMT_F32) {
        spec.format = SDL_AUDIO_F32;
    } else {
        return make_res(VSI_ERROR_INVALID_ARG,
                        VSI_AUDIO_OUT_ERR_FORMAT_UNSUPPORTED);
    }

    // Opens default playback device and binds an AudioStream in a single call
    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!stream) {
        return make_res(VSI_MODULE_ERROR, VSI_AUDIO_OUT_ERR_DEVICE_FAILURE);
    }

    // In SDL3, stream audio devices are paused by default upon creation
    if (!SDL_ResumeAudioStreamDevice(stream)) {
        SDL_DestroyAudioStream(stream);
        return make_res(VSI_MODULE_ERROR, VSI_AUDIO_OUT_ERR_DEVICE_FAILURE);
    }

    pthread_mutex_lock(&g_audio_mutex);

    // Finds an inactive slot for reuse or allocates a new one
    uint32_t slot_idx = UINT32_MAX;
    for (size_t i = 0; i < g_slots_count; ++i) {
        if (!g_slots[i].active) {
            slot_idx = (uint32_t)i;
            break;
        }
    }

    if (slot_idx == UINT32_MAX) {
        if (g_slots_count >= g_slots_capacity) {
            size_t new_cap = (g_slots_capacity == 0) ? 4 : g_slots_capacity * 2;
            AudioOutSlot* new_slots =
                (AudioOutSlot*)realloc(g_slots, new_cap * sizeof(AudioOutSlot));
            if (!new_slots) {
                pthread_mutex_unlock(&g_audio_mutex);
                SDL_DestroyAudioStream(stream);
                return make_res(VSI_ERROR_GENERIC, VSI_AUDIO_OUT_ERR_NONE);
            }
            g_slots = new_slots;
            g_slots_capacity = new_cap;
        }
        slot_idx = (uint32_t)g_slots_count;
        g_slots_count++;
        g_slots[slot_idx].generation = 1;
    }

    AudioOutSlot* slot = &g_slots[slot_idx];
    slot->stream = stream;
    slot->active = true;
    slot->spec = spec;

    // Packs [Generation (32 bits) | Index + 1 (32 bits)] inside the opaque handle pointer
    uint64_t handle_val =
        ((uint64_t)slot->generation << 32) | (uint64_t)(slot_idx + 1);
    *out_handle = (vsi_audio_out_handle_t)(uintptr_t)handle_val;

    pthread_mutex_unlock(&g_audio_mutex);

    return make_res(VSI_OK, VSI_AUDIO_OUT_ERR_NONE);
}

vsi_audio_out_result_t vsi_audio_out_write(vsi_audio_out_handle_t handle,
                                            const void* pcm_data,
                                            uint32_t bytes) {
    if (!pcm_data || bytes == 0) {
        return make_res(VSI_ERROR_INVALID_ARG, VSI_AUDIO_OUT_ERR_NONE);
    }

    pthread_mutex_lock(&g_audio_mutex);
    AudioOutSlot* slot = get_slot_locked(handle);
    if (!slot) {
        pthread_mutex_unlock(&g_audio_mutex);
        return make_res(VSI_ERROR_INVALID_HANDLE, VSI_AUDIO_OUT_ERR_NONE);
    }

    if (!SDL_PutAudioStreamData(slot->stream, pcm_data, (int)bytes)) {
        pthread_mutex_unlock(&g_audio_mutex);
        return make_res(VSI_MODULE_ERROR, VSI_AUDIO_OUT_ERR_DEVICE_FAILURE);
    }

    pthread_mutex_unlock(&g_audio_mutex);
    return make_res(VSI_OK, VSI_AUDIO_OUT_ERR_NONE);
}

vsi_audio_out_result_t vsi_audio_out_pause(vsi_audio_out_handle_t handle,
                                            uint32_t pause_on) {
    pthread_mutex_lock(&g_audio_mutex);
    AudioOutSlot* slot = get_slot_locked(handle);
    if (!slot) {
        pthread_mutex_unlock(&g_audio_mutex);
        return make_res(VSI_ERROR_INVALID_HANDLE, VSI_AUDIO_OUT_ERR_NONE);
    }

    bool ok = pause_on ? SDL_PauseAudioStreamDevice(slot->stream)
                       : SDL_ResumeAudioStreamDevice(slot->stream);

    pthread_mutex_unlock(&g_audio_mutex);

    if (!ok) {
        return make_res(VSI_MODULE_ERROR, VSI_AUDIO_OUT_ERR_DEVICE_FAILURE);
    }

    return make_res(VSI_OK, VSI_AUDIO_OUT_ERR_NONE);
}

vsi_audio_out_result_t vsi_audio_out_get_queued_bytes(
    vsi_audio_out_handle_t handle, uint32_t* out_bytes) {
    if (!out_bytes) {
        return make_res(VSI_ERROR_INVALID_ARG, VSI_AUDIO_OUT_ERR_NONE);
    }

    pthread_mutex_lock(&g_audio_mutex);
    AudioOutSlot* slot = get_slot_locked(handle);
    if (!slot) {
        pthread_mutex_unlock(&g_audio_mutex);
        return make_res(VSI_ERROR_INVALID_HANDLE, VSI_AUDIO_OUT_ERR_NONE);
    }

    int queued = SDL_GetAudioStreamQueued(slot->stream);
    if (queued < 0) {
        pthread_mutex_unlock(&g_audio_mutex);
        return make_res(VSI_MODULE_ERROR, VSI_AUDIO_OUT_ERR_DEVICE_FAILURE);
    }

    *out_bytes = (uint32_t)queued;
    pthread_mutex_unlock(&g_audio_mutex);
    return make_res(VSI_OK, VSI_AUDIO_OUT_ERR_NONE);
}

vsi_audio_out_result_t vsi_audio_out_close(vsi_audio_out_handle_t handle) {
    pthread_mutex_lock(&g_audio_mutex);
    AudioOutSlot* slot = get_slot_locked(handle);
    if (!slot) {
        pthread_mutex_unlock(&g_audio_mutex);
        return make_res(VSI_ERROR_INVALID_HANDLE, VSI_AUDIO_OUT_ERR_NONE);
    }

    if (slot->stream) {
        SDL_DestroyAudioStream(slot->stream);
        slot->stream = NULL;
    }

    slot->active = false;
    slot->generation++;  // Increments generation so stale handles are immediately rejected

    pthread_mutex_unlock(&g_audio_mutex);
    return make_res(VSI_OK, VSI_AUDIO_OUT_ERR_NONE);
}

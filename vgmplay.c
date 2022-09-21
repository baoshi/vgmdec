#include <stdio.h>
#include <memory.h>
#include <SDL.h>
#include "ansicon.h"
#include "vgm_conf.h"
#include "cached_file_reader.h"
#include "vgm.h"


#define SDL_BUFFER_SIZE 2048
#define READER_CACHE_SIZE 4096
#define SAMPLE_RATE 44100


unsigned long total_samples = 0;


void sdl_audio_callback(void* user, Uint8* stream, int len)
{
    static int16_t buffer[SDL_BUFFER_SIZE];
    unsigned int requested = (unsigned int)(len / 2); // len is in byte, each sample is 2 bytes
    if (requested > 0)
    {
        int samples = vgm_get_samples((vgm_t*)user, buffer, requested);
        if (samples == requested)   // all sampels are received
        {
            SDL_memcpy(stream, (void *)buffer, (size_t)len);
        }
        else if (samples > 0)     // not all samples are received
        {
            SDL_memset(stream, 0, (size_t)len);
            SDL_memcpy(stream, (void *)buffer, (size_t)(samples * 2));
        }
        else
        {
            SDL_memset(stream, 0, (size_t)len); // return silent
        }
        total_samples += samples;
    }
}


int main(int argc, char *argv[])
{
    file_reader_t *reader = 0;
    vgm_t *vgm = 0;
    SDL_AudioDeviceID audio_id = 0;
    SDL_Event event;
    int quit = 0;

    ansicon_setup();

    if (argc < 2)
    {
        VGM_PRINTERR("Usage: vgmdec input\n");
        return -1;
    }

    do
    {
        // Create reader
        reader = cfreader_create(argv[1], READER_CACHE_SIZE);
        if (!reader)
        {
            fprintf(stderr, "Unable to open %s\n", argv[1]);
            break;
        }
        // Create decoder
        vgm = vgm_create(reader);
        if (!vgm)
        {
            fprintf(stderr, "Error create vgm object\n");
            break;
        }
        if (SDL_Init(SDL_INIT_AUDIO) < 0)
        {
            fprintf(stderr, "SDL initialize error\n");
            break;
        }
        // SDL Audio
        SDL_AudioSpec want, have;
        SDL_zero(want);
        want.freq = SAMPLE_RATE;
        want.format = AUDIO_S16LSB;
        want.channels = 1;
        want.samples = SDL_BUFFER_SIZE;
        want.callback = sdl_audio_callback;
        want.userdata = (void*)vgm;
        audio_id = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        if (0 == audio_id)
        {
            fprintf(stderr, "Open audio device failed: %s\n", SDL_GetError());
            break;
        }
        // start play
        vgm_prepare_playback(vgm, SAMPLE_RATE, true);
        SDL_PauseAudioDevice(audio_id, 0);  // unpause
        // Event loop
        while (!quit)
        {
            SDL_Delay(100);
            if (27 == ansicon_getch_non_blocking()) // ESC
            {
                quit = true;
            }
        }
        cfreader_show_cache_status(reader);
    } while (0);
    if (audio_id != 0) SDL_CloseAudioDevice(audio_id);
    SDL_Quit();
    if (vgm != 0) vgm_destroy(vgm);
    if (reader != 0) cfreader_destroy(reader);
    
    ansicon_restore();
    return 0;
}

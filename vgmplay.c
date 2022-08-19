#include <stdio.h>
#include <memory.h>
#include <SDL.h>
#include "platform.h"
#include "file_reader_cached.h"
#include "vgm.h"

#define SDL_BUFFER_SIZE 1000
#define READER_CACHE_SIZE 4096
#define SAMPLE_RATE 44100


static int16_t buffer[SDL_BUFFER_SIZE];
static Uint32 buffer_ready_event_type;

void sdl_audio_callback(void* user, Uint8* stream, int len)
{
    unsigned int samples = (unsigned int)(len / 2); // len is in byte, each sample is 2 bytes
    if (samples > 0)
    {
        int r = vgm_get_samples((vgm_t*)user, buffer, samples);
        if (r == samples)   // all sampels are received
        {
            SDL_memcpy(stream, (void *)buffer, (size_t)len);
        }
        else if (r > 0)     // not all samples are received
        {
            SDL_memset(stream, 0, (size_t)len);
            SDL_memcpy(stream, (void *)buffer, (size_t)(r * 2));
        }
        else
        {
            SDL_memset(stream, 0, (size_t)len);
        }
        // Get a copy of data to main loop
        SDL_Event e;
        SDL_memset(&e, 0, sizeof(e));
        e.type = buffer_ready_event_type;
        e.user.code = 0;
        e.user.data1 = (void*)buffer;
        e.user.data2 = (void*)(intptr_t)r;
        SDL_PushEvent(&e);
    }
}


int main(int argc, char *argv[])
{
    file_reader_t *reader = 0;
    vgm_t *vgm = 0;
    SDL_AudioDeviceID audio_id = 0;
    SDL_Event event;
    int quit = 0;

    if (argc < 2)
    {
        VGM_PRINTERR("Usage: vgmdec input\n");
        return -1;
    }

    do
    {
        // Create reader
        reader = cfr_create(argv[1], READER_CACHE_SIZE);
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
        if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0)
        {
            fprintf(stderr, "SDL initialize error\n");
            break;
        }
        buffer_ready_event_type = SDL_RegisterEvents(1);
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
        // Window
        if (!SDL_CreateWindow("NSF Player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320, 200, 0))
        {
            fprintf(stderr, "Could not set video mode: %s\n", SDL_GetError());
            break;
        }
        // start play
        vgm_prepare_playback(vgm, SAMPLE_RATE, true);
        SDL_PauseAudioDevice(audio_id, 0);  // unpause
        // Event loop
        while (!quit)
        {
            /* Poll for events */
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                // Keyboard event
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym)
                    {
                    case SDLK_q:
                        quit = 1;
                        break;
                    }
                    break;
                // SDL_QUIT event (window close)
                case SDL_QUIT:
                    quit = 1;
                    break;
                default:
                    if (event.type == buffer_ready_event_type)
                    {
                        //int16_t* b = (int16_t*)event.user.data1;
                        int l = (int)(intptr_t)event.user.data2;
                        if (l < SDL_BUFFER_SIZE)
                        {
                            printf("Play finished.\n");
                            quit = 1;
                        }
                    }
                    break;
                }
            }
        }
        cfr_show_cache_status(reader);
    } while (0);
    
    if (audio_id != 0) SDL_CloseAudioDevice(audio_id);
    SDL_Quit();
    if (vgm != 0) vgm_destroy(vgm);
    if (reader != 0) cfr_destroy(reader);
    return 0;
}

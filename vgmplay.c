#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <SDL.h>
#include "vgm_conf.h"
#include "cached_file_reader.h"
#include "vgm.h"
#include "fft_q15.h"

#define SDL_BUFFER_SIZE 2048
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



/*
 * Given 44100Hz sample rate and 2048 data points, after FFT,
 * first 1024 data corresponds to 0-22050Hz.
 * We want first 128 points for spetrum, and we want 64 frequency bins,
 * each bin contains 2 points
  */
q15_t fftdata[SDL_BUFFER_SIZE];
#define SPECTRUM_HEIGHT 200
#define NUM_BINS 32
#define BIN_WIDTH 6
int16_t bin_max = 0;
static struct freq_bin_param_s
{
    int base_bin;
    q15_t scale;
} bin_param_q15[NUM_BINS] = 
{
    {2, 1154},
    {2, 9455},
    {2, 19789},
    {2, 32651},
    {3, 15893},
    {4, 3053},
    {4, 27858},
    {5, 25965},
    {6, 31629},
    {8, 13930},
    {10, 7937},
    {12, 16517},
    {15, 10467},
    {18, 26994},
    {23, 6087},
    {28, 20160},
    {35, 12238},
    {43, 25743},
    {54, 8403},
    {67, 9495},
    {83, 16800},
    {103, 23129},
    {128, 27551},
    {160, 4160},
    {199, 2285},
    {247, 17789},
    {307, 28794},
    {382, 32127},
    {476, 15126},
    {592, 26877},
    {737, 21462},
    {917, 30632}
};

void draw_spectrum(SDL_Renderer* renderer, int16_t* data, int len)
{
    if (len != SDL_BUFFER_SIZE)
        return;
    q15_t bin_data[NUM_BINS];
    int32_t temp;
    

    // Obtain data
    memcpy(fftdata, data, len * sizeof(int16_t));
    // FFT
    fft_q15(fftdata, len);
    // bin frequency
    for (int bin = 0; bin < NUM_BINS; ++bin)
    {
        temp = bin_param_q15[bin].scale * (fftdata[bin_param_q15[bin].base_bin] - fftdata[bin_param_q15[bin].base_bin - 1]);
        bin_data[bin] = fftdata[bin_param_q15[bin].base_bin - 1] + (temp >> 15);
        if (bin_data[bin] > bin_max) bin_max = bin_data[bin];
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 192, 255);
    for (int i = 0; i < NUM_BINS; ++i)
    {
        SDL_Rect rect;
        rect.x = i * BIN_WIDTH;
        rect.y = 210 - (int)bin_data[i] / 5;
        rect.w = BIN_WIDTH - 1;
        rect.h = (int)bin_data[i] / 5;
        SDL_RenderFillRect(renderer, &rect);
    }
    SDL_RenderPresent(renderer);
}


int main(int argc, char *argv[])
{
    file_reader_t *reader = 0;
    vgm_t *vgm = 0;
    SDL_AudioDeviceID audio_id = 0;
    SDL_Window *screen = NULL;
    SDL_Renderer *renderer = NULL;
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
        screen = SDL_CreateWindow("NSF Player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 240, 240, 0);
        if (!screen)
        {
            fprintf(stderr, "Could not set video mode: %s\n", SDL_GetError());
            break;
        }
        renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_SOFTWARE);
        if (!renderer)
        {
            fprintf(stderr, "Could not create renderer: %s\n", SDL_GetError());
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
                        int16_t* buf = (int16_t*)event.user.data1;
                        int l = (int)(intptr_t)event.user.data2;
                        if (l < SDL_BUFFER_SIZE)
                        {
                            printf("Play finished.\n");
                            quit = 1;
                        }
                        draw_spectrum(renderer, buf, l);
                    }
                    break;
                }
            }
        }
        cfreader_show_cache_status(reader);
    } while (0);
    if (screen) SDL_DestroyWindow(screen);
    if (audio_id != 0) SDL_CloseAudioDevice(audio_id);
    SDL_Quit();
    if (vgm != 0) vgm_destroy(vgm);
    if (reader != 0) cfreader_destroy(reader);
    printf("bin_max = %d\n", bin_max);
    return 0;
}

#include <stdio.h>
#include <memory.h>
#include <parg.h>
#include <SDL.h>
#include "ansicon.h"
#include "vgm_conf.h"
#include "cached_file_reader.h"
#include "vgm.h"


#define SDL_BUFFER_SIZE 2048
#define READER_CACHE_SIZE 4096
#define SAMPLE_RATE 44100


unsigned long played_samples = 0;


static void sdl_audio_callback(void* user, Uint8* stream, int len)
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
        played_samples += samples;
    }
}


static void usage()
{
    ansicon_puts(ANSI_GREEN, "Usage:\n");
    ansicon_puts(ANSI_GREEN, "vgmplay [-d] [-cChannels] file.vgm\n");
    ansicon_puts(ANSI_GREEN, "Options\n");
    ansicon_puts(ANSI_GREEN, "-d  Save output to .wav file\n");
    ansicon_puts(ANSI_GREEN, "-c  Enable selection of channels:\n");
    ansicon_puts(ANSI_GREEN, "    Channels for NESAPU: DNT21\n");
}


int main(int argc, char *argv[])
{
    file_reader_t *reader = 0;
    vgm_t *vgm = 0;
    SDL_AudioDeviceID audio_id = 0;
    unsigned long complete_samples;
    int quit = 0;

    ansicon_setup();
    ansicon_hide_cursor();

    do
    {
        const char *vgm_name = NULL;
        bool dump = false;
        const char *channels = "DNT21";

        // Parse command line options
        struct parg_state ps;
        int c;
        parg_init(&ps);
        while ((c = parg_getopt(&ps, argc, argv, "dc:h")) != -1)
        {
            switch (c)
            {
            case 1:
                vgm_name = ps.optarg;
                break;
            case 'h':
                break;
            case 'd':
                dump = true;
                break;
            case 'c':
                channels = ps.optarg;
                break;
            }
        }
        if ((NULL == vgm_name) || ('\0' == vgm_name[0]))
        {
            usage();
            break;
        }
              
        // Create reader
        reader = cfreader_create(vgm_name, READER_CACHE_SIZE);
        if (!reader)
        {
            fprintf(stderr, "Unable to open %s\n", vgm_name);
            break;
        }
        // Create decoder
        vgm = vgm_create(reader);
        if (!vgm)
        {
            fprintf(stderr, "Error create vgm object\n");
            break;
        }
        complete_samples = vgm->complete_samples;
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
        vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_ALL, false);
        //vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_PULSE1, true);
        //vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_PULSE2, true);
        //vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_TRIANGLE, true);
        vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_NOISE, true);
        vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_DMC, true);
        SDL_PauseAudioDevice(audio_id, 0);  // unpause
        // Event loop
        while (!quit)
        {
            SDL_Delay(100);
            if (27 == ansicon_getch_non_blocking()) // ESC
            {
                quit = true;
            }
            if (played_samples >= complete_samples)
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
    
    ansicon_show_cursor();
    ansicon_restore();
    return 0;
}

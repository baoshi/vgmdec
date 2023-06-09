#include <stdio.h>
#include <string.h>
#include <parg.h>
#include <cwalk.h>
#include <SDL.h>

#ifdef _MSC_VER
# include <direct.h>
# define getcwd _getcwd
# define strcasecmp _strcmpi
# define mkdir(d,m) _mkdir(d)
#else
# include <unistd.h>
# include <sys/stat.h>
#endif

#include "ansicon.h"
#include "vgm_conf.h"
#include "cached_file_reader.h"
#include "vgm.h"


#define SDL_BUFFER_SIZE 2048
#define READER_CACHE_SIZE 4096
#define SAMPLE_RATE 44100
#define MAX_PATH_NAME 256

typedef struct vgmplay_ctrl_s
{
    bool enable_apu_pulse1;
    bool enable_apu_pulse2;
    bool enable_apu_triangle;
    bool enable_apu_noise;
    bool enable_apu_dmc;
    unsigned long complete_samples;
} vgmplay_ctrl_t;



static void usage()
{
    ansicon_puts(ANSI_GREEN, "Usage:\n");
    ansicon_puts(ANSI_GREEN, "vgmplay [-d] [-cChannels] file.vgm\n");
    ansicon_puts(ANSI_GREEN, "Options\n");
    ansicon_puts(ANSI_GREEN, "-d  Save output to .wav file\n");
    ansicon_puts(ANSI_GREEN, "-c  Enable selection of channels:\n");
    ansicon_puts(ANSI_GREEN, "    Channels for NESAPU: DNT21\n");
}


static unsigned long played_samples = 0;

static void show_progress(vgmplay_ctrl_t *ctrl, bool newline)
{
    int save = 0;
    char progress[256];
    int percent;
    float t;
    progress[0] = '\0';
    strcat(progress, "Channels: ");
    if (ctrl->enable_apu_dmc)
        strcat(progress, "D");
    else
        strcat(progress, "-");
    if (ctrl->enable_apu_noise)
        strcat(progress, "N");
    else
        strcat(progress, "-");
    if (ctrl->enable_apu_triangle)
        strcat(progress, "T");
    else
        strcat(progress, "-");
    if (ctrl->enable_apu_pulse2)
        strcat(progress, "2");
    else
        strcat(progress, "-");
    if (ctrl->enable_apu_pulse1)
        strcat(progress, "1");
    else
        strcat(progress, "-");
    strcat(progress, "  Progress: ");
    
    strcat(progress, ANSI_YELLOW);
    percent = (int)(played_samples * 100.0f / ctrl->complete_samples);
    t = (float)played_samples / SAMPLE_RATE;
    save = (int)strlen(progress);
    snprintf(progress + save, 256 - save, "%d%% (%lu/%d:%02d.%03ds)", percent, played_samples, (int)t / 60, (int)t % 60, (int)((t - (int)t) * 1000));
    if (newline)
    {
        ansicon_puts(ANSI_YELLOW, progress);
        putchar('\n');
    }
    else
    {
        ansicon_set_string(ANSI_YELLOW, progress);
    }
}


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


static int play(vgm_t *vgm, file_reader_t *reader, vgmplay_ctrl_t *ctrl)
{
    int r = 0;
    int quit = 0;
    bool paused = false;

    SDL_AudioDeviceID audio_id = 0;
    do
    {
        if (SDL_Init(SDL_INIT_AUDIO) < 0)
        {
            r = -1;
            ansicon_puts(ANSI_RED, "SDL initialize error\n");
            break;
        }
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
            r = -1;
            ansicon_printf(ANSI_RED, "Open audio device failed: %s\n", SDL_GetError());
            break;
        }
        // start play
        vgm_prepare_playback(vgm, SAMPLE_RATE, true);
        vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_ALL, false);
        vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_PULSE1, ctrl->enable_apu_pulse1);
        vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_PULSE2, ctrl->enable_apu_pulse2);
        vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_TRIANGLE, ctrl->enable_apu_triangle);
        vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_NOISE, ctrl->enable_apu_noise);
        vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_DMC, ctrl->enable_apu_dmc);
        SDL_PauseAudioDevice(audio_id, 0);  // unpause
        // Play loop
        while (1)
        {
            SDL_Delay(100);
            if (played_samples >= ctrl->complete_samples)
            {
                break;
            }
            int ch = ansicon_getch_non_blocking();
            if (('q' == ch) || ('Q' == ch))
            {
                break;
            }
            else if ('1' == ch)
            {
                ctrl->enable_apu_pulse1 = !ctrl->enable_apu_pulse1;
                vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_PULSE1, ctrl->enable_apu_pulse1);
            }
            else if ('2' == ch)
            {
                ctrl->enable_apu_pulse2 = !ctrl->enable_apu_pulse2;
                vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_PULSE2, ctrl->enable_apu_pulse2);
            }
            else if (('t' == ch) || ('T' == ch))
            {
                ctrl->enable_apu_triangle = !ctrl->enable_apu_triangle;
                vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_TRIANGLE, ctrl->enable_apu_triangle);
            }
            else if (('n' == ch) || ('N') == ch)
            {
                ctrl->enable_apu_noise = !ctrl->enable_apu_noise;
                vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_NOISE, ctrl->enable_apu_noise);
            }
            else if (('d' == ch) || ('D') == ch)
            {
                ctrl->enable_apu_dmc = !ctrl->enable_apu_dmc;
                vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_DMC, ctrl->enable_apu_dmc);
            }
            else if (' ' == ch)
            {
                paused = !paused;
                SDL_PauseAudioDevice(audio_id, paused ? 1 : 0);
            }
            show_progress(ctrl, false);
        }
        show_progress(ctrl, true);
    } while (0);
    if (audio_id != 0) SDL_CloseAudioDevice(audio_id);
    SDL_Quit();
    return r;
}


/*
The header of a wav file Based on: https://docs.fileformat.com/audio/wav/
*/
typedef struct wavfile_header_s
{
    char    chunk_id[4];            // 1-4      "RIFF"
    int32_t chunk_size;             // 5-8      chunk size (header_size - 8 + data_size)
    char    format[4];              // 9-12     "WAVE"
    char    subchunk1_id[4];        // 13-16    format chunk marker "fmt "
    int32_t subchunk1_size;         // 17-20    chunk1 size (bytes 21-36, 16)
    int16_t audio_format;           // 21-22    type of format (PCM is 1)
    int16_t channels;               // 23-24    channels
    int32_t sample_rate;            // 25-28    sample rate
    int32_t byte_rate;              // 29-32    sample_rate * bits_per_sample * channels / 8
    int16_t block_align;            // 33-34    bits_per_sample * channels / 8
    int16_t bits_per_sample;        // 35-36    bits per sample
    char    subchunk2_id[4];        // 37-40    data chunk header mark
    int32_t subchunk2_size;         // 41-44    data chunck size
} wavfile_header_t;


static int dump(vgm_t *vgm, file_reader_t *reader, vgmplay_ctrl_t *ctrl, const char *out)
{
    int r = 0;
    FILE *fd = NULL;
    int16_t buffer[1024];
    wavfile_header_t header;
    int nsamples;
    do
    {
        fd = fopen(out, "wb");
        if (NULL == fd)
        {
            r = -1;
            ansicon_printf(ANSI_RED, "Unable to write to %s\n", out);
            break;
        }
        header.chunk_id[0] = 'R'; header.chunk_id[1] = 'I'; header.chunk_id[2] = 'F'; header.chunk_id[3] = 'F';
        header.chunk_size = vgm->complete_samples * 2 + 32;
        header.format[0] = 'W'; header.format[1] = 'A'; header.format[2] = 'V'; header.format[3] = 'E';
        header.subchunk1_id[0] = 'f'; header.subchunk1_id[1] = 'm'; header.subchunk1_id[2] = 't'; header.subchunk1_id[3] = ' ';
        header.subchunk1_size = 16;
        header.audio_format = 1;
        header.channels = 1;
        header.sample_rate = VGM_SAMPLE_RATE;
        header.byte_rate = 2 * VGM_SAMPLE_RATE;
        header.block_align = 2;
        header.bits_per_sample = 16;
        header.subchunk2_id[0] = 'd'; header.subchunk2_id[1] = 'a'; header.subchunk2_id[2] = 't'; header.subchunk2_id[3] = 'a';
        header.subchunk2_size = vgm->complete_samples * 2;
        fwrite(&header, sizeof(wavfile_header_t), 1, fd);

        vgm_prepare_playback(vgm, SAMPLE_RATE, false);
        vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_ALL, false);
        vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_PULSE1, ctrl->enable_apu_pulse1);
        vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_PULSE2, ctrl->enable_apu_pulse2);
        vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_TRIANGLE, ctrl->enable_apu_triangle);
        vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_NOISE, ctrl->enable_apu_noise);
        vgm_nesapu_enable_channel(vgm, VGM_NESAPU_CHANNEL_DMC, ctrl->enable_apu_dmc);
        played_samples = 0;
        while (played_samples < ctrl->complete_samples)
        {
            nsamples = vgm_get_samples(vgm, buffer, 1024);
            if (nsamples > 0)
                fwrite(buffer, sizeof(int16_t), (size_t)nsamples, fd);
            else
                break;
            played_samples += nsamples;
            if (played_samples % 4096 == 0)
                show_progress(ctrl, false);
        }
        show_progress(ctrl, true);
        if (played_samples != ctrl->complete_samples)
        {
            r = -1;
            break;
        }
    } while (0);
    if (fd) fclose(fd);
    return r;
}


int main(int argc, char *argv[])
{
    file_reader_t *reader = 0;
    vgm_t *vgm = 0;
    
    ansicon_setup();
    ansicon_hide_cursor();

    do
    {
        const char *vgm_file = NULL;
        bool dump_mode = false;
        const char *channels = "DNT21";
        vgmplay_ctrl_t ctrl;

        // Parse command line options
        struct parg_state ps;
        int c;
        parg_init(&ps);
        while ((c = parg_getopt(&ps, argc, argv, "dc:h")) != -1)
        {
            switch (c)
            {
            case 1:
                vgm_file = ps.optarg;
                break;
            case 'h':
                break;
            case 'd':
                dump_mode = true;
                break;
            case 'c':
                channels = ps.optarg;
                break;
            }
        }
        if ((NULL == vgm_file) || ('\0' == vgm_file[0]))
        {
            usage();
            break;
        }

        // Parse channel to vgmplay_ctrl
        memset(&ctrl, 0, sizeof(vgmplay_ctrl_t));
        if (strchr(channels, '1')) ctrl.enable_apu_pulse1 = true;
        if (strchr(channels, '2')) ctrl.enable_apu_pulse2 = true;
        if (strchr(channels, 'T')) ctrl.enable_apu_triangle = true;
        if (strchr(channels, 't')) ctrl.enable_apu_triangle = true;
        if (strchr(channels, 'N')) ctrl.enable_apu_noise = true;
        if (strchr(channels, 'n')) ctrl.enable_apu_noise = true;
        if (strchr(channels, 'D')) ctrl.enable_apu_dmc = true;
        if (strchr(channels, 'd')) ctrl.enable_apu_dmc = true;

        // Create reader
        reader = cfreader_create(vgm_file, READER_CACHE_SIZE);
        if (!reader)
        {
            ansicon_printf(ANSI_RED, "Unable to open %s\n", vgm_file);
            break;
        }
        // Create decoder
        vgm = vgm_create(reader);
        if (!vgm)
        {
            ansicon_printf(ANSI_RED, "Error parsing vgm file %s\n", vgm_file);
            break;
        }
        ctrl.complete_samples = vgm->complete_samples;

        ansicon_printf(ANSI_LIGHTBLUE, "Version        %X.%X\n", vgm->version >> 8, vgm->version & 0xff);
        if (vgm->loop_samples > 0)
            ansicon_printf(ANSI_LIGHTBLUE, "Total samples: %d+%d (%.2fs+%.2fs)\n", vgm->total_samples, vgm->loop_samples, vgm->total_samples / 44100.0f, vgm->loop_samples / 44100.0f);
        else
            ansicon_printf(ANSI_LIGHTBLUE, "Total samples: %d (%.2fs)\n", vgm->total_samples, vgm->total_samples / 44100.0f);
        ansicon_printf(ANSI_LIGHTBLUE, "Track Name:    %s\n", vgm->track_name_en);
        ansicon_printf(ANSI_LIGHTBLUE, "Game Name:     %s\n", vgm->game_name_en);
        ansicon_printf(ANSI_LIGHTBLUE, "Author:        %s\n", vgm->author_name_en);
        ansicon_printf(ANSI_LIGHTBLUE, "Release Date:  %s\n", vgm->release_date);
        ansicon_printf(ANSI_LIGHTBLUE, "Ripped by:     %s\n", vgm->creator);
        ansicon_printf(ANSI_LIGHTBLUE, "Notes:         %s\n", vgm->notes);
        if (!dump_mode)
        {
            play(vgm, reader, &ctrl);
        }
        else
        {
            const char *infile = vgm_file;
            char infile_abs[MAX_PATH_NAME];
            char outfile_abs[MAX_PATH_NAME];
            if (cwk_path_is_relative(infile))
            {
                char *cwd = getcwd(NULL, 0);
                cwk_path_get_absolute(cwd, infile, infile_abs, MAX_PATH_NAME);
                free(cwd);
                infile = infile_abs;
            }
            cwk_path_change_extension(infile_abs, "wav", outfile_abs, MAX_PATH_NAME);
            dump(vgm, reader, &ctrl, outfile_abs);
        }
        printf("\n");
    } while (0);
    
    if (vgm != 0) vgm_destroy(vgm);
    if (reader != 0) cfreader_destroy(reader);
    
    ansicon_show_cursor();
    ansicon_restore();
    return 0;
}

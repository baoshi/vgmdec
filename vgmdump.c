#include <stdio.h>
#include <memory.h>
#include "platform.h"
#include "file_reader_cached.h"
#include "vgm.h"

#define SOUND_BUFFER_SIZE 1000
#define READER_CACHE_SIZE 4096
#define SAMPLE_RATE 44100


static int16_t buffer[SOUND_BUFFER_SIZE];



int main(int argc, char *argv[])
{
    file_reader_t *reader = 0;
    vgm_t *vgm = 0;
    int nsamples;

    if (argc < 2)
    {
        VGM_PRINTERR("Usage: vgmdump input\n");
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

        FILE* fd = fopen("sound.bin", "wb");

        // start play
        vgm_prepare_playback(vgm, SAMPLE_RATE, true);
        do
        {
            nsamples = vgm_get_sample(vgm, buffer, SOUND_BUFFER_SIZE);
            if (nsamples > 0)
                fwrite(buffer, sizeof(int16_t), (size_t)nsamples, fd);
        } while (nsamples == SOUND_BUFFER_SIZE);
        fclose(fd);
    } while (0);
        
    if (vgm != 0) vgm_destroy(vgm);
    if (reader != 0) cfr_destroy(reader);
    return 0;
}

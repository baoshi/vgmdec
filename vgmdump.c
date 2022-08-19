#include <stdio.h>
#include <memory.h>
#include <time.h>       // for clock_t, clock(), CLOCKS_PER_SEC
#include "platform.h"
#include "file_reader_cached.h"
#include "vgm.h"

#define SOUND_BUFFER_SIZE 1430
#define READER_CACHE_SIZE 163840
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

        // FILE* fd = fopen("sound.bin", "wb");

        clock_t begin = clock();
        // start play
        vgm_prepare_playback(vgm, SAMPLE_RATE, true);
        do
        {
            nsamples = vgm_get_samples(vgm, buffer, SOUND_BUFFER_SIZE);
            //if (nsamples > 0)
            //    fwrite(buffer, sizeof(int16_t), (size_t)nsamples, fd);
        } while (nsamples == SOUND_BUFFER_SIZE);
        //fclose(fd);

        clock_t end = clock();
        double time_spent = ((double)end - begin) / CLOCKS_PER_SEC;
        printf("The elapsed time is %f seconds", time_spent);
    } while (0);
        
    if (vgm != 0) vgm_destroy(vgm);
    if (reader != 0) cfr_destroy(reader);
    return 0;
}

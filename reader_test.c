#include <stdio.h>
#include <time.h>
#include "platform.h"
#include "file_reader_cached.h"
#include "vgm.h"

const int buf_size = 4096;
uint8_t buf1[buf_size], buf2[buf_size];


bool compare_buf(uint8_t *buf1, uint8_t *buf2, int size)
{
    for (int i = 0; i < size; ++i)
    {
        if (buf1[i] != buf2[i]) 
            return false;
    }
    return true;
}


bool reader_test(file_reader_t *reader, FILE *fd, int offset, int len)
{
    int len1, len2;

    fseek(fd, offset, SEEK_SET);
    len1 = fread(buf1, 1, len, fd);
    len2 = reader->read(reader, buf2, offset, len);
    if ((len1 == len2) && compare_buf(buf1, buf2, len1))
    {
        VGM_PRINTF("ok\n");
        return true;
    }
    else
    {
        VGM_PRINTF("failed\n");
        return false;
    }
}


int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        VGM_PRINTERR("Usage: reader_test input\n");
        return -1;
    }
   
    FILE *fd;
    fd = fopen(argv[1], "rb");
    file_reader_t *reader = cfr_create(argv[1], 4096);

    time_t t;
    srand((unsigned)time(&t));

    int size = reader->size(reader);
    int offset = 0, len;

    for (int i = 0; i < 10000; ++i)
    {
        offset += (rand() % 18);
        len = rand() % 2048;
        VGM_PRINTF("Test\toff=%d,\tlen=%d:\t", offset, len);
        if (!reader_test(reader, fd, offset, len)) break;
    }
    
    cfr_show_cache_status(reader);

    cfr_destroy(reader);
    fclose(fd);

    return 0;
}

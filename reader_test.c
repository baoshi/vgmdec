#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "vgm_conf.h"
#include "cached_file_reader.h"


#define BUF_SIZE 4096
uint8_t buf1[BUF_SIZE], buf2[BUF_SIZE];


bool compare_buf(uint8_t *buf1, uint8_t *buf2, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        if (buf1[i] != buf2[i]) 
            return false;
    }
    return true;
}


bool reader_test(file_reader_t *reader, FILE *fd, size_t offset, size_t len)
{
    size_t len1, len2;

    fseek(fd, (long)offset, SEEK_SET);
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
    file_reader_t *reader = cfreader_create(argv[1], 4096);

    time_t t;
    srand((unsigned)time(&t));

    size_t offset = 0, len;

    for (int i = 0; i < 10000; ++i)
    {
        offset += (size_t)((rand() % 18));
        len = (size_t)(rand() % 2048);
        VGM_PRINTF("Test\toff=%lu,\tlen=%lu:\t", (unsigned long)offset, (unsigned long)len);
        if (!reader_test(reader, fd, offset, len)) break;
    }
    
    cfreader_show_cache_status(reader);

    cfreader_destroy(reader);
    fclose(fd);

    return 0;
}

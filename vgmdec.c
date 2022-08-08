#include <stdio.h>
#include "platform.h"
#include "file_reader_cached.h"
#include "vgm.h"



int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        VGM_PRINTERR("Usage: vgmdec input\n");
        return -1;
    }

    file_reader_t *reader = cfr_create(argv[1], 128);

    VGM_PRINTF("File size: %d\n", reader->size(reader));

    uint8_t buf[100];

    reader->read(reader, buf, 1, 3);
    reader->read(reader, buf, 1, 4);
    reader->read(reader, buf, 4, 1);
    reader->read(reader, buf, 6, 1);
    reader->read(reader, buf, 4, 3);
    reader->read(reader, buf, 2, 6);
    reader->read(reader, buf, 6, 1);
    reader->read(reader, buf, 6, 3);
    reader->read(reader, buf, 7, 3);

    //cfr_show_cache_status(reader);

    cfr_destroy(reader);

    return 0;
}

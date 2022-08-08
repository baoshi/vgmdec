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

    file_reader_t *reader = cfr_create(argv[1], 4096);

    VGM_PRINTF("File size: %d\n", reader->size(reader));

    reader->read(reader, NULL, 1, 3);
    reader->read(reader, NULL, 1, 4);
    reader->read(reader, NULL, 4, 1);
    reader->read(reader, NULL, 6, 1);
    reader->read(reader, NULL, 4, 3);
    reader->read(reader, NULL, 2, 6);
    reader->read(reader, NULL, 6, 1);
    reader->read(reader, NULL, 6, 3);
    reader->read(reader, NULL, 7, 3);

    //cfr_show_cache_status(reader);

    cfr_destroy(reader);

    return 0;
}

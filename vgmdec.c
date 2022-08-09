#include "platform.h"
#include "file_reader_cached.h"
#include "vgm.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        VGM_PRINTERR("Usage: reader_test input\n");
        return -1;
    }

    file_reader_t *reader = cfr_create(argv[1], 4096);
    vgm_t *vgm = vgm_create(reader);

    cfr_destroy(reader);

    return 0;
}

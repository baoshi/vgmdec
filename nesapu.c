#include <stdlib.h>
#include <memory.h>
#include "platform.h"
#include "nesapu.h"



nesapu_t * nesapu_create(bool format, uint32_t clock, uint32_t srate)
{
    nesapu_t *a = (nesapu_t*)VGM_MALLOC(sizeof(nesapu_t));
    if (NULL == a)
        return NULL;
    memset(a, 0, sizeof(nesapu_t));
    a->format = format;
    a->clock_rate = clock;
    a->sample_rate = srate;
    //nesapu_reset(a);
    return a;
}


void nesapu_destroy(nesapu_t *a)
{
    if (a != NULL)
        VGM_FREE(a);
}

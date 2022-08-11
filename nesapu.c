#include <stdlib.h>
#include <memory.h>
#include "platform.h"
#include "nesapu.h"



nesapu_t * nesapu_create(bool format, uint32_t clock, uint32_t srate, uint32_t max_sample_count)
{
    nesapu_t *a = (nesapu_t*)VGM_MALLOC(sizeof(nesapu_t));
    if (NULL == a)
        return NULL;
    memset(a, 0, sizeof(nesapu_t));
    a->format = format;
    a->clock_rate = clock;
    a->sample_rate = srate;
    // BLIP buffer
    a->blip_buffer_size = max_sample_count;
    a->blip = blip_new(max_sample_count);
    blip_set_rates(a->blip, a->clock_rate, a->sample_rate);
    a->blip_last_sample = 0;
    //nesapu_reset(a);
    return a;
}


void nesapu_destroy(nesapu_t *a)
{
    if (a != NULL)
    {
        if (a->blip)
        {
            blip_delete(a->blip);
            a->blip = 0;
        }
        VGM_FREE(a);
    }
}


void nesapu_buffer_sample(nesapu_t *a)
{

}


void nesapu_read_samples(nesapu_t *a)
{

}

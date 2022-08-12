#include <stdlib.h>
#include <memory.h>
#include "platform.h"
#include "nesapu.h"


static inline void update_frame_counter(fp16_t cycles)
{

}


nesapu_t * nesapu_create(bool format, uint32_t clock, uint32_t srate, uint32_t max_sample_count)
{
    nesapu_t *a = (nesapu_t*)VGM_MALLOC(sizeof(nesapu_t));
    if (NULL == a)
        return NULL;
    memset(a, 0, sizeof(nesapu_t));
    a->format = format;
    a->clock_rate = clock;
    a->sample_rate = srate;
    // blip
    a->blip_buffer_size = max_sample_count;
    a->blip = blip_new(max_sample_count);
    blip_set_rates(a->blip, a->clock_rate, a->sample_rate);
    // sampling control
    a->sample_accu_fp = 0;
    a->sample_period_fp = float_to_fp16((float)a->clock_rate / a->sample_rate);
    a->sample_timestamp = 0;
    a->blip_last_sample = 0;
    // frame counter
    a->frame_accu_fp = 0;
    a->frame_period_fp = float_to_fp16((float)a->clock_rate / 240.0f);  // 240Hz frame counter
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
    a->sample_accu_fp += a->sample_period_fp;
    fp16_t cycles_fp = fp16_round(a->sample_accu_fp);
    uint32_t cycles = fp16_to_int(cycles_fp);
    // step cycles and sample, put result in blip buffer
    update_frame_counter(cycles_fp);
    
    a->sample_timestamp += cycles;
    a->sample_accu_fp -= cycles_fp;
}


void nesapu_read_samples(nesapu_t *a, int16_t *buf, uint32_t samples)
{
    VGM_PRINTF("%d samples in %d cycles\n", samples, a->sample_timestamp);
    a->sample_timestamp = 0;
}

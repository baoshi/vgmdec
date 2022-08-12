#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "blip_buf.h"
#include "fixedpoint.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct nesapu_s
{
    bool format;        // true: PAL, false: NTSC
    uint32_t clock_rate;
    uint32_t sample_rate;
    // registers
    uint8_t reg4000, reg4001, reg4002, reg4003; // Pulse1
    uint8_t reg4004, reg4005, reg4006, reg4007; // Pulse 2
    uint8_t reg4008, reg4009, reg400a, reg400b; // Triangle
    uint8_t reg400c, reg400d, reg400e, reg400f; // Noise
    uint8_t reg4010, reg4011, reg4012, reg4013; // DMC
    uint8_t reg4017;    // All 
    // Blip
    uint16_t blip_buffer_size;
    blip_buffer_t *blip;
    int16_t blip_last_sample;
    // Sampling cycle control
    fp16_t   sample_accu_fp;    // sampling accumulator (fixed point)
    fp16_t   sample_period_fp;  // sampling period (fixed point)
    uint32_t sample_timestamp;  // samplling timestamp (for blip frame), in cycles
    // frame counter
    uint8_t  sequencer_step;    // sequencer step, 1-2-3-4 or 1-2-3-4-5
    bool     sequence_mode;     // false: 4-step sequence. true: 5-step sequence. Set by $4017 bit 7
    bool     quarter_frame;     // true @step 1,2,3,4 for 4-step sequence or @step 1,2,3,5 for 5-step sequence
    bool     half_frame;        // true @step 2,4 for 4-step sequence or @step 2,5 for 5-step sequence
    fp16_t   frame_accu_fp;     // frame counter accumulator (fixed point)
    fp16_t   frame_period_fp;   // frame clock (240Hz) period (fixed point)
} nesapu_t;


nesapu_t * nesapu_create(bool format, uint32_t clock, uint32_t srate, uint32_t max_sample_count);
void nesapu_destroy(nesapu_t *a);
void nesapu_buffer_sample(nesapu_t *a);
void nesapu_read_samples(nesapu_t *a, int16_t *buf, uint32_t samples);

#ifdef __cplusplus
}
#endif

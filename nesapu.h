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
    fp15_t   sample_accu_fp;    // sampling accumulator (fixed point)
    fp15_t   sample_period_fp;  // sampling period (fixed point)
    int32_t  sample_timestamp;
    fp15_t   sample_timestamp_fp;  
    // frame counter
    uint8_t  sequencer_step;    // sequencer step, 1-2-3-4 or 1-2-3-4-5
    bool     sequence_mode;     // false: 4-step sequence. true: 5-step sequence. Set by $4017 bit 7
    bool     quarter_frame;     // true @step 1,2,3,4 for 4-step sequence or @step 1,2,3,5 for 5-step sequence
    bool     half_frame;        // true @step 2,4 for 4-step sequence or @step 2,5 for 5-step sequence
    fp15_t   frame_accu_fp;     // frame counter accumulator (fixed point)
    fp15_t   frame_period_fp;   // frame clock (240Hz) period (fixed point)
    // Channel Pulse1
    bool     pulse1_enabled;
    uint8_t  pulse1_duty;                   // duty cycle, 0,1,2,3 -> 12.5%,25%,50%,-25%, see pulse_waveform_table
    uint8_t  pulse1_sequencer_value;        // Current suquence (8 steps of waveform)
    uint8_t  pulse1_length_counter;         // Length counter value
    bool     pulse1_lchalt_evloop;          // Length counter halt or Envelope loop
    bool     pulse1_cvol_envelope;          // Constant volume (true) or Envelope enable (false)
    bool     pulse1_envelope_start;         // Envelope start flag
    uint8_t  pulse1_envelope_decay;         // Envelope decay value (15, 14, ...)
    uint8_t  pulse1_envelope_divider;       // Envelope clock divider (counter value)
    uint8_t  pulse1_envelope_volume_period; // Volume or Envelope period
    bool     pulse1_sweep_enabled;          // Sweep enabled
    uint8_t  pulse1_sweep_period;           // Sweep period
    uint8_t  pulse1_sweep_divider;          // Sweep divider value
    bool     pulse1_sweep_negate;           // Sweep negate flag
    uint8_t  pulse1_sweep_shift;            // Sweep shift
    uint16_t pulse1_sweep_target;           // Sweep target
    bool     pulse1_sweep_reload;           // To reload sweep_value
    uint16_t pulse1_timer_period;           // Pulse1 channel main timer period
    uint16_t pulse1_timer_divider;          // Pulse1 channel main timer divider (counter value), counting from 2x timer_period
    
} nesapu_t;


nesapu_t * nesapu_create(bool format, uint32_t clock, uint32_t srate, uint32_t max_sample_count);
void nesapu_destroy(nesapu_t *a);
void nesapu_write_reg(nesapu_t *a, uint16_t reg, uint8_t val);
void nesapu_buffer_sample(nesapu_t *a);
void nesapu_read_samples(nesapu_t *a, int16_t *buf, uint32_t samples);

#ifdef __cplusplus
}
#endif

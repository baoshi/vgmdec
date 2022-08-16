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
    unsigned int clock_rate;
    unsigned int sample_rate;
    // Blip
    blip_buffer_t *blip;
    int16_t blip_last_sample;
    // frame counter
    uint8_t  sequencer_step;    // sequencer step, 1-2-3-4 or 1-2-3-4-5
    bool     sequence_mode;     // false: 4-step sequence. true: 5-step sequence. Set by $4017 bit 7
    bool     frame_quarter;     // true @step 1,2,3,4 for 4-step sequence or @step 1,2,3,5 for 5-step sequence
    bool     frame_half;        // true @step 2,4 for 4-step sequence or @step 2,5 for 5-step sequence
    bool     frame_force_clock; // When writing 0x80 to $4017, immediate clock all units at first step
    q16_t    frame_accu_fp;     // frame counter accumulator (fixed point)
    q16_t    frame_period_fp;   // frame clock (240Hz) period (fixed point)
    // Pulse1 Channel
    struct pulse_t
    {
        bool          enabled;
        unsigned int  duty;             // duty cycle, 0,1,2,3 -> 12.5%,25%,50%,-25%, see pulse_waveform_table
        unsigned int  length_counter;   // Length counter value
        bool          lchalt_evloop;    // Length counter halt or Envelope loop
        bool          constant_volume;  // Constant volume (true) or Envelope enable (false)
        bool          envelope_start;   // Envelope start flag
        unsigned int  envelope_decay;   // Envelope decay value (15, 14, ...)
        unsigned int  envelope_counter; // Envelope clock divider (counter value)
        unsigned int  volume_evperiod;  // Volume or Envelope period
        bool          sweep_enabled;    // Sweep enabled
        unsigned int  sweep_period;     // Sweep period
        unsigned int  sweep_divider;    // Sweep divider value
        bool          sweep_negate;     // Sweep negate flag
        unsigned int  sweep_shift;      // Sweep shift
        unsigned int  sweep_target;     // Sweep target
        bool          sweep_reload;     // To reload sweep_divider
        unsigned int  timer_period;     // Pulse1 channel main timer period
        unsigned int  timer_counter;    // Pulse1 channel main timer divider (counter value), counting from 2x timer_period
        unsigned int  sequencer_value;  // Current suquence (8 steps of waveform)
    } pulse[2];
    // Triangle Channel
    bool          triangle_enabled;
    unsigned int  triangle_length_counter;
    bool          triangle_lnrctl_lenhalt;      // Linear counter control / length counter halt
    unsigned int  triangle_linear_period;       // Linear counter period
    unsigned int  triangle_linear_counter;      // Linear counter value
    bool          triangle_linear_reload;       // Linear counter reload flag
    unsigned int  triangle_timer_period;
    bool          triangle_timer_period_bad;
    unsigned int  triangle_timer_counter;
    unsigned int  triangle_sequencer_value;     // Current suquence (32 steps of waveform)

} nesapu_t;


nesapu_t * nesapu_create(bool format, unsigned int clock, unsigned int srate, unsigned int max_sample_count);
void nesapu_destroy(nesapu_t *a);
void nesapu_write_reg(nesapu_t *a, uint16_t reg, uint8_t val);
void nesapu_get_samples(nesapu_t *apu, int16_t *buf, unsigned int samples);

#ifdef __cplusplus
}
#endif

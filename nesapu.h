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
    bool     quarter_frame;     // true @step 1,2,3,4 for 4-step sequence or @step 1,2,3,5 for 5-step sequence
    bool     half_frame;        // true @step 2,4 for 4-step sequence or @step 2,5 for 5-step sequence
    q16_t    frame_accu_fp;     // frame counter accumulator (fixed point)
    q16_t    frame_period_fp;   // frame clock (240Hz) period (fixed point)
    // Channel Pulse1
    bool            pulse1_enabled;
    unsigned int    pulse1_duty;                // duty cycle, 0,1,2,3 -> 12.5%,25%,50%,-25%, see pulse_waveform_table
    unsigned int    pulse1_sequencer_value;     // Current suquence (8 steps of waveform)
    unsigned int    pulse1_length_counter;      // Length counter value
    bool            pulse1_lchalt_evloop;       // Length counter halt or Envelope loop
    bool            pulse1_constant_volume;     // Constant volume (true) or Envelope enable (false)
    bool            pulse1_envelope_start;      // Envelope start flag
    unsigned int    pulse1_envelope_decay;      // Envelope decay value (15, 14, ...)
    unsigned int    pulse1_envelope_divider;    // Envelope clock divider (counter value)
    unsigned int    pulse1_volume_evperiod;     // Volume or Envelope period
    bool            pulse1_sweep_enabled;       // Sweep enabled
    unsigned int    pulse1_sweep_period;        // Sweep period
    unsigned int    pulse1_sweep_divider;       // Sweep divider value
    bool            pulse1_sweep_negate;        // Sweep negate flag
    unsigned int    pulse1_sweep_shift;         // Sweep shift
    unsigned int    pulse1_sweep_target;        // Sweep target
    bool            pulse1_sweep_reload;        // To reload sweep_divider
    unsigned int    pulse1_timer_period;        // Pulse1 channel main timer period
    unsigned int    pulse1_timer_divider;       // Pulse1 channel main timer divider (counter value), counting from 2x timer_period
    // Channel Pulse1
    bool            pulse2_enabled;
    unsigned int    pulse2_duty;                // duty cycle, 0,1,2,3 -> 12.5%,25%,50%,-25%, see pulse_waveform_table
    unsigned int    pulse2_sequencer_value;     // Current suquence (8 steps of waveform)
    unsigned int    pulse2_length_counter;      // Length counter value
    bool            pulse2_lchalt_evloop;       // Length counter halt or Envelope loop
    bool            pulse2_constant_volume;     // Constant volume (true) or Envelope enable (false)
    bool            pulse2_envelope_start;      // Envelope start flag
    unsigned int    pulse2_envelope_decay;      // Envelope decay value (15, 14, ...)
    unsigned int    pulse2_envelope_divider;    // Envelope clock divider (counter value)
    unsigned int    pulse2_volume_evperiod;     // Volume or Envelope period
    bool            pulse2_sweep_enabled;       // Sweep enabled
    unsigned int    pulse2_sweep_period;        // Sweep period
    unsigned int    pulse2_sweep_divider;       // Sweep divider value
    bool            pulse2_sweep_negate;        // Sweep negate flag
    unsigned int    pulse2_sweep_shift;         // Sweep shift
    unsigned int    pulse2_sweep_target;        // Sweep target
    bool            pulse2_sweep_reload;        // To reload sweep_divider
    unsigned int    pulse2_timer_period;        // Pulse2 channel main timer period
    unsigned int    pulse2_timer_divider;       // Pulse2 channel main timer divider (counter value), counting from 2x timer_period

} nesapu_t;


nesapu_t * nesapu_create(bool format, unsigned int clock, unsigned int srate, unsigned int max_sample_count);
void nesapu_destroy(nesapu_t *a);
void nesapu_write_reg(nesapu_t *a, uint16_t reg, uint8_t val);
void nesapu_get_samples(nesapu_t *apu, int16_t *buf, unsigned int samples);

#ifdef __cplusplus
}
#endif

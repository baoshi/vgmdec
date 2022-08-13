#include <stdlib.h>
#include <memory.h>
#include "platform.h"
#include "nesapu.h"

//
// Length Counter (Pulse1, Pulse2, Triangle, Noise)
// http://wiki.nesdev.org/w/index.php/APU_Length_Counter
//
static const uint8_t length_counter_table[32] = 
{
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};


// Duty lookup table, see wiki "Implementation details"
static const bool pulse_waveform_table[][8] = 
{
    { 0, 0, 0, 0, 0, 0, 0, 1 }, // 12.5%
    { 0, 0, 0, 0, 0, 0, 1, 1 }, // 25%
    { 0, 0, 0, 0, 1, 1, 1, 1 }, // 50%
    { 1, 1, 1, 1, 1, 1, 0, 0 }  // 25% negated
};


static inline void update_frame_counter(nesapu_t *apu, fp16_t cycles_fp)
{
    //
    // https://www.nesdev.org/wiki/APU_Frame_Counter
    //
    apu->frame_accu_fp += cycles_fp;
    if (apu->frame_accu_fp >= apu->frame_period_fp)
    {
        apu->frame_accu_fp -= apu->frame_period_fp;
        ++apu->sequencer_step;
        if (apu->sequence_mode)
        {
            // 5 step mode
            // step      quarter_frame         half_frame
            //  1          clock                  -
            //  2          clock                clock
            //  3          clock                  -
            //  4            -                    -
            //  5          clock                clock
            switch (apu->sequencer_step)
            {
            case 1:
                apu->quarter_frame = true;
                apu->half_frame = false;
                break;
            case 2:
                apu->quarter_frame = true;
                apu->half_frame = true;
                break;
            case 3:
                apu->quarter_frame = true;
                apu->half_frame = false;
                break;
            case 4:
                apu->quarter_frame = false;
                apu->half_frame = false;
                break;
            case 5:
                apu->quarter_frame = true;
                apu->half_frame = true;
                apu->sequencer_step = 0;
                break;
            }
        }
        else
        {
            // 4 step mode
            // step      quarter_frame         half_frame
            //  1          clock                  -
            //  2          clock                clock
            //  3          clock                  -
            //  4          clock                clock
            switch (apu->sequencer_step)
            {
            case 1:
                apu->quarter_frame = true;
                apu->half_frame = false;
                break;
            case 2:
                apu->quarter_frame = true;
                apu->half_frame = true;
                break;
            case 3:
                apu->quarter_frame = true;
                apu->half_frame = false;
                break;
            case 4:
                apu->quarter_frame = true;
                apu->half_frame = true;
                apu->sequencer_step = 0;
                break;
            }
        }
    }
}


static inline uint8_t update_pulse1(nesapu_t *apu, fp16_t cycles_fp)
{
    uint8_t vol = 0;
    if (!apu->pulse1_enabled)
    {
        apu->pulse1_duty_index = 0;
        return 0;
    }
    // clock envelope @ quater frame
    if (apu->quarter_frame)
    {
        // update envelope on quater frame clock
        if (apu->pulse1_envelope_start)
        {
            apu->pulse1_envelope_decay = 15;
            apu->pulse1_envelope_divider = apu->pulse1_vol_evperiod;
            apu->pulse1_envelope_start = false;
        }
        else
        {
            if (apu->pulse1_envelope_divider)
            {
                --apu->pulse1_envelope_divider;
            }
            else
            {
                apu->pulse1_envelope_divider = apu->pulse1_vol_evperiod;
                if (apu->pulse1_envelope_decay)
                {
                    --apu->pulse1_envelope_decay;
                }
                else if (apu->pulse1_lchalt_evloop)
                {
                    apu->pulse1_envelope_decay = 15;
                }
            }
        }
    }
    if (apu->pulse1_cvol_even)
    {
        // if constant volume is enabled
        vol = apu->pulse1_vol_evperiod;
    }
    else
    {
        // use envelope value
        vol = apu->pulse1_envelope_decay;
    }
    // clock length counter @ half frame
    if (!apu->pulse1_lchalt_evloop && apu->pulse1_lc_value && apu->half_frame)
    {
        --(apu->pulse1_lc_value);
    }
    // Two conditions cause the sweep unit to mute the channel:
    // 1. If the current period is less than 8, the sweep unit mutes the channel.
    // 2. If at any time the target period is greater than $7FF, the sweep unit mutes the channel.
    bool sweep_mute = ((apu->pulse1_timer_period < 8) || (apu->pulse1_sweep_target > 0x7ff));
    // clock sweep @ half frame
    if (apu->half_frame)
    {
        // 1. If the divider's counter is zero, the sweep is enabled, and the sweep unit is not muting the channel: The pulse's period is set to the target period.
        // 2. If the divider's counter is zero or the reload flag is true: The divider counter is set to P and the reload flag is cleared.
        //    Otherwise, the divider counter is decremented.
        if (apu->pulse1_sweep_reload)
        {
            apu->pulse1_sweep_value = apu->pulse1_sweep_period;
            apu->pulse1_sweep_reload = false;
        }
        else 
        {
            if (apu->pulse1_sweep_value)
            {
                --apu->pulse1_sweep_value;
            } 
            else if (apu->pulse1_sweep_enabled)
            {
                // sweep divider outputs clock
                apu->pulse1_sweep_value = apu->pulse1_sweep_period;
                uint16_t c = apu->pulse1_timer_period >> apu->pulse1_sweep_shift;
                if (apu->pulse1_sweep_negate)
                {
                    apu->pulse1_sweep_target -= c + 1;
                }
                else
                {
                    apu->pulse1_sweep_target += c;
                }
                if (apu->pulse1_sweep_enabled && apu->pulse1_sweep_shift && !sweep_mute)
                {
                    apu->pulse1_timer_period = apu->pulse1_sweep_shift;   
                }
            }
        }
    }
    if (!apu->pulse1_lc_value)              // length is 0, silence
    {
        return 0;
    }
    return 0;
}



nesapu_t * nesapu_create(bool format, uint32_t clock, uint32_t srate, uint32_t max_sample_count)
{
    nesapu_t *apu = (nesapu_t*)VGM_MALLOC(sizeof(nesapu_t));
    if (NULL == apu)
        return NULL;
    memset(apu, 0, sizeof(nesapu_t));
    apu->format = format;
    apu->clock_rate = clock;
    apu->sample_rate = srate;
    // blip
    apu->blip_buffer_size = max_sample_count;
    apu->blip = blip_new(max_sample_count);
    blip_set_rates(apu->blip, apu->clock_rate, apu->sample_rate);
    // sampling control
    apu->sample_accu_fp = 0;
    apu->sample_period_fp = float_to_fp16((float)apu->clock_rate / apu->sample_rate);
    apu->sample_timestamp = 0;
    apu->blip_last_sample = 0;
    // frame counter
    apu->sequencer_step = 0;
    apu->sequence_mode = false;
    apu->quarter_frame = false;
    apu->half_frame = false;
    apu->frame_accu_fp = 0;
    apu->frame_period_fp = float_to_fp16((float)apu->clock_rate / 240.0f);  // 240Hz frame counter
    //nesapu_reset(a);
    return apu;
}


void nesapu_destroy(nesapu_t *apu)
{
    if (apu != NULL)
    {
        if (apu->blip)
        {
            blip_delete(apu->blip);
            apu->blip = 0;
        }
        VGM_FREE(apu);
    }
}


void nesapu_buffer_sample(nesapu_t *apu)
{
    apu->sample_accu_fp += apu->sample_period_fp;
    fp16_t cycles_fp = fp16_round(apu->sample_accu_fp);
    uint32_t cycles = fp16_to_int(cycles_fp);
    // step cycles and sample, put result in blip buffer
    update_frame_counter(apu, cycles_fp);
    
    apu->sample_timestamp += cycles;
    apu->sample_accu_fp -= cycles_fp;
}


void nesapu_read_samples(nesapu_t *apu, int16_t *buf, uint32_t samples)
{
    VGM_PRINTF("%d samples in %d cycles\n", samples, apu->sample_timestamp);
    apu->sample_timestamp = 0;
}


void nesapu_write_reg(nesapu_t *apu, uint16_t reg, uint8_t val)
{
    switch (reg)
    {
    // Pulse1 regs
    case 0x00:  // $4000: DDLC VVVV
        apu->reg4000 = val;
        apu->pulse1_duty = (val & 0xc0) >> 6;       // Duty (DD), index to tbl_pulse_waveform
        apu->pulse1_lchalt_evloop = (val & 0x20);   // Length counter halt or Envelope loop (L)
        apu->pulse1_cvol_even = (val & 0x10);       // Constant volume/envelope enable (C)
        apu->pulse1_vol_evperiod = (val & 0xf);     // Volume or period of envelope (VVVV)
        break;
    case 0x01:  // $4001: EPPP NSSS
        apu->reg4001 = val;
        apu->pulse1_sweep_enabled = (val & 0x80);       // Sweep enable (E)
        apu->pulse1_sweep_period = (val & 0x70) >> 4;   // Sweep period (PPP)
        apu->pulse1_sweep_negate = (val & 0x08);        // Sweep negate (N)
        apu->pulse1_sweep_shift = val & 0x07;           // Sweep shift (SSS)
        apu->pulse1_sweep_reload = true;                // Side effects : Sets reload flag 
        break;
    case 0x02:  // $4002: LLLL LLLL
        apu->reg4002 = val;
        apu->pulse1_timer_period &= 0xff00;
        apu->pulse1_timer_period |= val;                // Timer period low (TTTT TTTT)
        apu->pulse1_sweep_target = apu->pulse1_timer_period;             // Whenever the current period changes, the target period also changes
        break;
    case 0x03:  // $4003: llll lHHH
        apu->reg4003 = val;
        apu->pulse1_lc_value = length_counter_table[(val & 0xf8) >> 3]; // Length counter load (lllll) 
        apu->pulse1_timer_period &= 0x00ff;
        apu->pulse1_timer_period |= ((uint16_t)(val & 0x07) << 8);      // Timer period high 3 bits (TTT)
        apu->pulse1_sweep_target = apu->pulse1_timer_period;             // Whenever the current period changes, the target period also changes  
        // Side effects : The sequencer is immediately restarted at the first value of the current sequence. 
        // The envelope is also restarted. The period divider is not reset.
        apu->pulse1_envelope_start = true;
        apu->pulse1_duty_index = 0;
        break;
    case 0x15:  // $4015: ---D NT21
        if (val & 0x01)
        {
            apu->pulse1_enabled = true;
        }
        else
        {
            apu->pulse1_enabled = true;
            apu->pulse1_timer_value = 0;
            apu->pulse1_lc_value = 0;
        }
        break;
    case 0x17:  // $4017:
        break;
    }
}

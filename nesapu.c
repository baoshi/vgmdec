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



//
// Mixer
// https://wiki.nesdev.org/w/index.php/APU_Mixer#Emulation
//
// Generate table using make_const_tables.c


static const fp29_t mixer_pulse_table[31] =
{
             0,    6232609,   12315540,   18254120,   24053428,   29718306,   35253376,   40663044,
      45951532,   51122860,   56180880,   61129276,   65971580,   70711160,   75351248,   79894960,
      84345240,   88704968,   92976872,   97163576,  101267592,  105291368,  109237216,  113107392,
     116904048,  120629256,  124285008,  127873240,  131395816,  134854496,  138251008
};


static const fp29_t mixer_tnd_table[203] =
{
             0,    3596940,    7164553,   10703196,   14213217,   17694966,   21148782,   24574998,
      27973946,   31345950,   34691328,   38010392,   41303460,   44570820,   47812788,   51029652,
      54221704,   57389228,   60532508,   63651820,   66747436,   69819624,   72868656,   75894784,
      78898280,   81879376,   84838328,   87775384,   90690792,   93584784,   96457600,   99309472,
     102140624,  104951272,  107741656,  110511992,  113262480,  115993344,  118704800,  121397032,
     124070264,  126724680,  129360504,  131977904,  134577088,  137158224,  139721536,  142267184,
     144795360,  147306224,  149799968,  152276768,  154736784,  157180208,  159607168,  162017872,
     164412480,  166791120,  169153984,  171501200,  173832960,  176149376,  178450624,  180736848,
     183008176,  185264784,  187506800,  189734352,  191947600,  194146672,  196331728,  198502848,
     200660208,  202803920,  204934128,  207050944,  209154512,  211244944,  213322352,  215386864,
     217438624,  219477696,  221504256,  223518400,  225520224,  227509856,  229487408,  231452976,
     233406688,  235348624,  237278928,  239197680,  241104976,  243000944,  244885648,  246759232,
     248621760,  250473328,  252314064,  254144032,  255963376,  257772096,  259570384,  261358256,
     263135856,  264903216,  266660496,  268407712,  270144992,  271872416,  273590048,  275297984,
     276996320,  278685088,  280364448,  282034432,  283695072,  285346528,  286988832,  288622080,
     290246336,  291861696,  293468160,  295065888,  296654912,  298235264,  299807136,  301370464,
     302925376,  304471968,  306010240,  307540288,  309062208,  310576032,  312081824,  313579648,
     315069568,  316551680,  318025984,  319492608,  320951584,  322402944,  323846752,  325283104,
     326712064,  328133664,  329547904,  330954912,  332354784,  333747488,  335133088,  336511680,
     337883296,  339248000,  340605792,  341956800,  343301056,  344638560,  345969408,  347293664,
     348611328,  349922464,  351227136,  352525408,  353817280,  355102848,  356382112,  357655168,
     358922016,  360182720,  361437312,  362685856,  363928384,  365164960,  366395584,  367620320,
     368839232,  370052352,  371259680,  372461344,  373657280,  374847584,  376032320,  377211456,
     378385120,  379553280,  380716000,  381873312,  383025248,  384171872,  385313216,  386449280,
     387580128,  388705792,  389826272,  390941696,  392052032,  393157312,  394257568,  395352864,
     396443264,  397528704,  398609248
};



/**
 * @brief Count down timer
 * 
 * @param divider   Input initial divider value. When counting finish, return finish divider value
 * @param period    Counter period
 * @return int      Number of times divider has reloaded from 0 to next period
 *
 */
static inline int timer16_count_down(uint16_t *divider, uint16_t period, int cycles)
{
    int clocks = cycles / period;
    int extra = cycles % period;
    if (extra > *divider)
    {
        *divider = *divider + period - extra;
        ++clocks;
    }
    else
    {
        *divider = *divider - extra;
    }
    return clocks;
}


/**
 * @brief Count down timer
 * 
 * @param divider   Input initial divider value. When counting finish, return finish divider value
 * @param period    Counter period
 * @return int      Number of times divider has reloaded from 0 to next period
 *
 */
static inline int timer8_count_down(uint8_t *divider, uint8_t period, int cycles)
{
    int clocks = cycles / period;
    int extra = cycles % period;
    if (extra > *divider)
    {
        *divider = *divider + period - extra;
        ++clocks;
    }
    else
    {
        *divider = *divider - extra;
    }
    return clocks;
}


static inline void update_frame_counter(nesapu_t *apu, fp15_t cycles_fp)
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


/*
 *
                   Sweep -----> Timer
                     |            |
                     |            |
                     |            v
                     |        Sequencer   Length Counter
                     |            |             |
                     |            |             |
                     v            v             v
  Envelope -------> Gate -----> Gate -------> Gate ---> (to mixer)
 */
static inline uint8_t update_pulse1(nesapu_t *apu, uint32_t cycles)
{
    //
    // Clock envelope @ quater frame
    if (apu->quarter_frame)
    {
        if (apu->pulse1_envelope_start)
        {
            apu->pulse1_envelope_decay = 15;
            apu->pulse1_envelope_divider = apu->pulse1_envelope_volume_period;
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
                apu->pulse1_envelope_divider = apu->pulse1_envelope_volume_period;
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
    //
    // Clock sweep unit @ half frame 
    // Two conditions cause the sweep unit to mute the channel:
    // 1. If the current period is less than 8, the sweep unit mutes the channel.
    // 2. If at any time the target period is greater than $7FF, the sweep unit mutes the channel.
    bool sweep_mute = ((apu->pulse1_timer_period < 8) || (apu->pulse1_sweep_target > 0x7ff));
    if (apu->half_frame)
    {
        // 1. If the divider's counter is zero, the sweep is enabled, and the sweep unit is not muting the channel: The pulse's period is set to the target period.
        // 2. If the divider's counter is zero or the reload flag is true: The divider counter is set to P and the reload flag is cleared.
        //    Otherwise, the divider counter is decremented.
        if (apu->pulse1_sweep_reload)
        {
            apu->pulse1_sweep_divider = apu->pulse1_sweep_period;
            apu->pulse1_sweep_reload = false;
        }
        else 
        {
            if (apu->pulse1_sweep_divider)
            {
                --apu->pulse1_sweep_divider;
            } 
            else if (apu->pulse1_sweep_enabled && apu->pulse1_sweep_shift)  // TODO: Not sure if pulse1_sweep_enabled must be true
            {
                // Calculate target period.
                uint16_t c = apu->pulse1_timer_period >> apu->pulse1_sweep_shift;
                if (apu->pulse1_sweep_negate)
                {
                    apu->pulse1_sweep_target -= c + 1;
                }
                else
                {
                    apu->pulse1_sweep_target += c;
                }
                if (!sweep_mute)
                {
                    // When the sweep unit is muting a pulse channel, the channel's current period remains unchanged.
                    apu->pulse1_timer_period = apu->pulse1_sweep_shift;   
                }
                // Reload divider
                apu->pulse1_sweep_divider = apu->pulse1_sweep_period;
            }
        }
    }
    //
    // Clock main timer and update sequencer
    // Timer counting downwards from 0 at every other CPU cycle. So we set timer limit to  2x (timer_period + 1).
    int seq_clk = timer16_count_down(&(apu->pulse1_timer_divider), (apu->pulse1_timer_period + 1) << 1, cycles);
    timer8_count_down(&(apu->pulse1_sequencer_value), 8, seq_clk);
    // 
    // clock length counter @ half frame if not halted
    if (!apu->pulse1_lchalt_evloop && apu->pulse1_length_counter && apu->half_frame)
    {
        --(apu->pulse1_length_counter);
    }
    // Determine Pulse1 output
    if (!apu->pulse1_enabled) return 0;
    if (!apu->pulse1_length_counter) return 0;
    if (sweep_mute) return 0;
    if (!pulse_waveform_table[apu->pulse1_duty][apu->pulse1_sequencer_value]) return 0;
    return (apu->pulse1_cvol_envelope ? apu->pulse1_envelope_volume_period: apu->pulse1_envelope_decay);
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
    apu->sample_period_fp = float_to_fp15((float)apu->clock_rate / apu->sample_rate);
    apu->sample_timestamp = 0;
    apu->sample_timestamp_fp = 0;
    apu->blip_last_sample = 0;
    // frame counter
    apu->sequencer_step = 0;
    apu->sequence_mode = false;
    apu->quarter_frame = false;
    apu->half_frame = false;
    apu->frame_accu_fp = 0;
    apu->frame_period_fp = float_to_fp15((float)apu->clock_rate / 240.0f);  // 240Hz frame counter
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
    fp15_t cycles_fp = fp15_round(apu->sample_accu_fp);
    uint32_t cycles = fp15_to_int(cycles_fp);
    // step cycles and sample, put result in blip buffer
    update_frame_counter(apu, cycles_fp);
    uint8_t p1 = update_pulse1(apu, cycles);
    uint8_t p2 = 0;
    uint8_t tr = 0;
    uint8_t ns = 0;
    uint8_t dm = 0;
    fp29_t f = mixer_pulse_table[p1 + p2] + mixer_tnd_table[3 * tr + 2 * ns + dm];
    int16_t s16 = fp29_to_s16(f);
    int16_t delta = s16 - apu->blip_last_sample;
    apu->blip_last_sample = s16;
    apu->sample_accu_fp -= cycles_fp;
    apu->sample_timestamp += cycles;
    blip_add_delta(apu->blip, apu->sample_timestamp, delta);
}


void nesapu_read_samples(nesapu_t *apu, int16_t *buf, uint32_t samples)
{
    unsigned int frame_cycles = (unsigned)blip_clocks_needed(apu->blip, samples);
    blip_end_frame(apu->blip, frame_cycles);
    blip_read_samples(apu->blip, (short *)buf, samples, 0);
    // frame_cycles can be less or more than actual clocked out, adjust
    VGM_PRINTF("Read buffer for %d cycles, actual clocked %d cycles\n", frame_cycles, apu->sample_timestamp);
    apu->sample_accu_fp += int_to_fp15(frame_cycles - apu->sample_timestamp);
    apu->sample_timestamp -= frame_cycles;
}


void nesapu_write_reg(nesapu_t *apu, uint16_t reg, uint8_t val)
{
    switch (reg)
    {
    // Pulse1 regs
    case 0x00:  // $4000: DDLC VVVV
        apu->reg4000 = val;
        apu->pulse1_duty = (val & 0xc0) >> 6;               // Duty (DD), index to tbl_pulse_waveform
        apu->pulse1_lchalt_evloop = (val & 0x20);           // Length counter halt or Envelope loop (L)
        apu->pulse1_cvol_envelope = (val & 0x10);           // Constant volume/envelope enable (C)
        apu->pulse1_envelope_volume_period = (val & 0xf);   // Volume or period of envelope (VVVV)
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
        apu->pulse1_timer_period |= val;                        // Pulse1 timer period low (TTTT TTTT)
        apu->pulse1_timer_divider = 0;                          // Timer value
        apu->pulse1_sweep_target = apu->pulse1_timer_period;    // Whenever the current period changes, the target period also changes
        break;
    case 0x03:  // $4003: llll lHHH     Pulse channel 1 length counter load and timer
        apu->reg4003 = val;
        apu->pulse1_length_counter = length_counter_table[(val & 0xf8) >> 3]; // Length counter load (lllll) 
        apu->pulse1_timer_period &= 0x00ff;
        apu->pulse1_timer_period |= ((uint16_t)(val & 0x07) << 8);  // Pulse1 timer period high 3 bits (TTT)
        apu->pulse1_timer_divider = 0;                              // Timer value
        apu->pulse1_sweep_target = apu->pulse1_timer_period;        // Whenever the current period changes, the target period also changes  
        // Side effects : The sequencer is immediately restarted at the first value of the current sequence. 
        // The envelope is also restarted. The period divider is not reset.
        apu->pulse1_envelope_start = true;
        apu->pulse1_sequencer_value = 0;
        break;
    case 0x15:  // $4015: ---D NT21
        if (val & 0x01)
        {
            apu->pulse1_enabled = true;
        }
        else
        {
            // Writing a zero to any of the channel enable bits will silence that channel and immediately set its length counter to 0.
            apu->pulse1_enabled = true;
            apu->pulse1_length_counter = 0;
        }
        break;
    // Frame counter
    case 0x17:  // $4017:
        apu->reg4017 = val;
        apu->sequence_mode = (val & 0x80);
        // Writing to $4017 with bit 7 set ($80) will immediately clock all of its controlled units at the beginning of the 5-step sequence;
        // with bit 7 clear, only the sequence is reset without clocking any of its units. 
        apu->sequencer_step = 0;
        apu->frame_accu_fp = 0;
        break;
    }
}

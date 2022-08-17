#include <stdlib.h>
#include <memory.h>
#include "platform.h"
#include "nesapu.h"

//
// Length Counter (Pulse1, Pulse2, Triangle, Noise)
// https://www.nesdev.org/wiki/APU_Length_Counter
//
static const unsigned int length_counter_table[32] = 
{
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};


// Pulse channel
// Duty lookup table, see wiki "Implementation details"
static const bool pulse_waveform_table[][8] = 
{
    { 0, 0, 0, 0, 0, 0, 0, 1 }, // 12.5%
    { 0, 0, 0, 0, 0, 0, 1, 1 }, // 25%
    { 0, 0, 0, 0, 1, 1, 1, 1 }, // 50%
    { 1, 1, 1, 1, 1, 1, 0, 0 }  // 25% negated
};


//
// Triangle wave table
// https://www.nesdev.org/wiki/APU_Triangle
//
static const unsigned int triangle_waveform_table[32] = 
{
    15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
};


//
// Mixer
// https://wiki.nesdev.org/w/index.php/APU_Mixer#Emulation
//
// Generate table using make_const_tables.c


static const q29_t mixer_pulse_table[31] =
{
             0,    6232609,   12315540,   18254120,   24053428,   29718306,   35253376,   40663044,
      45951532,   51122860,   56180880,   61129276,   65971580,   70711160,   75351248,   79894960,
      84345240,   88704968,   92976872,   97163576,  101267592,  105291368,  109237216,  113107392,
     116904048,  120629256,  124285008,  127873240,  131395816,  134854496,  138251008
};


static const q29_t mixer_tnd_table[203] =
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


//
// Noise channel
// https://www.nesdev.org/wiki/APU_Noise
static const uint16_t noise_timer_period_ntsc[16] =
{
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

static const uint16_t noise_timer_period_pal[16] =
{
    4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778
};



/**
 * @brief Count down timer
 * 
 * @param counter       Input initial counter value. When counting finish, return finish counter value.
 * @param period        Counting period.
 * @return unsigned int Number of times counter has reloaded from 0 to next period
 *
 */
static inline unsigned int timer_count_down(unsigned int *counter, unsigned int period, unsigned int cycles)
{
    unsigned int clocks = cycles / period;
    unsigned int extra = cycles % period;
    if (extra > *counter)
    {
        *counter = *counter + period - extra;
        ++clocks;
    }
    else
    {
        *counter = *counter - extra;
    }
    return clocks;
}


/**
 * @brief Count up timer
 * 
 * @param counter       Input initial counter value. When counting finish, return finish counter value
 * @param period        Countering period
 * @return unsigned int Number of times counter has reloaded from period to 0
 *
 */
static inline unsigned int timer_count_up(unsigned int *counter, unsigned int period, unsigned int cycles)
{
    unsigned int clocks = cycles / period;
    unsigned int extra = cycles % period;
    if (*counter + extra >= period)
    {
        *counter = *counter + extra - period;
        ++clocks;
    }
    else
    {
        *counter = *counter + extra;
    }
    return clocks;
}


static inline void update_frame_counter(nesapu_t *apu, unsigned int cycles)
{
    //
    // https://www.nesdev.org/wiki/APU_Frame_Counter
    //
    // Frame counter clocks 4 or 5 sequencer at 240Hz or 7457.38 CPU clocks.
    // Fixed point is used here to increase accuracy.
    if (apu->frame_force_clock)
    {
        apu->quarter_frame = true;
        apu->half_frame = true;
        apu->frame_force_clock = false;
    }
    else
    {
        apu->quarter_frame = false;
        apu->half_frame = false;
    }
    q16_t cycles_fp = int_to_q16(cycles);
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
                break;
            case 2:
                apu->quarter_frame = true;
                apu->half_frame = true;
                break;
            case 3:
                apu->quarter_frame = true;
                break;
            case 4:
                apu->quarter_frame = false;
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
                break;
            case 2:
                apu->quarter_frame = true;
                apu->half_frame = true;
                break;
            case 3:
                apu->quarter_frame = true;
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
 * https://www.nesdev.org/wiki/APU_Pulse
 *
 *                  Sweep -----> Timer
 *                    |            |
 *                    |            |
 *                    |            v
 *                    |        Sequencer   Length Counter
 *                    |            |             |
 *                    |            |             |
 *                    v            v             v
 * Envelope -------> Gate -----> Gate -------> Gate ---> (to mixer)
 */
static inline unsigned int update_pulse(nesapu_t *apu, int ch, unsigned int cycles)
{
    //
    // Clock envelope @ quater frame
    // https://www.nesdev.org/wiki/APU_Envelope
    if (apu->quarter_frame)
    {
        if (apu->pulse[ch].envelope_start)
        {
            apu->pulse[ch].envelope_decay = 15;
            apu->pulse[ch].envelope_value = apu->pulse[ch].volume_envperiod;
            apu->pulse[ch].envelope_start = false;
        }
        else
        {
            if (apu->pulse[ch].envelope_value)
            {
                --(apu->pulse[ch].envelope_value);
            }
            else
            {
                apu->pulse[ch].envelope_value = apu->pulse[ch].volume_envperiod;
                if (apu->pulse[ch].envelope_decay)
                {
                    --(apu->pulse[ch].envelope_decay);
                }
                else if (apu->pulse[ch].lenhalt_envloop)
                {
                    apu->pulse[ch].envelope_decay = 15;
                }
            }
        }
    }
    //
    // Clock sweep unit @ half frame 
    // https://www.nesdev.org/wiki/APU_Sweep
    //
    if (apu->half_frame)
    {
        // 1. If the divider's counter is zero, the sweep is enabled, and the sweep unit is not muting the channel: The pulse's period is set to the target period.
        // 2. If the divider's counter is zero or the reload flag is true: The divider counter is set to P and the reload flag is cleared.
        //    Otherwise, the divider counter is decremented.
        if (apu->pulse[ch].sweep_reload)
        {
            apu->pulse[ch].sweep_value = apu->pulse[ch].sweep_period;
            apu->pulse[ch].sweep_reload = false;
        }
        else 
        {
            if (apu->pulse[ch].sweep_value)
            {
                --(apu->pulse[ch].sweep_value);
            } 
            else if (apu->pulse[ch].sweep_enabled && apu->pulse[ch].sweep_shift)  // TODO: Not sure if pulse_sweep_enabled must be true
            {
                // Calculate target period.
                unsigned int c = apu->pulse[ch].timer_period >> apu->pulse[ch].sweep_shift;
                if (apu->pulse[ch].sweep_negate)
                {
                    if (ch == 0) apu->pulse[ch].sweep_target -= c + 1;
                    else if (ch == 1) apu->pulse[ch].sweep_target -= c;
                }
                else
                {
                    apu->pulse[ch].sweep_target += c;
                }
                if (!apu->pulse[ch].sweep_timer_mute)
                {
                    // When the sweep unit is muting a pulse channel, the channel's current period remains unchanged.
                    apu->pulse[ch].timer_period = apu->pulse[ch].sweep_target;
                }
                // Two conditions cause the sweep unit to mute the channel:
                // 1. If the current period is less than 8, the sweep unit mutes the channel.
                // 2. If at any time the target period is greater than $7FF, the sweep unit mutes the channel.
                apu->pulse[ch].sweep_timer_mute = ((apu->pulse[ch].timer_period < 8) || (apu->pulse[ch].sweep_target > 0x7ff));
                // Reload divider
                apu->pulse[ch].sweep_value = apu->pulse[ch].sweep_period;
            }
        }
    }
    //
    // Clock pulse channel timer and update sequencer
    // https://www.nesdev.org/wiki/APU_Pulse
    // Timer counting downwards from 0 at every other CPU cycle. So we set timer limit to  2x (timer_period + 1).
    unsigned int seq_clk = timer_count_down(&(apu->pulse[ch].timer_value), (apu->pulse[ch].timer_period + 1) << 1, cycles);
    if (seq_clk) timer_count_down(&(apu->pulse[ch].sequencer_value), 8, seq_clk);
    // 
    // Clock length counter @ half frame if not halted
    // https://www.nesdev.org/wiki/APU_Length_Counter
    if (!apu->pulse[ch].lenhalt_envloop && apu->pulse[ch].length_value && apu->half_frame)
    {
        --(apu->pulse[ch].length_value);
    }
    // Determine Pulse channel output
    if (!apu->pulse[ch].enabled) return 0;
    if (!apu->pulse[ch].length_value) return 0;
    if (apu->pulse[ch].sweep_timer_mute) return 0;
    if (!pulse_waveform_table[apu->pulse[ch].duty][apu->pulse[ch].sequencer_value]) return 0;
    return (apu->pulse[ch].constant_volume ? apu->pulse[ch].volume_envperiod : apu->pulse[ch].envelope_decay);
}


/*
 * https://www.nesdev.org/wiki/APU_Triangle
 *
 *       Linear Counter    Length Counter
 *              |                |
 *              v                v
 *  Timer ---> Gate ----------> Gate ---> Sequencer ---> (to mixer)
 */
static inline unsigned int update_triangle(nesapu_t *apu, unsigned int cycles)
{
    //
    // Clock linear counter @ quater frame
    // https://www.nesdev.org/wiki/APU_Triangle
    if (apu->quarter_frame)
    {
        // In order:
        // 1. If the linear counter reload flag is set, the linear counter is reloaded with the counter reload value,
        //    otherwise if the linear counter is non-zero, it is decremented.
        // 2. If the control flag is clear, the linear counter reload flag is cleared.
        if (apu->triangle_linear_reload)
        {
            apu->triangle_linear_value = apu->triangle_linear_period;
        }
        else if (apu->triangle_linear_value)
        {
            --(apu->triangle_linear_value);
        }
        if (!apu->triangle_lnrctl_lenhalt)
        {
            apu->triangle_linear_reload = false;
        }
    }
    // 
    // Clock length counter @ half frame if not halted
    // https://www.nesdev.org/wiki/APU_Length_Counter
    if (!apu->triangle_lnrctl_lenhalt && apu->triangle_length_value && apu->half_frame)
    {
        --(apu->triangle_length_value);
    }
    // Clock triangle channel timer and return
    // Trick: Just keep sequencer unchanged if the channel should be silenced, it minizes pop
    if (apu->triangle_enabled 
        &&
        !apu->triangle_timer_period_bad
        &&
        apu->triangle_length_value
        &&
        apu->triangle_linear_value)
    {
        unsigned int seq_clk = timer_count_down(&(apu->triangle_timer_value), apu->triangle_timer_period + 1, cycles);
        if (seq_clk) timer_count_up(&(apu->triangle_sequencer_value), 32, seq_clk);
    }
    return triangle_waveform_table[apu->triangle_sequencer_value];
}


/*
 * https://www.nesdev.org/wiki/APU_Noise
 * 
 *     Timer --> Shift Register   Length Counter
 *                    |                |
 *                    v                v
 * Envelope -------> Gate ----------> Gate --> (to mixer)
 */
static inline unsigned int update_noise(nesapu_t* apu, unsigned int cycles)
{
    //
    // Clock envelope @ quater frame
    // https://www.nesdev.org/wiki/APU_Envelope
    if (apu->quarter_frame)
    {
        if (apu->noise_envelope_start)
        {
            apu->noise_envelope_decay = 15;
            apu->noise_envelope_value = apu->noise_volume_envperiod;
            apu->noise_envelope_start = false;
        }
        else
        {
            if (apu->noise_envelope_value)
            {
                --(apu->noise_envelope_value);
            }
            else
            {
                apu->noise_envelope_value = apu->noise_volume_envperiod;
                if (apu->noise_envelope_decay)
                {
                    --(apu->noise_envelope_decay);
                }
                else if (apu->noise_lenhalt_envloop)
                {
                    apu->noise_envelope_decay = 15;
                }
            }
        }
    }
    // Clock noise channel timer
    unsigned int seq_clk = timer_count_down(&(apu->noise_timer_value), apu->noise_timer_period + 1, cycles);
    for (; seq_clk > 0; --seq_clk)
    {
        // When the timer clocks the shift register, the following occur in order:
        // 1) Feedback is calculated as the exclusive-OR of bit 0 and one other bit: bit 6 if Mode flag is set, otherwise bit 1.
        uint16_t feedback = (apu->noise_shift_reg & 0x0001) ^ (apu->noise_mode ? ((apu->noise_shift_reg >> 6) & 0x0001) : ((apu->noise_shift_reg >> 1) & 0x0001));
        // 2) The shift register is shifted right by one bit.
        apu->noise_shift_reg = apu->noise_shift_reg >> 1;
        // 3) Bit 14, the leftmost bit, is set to the feedback calculated earlier.
        apu->noise_shift_reg |= (feedback << 14);
    }
    // 
    // Clock length counter @ half frame if not halted
    // https://www.nesdev.org/wiki/APU_Length_Counter
    if (!apu->noise_lenhalt_envloop && apu->noise_length_value && apu->half_frame)
    {
        --(apu->noise_length_value);
    }
    // return value
    if (!apu->noise_enabled) return 0;
    // The mixer receives the current envelope volume except when bit 0 of the shift register is set, or the length counter is 0
    if (apu->noise_shift_reg & 0x01) return 0;
    if (!apu->noise_length_value) return 0;
    return (apu->noise_constant_volume ? apu->noise_volume_envperiod : apu->noise_envelope_decay);
}


nesapu_t * nesapu_create(bool format, unsigned int clock, unsigned int srate, unsigned int max_sample_count)
{
    nesapu_t *apu = (nesapu_t*)VGM_MALLOC(sizeof(nesapu_t));
    if (NULL == apu)
        return NULL;
    memset(apu, 0, sizeof(nesapu_t));
    apu->format = format;
    apu->clock_rate = clock;
    apu->sample_rate = srate;
    // blip
    apu->blip = blip_new((int)max_sample_count);
    blip_set_rates(apu->blip, apu->clock_rate, apu->sample_rate);
    apu->frame_period_fp = float_to_q16((float)apu->clock_rate / 240.0f);  // 240Hz frame counter period
    nesapu_reset(apu);
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


void nesapu_reset(nesapu_t* apu)
{
    // frame counter
    apu->sequencer_step = 0;
    apu->sequence_mode = false;
    apu->quarter_frame = false;
    apu->half_frame = false;
    apu->frame_accu_fp = 0;
    //
    // Enable Channel NT21 on startup
    // From: https://wiki.nesdev.com/w/index.php/Talk:NSF
    // Regarding 4015h, well... it's empirical. My experience says that setting 4015h to 0Fh
    // is required in order to get *a lot of* tunes starting playing. I don't remember of *any* broken
    // tune by setting such value. So, it's recommended *to follow* such thing. --Zepper 14:25, 29 March 2012 (PDT)
    // 
    // Pulse1
    apu->pulse[0].enabled = true;
    apu->pulse[0].duty = 0;
    apu->pulse[0].lenhalt_envloop = false;
    apu->pulse[0].constant_volume = false;
    apu->pulse[0].envelope_start = false;
    apu->pulse[0].envelope_value = 0;
    apu->pulse[0].volume_envperiod = 0;
    apu->pulse[0].sweep_enabled = false;
    apu->pulse[0].sweep_period = 0;
    apu->pulse[0].sweep_value = 0;
    apu->pulse[0].sweep_negate = false;
    apu->pulse[0].sweep_shift = 0;
    apu->pulse[0].sweep_target = 0;
    apu->pulse[0].sweep_reload = false;
    apu->pulse[0].timer_period = 0;
    apu->pulse[0].timer_value = 0;
    apu->pulse[0].sequencer_value = 0;
    apu->pulse[0].sweep_timer_mute = true;
    // Pulse2
    apu->pulse[1].enabled = true;
    apu->pulse[1].duty = 0;
    apu->pulse[1].lenhalt_envloop = false;
    apu->pulse[1].constant_volume = false;
    apu->pulse[1].envelope_start = false;
    apu->pulse[1].envelope_value = 0;
    apu->pulse[1].volume_envperiod = 0;
    apu->pulse[1].sweep_enabled = false;
    apu->pulse[1].sweep_period = 0;
    apu->pulse[1].sweep_value = 0;
    apu->pulse[1].sweep_negate = false;
    apu->pulse[1].sweep_shift = 0;
    apu->pulse[1].sweep_target = 0;
    apu->pulse[1].sweep_reload = false;
    apu->pulse[1].timer_period = 0;
    apu->pulse[1].timer_value = 0;
    apu->pulse[1].sequencer_value = 0;
    apu->pulse[1].sweep_timer_mute = true;
    // Triangle
    apu->triangle_enabled = true;
    apu->triangle_length_value = 0;
    apu->triangle_lnrctl_lenhalt = false;
    apu->triangle_linear_period = 0;
    apu->triangle_linear_value = 0;
    apu->triangle_linear_reload = false;
    apu->triangle_timer_period = 0;
    apu->triangle_timer_period_bad = true;
    apu->triangle_timer_value = 0;
    apu->triangle_sequencer_value = 0;
    // Noise
    apu->noise_enabled = true;
    apu->noise_lenhalt_envloop = false;
    apu->noise_constant_volume = false;
    apu->noise_constant_volume = 0;
    apu->noise_volume_envperiod = 0;
    apu->noise_mode = false;
    apu->noise_envelope_start = false;
    apu->noise_envelope_decay = 0;
    apu->noise_envelope_value = 0;
    apu->noise_length_value = 0;
    apu->noise_timer_period = 0;
    apu->noise_timer_value = 0;
    apu->noise_shift_reg = 1;
}


static inline int16_t nesapu_run_and_sample(nesapu_t *apu, unsigned int cycles)
{
    update_frame_counter(apu, cycles);
    unsigned int p1 = update_pulse(apu, 0, cycles);
    unsigned int p2 = update_pulse(apu, 1, cycles);
    unsigned int tr = update_triangle(apu, cycles);
    unsigned int ns = update_noise(apu, cycles);
    unsigned int dm = 0;
    q29_t f = mixer_pulse_table[p1 + p2] + mixer_tnd_table[3 * tr + 2 * ns + dm];
    int16_t s16 = q29_to_s16(f);
    return s16;
}


void nesapu_get_samples(nesapu_t *apu, int16_t *buf, unsigned int samples)
{
    unsigned int cycles = (unsigned int)blip_clocks_needed(apu->blip, (int)samples);
    unsigned int period = cycles / samples; // rough sampling period. blip helps resampling
    unsigned int time = 0;
    int16_t s;
    int delta;
    while (cycles > period)
    {
        // run period clocks
        s = nesapu_run_and_sample(apu, period);
        delta = s - apu->blip_last_sample;
        apu->blip_last_sample = s;
        time += period;
        blip_add_delta(apu->blip, time, delta);
        cycles -= period;
    }
    // run remaining clocks
    s = nesapu_run_and_sample(apu, cycles);
    delta = s - apu->blip_last_sample;
    apu->blip_last_sample = s;
    time += cycles;
    blip_add_delta(apu->blip, time, delta);
    blip_end_frame(apu->blip, time);
    blip_read_samples(apu->blip, (short *)buf, (int)samples, 0);
}


void nesapu_write_reg(nesapu_t *apu, uint16_t reg, uint8_t val)
{
    switch (reg)
    {
    // Pulse1 regs
    case 0x00:  // $4000: DDLC VVVV, Duty (D), envelope loop / length counter halt (L), constant volume (C), volume/envelope (V)
        apu->pulse[0].duty = (val & 0xc0) >> 6;         // Duty (DD), index to tbl_pulse_waveform
        apu->pulse[0].lenhalt_envloop = (val & 0x20);     // Length counter halt or Envelope loop (L)
        apu->pulse[0].constant_volume = (val & 0x10);   // Constant volume (true) or Envelope enable (false) (C)
        apu->pulse[0].volume_envperiod = (val & 0xf);    // Volume or period of envelope (VVVV)
        break;
    case 0x01:  // $4001: EPPP NSSS, Sweep unit: enabled (E), period (P), negate (N), shift (S)
        apu->pulse[0].sweep_enabled = (val & 0x80);     // Sweep enable (E)
        apu->pulse[0].sweep_period = (val & 0x70) >> 4; // Sweep period (PPP)
        apu->pulse[0].sweep_negate = (val & 0x08);      // Sweep negate (N)
        apu->pulse[0].sweep_shift = val & 0x07;         // Sweep shift (SSS)
        apu->pulse[0].sweep_reload = true;              // Side effects : Sets reload flag 
        break;
    case 0x02:  // $4002: LLLL LLLL, Timer low (T)
        apu->pulse[0].timer_period &= 0xff00;
        apu->pulse[0].timer_period |= val;                          // Pulse1 timer period low (TTTT TTTT)
        apu->pulse[0].sweep_target = apu->pulse[0].timer_period;    // Whenever the current period changes, the target period also changes
        apu->pulse[0].sweep_timer_mute = ((apu->pulse[0].timer_period < 8) || (apu->pulse[0].sweep_target > 0x7ff));
        break;
    case 0x03:  // $4003: llll lHHH, Length counter load (L), timer high (T)
        apu->pulse[0].length_value = length_counter_table[(val & 0xf8) >> 3]; // Length counter load (lllll) 
        apu->pulse[0].timer_period &= 0x00ff;
        apu->pulse[0].timer_period |= ((unsigned int)(val & 0x07)) << 8;        // Pulse1 timer period high 3 bits (TTT)
        apu->pulse[0].sweep_target = apu->pulse[0].timer_period;    // Whenever the current period changes, the target period also changes
        apu->pulse[0].sweep_timer_mute = ((apu->pulse[0].timer_period < 8) || (apu->pulse[0].sweep_target > 0x7ff));
        // Side effects : The sequencer is immediately restarted at the first value of the current sequence. 
        // The envelope is also restarted. The period divider is not reset.
        apu->pulse[0].envelope_start = true;
        apu->pulse[0].sequencer_value = 0;
        break;
    // Pulse2
    case 0x04:  // $4004: DDLC VVVV, Duty (D), envelope loop / length counter halt (L), constant volume (C), volume/envelope (V)
        apu->pulse[1].duty = (val & 0xc0) >> 6;               // Duty (DD), index to tbl_pulse_waveform
        apu->pulse[1].lenhalt_envloop = (val & 0x20);         // Length counter halt or Envelope loop (L)
        apu->pulse[1].constant_volume = (val & 0x10);         // Constant volume (true) or Envelope enable (false) (C)
        apu->pulse[1].volume_envperiod = (val & 0xf);         // Volume or period of envelope (VVVV)
        break;
    case 0x05:  // $4005: EPPP NSSS, Sweep unit: enabled (E), period (P), negate (N), shift (S)
        apu->pulse[1].sweep_enabled = (val & 0x80);           // Sweep enable (E)
        apu->pulse[1].sweep_period = (val & 0x70) >> 4;       // Sweep period (PPP)
        apu->pulse[1].sweep_negate = (val & 0x08);            // Sweep negate (N)
        apu->pulse[1].sweep_shift = val & 0x07;               // Sweep shift (SSS)
        apu->pulse[1].sweep_reload = true;                    // Side effects : Sets reload flag 
        break;
    case 0x06:  // $4006: LLLL LLLL, Timer low (T)
        apu->pulse[1].timer_period &= 0xff00;
        apu->pulse[1].timer_period |= val;                          // Pulse2 timer period low (TTTT TTTT)
        apu->pulse[1].sweep_target = apu->pulse[1].timer_period;    // Whenever the current period changes, the target period also changes
        apu->pulse[1].sweep_timer_mute = ((apu->pulse[1].timer_period < 8) || (apu->pulse[1].sweep_target > 0x7ff));
        break;
    case 0x07:  // $4007: llll lHHH, Length counter load (L), timer high (T)
        apu->pulse[1].length_value = length_counter_table[(val & 0xf8) >> 3];   // Length counter load (lllll) 
        apu->pulse[1].timer_period &= 0x00ff;
        apu->pulse[1].timer_period |= ((unsigned int)(val & 0x07)) << 8;          // Pulse2 timer period high 3 bits (TTT)
        apu->pulse[1].sweep_target = apu->pulse[1].timer_period;    // Whenever the current period changes, the target period also changes
        apu->pulse[1].sweep_timer_mute = ((apu->pulse[1].timer_period < 8) || (apu->pulse[1].sweep_target > 0x7ff));
        // Side effects : The sequencer is immediately restarted at the first value of the current sequence. 
        // The envelope is also restarted. The period divider is not reset.
        apu->pulse[1].envelope_start = true;
        apu->pulse[1].sequencer_value = 0;
        break;
    // Triangle
    case 0x08:  // $4008: CRRR RRRR, Linear counter setup
        apu->triangle_lnrctl_lenhalt = (val & 0x80);    // Control flag / length counter halt
        apu->triangle_linear_period = (val & 0x7f);     // Linear counter reload value 
        break;
    case 0x09:  // Not used
        break;
    case 0x0a:  // $400A: LLLL LLLL, Timer low (write) 
        apu->triangle_timer_period &= 0xff00;
        apu->triangle_timer_period |= val;  // Timer low (LLLL LLLL)
        apu->triangle_timer_period_bad = ((apu->triangle_timer_period >= 0x7fe) || (apu->triangle_timer_period <= 1));
        break;
    case 0x0b:  // $400B: llll lHHH, Length counter load and timer high
        apu->triangle_timer_period &= 0x00ff;
        apu->triangle_timer_period |= ((unsigned int)(val & 0x07)) << 8;        // Timer period high (HHH)
        apu->triangle_timer_period_bad = ((apu->triangle_timer_period >= 0x7fe) || (apu->triangle_timer_period <= 1));
        apu->triangle_length_value = length_counter_table[(val & 0xf8) >> 3]; // Length counter load (lllll)
        apu->triangle_linear_reload = true;  // site effect: sets the linear counter reload flag 
        //apu->triangle_timer_value = apu->triangle_timer_period;
        break;
    // Noise
    case 0x0c:  // $400C: --LC VVVV, Length counter halt, constant volume/envelope flag, and volume/envelope divider period
        apu->noise_lenhalt_envloop = (val & 0x20);      // Envelope loop/length counter halt (L)
        apu->noise_constant_volume = (val & 0x10);      // Constant volume (C)
        apu->noise_volume_envperiod = (val & 0x0f);     // Volume / Envelope period (VVVV)
        break;
    case 0x0d:  // Not used
        break;
    case 0x0e:  // $400E: M--- PPPP, Mode and period
        apu->noise_mode = (val & 0x80); // Mode Flag (M)
        if (apu->format)    // PAL
        {
            apu->noise_timer_period = noise_timer_period_pal[val & 0x0f];   // Noise period (PPPP) 
        }
        else
        {
            apu->noise_timer_period = noise_timer_period_ntsc[val & 0x0f];  // Noise period (PPPP)
        }
        break;
    case 0x0f:  // $400F: llll l---, Length counter load and envelope restart 
        apu->noise_length_value = length_counter_table[(val & 0xf8) >> 3];  // Length counter value (lllll)
        apu->noise_envelope_start = true;   //Side effects: Sets start flag
        break;
    // Status
    case 0x15:  // $4015: ---D NT21, Enable DMC (D), noise (N), triangle (T), and pulse channels (2/1)
        if (val & 0x01)
        {
            apu->pulse[0].enabled = true;
        }
        else
        {
            // Writing a zero to any of the channel enable bits will silence that channel and immediately set its length counter to 0.
            apu->pulse[0].enabled = false;
            apu->pulse[0].length_value = 0;
        }
        if (val & 0x02)
        {
            apu->pulse[1].enabled = true;
        }
        else
        {
            // Writing a zero to any of the channel enable bits will silence that channel and immediately set its length counter to 0.
            apu->pulse[1].enabled = false;
            apu->pulse[1].length_value = 0;
        }
        if (val & 0x04)
        {
            apu->triangle_enabled = true;
        }
        else
        {
            apu->triangle_enabled = false;
            apu->triangle_length_value = 0;
        }
        if (val & 0x08)
        {
            apu->noise_enabled = true;
        }
        else
        {
            // Writing a zero to any of the channel enable bits will silence that channel and immediately set its length counter to 0.
            apu->noise_enabled = false;
            apu->noise_length_value = 0;
        }
        break;
    // Frame counter
    case 0x17:  // $4017:
        apu->sequence_mode = (val & 0x80);
        apu->sequencer_step = 0;
        apu->frame_accu_fp = 0;
        // TODO: Needs test
        // Writing to $4017 with bit 7 set ($80) will immediately clock all of its controlled units at the beginning of the 5-step sequence;
        // with bit 7 clear, only the sequence is reset without clocking any of its units. 
        if (apu->sequence_mode)
        {
            apu->frame_force_clock = true;
        }
        else
        {
            apu->frame_force_clock = false;
        }
        break;
    }
}

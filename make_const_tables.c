#include <stdio.h>
#include "fixedpoint.h"


static void apu_mixer_dump()
{
    // Use this to generate the table above
    int i;
    float pulse_table[31];
    for (i = 0; i < 31; ++i)
        pulse_table[i] = 95.52f / (8128.0f / i + 100.0f);
    printf("static const float mixer_pulse_table[31] =\n{\n");
    for (i = 0; i < 30; i++)
    {
        if (i % 8 == 0) printf("    ");
        printf("%.8ff, ", pulse_table[i]);
        if ((i + 1) % 8 == 0)
            printf("\n");
    }
    printf("%.8ff\n};\n", pulse_table[i]);
    float tnd_table[203];
    for (i = 0; i < 203; ++i)
        tnd_table[i] = 163.67f / (24329.0f / i + 100.0f);
    printf("\n\nstatic const float mixer_tnd_table[203] =\n{\n");
    for (i = 0; i < 202; i++)
    {
        if (i % 8 == 0) printf("    ");
        printf("%.8ff, ", tnd_table[i]);
        if ((i + 1) % 8 == 0)
            printf("\n");
    }
    printf("%.8ff\n};\n", tnd_table[i]);
}



static void apu_mixer_dump_fp()
{
    // Use this to generate the table above
    int i;
    float pulse_table[31];
    for (i = 0; i < 31; ++i)
        pulse_table[i] = 95.52f / (8128.0f / i + 100.0f);
    printf("static const q29_t mixer_pulse_table[31] =\n{\n");
    for (i = 0; i < 30; i++)
    {
        if (i % 8 == 0) printf("    ");
        printf("%10d, ", float_to_q29(pulse_table[i]));
        if ((i + 1) % 8 == 0)
            printf("\n");
    }
    printf("%10d\n};\n", float_to_q29(pulse_table[i]));
    float tnd_table[203];
    for (i = 0; i < 203; ++i)
        tnd_table[i] = 163.67f / (24329.0f / i + 100.0f);
    printf("\n\nstatic const q29_t mixer_tnd_table[203] =\n{\n");
    for (i = 0; i < 202; i++)
    {
        if (i % 8 == 0) printf("    ");
        printf("%10d, ", float_to_q29(tnd_table[i]));
        if ((i + 1) % 8 == 0)
            printf("\n");
    }
    printf("%10d\n};\n", float_to_q29(tnd_table[i]));
}

#define NESAPU_FADE_STEPS 256
static void fade_dump_fp()
{
    int i;
    int max = NESAPU_FADE_STEPS - 1;
    float fade_table[NESAPU_FADE_STEPS];
    for (i = 0; i < NESAPU_FADE_STEPS; ++i)
    {
        fade_table[i] = ((float)i) / max;
    }
    printf("static const q29_t fadeout_table[NESAPU_FADE_STEPS] =\n{\n");
    for (i = 0; i < NESAPU_FADE_STEPS; ++i)
    {
        if (i % 8 == 0) printf("    ");
        printf("%10d, ", float_to_q29(fade_table[i]));
        if ((i + 1) % 8 == 0)
            printf("\n");
    }
    if (i % 8)
        printf("%10d\n};\n", float_to_q29(fade_table[i]));
    else
        printf("};\n");
}


int main()
{
    apu_mixer_dump();
    apu_mixer_dump_fp();
    fade_dump_fp();
    return 0;
}
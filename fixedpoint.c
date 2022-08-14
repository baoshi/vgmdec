#include "fixedpoint.h"

// Convert sound sample in fp29_t to S16
int16_t fp29_to_s16(fp29_t x)
{
    int32_t t = (x >> 13) - 32768;
    int16_t s;
    if (t > 32767) s = 32767;
    else if (t < -32768) s = -32768;
    else s = t;
    return s;
}
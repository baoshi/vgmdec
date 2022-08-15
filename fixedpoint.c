#include "fixedpoint.h"

// Convert sound sample in q29_t to S16
int16_t q29_to_s16(q29_t x)
{
    int32_t t = (x >> 13) - 32768;
    int16_t s;
    if (t > 32767) s = 32767;
    else if (t < -32768) s = -32768;
    else s = (int16_t)t;
    return s;
}
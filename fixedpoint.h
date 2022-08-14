#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t fp16_t;

// 0000 0000 0000 0000. 0000 0000 0000 0000
// 16:16 fixed number, +/- 32768

#define int_to_fp16(x) ((fp16_t)((x) << 16))
#define float_to_fp16(x) ((fp16_t)((x) * (float)(1 << 16)))
#define fp16_to_float(x) ((x) / (float)(1 << 16))
#define fp16_round(x) ((x + 0x00008000) & 0xffff0000)
#define fp16_to_int(x) ((x) >> 16)



typedef int32_t fp15_t;

// 0000 0000 0000 0000 0. 000 0000 0000 0000
// 17:15 fixed number, +/- 65536

#define int_to_fp15(x) ((fp15_t)((x) << 15))
#define float_to_fp15(x) ((fp15_t)((x) * (float)(1 << 15)))
#define fp15_to_float(x) ((x) / (float)(1 << 15))
#define fp15_round(x) ((x + 0x00004000) & 0xffff8000)
#define fp15_to_int(x) ((x) >> 15)

// All NES APU mathematics are dealing with float number within range 0.0-1.0. Use 3:29 fixed point
typedef int32_t fp29_t;
#define float_to_fp29(x) ((fp29_t)((x) * (float)(1 << 29)))
#define fp29_to_float(x) ((x) / (float)(1 << 29))
#define fp29_mul(x, y) (((x)>>15)*((y)>>14))
int16_t fp29_to_s16(fp29_t x);

#ifdef __cplusplus
}
#endif
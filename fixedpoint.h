#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t fp16_t;

// 0000 0000 0000 0000. 0000 0000 0000 0000
// 16:16 fixed number, +/- 32768

#define float_to_fp16(x) ((fp16_t)((x) * (float)(1 << 16)))
#define fp16_to_float(x) ((x) / (float)(1 << 16))
#define fp16_round(x) ((x + 0x00008000) & 0xffff0000)
#define fp16_to_int(x) ((x) >> 16)

#ifdef __cplusplus
}
#endif
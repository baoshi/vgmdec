#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t fixed4_t;

// 0000 0000 0000 0000 0000 0000 0000 .0000
// 28:4 fixed number, ~ +-134217728

#define float_to_fixed4(x) ((fixed4_t)((x) * (float)(1 << 4)))
#define fixed4_to_float(x) ((x) / (float)(1 << 4))


#ifdef __cplusplus
}
#endif
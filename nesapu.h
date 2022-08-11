#pragma once
#include <stdint.h>
#include <stdbool.h>



#ifdef __cplusplus
extern "C" {
#endif

typedef struct nesapu_s
{
    bool format;        // true: PAL, false: NTSC
    uint32_t clock_rate;
    uint32_t sample_rate;
    // registers
    uint8_t reg4000, reg4001, reg4002, reg4003; // Pulse1
    uint8_t reg4004, reg4005, reg4006, reg4007; // Pulse 2
    uint8_t reg4008, reg4009, reg400a, reg400b; // Triangle
    uint8_t reg400c, reg400d, reg400e, reg400f; // Noise
    uint8_t reg4010, reg4011, reg4012, reg4013; // DMC
    uint8_t reg4017;    // All 
} nesapu_t;


nesapu_t * nesapu_create(bool format, uint32_t clock, uint32_t srate);
void nesapu_destroy(nesapu_t *a);

#ifdef __cplusplus
}
#endif

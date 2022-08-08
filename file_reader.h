#pragma once

// General interface for file reader

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct file_reader_s file_reader_t;

struct file_reader_s
{
    file_reader_t* self;
    uint32_t(*read)(file_reader_t* self, uint8_t* out, uint32_t offset, uint32_t length);
    uint32_t(*size)(file_reader_t* self);
};


#ifdef __cplusplus
}
#endif
#pragma once

#include "file_reader.h"

#ifdef __cplusplus
extern "C" {
#endif


#define CFR_MEASURE_CACHE_PERFORMACE


file_reader_t * cfr_create(const char* fn, size_t cache_size);

void cfr_destroy(file_reader_t* cfr);

#ifdef CFR_MEASURE_CACHE_PERFORMACE

void cfr_show_cache_status(file_reader_t* cfr);

#else

#define cfr_show_cache_status(x) void(x)

#endif


#ifdef __cplusplus
}
#endif
#pragma once

#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#define VGM_PRINTF(...)
#define VGM_PRINTINF(...)
#define VGM_PRINTDBG(...) printf(__VA_ARGS__)
#define VGM_PRINTERR(...) fprintf(stderr, __VA_ARGS__)
#define VGM_ASSERT assert
#define VGM_MALLOC malloc
#define VGM_FREE free
#pragma once

#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))

#include<stdio.h>
#include <stdlib.h>
#define VGM_PRINTF(...) printf(__VA_ARGS__)
#define VGM_PRINTINF(...) printf(__VA_ARGS__)
#define VGM_PRINTDBG(...) printf(__VA_ARGS__)
#define VGM_PRINTERR(...) fprintf(stderr, __VA_ARGS__)
#define VGM_MALLOC malloc
#define VGM_FREE free
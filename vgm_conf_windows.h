#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


#ifdef _MSC_VER
# define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
# else
#  error "MSVC required"
# endif


#define VGM_PRINTF(...) printf(__VA_ARGS__)
#define VGM_PRINTINF(...) printf(__VA_ARGS__)
#define VGM_PRINTDBG(...) printf(__VA_ARGS__)
#define VGM_PRINTERR(...) fprintf(stderr, __VA_ARGS__)
#define VGM_ASSERT assert
#define VGM_MALLOC malloc
#define VGM_FREE free

#define NESAPU_USE_BLIPBUF 1

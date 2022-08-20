#pragma once


#ifdef __GNUC__
# define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
# define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

#ifdef VGMDEC_EMBEDDED
# include "my_debug.h"
# include "my_mem.h"
# define VGM_PRINTF(...) MY_LOGI(__VA_ARGS__)
# define VGM_PRINTINF(...) MY_LOGI(__VA_ARGS__)
# define VGM_PRINTDBG(...) MY_LOGD(__VA_ARGS__)
# define VGM_PRINTERR(...) MY_LOGE(__VA_ARGS__)
# define VGM_MALLOC MY_MALLOC
# define VGM_FREE MY_FREE
#elif
# include<stdio.h>
# include <stdlib.h>
# define VGM_PRINTF(...) printf(__VA_ARGS__)
# define VGM_PRINTINF(...) printf(__VA_ARGS__)
# define VGM_PRINTDBG(...) printf(__VA_ARGS__)
# define VGM_PRINTERR(...) fprintf(stderr, __VA_ARGS__)
# define VGM_MALLOC malloc
# define VGM_FREE free
#endif

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif
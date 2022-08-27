#pragma once


#if defined(VGM_LINUX)
# include "vgm_conf_linux.h"
#elif defined(VGM_WINDOWS)
# include "vgm_conf_windows.h"
#elif defined(VGM_MACOSX)
# include "vgm_conf_macosx.h"
#else
#ifdef VGM_CONF_CUSTOM
# include VGM_CONF_CUSTOM
#else
# error "Custom configuration for VGM needed" 
#endif
#endif

#define VGM_FILE_CACHE_SIZE     2048

#define NESAPU_USE_BLIPBUF      1
#define NESAPU_MAX_SAMPLE_SIZE  1500
#define NESAPU_RAM_CACHE_SIZE   4096

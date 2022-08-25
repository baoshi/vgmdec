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


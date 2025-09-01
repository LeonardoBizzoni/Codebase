#ifndef OS_GFX_WIN32_INC_H
#define OS_GFX_WIN32_INC_H

#include "os_gfx_win32_core.c"

#if GFX_OPENGL
#  include "OpenGL/gfx_opengl_win32.c"
#else
#  error invalid graphics api
#endif

#endif

#ifndef OS_GFX_WIN32_H
#define OS_GFX_WIN32_H

#include "os_gfx_win32_core.h"

#if GFX_OPENGL
#  include "OpenGL/gfx_opengl_win32.h"
#else
#  error invalid graphics api
#endif

#endif

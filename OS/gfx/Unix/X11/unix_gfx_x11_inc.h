#ifndef UNIX_GFX_X11_INC_H
#define UNIX_GFX_X11_INC_H

#include "unix_gfx_x11_core.h"

#if GFX_OPENGL
#  include "OpenGL/gfx_opengl_unix_x11.h"
#elif GFX_VULKAN
#  include "Vulkan/gfx_vulkan_unix_x11.h"
#else
#  error invalid graphics api
#endif

#endif

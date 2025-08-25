#ifndef UNIX_GFX_WAYLAND_INC_H
#define UNIX_GFX_WAYLAND_INC_H

#include "unix_gfx_wayland_core.h"

#if GFX_OPENGL
#  include "OpenGL/gfx_opengl_unix_wayland.h"
#elif GFX_VULKAN
#  include "Vulkan/gfx_vulkan_unix_wayland.h"
#else
#  error invalid graphics api
#endif

#endif

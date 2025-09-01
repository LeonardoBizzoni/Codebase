#include "unix_gfx_x11_core.c"

#if GFX_OPENGL
#  include "OpenGL/gfx_opengl_unix_x11.c"
#elif GFX_VULKAN
#  include "Vulkan/gfx_vulkan_unix_x11.c"
#else
#  error invalid graphics api
#endif

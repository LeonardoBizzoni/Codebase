#include "xdg-shell-protocol.c"
#include "unix_gfx_wayland_core.c"

#if GFX_OPENGL
#  include "OpenGL/gfx_opengl_unix_wayland.c"
#elif GFX_VULKAN
#  include "Vulkan/gfx_vulkan_unix_wayland.c"
#else
#  error invalid graphics api
#endif

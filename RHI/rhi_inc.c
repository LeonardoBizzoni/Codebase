#include "rhi_core.c"

#if RHI_OPENGL
#  include "OpenGL/rhi_opengl_core.c"
#  if OS_WINDOWS
#    include "OpenGL/Win32/rhi_opengl_win32.c"
#  elif OS_UNIXLIKE
#    if UNX_WAYLAND
#      include "OpenGL/Unix/Wayland/rhi_opengl_unix_wayland.c"
#    elif UNX_X11
#      include "OpenGL/Unix/X11/rhi_opengl_unix_x11.c"
#    endif
#  endif
#elif RHI_VULKAN
#  include "Vulkan/rhi_vulkan_core.c"
#  if OS_WINDOWS
#    include "Vulkan/Win32/rhi_vulkan_win32.c"
#  elif OS_UNIXLIKE
#    if UNX_WAYLAND
#      include "Vulkan/Unix/Wayland/rhi_vulkan_unix_wayland.c"
#    elif UNX_X11
#      include "Vulkan/Unix/X11/rhi_vulkan_unix_x11.c"
#    endif
#  endif
#endif

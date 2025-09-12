#include "rhi_core.c"

#if RHI_OPENGL
#  include "OpenGL/rhi_opengl_core.c"
#  if OS_WINDOWS
#    include "OpenGL/Win32/rhi_opengl_win32.c"
#  elif OS_UNIXLIKE
#    include "OpenGL/Unix/rhi_opengl_unix.c"
#  endif
#elif RHI_VULKAN
#  include "Vulkan/rhi_vulkan_core.c"
#  if OS_WINDOWS
#    include "Vulkan/Win32/rhi_vulkan_win32.c"
#  elif OS_UNIXLIKE
#    include "Vulkan/Unix/rhi_vulkan_unix.c"
#  endif
#endif

#include "rhi_core.c"

#if RHI_OPENGL
#  include "OpenGL/rhi_opengl.c"
#  if OS_WINDOWS
#    include "OpenGL/Win32/rhi_opengl_win32.c"
#  elif OS_POSIX
#    include "OpenGL/Posix/rhi_opengl_posix.c"
#  endif
#elif RHI_VULKAN
#  include "Vulkan/rhi_vulkan.c"
#  if OS_WINDOWS
#    include "Vulkan/Win32/rhi_vulkan_win32.c"
#  elif OS_POSIX
#    include "Vulkan/Posix/rhi_vulkan_posix.c"
#  endif
#endif

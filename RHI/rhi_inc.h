#ifndef RHI_INC_H
#define RHI_INC_H

#include "rhi_core.h"

#if RHI_OPENGL
#  include "OpenGL/rhi_opengl_core.h"
#  if OS_WINDOWS
#    include "OpenGL/Win32/rhi_opengl_win32.h"
#  elif OS_UNIXLIKE
#    include "OpenGL/Unix/rhi_opengl_unix.h"
#  endif
#elif RHI_VULKAN
#  include "Vulkan/rhi_vulkan_core.h"
#  if OS_WINDOWS
#    include "Vulkan/Win32/rhi_vulkan_win32.h"
#  elif OS_UNIXLIKE
#    include "Vulkan/Unix/rhi_vulkan_unix.h"
#  endif
#endif

#endif

#ifndef RHI_INC_H
#define RHI_INC_H

#include "rhi_core.h"

#if RHI_OPENGL
#  if OS_WINDOWS
#    include "OpenGL/Win32/rhi_opengl_win32.h"
#  elif OS_POSIX
#    include "OpenGL/Posix/rhi_opengl_posix.h"
#  endif
#  include "OpenGL/rhi_opengl.h"
#elif RHI_VULKAN
#  include "Vulkan/rhi_vulkan.h"
#  if OS_WINDOWS
#    include "Vulkan/Win32/rhi_vulkan_win32.h"
#  elif OS_POSIX
#    include "Vulkan/Posix/rhi_vulkan_posix.h"
#  endif
#endif

#endif

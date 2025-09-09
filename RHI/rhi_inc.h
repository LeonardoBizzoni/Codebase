#ifndef RHI_INC_H
#define RHI_INC_H

#include "rhi_core.h"

#if RHI_OPENGL
#  include "OpenGL/rhi_opengl_core.h"
#  if OS_WINDOWS
#    include "OpenGL/Win32/rhi_opengl_win32.h"
#  elif OS_UNIXLIKE
#    if UNX_WAYLAND
#      include "OpenGL/Unix/Wayland/rhi_opengl_unix_wayland.h"
#    elif UNX_X11
#      include "OpenGL/Unix/X11/rhi_opengl_unix_x11.h"
#    endif
#  endif
#elif RHI_VULKAN
#  include "Vulkan/rhi_vulkan_core.h"
#  if OS_WINDOWS
#    include "Vulkan/Win32/rhi_vulkan_win32.h"
#  elif OS_UNIXLIKE
#    if UNX_WAYLAND
#      include "Vulkan/Unix/Wayland/rhi_vulkan_unix_wayland.h"
#    elif UNX_X11
#      include "Vulkan/Unix/X11/rhi_vulkan_unix_x11.h"
#    endif
#  endif
#endif

#endif

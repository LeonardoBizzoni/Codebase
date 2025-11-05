#ifndef OS_INC_H
#define OS_INC_H

#include "core/os_core.h"

#if OS_POSIX
#  include "core/Posix/os_core_posix.h"
#  if OS_LINUX
#    include "core/Posix/Linux/os_core_posix_linux.h"
#  elif OS_BSD
#    include "core/Posix/BSD/os_core_posix_bsd.h"
#  else
#    error posix os not recognized
#  endif
#elif OS_WINDOWS
#  include "core/Win32/os_core_win32.h"
#else
#  error os layer is not supported for this platform
#endif

#if OS_GUI
#  include "gfx/os_gfx.h"
#  if OS_POSIX
#    include "gfx/Posix/os_gfx_posix.h"
#  elif OS_WINDOWS
#    include "gfx/Win32/os_gfx_win32.h"
#  else
#    error os graphical layer is not supported for this platform
#  endif
#endif

#endif

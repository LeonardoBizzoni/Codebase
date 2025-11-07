#include "core/os_core.c"

#if OS_POSIX
#  include "core/Posix/os_core_posix.c"
#  if OS_LINUX
#    include "core/Posix/Linux/os_core_posix_linux.c"
#  elif OS_BSD
#    include "core/Posix/BSD/os_core_posix_bsd.c"
#  else
#    error posix os not recognized
#  endif
#elif OS_WINDOWS
#  include "core/Win32/os_core_win32.c"
#elif OS_WEB
#  include "core/Web/os_core_web.c"
#else
#  error os layer is not supported for this platform
#endif

#if OS_GUI
#  include "gfx/os_gfx.c"
#  if OS_POSIX
#    include "gfx/Posix/os_gfx_posix.c"
#  elif OS_WINDOWS
#    include "gfx/Win32/os_gfx_win32.c"
#  else
#    error os graphical layer is not supported for this platform
#  endif
#endif

#include "core/os_core.c"

#if OS_LINUX
#  include "core/Linux/linux_core.c"
#elif OS_BSD
#  include "core/BSD/bsd_core.c"
#  include "core/BSD/file.c"
#elif OS_WINDOWS
#  include "core/Win32/os_core_win32.c"
#endif

// =============================================================================
// Gui
#ifndef OS_GUI
#  define OS_GUI 0
#endif

#if OS_GUI
#  if OS_LINUX
#    include "gfx/Linux/linux_gfx.c"
#  elif OS_BSD
#  elif OS_WINDOWS
#    include "gfx/Win32/os_gfx_win32.c"
#  else
#    error os graphical layer is not supported for this platform
#  endif
#endif

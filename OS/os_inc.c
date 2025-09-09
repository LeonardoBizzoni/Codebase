#include "core/os_core.c"

#if OS_UNIXLIKE
#  include "core/Unix/unix_core.c"
#  if OS_LINUX
#    include "core/Unix/Linux/linux_core.c"
#  elif OS_BSD
#    include "core/Unix/BSD/bsd_core.c"
#  endif
#elif OS_WINDOWS
#  include "core/Win32/os_core_win32.c"
#endif

#if OS_GUI
#  include "gfx/os_gfx.c"
#  if OS_UNIXLIKE
#    include "gfx/Unix/unix_gfx.c"
#    if OS_LINUX
#      include "gfx/Unix/Linux/linux_gfx.c"
#    elif OS_BSD
#      include "gfx/Unix/BSD/bsd_gfx.c"
#  endif
#  elif OS_WINDOWS
#    include "gfx/Win32/os_gfx_win32.c"
#  else
#    error os graphical layer is not supported for this platform
#  endif
#endif

#if OS_SOUND
#  include "sound/os_sound.c"
#  if OS_LINUX
#    include "sound/Linux/os_sound_linux.c"
#  elif OS_BSD
#    include "sound/BSD/os_sound_bsd.c"
#  elif OS_WINDOWS
#    include "sound/Win32/os_sound_win32.c"
#  else
#    error os sound layer is not supported for this platform
#  endif
#endif

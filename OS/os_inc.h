#ifndef OS_INC_H
#define OS_INC_H

#include "core/os_core.h"

#if OS_LINUX
#  include "core/Linux/linux_core.h"
#elif OS_BSD
#  include "core/BSD/bsd_core.h"
#elif OS_WINDOWS
#  include "core/Win32/os_core_win32.h"
#else
#  error os layer is not supported for this platform
#endif

#if OS_GUI
#  include "gfx/os_gfx.h"
#  if OS_LINUX
#    include "gfx/Linux/linux_gfx.h"
#  elif OS_BSD
#  elif OS_WINDOWS
#    include "gfx/Win32/os_gfx_win32.h"
#  else
#    error os graphical layer is not supported for this platform
#  endif
#endif

#if OS_SOUND
#  include "sound/os_sound.h"
#  if OS_LINUX
#    include "sound/Linux/os_sound_linux.h"
#  elif OS_BSD
#    include "sound/BSD/os_sound_bsd.h"
#  elif OS_WINDOWS
#    include "sound/Win32/os_sound_win32.h"
#  else
#    error os sound layer is not supported for this platform
#  endif
#endif

#endif

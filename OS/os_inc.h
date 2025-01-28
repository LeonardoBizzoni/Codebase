#ifndef OS_INC_H
#define OS_INC_H

#include "os_core.h"

/* Super temporary */
#if OS_LINUX
#  include "Linux/linux_core.h"
#  include "Linux/X11/window.h"
#  include "Linux/opengl.h"
#elif OS_BSD
#  include "BSD/opengl.h"
#  include "BSD/X11/window.h"
#elif OS_WINDOWS
#  include "Win32/os_core_win32.h"
#else
#  error os layer is not supported for this platform
#endif

#endif

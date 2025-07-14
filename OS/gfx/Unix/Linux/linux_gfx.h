#ifndef OS_GFX_LINUX_H
#define OS_GFX_LINUX_H

#if LNX_X11
#  include "X11/linux_gfx_x11.h"
#elif LNX_WAYLAND
#  include "Wayland/linux_gfx_wayland.h"
#endif

#endif

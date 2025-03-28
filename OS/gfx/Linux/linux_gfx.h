#ifndef OS_GFX_LINUX_H
#define OS_GFX_LINUX_H

fn void lnx_gfx_init(void);

#ifndef LNX_X11
#  define LNX_X11 0
#endif
#ifndef LNX_WAYLAND
#  define LNX_WAYLAND 0
#endif
#if LNX_X11 && LNX_WAYLAND
#  error pick either X11 or Wayland
#endif
#if !LNX_X11 && !LNX_WAYLAND
#  undef LNX_X11
#  define LNX_X11 1
#endif

#if LNX_X11
#  include "X11/linux_gfx_x11.h"
#elif LNX_WAYLAND
#  include "Wayland/linux_gfx_wayland.h"
#endif

#endif

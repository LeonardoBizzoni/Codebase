#ifndef OS_GFX_UNIX_H
#define OS_GFX_UNIX_H

fn void unx_gfx_init(void);
fn void unx_gfx_deinit(void);

#if LNX_X11 || BSD_X11
#  define UNX_X11 1
#else
#  define UNX_X11 0
#endif

#if LNX_WAYLAND || BSD_WAYLAND
#  define UNX_WAYLAND 1
#else
#  define UNX_WAYLAND 0
#endif

#if UNX_X11 && UNX_WAYLAND
#  error pick either X11 or Wayland
#elif !UNX_X11 && !UNX_WAYLAND
#  undef UNX_X11
#  define UNX_X11 1
#endif

#if UNX_X11
#  include "X11/unix_gfx_x11.h"
#elif UNX_WAYLAND
#  include "Wayland/unix_gfx_wayland.h"
#endif

#endif

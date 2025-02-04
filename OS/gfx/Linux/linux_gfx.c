#if LNX_X11
#  include "X11/linux_gfx_x11.c"
#elif LNX_WAYLAND
#  include "Wayland/linux_gfx_wayland.c"
#endif

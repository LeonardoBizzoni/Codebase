#ifndef OS_GFX_POSIX_H
#define OS_GFX_POSIX_H

#undef internal
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#define internal static

typedef struct OS_GFX_Posix_Window {
  struct OS_GFX_Posix_Window *next;
  struct OS_GFX_Posix_Window *prev;
  Window xwindow;
  GC xgc;
} OS_GFX_Posix_Window;

typedef struct {
  Arena *arena;
  OS_GFX_Posix_Window *first_window;
  OS_GFX_Posix_Window *last_window;
  OS_GFX_Posix_Window *freelist_window;

  Display *xdisplay;
  XVisualInfo xvisual;
  i32 xscreen;
  Window xroot;

  u64 xatom_close;
} OS_GFX_Posix_State;

internal void os_gfx_init(void);
internal void os_gfx_deinit(void);

internal OS_GFX_Posix_Window* os_posix_window_from_xwindow(Window xwindow);
internal KeySym os_posix_keysym_from_os_key(OS_Key key);

#endif

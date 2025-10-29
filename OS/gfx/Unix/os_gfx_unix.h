#ifndef OS_GFX_UNIX_H
#define OS_GFX_UNIX_H

#undef internal
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#define internal static

typedef struct X11_Window {
  Window xwindow;
  GC xgc;

  struct X11_Window *next;
  struct X11_Window *prev;
} X11_Window;

typedef struct {
  Arena *arena;
  X11_Window *first_window;
  X11_Window *last_window;
  X11_Window *freelist_window;

  Display *xdisplay;
  XVisualInfo xvisual;
  i32 xscreen;
  Window xroot;

  u64 xatom_close;
} X11_State;

fn void unx_gfx_init(void);
fn void unx_gfx_deinit(void);

internal X11_Window* lnx_window_from_xwindow(Window xwindow);

#endif

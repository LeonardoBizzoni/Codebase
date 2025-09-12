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

  struct {
    OS_Event *first;
    OS_Event *last;
    OS_Handle lock;
  } eventlist;
  OS_Handle eventlistener;
  GFX_Handle gfx_context;

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

fn Bool x11_window_event_for_xwindow(Display *_display, XEvent *event,
                                     XPointer arg);
fn OS_Event x11_handle_xevent(X11_Window *window, XEvent *xevent);

fn void unx_gfx_init(void);

#endif

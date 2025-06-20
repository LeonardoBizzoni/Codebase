#ifndef OS_GFX_LINUX_X11_H
#define OS_GFX_LINUX_X11_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

#if USING_OPENGL
#  include<GL/gl.h>
#  include<GL/glx.h>
#  include<GL/glu.h>
#endif

typedef struct LNX11_Window {
  i32 xscreen;
  Window xwindow;
  XVisualInfo xvisual;
  XSetWindowAttributes xattrib;
  GC xgc;

  struct {
    OS_Event *first;
    OS_Event *last;
    OS_Handle lock;
  } eventlist;
  OS_Handle eventlistener;

#if USING_OPENGL
  GLXContext context;
#endif

  struct LNX11_Window *next;
  struct LNX11_Window *prev;
} LNX11_Window;

typedef struct {
  Arena *arena;
  LNX11_Window *first_window;
  LNX11_Window *last_window;
  LNX11_Window *freelist_window;

  Display *xdisplay;
  u64 xatom_close;
} LNX11_State;

#endif

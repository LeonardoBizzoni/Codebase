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

typedef struct X11_Window {
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

  struct X11_Window *next;
  struct X11_Window *prev;
} X11_Window;

typedef struct {
  Arena *arena;
  X11_Window *first_window;
  X11_Window *last_window;
  X11_Window *freelist_window;

  Display *xdisplay;
  u64 xatom_close;
} X11_State;

#endif

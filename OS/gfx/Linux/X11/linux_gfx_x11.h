#ifndef OS_GFX_LINUX_X11_H
#define OS_GFX_LINUX_X11_H

#include <X11/Xlib.h>

#if USING_OPENGL
#  include<GL/gl.h>
#  include<GL/glx.h>
#  include<GL/glu.h>
#endif

typedef struct LNX11_Window {
  String8 name;
  u32 width;
  u32 height;

  Window xwindow;
  XVisualInfo *xvisual;
  XSetWindowAttributes xattrib;

  GFX_Api api;
  union {
    GLXContext opengl;
  } context;

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

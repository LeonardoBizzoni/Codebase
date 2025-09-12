#ifndef RHI_OPENGL_UNIX_H
#define RHI_OPENGL_UNIX_H

#include <GLES3/gl3.h>
#include <EGL/egl.h>

typedef struct X11Gl_Window {
  EGLSurface egl_surface;
  X11_Window *os_window;

  struct X11Gl_Window *next;
  struct X11Gl_Window *prev;
} X11Gl_Window;

typedef struct {
  Arena *arena;

  EGLDisplay egl_display;
  EGLConfig egl_config;
  EGLContext egl_context;

  X11Gl_Window *freequeue_first;
  X11Gl_Window *freequeue_last;
} X11Gl_State;

#endif

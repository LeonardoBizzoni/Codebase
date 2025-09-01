#ifndef GFX_OPENGL_UNIX_X11_H
#define GFX_OPENGL_UNIX_X11_H

#include <GLES2/gl2.h>
#include <EGL/egl.h>

typedef struct X11Gl_Window {
  EGLSurface egl_surface;

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

#ifndef RHI_OPENGL_UNIX_H
#define RHI_OPENGL_UNIX_H

#include <GLES3/gl3.h>
#include <EGL/egl.h>

typedef struct LNX_GL_Context {
  EGLSurface egl_surface;
  X11_Window *os_window;
  struct LNX_GL_Context *next;
  struct LNX_GL_Context *prev;
} LNX_GL_Context;

typedef struct {
  Arena *arena;
  struct {
    LNX_GL_Context *first;
    LNX_GL_Context *last;
  } ctx_freequeue;

  EGLDisplay egl_display;
  EGLConfig egl_config;
  EGLContext egl_context;
} LNX_GL_State;

#endif

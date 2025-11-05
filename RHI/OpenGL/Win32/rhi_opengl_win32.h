#ifndef RHI_OPENGL_WIN32_H
#define RHI_OPENGL_WIN32_H

#include "glcorearb.h"
#include "wglext.h"

typedef struct W32Gl_Window {
  HGLRC gl_context;
  W32_Window *os_window;

  struct W32Gl_Window *next;
  struct W32Gl_Window *prev;
} W32Gl_Window;

typedef struct {
  Arena *arena;

  W32Gl_Window *freequeue_first;
  W32Gl_Window *freequeue_last;
} W32Gl_State;

#endif

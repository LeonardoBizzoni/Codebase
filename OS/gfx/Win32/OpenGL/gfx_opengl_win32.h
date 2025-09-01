#ifndef OS_GFX_WIN32_OPENGL_H
#define OS_GFX_WIN32_OPENGL_H

#include <GL/gl.h>

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

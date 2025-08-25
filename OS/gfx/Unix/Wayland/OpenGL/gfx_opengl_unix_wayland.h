#ifndef GFX_OPENGL_UNIX_WAYLAND_H
#define GFX_OPENGL_UNIX_WAYLAND_H

#include <wayland-egl.h>

#include <GLES2/gl2.h>
#include <EGL/egl.h>

typedef struct WayGl_Window {
  Wl_Window *os_window;

  EGLSurface egl_surface;
  struct wl_egl_window *egl_window;

  struct WayGl_Window *next;
  struct WayGl_Window *prev;
} WayGl_Window;

typedef struct {
  Arena *arena;
  WayGl_Window *freequeue_first;
  WayGl_Window *freequeue_last;

  EGLDisplay egl_display;
  EGLContext egl_context;
  EGLConfig egl_config;
} WayGl_State;

#endif

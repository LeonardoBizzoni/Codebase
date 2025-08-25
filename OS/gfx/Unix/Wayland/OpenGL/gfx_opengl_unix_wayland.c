global WayGl_State waygl_state = {};

fn void os_gfx_init(void) {
  waygl_state.arena = ArenaBuild();

  Assert(waystate.wl_display);
  waygl_state.egl_display = eglGetDisplay((EGLNativeDisplayType)waystate.wl_display);
  Assert(waygl_state.egl_display != EGL_NO_DISPLAY);
  Assert(eglInitialize(waygl_state.egl_display, 0, 0));

  local EGLint attribs[] = {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };

  EGLint num_config = {};
  eglChooseConfig(waygl_state.egl_display, attribs, &waygl_state.egl_config, 1, &num_config);

  local EGLint ctx_attr[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };
  eglBindAPI(EGL_OPENGL_ES_API);
  waygl_state.egl_context = eglCreateContext(waygl_state.egl_display,
                                              waygl_state.egl_config,
                                              EGL_NO_CONTEXT, ctx_attr);
}

fn void os_gfx_deinit(void) {
  eglTerminate(waygl_state.egl_display);
}

fn GFX_Handle os_gfx_context_window_init(OS_Handle window) {
  Wl_Window *os_window = (Wl_Window*)window.h[0];

  WayGl_Window *gfx_window = waygl_state.freequeue_first;
  if (gfx_window) {
    memzero(gfx_window, sizeof *gfx_window);
    QueuePop(waygl_state.freequeue_first);
  } else {
    gfx_window = New(waygl_state.arena, WayGl_Window);
  }

  gfx_window->os_window = os_window;
  gfx_window->egl_window = wl_egl_window_create(os_window->wl_surface,
                                                os_window->width,
                                                os_window->height);
  Assert(gfx_window->egl_window);
  gfx_window->egl_surface =
    eglCreateWindowSurface(waygl_state.egl_display, waygl_state.egl_config,
                           (EGLNativeWindowType)gfx_window->egl_window, 0);
  Assert(gfx_window->egl_surface != EGL_NO_SURFACE);

  GFX_Handle res = {{(u64)gfx_window}};
  os_window->gfx_context = res;
  return res;
}

fn void os_gfx_context_window_deinit(GFX_Handle handle) {
  WayGl_Window *window = (WayGl_Window*)handle.h[0];
  if (!window) { return; }

  window->os_window->gfx_context.h[0] = 0;
  eglDestroySurface(waygl_state.egl_display, window->egl_surface);
  wl_egl_window_destroy(window->egl_window);
  QueuePush(waygl_state.freequeue_first, waygl_state.freequeue_last, window);
}

fn void os_gfx_window_make_current(GFX_Handle handle) {
  WayGl_Window *window = (WayGl_Window*)handle.h[0];
  eglMakeCurrent(waygl_state.egl_display, window->egl_surface,
                 window->egl_surface, waygl_state.egl_context);
}

fn void os_gfx_window_commit(GFX_Handle handle) {
  WayGl_Window *window = (WayGl_Window*)handle.h[0];
  eglMakeCurrent(waygl_state.egl_display, window->egl_surface,
                 window->egl_surface, waygl_state.egl_context);
  eglSwapBuffers(waygl_state.egl_display, window->egl_surface);
}

internal void os_gfx_window_resize(GFX_Handle context, u32 width, u32 height) {
  WayGl_Window *window = (WayGl_Window*)context.h[0];
  if (!window) { return; }
  wl_egl_window_resize(window->egl_window, width, height, 0, 0);
}

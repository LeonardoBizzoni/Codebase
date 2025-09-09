global WayGl_State waygl_state = {};

fn void rhi_init(void) {
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

fn void rhi_deinit(void) {
  eglTerminate(waygl_state.egl_display);
}

// =============================================================================
// API dependent code
fn RHI_Handle rhi_gl_window_init(OS_Handle window) {
  Wl_Window *os_window = (Wl_Window*)window.h[0];

  WayGl_Window *rhi_window = waygl_state.freequeue_first;
  if (rhi_window) {
    memzero(rhi_window, sizeof *rhi_window);
    QueuePop(waygl_state.freequeue_first);
  } else {
    rhi_window = New(waygl_state.arena, WayGl_Window);
  }

  rhi_window->os_window = os_window;
  rhi_window->egl_window = wl_egl_window_create(os_window->wl_surface,
                                                os_window->width,
                                                os_window->height);
  Assert(rhi_window->egl_window);
  rhi_window->egl_surface =
    eglCreateWindowSurface(waygl_state.egl_display, waygl_state.egl_config,
                           (EGLNativeWindowType)rhi_window->egl_window, 0);
  Assert(rhi_window->egl_surface != EGL_NO_SURFACE);

  RHI_Handle res = {{(u64)rhi_window}};
  return res;
}

fn void rhi_gl_window_deinit(RHI_Handle handle) {
  WayGl_Window *window = (WayGl_Window*)handle.h[0];
  if (!window) { return; }

  eglDestroySurface(waygl_state.egl_display, window->egl_surface);
  wl_egl_window_destroy(window->egl_window);
  QueuePush(waygl_state.freequeue_first, waygl_state.freequeue_last, window);
}

fn void rhi_gl_window_make_current(RHI_Handle handle) {
  WayGl_Window *window = (WayGl_Window*)handle.h[0];
  eglMakeCurrent(waygl_state.egl_display, window->egl_surface,
                 window->egl_surface, waygl_state.egl_context);
}

fn void rhi_gl_window_commit(RHI_Handle handle) {
  WayGl_Window *window = (WayGl_Window*)handle.h[0];
  eglMakeCurrent(waygl_state.egl_display, window->egl_surface,
                 window->egl_surface, waygl_state.egl_context);
  eglSwapBuffers(waygl_state.egl_display, window->egl_surface);
}

internal void rhi_gl_window_resize(RHI_Handle context, u32 width, u32 height) {
  WayGl_Window *window = (WayGl_Window*)context.h[0];
  if (!window) { return; }
  wl_egl_window_resize(window->egl_window, width, height, 0, 0);
}

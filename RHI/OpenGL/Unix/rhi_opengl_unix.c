global X11Gl_State x11gl_state = {};

fn void rhi_init(void) {
  x11gl_state.arena = ArenaBuild();
  x11gl_state.egl_display = eglGetDisplay((EGLNativeDisplayType)x11_state.xdisplay);
  Assert(x11gl_state.egl_display != EGL_NO_DISPLAY);
  EGLBoolean init_result = eglInitialize(x11gl_state.egl_display, 0, 0);
  Assert(init_result == EGL_TRUE);

  i32 num_config;
  EGLint attr[] = {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE,   8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE,  8,
    EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 24,
    EGL_NONE
  };
  EGLBoolean choose_config_result = eglChooseConfig(x11gl_state.egl_display, attr,
                                                    &x11gl_state.egl_config, 1,
                                                    &num_config);
  Assert(choose_config_result == EGL_TRUE && num_config == 1);

  EGLint visual_id;
  eglGetConfigAttrib(x11gl_state.egl_display, x11gl_state.egl_config,
                     EGL_NATIVE_VISUAL_ID, &visual_id);

  XVisualInfo visual_template;
  visual_template.visualid = (VisualID)visual_id;
  i32 visual_num;
  XVisualInfo *visuals = XGetVisualInfo(x11_state.xdisplay,
                                        VisualIDMask, &visual_template, &visual_num);
  Assert(visuals && visual_num > 0);
  x11_state.xvisual = *visuals;
  XFree(visuals);

  local EGLint ctx_attr[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };
  eglBindAPI(EGL_OPENGL_ES_API);
  x11gl_state.egl_context = eglCreateContext(x11gl_state.egl_display,
                                             x11gl_state.egl_config,
                                             EGL_NO_CONTEXT, ctx_attr);
  Assert(x11gl_state.egl_context != EGL_NO_CONTEXT);
}

// =============================================================================
// API dependent code
fn RHI_Handle rhi_gl_window_init(OS_Handle window_) {
  X11_Window *os_window = (X11_Window*)window_.h[0];

  X11Gl_Window *rhi_window = x11gl_state.freequeue_first;
  if (rhi_window) {
    memzero(rhi_window, sizeof (*rhi_window));
    QueuePop(x11gl_state.freequeue_first);
  } else {
    rhi_window = New(x11gl_state.arena, X11Gl_Window);
  }
  Assert(rhi_window);

  rhi_window->os_window = os_window;
  rhi_window->egl_surface = eglCreateWindowSurface(x11gl_state.egl_display,
                                                   x11gl_state.egl_config,
                                                   os_window->xwindow, 0);
  Assert(rhi_window->egl_surface != EGL_NO_SURFACE);

  eglMakeCurrent(x11gl_state.egl_display, rhi_window->egl_surface,
                 rhi_window->egl_surface, x11gl_state.egl_context);

  RHI_Handle res = {(u64)rhi_window};
  return res;
}

fn void rhi_gl_window_deinit(RHI_Handle handle) {}

fn void rhi_gl_window_make_current(RHI_Handle handle) {
  X11Gl_Window *rhi_window = (X11Gl_Window *)handle.h[0];
  eglMakeCurrent(x11gl_state.egl_display, rhi_window->egl_surface,
                 rhi_window->egl_surface, x11gl_state.egl_context);
}

fn void rhi_gl_window_commit(RHI_Handle handle) {
  X11Gl_Window *rhi_window = (X11Gl_Window *)handle.h[0];
  eglMakeCurrent(x11gl_state.egl_display, rhi_window->egl_surface,
                 rhi_window->egl_surface, x11gl_state.egl_context);
  eglSwapBuffers(x11gl_state.egl_display, rhi_window->egl_surface);
}

internal void rhi_gl_window_resize(RHI_Handle context, u32 width, u32 height) {}

global LNX_GL_State lnx_gl_state = {0};

fn void rhi_init(void) {
  lnx_gl_state.arena = arena_build();
  lnx_gl_state.egl_display = eglGetDisplay((EGLNativeDisplayType)x11_state.xdisplay);
  Assert(lnx_gl_state.egl_display != EGL_NO_DISPLAY);
  EGLBoolean init_result = eglInitialize(lnx_gl_state.egl_display, 0, 0);
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
  EGLBoolean choose_config_result =
    eglChooseConfig(lnx_gl_state.egl_display, attr,
                    &lnx_gl_state.egl_config, 1, &num_config);
  Assert(choose_config_result == EGL_TRUE && num_config == 1);

  EGLint visual_id;
  eglGetConfigAttrib(lnx_gl_state.egl_display, lnx_gl_state.egl_config,
                     EGL_NATIVE_VISUAL_ID, &visual_id);

  XVisualInfo visual_template;
  visual_template.visualid = (VisualID)visual_id;
  i32 visual_num;
  XVisualInfo *visuals = XGetVisualInfo(x11_state.xdisplay, VisualIDMask,
                                        &visual_template, &visual_num);
  Assert(visuals && visual_num > 0);
  x11_state.xvisual = *visuals;
  XFree(visuals);

  local EGLint ctx_attr[] = {
    EGL_CONTEXT_MAJOR_VERSION, 3,
    EGL_CONTEXT_MINOR_VERSION, 3,
    EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
    EGL_NONE
  };
  eglBindAPI(EGL_OPENGL_API);
  lnx_gl_state.egl_context = eglCreateContext(lnx_gl_state.egl_display,
                                             lnx_gl_state.egl_config,
                                             EGL_NO_CONTEXT, ctx_attr);
  Assert(lnx_gl_state.egl_context != EGL_NO_CONTEXT);
}

fn void rhi_deinit(void) {
  eglDestroyContext(lnx_gl_state.egl_display, lnx_gl_state.egl_context);
  eglTerminate(lnx_gl_state.egl_display);
}

// =============================================================================
// API dependent code
fn RHI_Handle rhi_context_create(Arena *arena, OS_Handle window) {
  Unused(arena);
  X11_Window *os_window = (X11_Window*)window.h[0];

  LNX_GL_Context *ctx = lnx_gl_state.ctx_freequeue.first;
  if (ctx) {
    QueuePop(lnx_gl_state.ctx_freequeue.first);
    memzero(ctx, sizeof (*ctx));
  } else {
    ctx = arena_push(lnx_gl_state.arena, LNX_GL_Context);
  }
  Assert(ctx);

  ctx->os_window = os_window;
  ctx->egl_surface = eglCreateWindowSurface(lnx_gl_state.egl_display,
                                                   lnx_gl_state.egl_config,
                                                   os_window->xwindow, 0);
  Assert(ctx->egl_surface != EGL_NO_SURFACE);
  eglMakeCurrent(lnx_gl_state.egl_display, ctx->egl_surface,
                 ctx->egl_surface, lnx_gl_state.egl_context);
  rhi_opengl_vao_setup();

  Info("OpenGL renderer: %s", glGetString(GL_RENDERER));
  Info("OpenGL vendor: %s", glGetString(GL_VENDOR));
  Info("OpenGL version: %s\n", glGetString(GL_VERSION));

  RHI_Handle res = {{(u64)ctx}};
  return res;
}

fn void rhi_opengl_context_destroy(RHI_Handle handle) {
  LNX_GL_Context *ctx = (LNX_GL_Context *)handle.h[0];
  eglDestroySurface(lnx_gl_state.egl_display, ctx->egl_surface);
}

fn void rhi_opengl_context_set_active(RHI_Handle handle) {
  LNX_GL_Context *ctx = (LNX_GL_Context *)handle.h[0];
  eglMakeCurrent(lnx_gl_state.egl_display, ctx->egl_surface,
                 ctx->egl_surface, lnx_gl_state.egl_context);
}

fn void rhi_context_commit(RHI_Handle handle) {
  LNX_GL_Context *ctx = (LNX_GL_Context *)handle.h[0];
  eglSwapBuffers(lnx_gl_state.egl_display, ctx->egl_surface);
}

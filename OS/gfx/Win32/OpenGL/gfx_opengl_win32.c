global W32Gl_State w32gl_state = {};

fn void os_gfx_init(void) {
  w32gl_state.arena = ArenaBuild();
}

fn GFX_Handle os_gfx_context_window_init(OS_Handle window_) {
  W32_Window *os_window = (W32_Window *)window_.h[0];

  W32Gl_Window *gfx_window = w32gl_state.freequeue_first;
  if (gfx_window) {
    memzero(gfx_window, sizeof (*gfx_window));
    QueuePop(w32gl_state.freequeue_first);
  } else {
    gfx_window = New(w32gl_state.arena, W32Gl_Window);
  }
  gfx_window->os_window = os_window;

  PIXELFORMATDESCRIPTOR pfd = {0};
  PIXELFORMATDESCRIPTOR desired_pfd = {
    .nSize = sizeof(PIXELFORMATDESCRIPTOR),
    .nVersion = 1,
    .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
    .iPixelType = PFD_TYPE_RGBA,
    // NOTE(lb): docs says without alpha but returned pfd has .cColorBits=32
    //           so im not sure if 24 or 32 is the right value
    .cColorBits = 32,
    .cAlphaBits = 8,
    .iLayerType = PFD_MAIN_PLANE,
  };
  HDC dc = GetDC(os_window->winhandle);
  i32 pixel_format = ChoosePixelFormat(dc, &desired_pfd);
  Assert(pixel_format);
  DescribePixelFormat(dc, pixel_format,
                      sizeof(PIXELFORMATDESCRIPTOR), &pfd);
  SetPixelFormat(dc, pixel_format, &pfd);

  gfx_window->gl_context = wglCreateContext(dc);
  Assert(gfx_window->gl_context);
  wglMakeCurrent(dc, gfx_window->gl_context);

  GFX_Handle res = {(u64)gfx_window};
  os_window->gfx_context = res;
  return res;
}

fn void os_gfx_context_window_deinit(GFX_Handle context) {}

fn void os_gfx_window_commit(GFX_Handle context) {
  W32Gl_Window *gfx_window = (W32Gl_Window*)context.h[0];
  HDC dc = GetDC(gfx_window->os_window->winhandle);
  SwapBuffers(dc);
}

fn void os_gfx_window_make_current(GFX_Handle handle) {
  W32Gl_Window *gfx_window = (W32Gl_Window*)handle.h[0];
  HDC dc = GetDC(gfx_window->os_window->winhandle);
  wglMakeCurrent(dc, gfx_window->gl_context);
}

internal void os_gfx_window_resize(GFX_Handle context, u32 width, u32 height) {}

global W32Gl_State w32gl_state = {};

fn void os_gfx_init(void) {
  w32gl_state.arena = ArenaBuild();

  WNDCLASSA window_class = {
    .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
    .lpfnWndProc = DefWindowProcA,
    .hInstance = GetModuleHandle(0),
    .lpszClassName = "Dummy_WGL",
  };

  Assert(RegisterClassA(&window_class));
  HWND dummy_window = CreateWindowExA(0, window_class.lpszClassName,
                                      "Dummy OpenGL Window",
                                      0, CW_USEDEFAULT, CW_USEDEFAULT,
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                      0, 0, window_class.hInstance, 0);
  Assert(dummy_window);

  HDC dummy_dc = GetDC(dummy_window);
  PIXELFORMATDESCRIPTOR pfd = {
    .nSize = sizeof(pfd),
    .nVersion = 1,
    .iPixelType = PFD_TYPE_RGBA,
    .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
    .cColorBits = 32,
    .cAlphaBits = 8,
    .iLayerType = PFD_MAIN_PLANE,
    .cDepthBits = 24,
    .cStencilBits = 8,
  };
  int pixel_format = ChoosePixelFormat(dummy_dc, &pfd);
  Assert(pixel_format && SetPixelFormat(dummy_dc, pixel_format, &pfd));

  HGLRC dummy_context = wglCreateContext(dummy_dc);
  Assert(dummy_context && wglMakeCurrent(dummy_dc, dummy_context));

  wglCreateContextAttribsARB =
    (wglCreateContextAttribsARB_type*)wglGetProcAddress("wglCreateContextAttribsARB");
  wglChoosePixelFormatARB =
    (wglChoosePixelFormatARB_type*)wglGetProcAddress("wglChoosePixelFormatARB");
  Assert(wglCreateContextAttribsARB && wglChoosePixelFormatARB);

  wglMakeCurrent(dummy_dc, 0);
  wglDeleteContext(dummy_context);
  ReleaseDC(dummy_window, dummy_dc);
  DestroyWindow(dummy_window);
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

  local const i32 pixel_format_attribs[] = {
    WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
    WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
    WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
    WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
    WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
    WGL_COLOR_BITS_ARB,     32,
    WGL_DEPTH_BITS_ARB,     24,
    WGL_STENCIL_BITS_ARB,   8,
    0
  };

  HDC dc = GetDC(os_window->winhandle);

  i32 pixel_format;
  u32 num_formats;
  wglChoosePixelFormatARB(dc, pixel_format_attribs, 0, 1, &pixel_format, &num_formats);
  Assert(num_formats);

  PIXELFORMATDESCRIPTOR pfd = {};
  DescribePixelFormat(dc, pixel_format, sizeof(pfd), &pfd);
  SetPixelFormat(dc, pixel_format, &pfd);

  local i32  gl33_attribs[] = {
    WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    WGL_CONTEXT_MINOR_VERSION_ARB, 3,
    WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
    0,
  };

  gfx_window->gl_context = wglCreateContextAttribsARB(dc, 0, gl33_attribs);
  Assert(gfx_window->gl_context);
  wglMakeCurrent(dc, gfx_window->gl_context);

  /* Assert(gladLoadGLLoader((GLADloadproc)wglGetProcAddress)); */

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

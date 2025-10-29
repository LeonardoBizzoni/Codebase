global W32Gl_State w32gl_state = {};

#define GL_FUNCTIONS(X)                                            \
  X(PFNGLGENBUFFERSPROC, glGenBuffers)                             \
  X(PFNGLBINDBUFFERPROC, glBindBuffer)                             \
  X(PFNGLBUFFERDATAPROC, glBufferData)                             \
  X(PFNGLCREATEBUFFERSPROC, glCreateBuffers)                       \
  X(PFNGLNAMEDBUFFERSTORAGEPROC, glNamedBufferStorage)             \
                                                                   \
  X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays)                   \
  X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray)                   \
  X(PFNGLCREATEVERTEXARRAYSPROC, glCreateVertexArrays)             \
  X(PFNGLVERTEXARRAYVERTEXBUFFERPROC, glVertexArrayVertexBuffer)   \
  X(PFNGLVERTEXARRAYATTRIBFORMATPROC, glVertexArrayAttribFormat)   \
  X(PFNGLENABLEVERTEXARRAYATTRIBPROC, glEnableVertexArrayAttrib)   \
  X(PFNGLVERTEXARRAYATTRIBBINDINGPROC, glVertexArrayAttribBinding) \
  X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer)           \
  X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray)   \
                                                                   \
  X(PFNGLCREATESHADERPROC, glCreateShader)                         \
  X(PFNGLDELETESHADERPROC, glDeleteShader)                         \
  X(PFNGLATTACHSHADERPROC, glAttachShader)                         \
  X(PFNGLLINKPROGRAMPROC, glLinkProgram)                           \
  X(PFNGLSHADERSOURCEPROC, glShaderSource)                         \
  X(PFNGLCOMPILESHADERPROC, glCompileShader)                       \
  X(PFNGLCREATESHADERPROGRAMVPROC, glCreateShaderProgramv)         \
                                                                   \
  X(PFNGLUSEPROGRAMPROC, glUseProgram)                             \
  X(PFNGLCREATEPROGRAMPROC, glCreateProgram)                       \
  X(PFNGLGETPROGRAMIVPROC, glGetProgramiv)                         \
  X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog)               \
  X(PFNGLGENPROGRAMPIPELINESPROC, glGenProgramPipelines)           \
  X(PFNGLUSEPROGRAMSTAGESPROC, glUseProgramStages)                 \
  X(PFNGLBINDPROGRAMPIPELINEPROC, glBindProgramPipeline)           \
  X(PFNGLPROGRAMUNIFORMMATRIX2FVPROC, glProgramUniformMatrix2fv)   \
                                                                   \
  X(PFNGLBINDTEXTUREUNITPROC, glBindTextureUnit)                   \
  X(PFNGLCREATETEXTURESPROC, glCreateTextures)                     \
  X(PFNGLTEXTUREPARAMETERIPROC, glTextureParameteri)               \
  X(PFNGLTEXTURESTORAGE2DPROC, glTextureStorage2D)                 \
  X(PFNGLTEXTURESUBIMAGE2DPROC, glTextureSubImage2D)               \
  X(PFNGLDEBUGMESSAGECALLBACKPROC, glDebugMessageCallback)

#define X(Type, Name) global Type Name;
GL_FUNCTIONS(X)
#undef X

global PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = NULL;
global PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;

fn void rhi_init(void) {
  w32gl_state.arena = arena_build();

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
    .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
    .iPixelType = PFD_TYPE_RGBA,
    .cColorBits = 24,
  };
  int pixel_format = ChoosePixelFormat(dummy_dc, &pfd);
  Assert(pixel_format);
  BOOL set_pixel_format = SetPixelFormat(dummy_dc, pixel_format, &pfd);
  Assert(set_pixel_format);

  HGLRC dummy_context = wglCreateContext(dummy_dc);
  Assert(dummy_context);
  wglMakeCurrent(dummy_dc, dummy_context);

  PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB =
    (void*)wglGetProcAddress("wglGetExtensionsStringARB");
  Assert(wglGetExtensionsStringARB);
  const char *ext = wglGetExtensionsStringARB(dummy_dc);
  Assert(ext);


  for (const char *ext_start = ext; *ext; ++ext, ext_start = ext) {
    for (; *ext != ' ' && *ext != 0; ++ext) {}
    size_t length = ext - ext_start;

    if (str8_eq(Strlit("WGL_ARB_pixel_format"), str8((u8*)ext_start, length))) {
      wglChoosePixelFormatARB =
        (void*)wglGetProcAddress("wglChoosePixelFormatARB");
    } else if (str8_eq(Strlit("WGL_ARB_create_context"),
                       str8((u8*)ext_start, length))) {
      wglCreateContextAttribsARB =
        (void*)wglGetProcAddress("wglCreateContextAttribsARB");
    }
  }

  Assert(wglChoosePixelFormatARB);
  Assert(wglCreateContextAttribsARB);

#define X(Type, Name)                    \
  Name = (Type)wglGetProcAddress(#Name); \
  Assert(Name);

  GL_FUNCTIONS(X)
#undef X

  wglMakeCurrent(dummy_dc, 0);
  wglDeleteContext(dummy_context);
  ReleaseDC(dummy_window, dummy_dc);
  DestroyWindow(dummy_window);
}

// =============================================================================
// API dependent code
fn RHI_Handle rhi_opengl_context_create(OS_Handle window) {
  W32_Window *os_window = (W32_Window *)window.h[0];

  W32Gl_Window *rhi_window = w32gl_state.freequeue_first;
  if (rhi_window) {
    memzero(rhi_window, sizeof (*rhi_window));
    QueuePop(w32gl_state.freequeue_first);
  } else {
    rhi_window = arena_push(w32gl_state.arena, W32Gl_Window);
  }
  rhi_window->os_window = os_window;

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

  rhi_window->gl_context = wglCreateContextAttribsARB(dc, NULL, gl33_attribs);
  Assert(rhi_window->gl_context);
  wglMakeCurrent(dc, rhi_window->gl_context);
  rhi_opengl_vao_setup();

  RHI_Handle res = {(u64)rhi_window};
  return res;
}

fn void rhi_opengl_context_destroy(RHI_Handle context) {
  Unused(context);
}

fn void rhi_opengl_context_set_active(RHI_Handle handle) {
  W32Gl_Window *rhi_window = (W32Gl_Window*)handle.h[0];
  HDC dc = GetDC(rhi_window->os_window->winhandle);
  wglMakeCurrent(dc, rhi_window->gl_context);
}

fn void rhi_opengl_context_commit(RHI_Handle context) {
  W32Gl_Window *rhi_window = (W32Gl_Window*)context.h[0];
  HDC dc = GetDC(rhi_window->os_window->winhandle);
  SwapBuffers(dc);
}

global RHI_Opengl_Posix_State rhi_gl_posix_state = {0};

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

fn void rhi_init(void) {
  rhi_gl_posix_state.arena = arena_build();

  GLint glx_attribs[] = {
    GLX_X_RENDERABLE    , True,
    GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
    GLX_RENDER_TYPE     , GLX_RGBA_BIT,
    GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
    GLX_RED_SIZE        , 8,
    GLX_GREEN_SIZE      , 8,
    GLX_BLUE_SIZE       , 8,
    GLX_ALPHA_SIZE      , 8,
    GLX_DEPTH_SIZE      , 24,
    GLX_STENCIL_SIZE    , 8,
    GLX_DOUBLEBUFFER    , True,
    None
  };
  i32 fbcount;
  GLXFBConfig* fbc = glXChooseFBConfig(os_gfx_posix_state.xdisplay, os_gfx_posix_state.xscreen,
                                       glx_attribs, &fbcount);
  Assert(fbcount > 0 && fbc);

  // Pick the FB config/visual with the most samples per pixel
  int best_fbc_idx = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;
  for (int i = 0; i < fbcount; ++i) {
    XVisualInfo *vi = glXGetVisualFromFBConfig(os_gfx_posix_state.xdisplay, fbc[i]);
    if (vi && vi->screen == os_gfx_posix_state.xscreen) {
      int samp_buf, samples;
      glXGetFBConfigAttrib(os_gfx_posix_state.xdisplay, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf);
      glXGetFBConfigAttrib(os_gfx_posix_state.xdisplay, fbc[i], GLX_SAMPLES, &samples);

      if (best_fbc_idx < 0 || (samp_buf && samples > best_num_samp)) {
        best_fbc_idx = i;
        best_num_samp = samples;
      }
      if (worst_fbc < 0 || !samp_buf || samples < worst_num_samp) {
        worst_fbc = i;
        worst_num_samp = samples;
      }
    }
    XFree(vi);
  }
  GLXFBConfig best_fbc = fbc[best_fbc_idx];
  XFree( fbc );

  XVisualInfo *visuals = glXGetVisualFromFBConfig(os_gfx_posix_state.xdisplay, best_fbc);
  Assert(visuals);
  Assert(os_gfx_posix_state.xscreen == visuals->screen);
  os_gfx_posix_state.xvisual = *visuals;
  XFree(visuals);

  glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
  glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc) glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );

  local i32 context_attribs[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
    GLX_CONTEXT_MINOR_VERSION_ARB, 3,
    GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
    None
  };

  const char *glx_extensions = glXQueryExtensionsString(os_gfx_posix_state.xdisplay,
                                                        os_gfx_posix_state.xscreen);
  AssertAlways(str8_find_first_str8(str8_from_cstr((char*)glx_extensions),
                                    Strlit("GLX_ARB_create_context")));
  rhi_gl_posix_state.glx_context = glXCreateContextAttribsARB(os_gfx_posix_state.xdisplay,
                                                        best_fbc, 0, true,
                                                        context_attribs);
  XSync(os_gfx_posix_state.xdisplay, False);
}

fn void rhi_deinit(void) {
  glXDestroyContext(os_gfx_posix_state.xdisplay, rhi_gl_posix_state.glx_context);
}

// =============================================================================
// API dependent code
fn RHI_Handle rhi_context_create(Arena *arena, OS_Handle hwindow) {
  Unused(arena);
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)hwindow.h[0];
  glXMakeCurrent(os_gfx_posix_state.xdisplay, window->xwindow, rhi_gl_posix_state.glx_context);

#define X(Type, Name)                                       \
  Name = (Type)glXGetProcAddressARB((const GLubyte*)#Name); \
  Assert(Name);

  GL_FUNCTIONS(X)
#undef X

  i32 major = 0, minor = 0;
  glGetIntegerv(GL_MAJOR_VERSION, &major);
  glGetIntegerv(GL_MINOR_VERSION, &minor);
  Assert(major >= 3 && minor >= 3);
  Info("OpenGL context version: %d.%d", major, minor);
  Info("OpenGL renderer: %s", glGetString(GL_RENDERER));
  Info("OpenGL vendor: %s", glGetString(GL_VENDOR));
  Info("OpenGL version: %s\n", glGetString(GL_VERSION));

  rhi_opengl_vao_setup();

  RHI_Handle res = {{(u64)window}};
  return res;
}

fn void rhi_context_destroy(RHI_Handle hcontext) {
  Unused(hcontext);
}

fn void rhi_opengl_context_set_active(RHI_Handle hcontext) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window *)hcontext.h[0];
  glXMakeCurrent(os_gfx_posix_state.xdisplay, window->xwindow, rhi_gl_posix_state.glx_context);
}

fn void rhi_context_commit(RHI_Handle hcontext) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window *)hcontext.h[0];
  glXSwapBuffers(os_gfx_posix_state.xdisplay, window->xwindow);
}

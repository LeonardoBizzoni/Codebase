global LNX11_State lnx11_state = {0};

fn void lnx_gfx_init(void) {
  lnx11_state.arena = ArenaBuild();
  lnx11_state.xdisplay = XOpenDisplay(0);

  lnx11_state.xatom_close = XInternAtom(lnx11_state.xdisplay, "WM_DELETE_WINDOW", False);
}

fn OS_Handle os_window_open(String8 name, u32 x, u32 y, u32 width, u32 height) {
  LNX11_Window *window = lnx11_state.freelist_window;
  if (window) {
    memZero(window, sizeof(LNX11_Window));
    StackPop(lnx11_state.freelist_window);
  } else {
    window = New(lnx11_state.arena, LNX11_Window);
  }
  DLLPushBack(lnx11_state.first_window, lnx11_state.last_window, window);

  window->name = name;
  window->width = width;
  window->height = height;

  window->xscreen = DefaultScreen(lnx11_state.xdisplay);
  window->xattrib.event_mask = ExposureMask | FocusChangeMask |
                               EnterWindowMask | LeaveWindowMask |
                               ButtonPressMask | PointerMotionMask |
                               KeyPressMask | KeymapStateMask;
  window->xattrib.background_pixel = 0;

  Assert(XMatchVisualInfo(lnx11_state.xdisplay, window->xscreen,
                          LNX_SCREEN_DEPTH, TrueColor, &window->xvisual));
  window->xgc = XDefaultGC(lnx11_state.xdisplay, window->xscreen);

  window->xattrib.colormap = XCreateColormap(lnx11_state.xdisplay,
                                             XDefaultRootWindow(lnx11_state.xdisplay),
                                             window->xvisual.visual, AllocNone);

  window->xwindow = XCreateWindow(lnx11_state.xdisplay, XDefaultRootWindow(lnx11_state.xdisplay),
                                  x, y, width, height, 0, window->xvisual.depth, InputOutput,
                                  window->xvisual.visual, CWColormap | CWEventMask | CWBackPixel,
                                  &window->xattrib);
  XSetWMProtocols(lnx11_state.xdisplay, window->xwindow, &lnx11_state.xatom_close, 1);

  Scratch scratch = ScratchBegin(&lnx11_state.arena, 1);
  XStoreName(lnx11_state.xdisplay, window->xwindow, cstr_from_str8(scratch.arena, name));
  ScratchEnd(scratch);

  XAutoRepeatOn(lnx11_state.xdisplay);
  XFlush(lnx11_state.xdisplay);

  OS_Handle res = {(u64)window};
  return res;
}

fn void os_window_show(OS_Handle handle) {
  LNX11_Window *window = (LNX11_Window*)handle.h[0];
  XMapWindow(lnx11_state.xdisplay, window->xwindow);
  XFlush(lnx11_state.xdisplay);
}

fn void os_window_hide(OS_Handle handle) {
  LNX11_Window *window = (LNX11_Window*)handle.h[0];
  XUnmapWindow(lnx11_state.xdisplay, window->xwindow);
  XFlush(lnx11_state.xdisplay);
}

fn void os_window_close(OS_Handle handle) {
  LNX11_Window *window = (LNX11_Window*)handle.h[0];
  XUnmapWindow(lnx11_state.xdisplay, window->xwindow);

#if USING_OPENGL
  opengl_deinit(handle);
#endif

  XDestroyWindow(lnx11_state.xdisplay, window->xwindow);
  XFlush(lnx11_state.xdisplay);

  DLLDelete(lnx11_state.first_window, lnx11_state.last_window, window);
  StackPush(lnx11_state.freelist_window, window);
  if (!lnx11_state.first_window) {
    XAutoRepeatOn(lnx11_state.xdisplay);
    XCloseDisplay(lnx11_state.xdisplay);
  }
}

fn void os_window_swapBuffers(OS_Handle handle) {
#if USING_OPENGL
  glXSwapBuffers(lnx11_state.xdisplay, ((LNX11_Window*)handle.h[0])->xwindow);
#endif
}

fn Bool lnx_window_event_for_xwindow(Display *_display, XEvent *event, XPointer arg) {
  return event->xany.window == *(Window*)arg;
}

fn OS_Event os_window_get_event(OS_Handle handle) {
  LNX11_Window *window = (LNX11_Window*)handle.h[0];
  OS_Event res = {0};

  XEvent xevent;
  if (XCheckIfEvent(lnx11_state.xdisplay, &xevent,
                    lnx_window_event_for_xwindow, (XPointer)&window->xwindow)) {
    switch (xevent.type) {
    case Expose: {
      XWindowAttributes gwa;
      XGetWindowAttributes(lnx11_state.xdisplay, window->xwindow, &gwa);

      res.type = OS_EventType_Expose;
      res.expose.width = gwa.width;
      res.expose.height = gwa.height;
    } break;
    case ClientMessage: {
      if ((Atom)xevent.xclient.data.l[0] == lnx11_state.xatom_close) {
        res.type = OS_EventType_Kill;
      }
    } break;
    }
  }

  return res;
}

fn OS_Event os_window_wait_event(OS_Handle handle) {
  LNX11_Window *window = (LNX11_Window*)handle.h[0];
  OS_Event res = {0};

  XEvent xevent;
  XIfEvent(lnx11_state.xdisplay, &xevent,
           lnx_window_event_for_xwindow, (XPointer)&window->xwindow);
  switch (xevent.type) {
  case Expose: {
    XWindowAttributes gwa;
    XGetWindowAttributes(lnx11_state.xdisplay, window->xwindow, &gwa);

    res.type = OS_EventType_Expose;
    res.expose.width = gwa.width;
    res.expose.height = gwa.height;
  } break;
  case ClientMessage: {
    if ((Atom)xevent.xclient.data.l[0] == lnx11_state.xatom_close) {
      res.type = OS_EventType_Kill;
    }
  } break;
  }

  return res;
}

#if USING_OPENGL
fn void opengl_init(OS_Handle handle) {
  LNX11_Window *window = (LNX11_Window *)handle.h[0];

  local i32 attr[] = { GLX_RGBA, GLX_DEPTH_SIZE, LNX_SCREEN_DEPTH, GLX_DOUBLEBUFFER, None };
  XVisualInfo *info = glXChooseVisual(lnx11_state.xdisplay, window->xscreen, attr);
  Assert(info);

  window->xvisual = *info;
  window->context = glXCreateContext(lnx11_state.xdisplay, info, 0, true);
  glXMakeCurrent(lnx11_state.xdisplay, window->xwindow, window->context);
  glEnable(GL_DEPTH_TEST);
}

fn void opengl_deinit(OS_Handle handle) {
  LNX11_Window *window = (LNX11_Window *)handle.h[0];
  glXMakeCurrent(lnx11_state.xdisplay, None, 0);
  glXDestroyContext(lnx11_state.xdisplay, window->context);
  XFreeColormap(lnx11_state.xdisplay, window->xattrib.colormap);
}

fn void opengl_make_current(OS_Handle handle) {
  LNX11_Window *window = (LNX11_Window*)handle.h[0];
  glXMakeCurrent(lnx11_state.xdisplay, window->xwindow, window->context);
}
#endif

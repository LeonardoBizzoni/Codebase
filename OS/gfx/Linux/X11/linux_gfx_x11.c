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

  window->xattrib.event_mask = ExposureMask | FocusChangeMask |
                               EnterWindowMask | LeaveWindowMask |
                               ButtonPressMask | PointerMotionMask |
                               KeyPressMask | KeymapStateMask;

  // TODO(lb): these #if will be moved to opaque functions that depend on the
  //           target graphics API. I don't know how vulkan is initialized nor
  //           how to initialize software rendering in X11 so for now it
  //           stays like this.
#if USING_OPENGL
  local GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
  window->xvisual = glXChooseVisual(lnx11_state.xdisplay, 0, att);
  Assert(window->xvisual);
  window->xattrib.colormap = XCreateColormap(lnx11_state.xdisplay,
                                             XDefaultRootWindow(lnx11_state.xdisplay),
                                             window->xvisual->visual, AllocNone);
#endif

  window->xwindow = XCreateWindow(lnx11_state.xdisplay,
                                  XDefaultRootWindow(lnx11_state.xdisplay),
                                  x, y, width, height, 0, window->xvisual->depth,
                                  InputOutput, window->xvisual->visual,
                                  CWColormap | CWEventMask, &window->xattrib);
  XSetWMProtocols(lnx11_state.xdisplay, window->xwindow, &lnx11_state.xatom_close, 1);

  Scratch scratch = ScratchBegin(&lnx11_state.arena, 1);
  XStoreName(lnx11_state.xdisplay, window->xwindow, cstr_from_str8(scratch.arena, name));
  ScratchEnd(scratch);

#if USING_OPENGL
  window->context = glXCreateContext(lnx11_state.xdisplay, window->xvisual, 0, true);
  glXMakeCurrent(lnx11_state.xdisplay, window->xwindow, window->context);
  glEnable(GL_DEPTH_TEST);
#endif

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
  glXMakeCurrent(lnx11_state.xdisplay, None, 0);
  glXDestroyContext(lnx11_state.xdisplay, window->context);
#endif

  XFreeColormap(lnx11_state.xdisplay, window->xattrib.colormap);
  XFree(window->xvisual);
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
  glXSwapBuffers(lnx11_state.xdisplay, ((LNX11_Window*)handle.h[0])->xwindow);
}

fn OS_Event os_window_get_event(OS_Handle handle) {
  LNX11_Window *window = (LNX11_Window*)handle.h[0];
  OS_Event res = {0};

  XEvent xevent;
  if (XPending(lnx11_state.xdisplay)) {
    XNextEvent(lnx11_state.xdisplay, &xevent);

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

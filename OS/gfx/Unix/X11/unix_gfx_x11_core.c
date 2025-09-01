global X11_State x11_state = {0};

// =============================================================================
// Common Linux definitions
fn void unx_gfx_init(void) {
  x11_state.arena = ArenaBuild();
  x11_state.xdisplay = XOpenDisplay(0);
  XSynchronize(x11_state.xdisplay, True);
  x11_state.xscreen = DefaultScreen(x11_state.xdisplay);
  x11_state.xatom_close = XInternAtom(x11_state.xdisplay, "WM_DELETE_WINDOW", False);

  os_gfx_init();
}

fn void unx_gfx_deinit(void) {
  os_gfx_deinit();
}

fn OS_Handle os_window_open(String8 name, u32 width, u32 height) {
  X11_Window *window = x11_state.freelist_window;
  if (window) {
    memzero(window, sizeof(X11_Window));
    StackPop(x11_state.freelist_window);
  } else {
    window = New(x11_state.arena, X11_Window);
  }
  DLLPushBack(x11_state.first_window, x11_state.last_window, window);

  Window xroot = RootWindow(x11_state.xdisplay, x11_state.xscreen);
  XSetWindowAttributes xattrib = {};
  xattrib.event_mask = ExposureMask | FocusChangeMask |
                       EnterWindowMask | LeaveWindowMask |
                       ButtonPressMask | PointerMotionMask |
                       KeyPressMask | KeymapStateMask;
  xattrib.border_pixel = 0;
  xattrib.background_pixel = 0;
  xattrib.colormap = XCreateColormap(x11_state.xdisplay, xroot,
                                     x11_state.xvisual.visual, AllocNone);
  window->xwindow = XCreateWindow(x11_state.xdisplay, xroot,
                                  0, 0, width, height, 0, x11_state.xvisual.depth,
                                  InputOutput, x11_state.xvisual.visual,
                                  CWColormap | CWEventMask | CWBorderPixel,
                                  &xattrib);
  XSetWMProtocols(x11_state.xdisplay, window->xwindow, &x11_state.xatom_close, 1);

  XGCValues gcv;
  window->xgc = XCreateGC(x11_state.xdisplay, window->xwindow, 0, &gcv);

  Scratch scratch = ScratchBegin(&x11_state.arena, 1);
  XStoreName(x11_state.xdisplay, window->xwindow, cstr_from_str8(scratch.arena, name));
  ScratchEnd(scratch);

  XAutoRepeatOn(x11_state.xdisplay);
  XFlush(x11_state.xdisplay);

  OS_Handle res = {(u64)window};
  return res;
}

fn void os_window_show(OS_Handle window_) {
  X11_Window *window = (X11_Window*)window_.h[0];
  XMapWindow(x11_state.xdisplay, window->xwindow);
  XFlush(x11_state.xdisplay);
}

fn void os_window_close(OS_Handle window_) {
  X11_Window *window = (X11_Window*)window_.h[0];
  XUnmapWindow(x11_state.xdisplay, window->xwindow);
  os_gfx_context_window_deinit(window->gfx_context);

  XFreeGC(x11_state.xdisplay, window->xgc);
  XDestroyWindow(x11_state.xdisplay, window->xwindow);
  XFlush(x11_state.xdisplay);

  DLLDelete(x11_state.first_window, x11_state.last_window, window);
  StackPush(x11_state.freelist_window, window);
  if (!x11_state.first_window) {
    XAutoRepeatOn(x11_state.xdisplay);
    XCloseDisplay(x11_state.xdisplay);
  }
}

fn OS_Event os_window_get_event(OS_Handle window_) {
  X11_Window *window = (X11_Window*)window_.h[0];
  OS_Event res = {0};

  XEvent xevent;
  if (XCheckIfEvent(x11_state.xdisplay, &xevent,
                    x11_window_event_for_xwindow, (XPointer)&window->xwindow)) {
    res = x11_handle_xevent(window, &xevent);
  }

  return res;
}

fn OS_Event os_window_wait_event(OS_Handle window_) {
  X11_Window *window = (X11_Window*)window_.h[0];

  XEvent xevent;
  XIfEvent(x11_state.xdisplay, &xevent,
           x11_window_event_for_xwindow, (XPointer)&window->xwindow);
  return x11_handle_xevent(window, &xevent);
}

fn String8 os_keyname_from_event(Arena *arena, OS_Event event) {
  String8 res = {0};
  res.str = (u8*)XKeysymToString(event.key.keycode);
  if (res.str) { res.size = str8_len((char*)res.str); }
  return res;
}

// =============================================================================
// x11 specific definitions
fn Bool x11_window_event_for_xwindow(Display *_display, XEvent *event, XPointer arg) {
  return event->xany.window == *(Window*)arg;
}

fn OS_Event x11_handle_xevent(X11_Window *window, XEvent *xevent) {
  OS_Event res = {0};
  switch (xevent->type) {
  case Expose: {
    XWindowAttributes gwa;
    XGetWindowAttributes(x11_state.xdisplay, window->xwindow, &gwa);

    res.type = OS_EventType_Expose;
    res.expose.width = gwa.width;
    res.expose.height = gwa.height;
  } break;
  case KeyRelease:
  case KeyPress: {
    XKeyEvent *xkey = &xevent->xkey;
    res.type = xevent->type == KeyPress ? OS_EventType_KeyDown
                                        : OS_EventType_KeyUp;
    res.key.scancode = xkey->keycode;
    res.key.keycode = (u32)XkbKeycodeToKeysym(x11_state.xdisplay, (KeyCode)xkey->keycode, 0,
                                              xkey->state & ShiftMask ? 1 : 0);
  } break;
  case ClientMessage: {
    if ((Atom)xevent->xclient.data.l[0] == x11_state.xatom_close) {
      res.type = OS_EventType_Kill;
    }
  } break;
  }
  return res;
}

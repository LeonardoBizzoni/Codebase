global X11_State x11_state = {0};

fn void unx_gfx_init(void) {
  x11_state.arena = ArenaBuild();
  x11_state.xdisplay = XOpenDisplay(0);
  XSynchronize(x11_state.xdisplay, True);
  x11_state.xatom_close = XInternAtom(x11_state.xdisplay, "WM_DELETE_WINDOW", False);
}

fn void unx_gfx_deinit(void) {}

fn OS_Window os_window_open(String8 name, u32 x, u32 y, u32 width, u32 height) {
  X11_Window *window = x11_state.freelist_window;
  if (window) {
    memzero(window, sizeof(X11_Window));
    StackPop(x11_state.freelist_window);
  } else {
    window = New(x11_state.arena, X11_Window);
  }
  DLLPushBack(x11_state.first_window, x11_state.last_window, window);

  window->xscreen = DefaultScreen(x11_state.xdisplay);
  window->xattrib.event_mask = ExposureMask | FocusChangeMask |
                               EnterWindowMask | LeaveWindowMask |
                               ButtonPressMask | PointerMotionMask |
                               KeyPressMask | KeymapStateMask;

  Assert(XMatchVisualInfo(x11_state.xdisplay, window->xscreen, 32, TrueColor, &window->xvisual) &&
         window->xvisual.depth == 32 && window->xvisual.class == TrueColor);
  Window xroot = RootWindow(x11_state.xdisplay, window->xscreen);
  window->xattrib.colormap = XCreateColormap(x11_state.xdisplay, xroot,
                                             window->xvisual.visual, AllocNone);
  window->xattrib.background_pixel = 0;
  window->xattrib.border_pixel = 0;
  window->xwindow = XCreateWindow(x11_state.xdisplay, xroot,
                                  x, y, width, height, 0, window->xvisual.depth, InputOutput,
                                  window->xvisual.visual, CWColormap | CWEventMask | CWBorderPixel,
                                  &window->xattrib);
  XSetWMProtocols(x11_state.xdisplay, window->xwindow, &x11_state.xatom_close, 1);

  XGCValues gcv;
  window->xgc = XCreateGC(x11_state.xdisplay, window->xwindow, 0, &gcv);

  Scratch scratch = ScratchBegin(&x11_state.arena, 1);
  XStoreName(x11_state.xdisplay, window->xwindow, cstr_from_str8(scratch.arena, name));
  ScratchEnd(scratch);

  XAutoRepeatOn(x11_state.xdisplay);
  XFlush(x11_state.xdisplay);

  OS_Window res = {0};
  res.width = width;
  res.height = height;
  res.handle.h[0] = (u64)window;
  return res;
}

fn void os_window_show(OS_Window window_) {
  X11_Window *window = (X11_Window*)window_.handle.h[0];
  XMapWindow(x11_state.xdisplay, window->xwindow);
  XFlush(x11_state.xdisplay);
}

fn void os_window_hide(OS_Window window_) {
  X11_Window *window = (X11_Window*)window_.handle.h[0];
  XUnmapWindow(x11_state.xdisplay, window->xwindow);
  XFlush(x11_state.xdisplay);
}

fn void os_window_close(OS_Window window_) {
  X11_Window *window = (X11_Window*)window_.handle.h[0];
  XUnmapWindow(x11_state.xdisplay, window->xwindow);

#if USING_OPENGL
  opengl_deinit(window_);
#endif

  XDestroyWindow(x11_state.xdisplay, window->xwindow);
  XFlush(x11_state.xdisplay);

  DLLDelete(x11_state.first_window, x11_state.last_window, window);
  StackPush(x11_state.freelist_window, window);
  if (!x11_state.first_window) {
    XAutoRepeatOn(x11_state.xdisplay);
    XCloseDisplay(x11_state.xdisplay);
  }
}

fn void os_window_swapBuffers(OS_Window window) {
#if USING_OPENGL
  glXSwapBuffers(x11_state.xdisplay, ((X11_Window*)window.handle.h[0])->xwindow);
#endif
}

fn Bool unx_window_event_for_xwindow(Display *_display, XEvent *event, XPointer arg) {
  return event->xany.window == *(Window*)arg;
}

fn OS_Event unx_handle_xevent(X11_Window *window, XEvent *xevent) {
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
    res.key.keycode = XkbKeycodeToKeysym(x11_state.xdisplay, xkey->keycode, 0,
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

fn OS_Event os_window_get_event(OS_Window window_) {
  X11_Window *window = (X11_Window*)window_.handle.h[0];
  OS_Event res = {0};

  XEvent xevent;
  if (XCheckIfEvent(x11_state.xdisplay, &xevent,
                    unx_window_event_for_xwindow, (XPointer)&window->xwindow)) {
    res = unx_handle_xevent(window, &xevent);
  }

  return res;
}

fn OS_Event os_window_wait_event(OS_Window window_) {
  X11_Window *window = (X11_Window*)window_.handle.h[0];

  XEvent xevent;
  XIfEvent(x11_state.xdisplay, &xevent,
           unx_window_event_for_xwindow, (XPointer)&window->xwindow);
  return unx_handle_xevent(window, &xevent);
}

fn String8 os_keyname_from_event(Arena *arena, OS_Event event) {
  String8 res = {0};
  res.str = (u8*)XKeysymToString(event.key.keycode);
  if (res.str) { res.size = str8_len((char*)res.str); }
  return res;
}

fn void os_window_render(OS_Window window_, void *mem) {
  X11_Window *window = (X11_Window*)window_.handle.h[0];
  XImage *img = XCreateImage(x11_state.xdisplay, window->xvisual.visual,
                             window->xvisual.depth, ZPixmap, 0, (char*)mem,
                             window_.width, window_.height, 32, 0);
  Assert(img);
  XPutImage(x11_state.xdisplay, window->xwindow, window->xgc, img, 0, 0, 0, 0,
            window_.width, window_.height);
  XFlush(x11_state.xdisplay);
  img->data = 0; // XDestroyImage tries to free mem
  XDestroyImage(img);
}

#if USING_OPENGL
fn void opengl_init(OS_Window window_) {
  X11_Window *window = (X11_Window *)window_.handle.h[0];

  local i32 attr[] = { GLX_RGBA, GLX_DEPTH_SIZE, 32, GLX_DOUBLEBUFFER, None };
  XVisualInfo *info = glXChooseVisual(x11_state.xdisplay, window->xscreen, attr);
  Assert(info);

  window->xvisual = *info;
  window->context = glXCreateContext(x11_state.xdisplay, info, 0, true);
  glXMakeCurrent(x11_state.xdisplay, window->xwindow, window->context);
  glEnable(GL_DEPTH_TEST);
}

fn void opengl_deinit(OS_Window window_) {
  X11_Window *window = (X11_Window *)window_.handle.h[0];
  glXMakeCurrent(x11_state.xdisplay, None, 0);
  glXDestroyContext(x11_state.xdisplay, window->context);
  XFreeColormap(x11_state.xdisplay, window->xattrib.colormap);
}

fn void opengl_make_current(OS_Window window_) {
  X11_Window *window = (X11_Window*)window_.handle.h[0];
  glXMakeCurrent(x11_state.xdisplay, window->xwindow, window->context);
}
#endif

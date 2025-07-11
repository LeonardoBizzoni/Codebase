global LNX11_State lnx11_state = {0};

fn void lnx_gfx_init(void) {
  lnx11_state.arena = ArenaBuild();
  lnx11_state.xdisplay = XOpenDisplay(0);
  XSynchronize(lnx11_state.xdisplay, True);
  lnx11_state.xatom_close = XInternAtom(lnx11_state.xdisplay, "WM_DELETE_WINDOW", False);
}

fn OS_Window os_window_open(String8 name, u32 x, u32 y, u32 width, u32 height) {
  LNX11_Window *window = lnx11_state.freelist_window;
  if (window) {
    memzero(window, sizeof(LNX11_Window));
    StackPop(lnx11_state.freelist_window);
  } else {
    window = New(lnx11_state.arena, LNX11_Window);
  }
  DLLPushBack(lnx11_state.first_window, lnx11_state.last_window, window);

  window->xscreen = DefaultScreen(lnx11_state.xdisplay);
  window->xattrib.event_mask = ExposureMask | FocusChangeMask |
                               EnterWindowMask | LeaveWindowMask |
                               ButtonPressMask | PointerMotionMask |
                               KeyPressMask | KeymapStateMask;

  Assert(XMatchVisualInfo(lnx11_state.xdisplay, window->xscreen, 32, TrueColor, &window->xvisual) &&
         window->xvisual.depth == 32 && window->xvisual.class == TrueColor);
  Window xroot = RootWindow(lnx11_state.xdisplay, window->xscreen);
  window->xattrib.colormap = XCreateColormap(lnx11_state.xdisplay, xroot,
                                             window->xvisual.visual, AllocNone);
  window->xattrib.background_pixel = 0;
  window->xattrib.border_pixel = 0;
  window->xwindow = XCreateWindow(lnx11_state.xdisplay, xroot,
                                  x, y, width, height, 0, window->xvisual.depth, InputOutput,
                                  window->xvisual.visual, CWColormap | CWEventMask | CWBorderPixel,
                                  &window->xattrib);
  XSetWMProtocols(lnx11_state.xdisplay, window->xwindow, &lnx11_state.xatom_close, 1);

  XGCValues gcv;
  window->xgc = XCreateGC(lnx11_state.xdisplay, window->xwindow, 0, &gcv);

  Scratch scratch = ScratchBegin(&lnx11_state.arena, 1);
  XStoreName(lnx11_state.xdisplay, window->xwindow, cstr_from_str8(scratch.arena, name));
  ScratchEnd(scratch);

  XAutoRepeatOn(lnx11_state.xdisplay);
  XFlush(lnx11_state.xdisplay);

  OS_Window res = {0};
  res.width = width;
  res.height = height;
  res.handle.h[0] = (u64)window;
  return res;
}

fn void os_window_show(OS_Window window_) {
  LNX11_Window *window = (LNX11_Window*)window_.handle.h[0];
  XMapWindow(lnx11_state.xdisplay, window->xwindow);
  XFlush(lnx11_state.xdisplay);
}

fn void os_window_hide(OS_Window window_) {
  LNX11_Window *window = (LNX11_Window*)window_.handle.h[0];
  XUnmapWindow(lnx11_state.xdisplay, window->xwindow);
  XFlush(lnx11_state.xdisplay);
}

fn void os_window_close(OS_Window window_) {
  LNX11_Window *window = (LNX11_Window*)window_.handle.h[0];
  XUnmapWindow(lnx11_state.xdisplay, window->xwindow);

#if USING_OPENGL
  opengl_deinit(window_);
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

fn void os_window_swapBuffers(OS_Window window) {
#if USING_OPENGL
  glXSwapBuffers(lnx11_state.xdisplay, ((LNX11_Window*)window.handle.h[0])->xwindow);
#endif
}

fn Bool lnx_window_event_for_xwindow(Display *_display, XEvent *event, XPointer arg) {
  return event->xany.window == *(Window*)arg;
}

fn OS_Event lnx_handle_xevent(LNX11_Window *window, XEvent *xevent) {
  OS_Event res = {0};
  switch (xevent->type) {
  case Expose: {
    XWindowAttributes gwa;
    XGetWindowAttributes(lnx11_state.xdisplay, window->xwindow, &gwa);

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
    res.key.keycode = XkbKeycodeToKeysym(lnx11_state.xdisplay, xkey->keycode, 0,
                                         xkey->state & ShiftMask ? 1 : 0);
  } break;
  case ClientMessage: {
    if ((Atom)xevent->xclient.data.l[0] == lnx11_state.xatom_close) {
      res.type = OS_EventType_Kill;
    }
  } break;
  }
  return res;
}

fn OS_Event os_window_get_event(OS_Window window_) {
  LNX11_Window *window = (LNX11_Window*)window_.handle.h[0];
  OS_Event res = {0};

  XEvent xevent;
  if (XCheckIfEvent(lnx11_state.xdisplay, &xevent,
                    lnx_window_event_for_xwindow, (XPointer)&window->xwindow)) {
    res = lnx_handle_xevent(window, &xevent);
  }

  return res;
}

fn OS_Event os_window_wait_event(OS_Window window_) {
  LNX11_Window *window = (LNX11_Window*)window_.handle.h[0];

  XEvent xevent;
  XIfEvent(lnx11_state.xdisplay, &xevent,
           lnx_window_event_for_xwindow, (XPointer)&window->xwindow);
  return lnx_handle_xevent(window, &xevent);
}

fn String8 os_keyname_from_event(Arena *arena, OS_Event event) {
  String8 res = {0};
  res.str = (u8*)XKeysymToString(event.key.keycode);
  if (res.str) { res.size = str8_len((char*)res.str); }
  return res;
}

fn void os_window_render(OS_Window window_, void *mem) {
  LNX11_Window *window = (LNX11_Window*)window_.handle.h[0];

  // NOTE(lb): apperantly alpha=0 means transparent?
  u32 *pixels = mem;
  for (usize i = 0; i < window_.height * window_.width; ++i) {
    u32 pixel = pixels[i];
    u8 alpha = 255 - (pixel >> 24);
    pixels[i] = ((u32)alpha << 24) | (pixel & 0x00FFFFFF);
  }

  XImage *img = XCreateImage(lnx11_state.xdisplay, window->xvisual.visual,
                             window->xvisual.depth, ZPixmap, 0, (char*)mem,
                             window_.width, window_.height, 32, 0);
  Assert(img);
  XPutImage(lnx11_state.xdisplay, window->xwindow, window->xgc, img, 0, 0, 0, 0,
            window_.width, window_.height);
  XFlush(lnx11_state.xdisplay);
  img->data = 0; // XDestroyImage tries to free mem
  XDestroyImage(img);
}

#if USING_OPENGL
fn void opengl_init(OS_Window window_) {
  LNX11_Window *window = (LNX11_Window *)window_.handle.h[0];

  local i32 attr[] = { GLX_RGBA, GLX_DEPTH_SIZE, 32, GLX_DOUBLEBUFFER, None };
  XVisualInfo *info = glXChooseVisual(lnx11_state.xdisplay, window->xscreen, attr);
  Assert(info);

  window->xvisual = *info;
  window->context = glXCreateContext(lnx11_state.xdisplay, info, 0, true);
  glXMakeCurrent(lnx11_state.xdisplay, window->xwindow, window->context);
  glEnable(GL_DEPTH_TEST);
}

fn void opengl_deinit(OS_Window window_) {
  LNX11_Window *window = (LNX11_Window *)window_.handle.h[0];
  glXMakeCurrent(lnx11_state.xdisplay, None, 0);
  glXDestroyContext(lnx11_state.xdisplay, window->context);
  XFreeColormap(lnx11_state.xdisplay, window->xattrib.colormap);
}

fn void opengl_make_current(OS_Window window_) {
  LNX11_Window *window = (LNX11_Window*)window_.handle.h[0];
  glXMakeCurrent(lnx11_state.xdisplay, window->xwindow, window->context);
}
#endif

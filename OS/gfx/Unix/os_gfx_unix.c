global X11_State x11_state = {0};

fn OS_Handle os_window_open(String8 name, i32 width, i32 height) {
  Assert(width > 0 && height > 0);
  X11_Window *window = x11_state.freelist_window;
  if (window) {
    memzero(window, sizeof(X11_Window));
    StackPop(x11_state.freelist_window);
  } else {
    window = arena_push(x11_state.arena, X11_Window);
  }
  DLLPushBack(x11_state.first_window, x11_state.last_window, window);

  XSetWindowAttributes xattrib = {0};
  xattrib.colormap = XCreateColormap(x11_state.xdisplay, x11_state.xroot,
                                     x11_state.xvisual.visual, AllocNone);
  xattrib.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask |
                       PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
                       ExposureMask | FocusChangeMask | VisibilityChangeMask |
                       EnterWindowMask | LeaveWindowMask | PropertyChangeMask;
  window->xwindow = XCreateWindow(x11_state.xdisplay, x11_state.xroot,
                                  0, 0, (u32)width, (u32)height, 0,
                                  x11_state.xvisual.depth,
                                  InputOutput, x11_state.xvisual.visual,
                                  CWColormap | CWEventMask | CWBorderPixel,
                                  &xattrib);
  XSetWMProtocols(x11_state.xdisplay, window->xwindow, &x11_state.xatom_close, 1);

  XGCValues gcv;
  window->xgc = XCreateGC(x11_state.xdisplay, window->xwindow, 0, &gcv);

  Scratch scratch = ScratchBegin(&x11_state.arena, 1);
  XStoreName(x11_state.xdisplay, window->xwindow,
             cstr_from_str8(scratch.arena, name));
  ScratchEnd(scratch);
  XFlush(x11_state.xdisplay);

  OS_Handle res = {{(u64)window}};
  return res;
}

fn void os_window_show(OS_Handle window_) {
  X11_Window *window = (X11_Window*)window_.h[0];
  XMapWindow(x11_state.xdisplay, window->xwindow);
  XFlush(x11_state.xdisplay);
}

fn void os_window_hide(OS_Handle window_) {
  X11_Window *window = (X11_Window*)window_.h[0];
  XUnmapWindow(x11_state.xdisplay, window->xwindow);
  XFlush(x11_state.xdisplay);
}

fn void os_window_close(OS_Handle window_) {
  X11_Window *window = (X11_Window*)window_.h[0];
  XUnmapWindow(x11_state.xdisplay, window->xwindow);

  XFreeGC(x11_state.xdisplay, window->xgc);
  XDestroyWindow(x11_state.xdisplay, window->xwindow);
  XFlush(x11_state.xdisplay);

  DLLDelete(x11_state.first_window, x11_state.last_window, window);
  StackPush(x11_state.freelist_window, window);
}

fn void os_window_get_size(OS_Handle hwindow, i32 *width, i32 *height) {
  X11_Window *window = (X11_Window*)hwindow.h[0];
  XWindowAttributes attribs = {0};
  XGetWindowAttributes(x11_state.xdisplay, window->xwindow, &attribs);
  *width = attribs.width;
  *height = attribs.height;
}

fn String8 os_keyname_from_event(Arena *arena, OS_Event event) {
  Unused(arena);
  String8 res = {0};
  res.str = (u8*)XKeysymToString(event.key.keycode);
  if (res.str) { res.size = str8_len((char*)res.str); }
  return res;
}

fn OS_EventList os_get_events(Arena *arena, bool wait) {
  i32 repeated_expose_events = 0;

  OS_EventList res = {0};
  for (; XPending(x11_state.xdisplay) || (wait && res.count == 0); ) {
    XEvent xevent = {0};
    XNextEvent(x11_state.xdisplay, &xevent);
    X11_Window *target_window = lnx_window_from_xwindow(xevent.xany.window);
    if (!target_window) { continue; }

    OS_Event *event = arena_push(arena, OS_Event);
    event->window.h[0] = (u64)target_window;

    switch (xevent.type) {
    case Expose: {
      if (repeated_expose_events) {
        repeated_expose_events -= 1;
      } else {
        XWindowAttributes gwa;
        XGetWindowAttributes(x11_state.xdisplay, xevent.xany.window, &gwa);
        event->type = OS_EventType_Expose;
        event->expose.width = gwa.width;
        Assert(event->expose.width != 0);
        event->expose.height = gwa.height;
        Assert(event->expose.height != 0);
        repeated_expose_events = xevent.xexpose.count;
      }
    } break;
    case KeyRelease:
    case KeyPress: {
      XKeyEvent *xkey = &xevent.xkey;
      event->type = xevent.type == KeyPress
                    ? OS_EventType_KeyDown : OS_EventType_KeyUp;
      event->key.scancode = xkey->keycode;
      event->key.keycode =
        (u32)XkbKeycodeToKeysym(x11_state.xdisplay, (KeyCode)xkey->keycode, 0,
                                xkey->state & ShiftMask ? 1 : 0);
    } break;
    case ClientMessage: {
      if ((Atom)xevent.xclient.data.l[0] == x11_state.xatom_close) {
        event->type = OS_EventType_Kill;
      }
    } break;
    }

    QueuePush(res.first, res.last, event);
  }

  return res;
}

// =============================================================================
// Platform specific definitions
fn void unx_gfx_init(void) {
  x11_state.arena = arena_build();
  x11_state.xdisplay = XOpenDisplay(0);
  x11_state.xroot = RootWindow(x11_state.xdisplay, x11_state.xscreen);
  XSynchronize(x11_state.xdisplay, True);
  x11_state.xscreen = DefaultScreen(x11_state.xdisplay);
  x11_state.xatom_close = XInternAtom(x11_state.xdisplay, "WM_DELETE_WINDOW", False);

  i32 visuals_count = 0;
  XVisualInfo xvisual = { .screen = x11_state.xscreen };
  XVisualInfo *visuals = XGetVisualInfo(x11_state.xdisplay, VisualScreenMask,
                                        &xvisual, &visuals_count);
  Assert(visuals && visuals_count > 0);
  x11_state.xvisual = *visuals;
  XFree(visuals);

  rhi_init();
}

fn void unx_gfx_deinit(void) {
  XAutoRepeatOn(x11_state.xdisplay);
  XCloseDisplay(x11_state.xdisplay);
}

internal X11_Window* lnx_window_from_xwindow(Window xwindow) {
  for (X11_Window *curr = x11_state.first_window; curr; curr = curr->next) {
    if (curr->xwindow == xwindow) { return curr; }
  }
  return 0;
}

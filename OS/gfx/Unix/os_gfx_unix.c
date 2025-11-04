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
    X11_Window *target_window = unx_window_from_xwindow(xevent.xany.window);
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

fn bool os_key_is_down(OS_Key key) {
  char bitarray[32] = {0};
  XQueryKeymap(x11_state.xdisplay, bitarray);
  KeySym keysym = unx_keysym_from_os_key(key);
  KeyCode keycode = XKeysymToKeycode(x11_state.xdisplay, keysym);
  return bitarray[keycode >> 3] & (1 << (keycode & 7));
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

internal X11_Window* unx_window_from_xwindow(Window xwindow) {
  for (X11_Window *curr = x11_state.first_window; curr; curr = curr->next) {
    if (curr->xwindow == xwindow) { return curr; }
  }
  return 0;
}

internal KeySym unx_keysym_from_os_key(OS_Key key) {
  switch (key) {
  case OS_Key_Esc:          return XK_Escape;
  case OS_Key_F1:           return XK_F1;
  case OS_Key_F2:           return XK_F2;
  case OS_Key_F3:           return XK_F3;
  case OS_Key_F4:           return XK_F4;
  case OS_Key_F5:           return XK_F5;
  case OS_Key_F6:           return XK_F6;
  case OS_Key_F7:           return XK_F7;
  case OS_Key_F8:           return XK_F8;
  case OS_Key_F9:           return XK_F9;
  case OS_Key_F10:          return XK_F10;
  case OS_Key_F11:          return XK_F11;
  case OS_Key_F12:          return XK_F12;
  case OS_Key_Tick:         return XK_grave;
  case OS_Key_1:            return XK_1;
  case OS_Key_2:            return XK_2;
  case OS_Key_3:            return XK_3;
  case OS_Key_4:            return XK_4;
  case OS_Key_5:            return XK_5;
  case OS_Key_6:            return XK_6;
  case OS_Key_7:            return XK_7;
  case OS_Key_8:            return XK_8;
  case OS_Key_9:            return XK_9;
  case OS_Key_0:            return XK_0;
  case OS_Key_Minus:        return XK_minus;
  case OS_Key_Equal:        return XK_equal;
  case OS_Key_Backspace:    return XK_BackSpace;
  case OS_Key_Tab:          return XK_Tab;
  case OS_Key_Q:            return XK_q;
  case OS_Key_W:            return XK_w;
  case OS_Key_E:            return XK_e;
  case OS_Key_R:            return XK_r;
  case OS_Key_T:            return XK_t;
  case OS_Key_Y:            return XK_y;
  case OS_Key_U:            return XK_u;
  case OS_Key_I:            return XK_i;
  case OS_Key_O:            return XK_o;
  case OS_Key_P:            return XK_p;
  case OS_Key_LeftBracket:  return XK_bracketleft;
  case OS_Key_RightBracket: return XK_bracketright;
  case OS_Key_BackSlash:    return XK_backslash;
  case OS_Key_CapsLock:     return XK_Caps_Lock;
  case OS_Key_A:            return XK_a;
  case OS_Key_S:            return XK_s;
  case OS_Key_D:            return XK_d;
  case OS_Key_F:            return XK_f;
  case OS_Key_G:            return XK_g;
  case OS_Key_H:            return XK_h;
  case OS_Key_J:            return XK_j;
  case OS_Key_K:            return XK_k;
  case OS_Key_L:            return XK_l;
  case OS_Key_Semicolon:    return XK_semicolon;
  case OS_Key_Quote:        return XK_apostrophe;
  case OS_Key_Return:       return XK_Return;
  case OS_Key_Shift:        return XK_Shift_L;
  case OS_Key_Z:            return XK_z;
  case OS_Key_X:            return XK_x;
  case OS_Key_C:            return XK_c;
  case OS_Key_V:            return XK_v;
  case OS_Key_B:            return XK_b;
  case OS_Key_N:            return XK_n;
  case OS_Key_M:            return XK_m;
  case OS_Key_Comma:        return XK_comma;
  case OS_Key_Period:       return XK_period;
  case OS_Key_Slash:        return XK_slash;
  case OS_Key_Ctrl:         return XK_Control_L;
  case OS_Key_Alt:          return XK_Alt_L;
  case OS_Key_Space:        return XK_space;
  case OS_Key_ScrollLock:   return XK_Scroll_Lock;
  case OS_Key_Pause:        return XK_Pause;
  case OS_Key_Insert:       return XK_Insert;
  case OS_Key_Home:         return XK_Home;
  case OS_Key_Delete:       return XK_Delete;
  case OS_Key_End:          return XK_End;
  case OS_Key_PageUp:       return XK_Page_Up;
  case OS_Key_PageDown:     return XK_Page_Down;
  case OS_Key_Up:           return XK_Up;
  case OS_Key_Left:         return XK_Left;
  case OS_Key_Down:         return XK_Down;
  case OS_Key_Right:        return XK_Right;
  default:                  Unreachable();
  }
  return NoSymbol;
}

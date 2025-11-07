global OS_GFX_Posix_State os_gfx_posix_state = {0};

fn OS_Handle os_window_open(String8 name, i32 width, i32 height) {
  Assert(width > 0 && height > 0);
  OS_GFX_Posix_Window *window = os_gfx_posix_state.freelist_window;
  if (window) {
    memzero(window, sizeof(OS_GFX_Posix_Window));
    StackPop(os_gfx_posix_state.freelist_window);
  } else {
    window = arena_push(os_posix_state.arena, OS_GFX_Posix_Window);
  }
  DLLPushBack(os_gfx_posix_state.first_window, os_gfx_posix_state.last_window, window);

  XSetWindowAttributes xattrib = {0};
  xattrib.colormap = XCreateColormap(os_gfx_posix_state.xdisplay, os_gfx_posix_state.xroot,
                                     os_gfx_posix_state.xvisual.visual, AllocNone);
  xattrib.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask |
                       PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
                       ExposureMask | FocusChangeMask | VisibilityChangeMask |
                       EnterWindowMask | LeaveWindowMask | PropertyChangeMask;
  window->xwindow = XCreateWindow(os_gfx_posix_state.xdisplay, os_gfx_posix_state.xroot,
                                  0, 0, (u32)width, (u32)height, 0,
                                  os_gfx_posix_state.xvisual.depth,
                                  InputOutput, os_gfx_posix_state.xvisual.visual,
                                  CWColormap | CWEventMask | CWBorderPixel,
                                  &xattrib);
  XSetWMProtocols(os_gfx_posix_state.xdisplay, window->xwindow, &os_gfx_posix_state.xatom_close, 1);

  XGCValues gcv;
  window->xgc = XCreateGC(os_gfx_posix_state.xdisplay, window->xwindow, 0, &gcv);

  {
    Scratch scratch = ScratchBegin(&os_posix_state.arena, 1);
    XStoreName(os_gfx_posix_state.xdisplay, window->xwindow,
               cstr_from_str8(scratch.arena, name));
    ScratchEnd(scratch);
  }
  XFlush(os_gfx_posix_state.xdisplay);

  OS_Handle res = {{(u64)window}};
  return res;
}

fn void os_window_show(OS_Handle hwindow) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)hwindow.h[0];
  XMapWindow(os_gfx_posix_state.xdisplay, window->xwindow);
  XFlush(os_gfx_posix_state.xdisplay);
}

fn void os_window_hide(OS_Handle hwindow) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)hwindow.h[0];
  XUnmapWindow(os_gfx_posix_state.xdisplay, window->xwindow);
  XFlush(os_gfx_posix_state.xdisplay);
}

fn void os_window_minimize(OS_Handle hwindow) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)hwindow.h[0];

  XEvent event = {0};
  event.xclient.type = ClientMessage;
  event.xclient.window = window->xwindow;
  event.xclient.message_type = os_gfx_posix_state.xatom_change_state;
  event.xclient.format = 32;
  event.xclient.data.l[0] = IconicState;

  XSendEvent(os_gfx_posix_state.xdisplay, os_gfx_posix_state.xroot, False,
             SubstructureRedirectMask | SubstructureNotifyMask, &event);
  XFlush(os_gfx_posix_state.xdisplay);
}

fn void os_window_close(OS_Handle hwindow) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)hwindow.h[0];
  XUnmapWindow(os_gfx_posix_state.xdisplay, window->xwindow);

  XFreeGC(os_gfx_posix_state.xdisplay, window->xgc);
  XDestroyWindow(os_gfx_posix_state.xdisplay, window->xwindow);
  XFlush(os_gfx_posix_state.xdisplay);

  DLLDelete(os_gfx_posix_state.first_window, os_gfx_posix_state.last_window, window);
  StackPush(os_gfx_posix_state.freelist_window, window);
}

fn void os_window_toggle_fullscreen(OS_Handle hwindow) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)hwindow.h[0];
  XEvent event = {0};
  event.xclient.type = ClientMessage;
  event.xclient.send_event = True;
  event.xclient.window = window->xwindow;
  event.xclient.message_type = os_gfx_posix_state.xatom_state;
  event.xclient.format = 32;
  event.xclient.data.l[0] = os_window_is_fullscreen(hwindow) ? 0 : 1;
  event.xclient.data.l[1] = (i64)os_gfx_posix_state.xatom_state_fullscreen;
  event.xclient.data.l[3] = 1;

  XSendEvent(os_gfx_posix_state.xdisplay, os_gfx_posix_state.xroot, False,
             SubstructureRedirectMask | SubstructureNotifyMask, &event);
  XFlush(os_gfx_posix_state.xdisplay);
}

fn void os_window_toggle_maximized(OS_Handle hwindow) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)hwindow.h[0];

  XEvent event = {0};
  event.xclient.type = ClientMessage;
  event.xclient.window = window->xwindow;
  event.xclient.message_type = os_gfx_posix_state.xatom_state;
  event.xclient.format = 32;
  event.xclient.data.l[0] = os_window_is_maximized(hwindow) ? 0 : 1;
  event.xclient.data.l[1] = (i32)os_gfx_posix_state.xatom_state_maximized_horz;
  event.xclient.data.l[2] = (i32)os_gfx_posix_state.xatom_state_maximized_vert;
  event.xclient.data.l[3] = 1;
  event.xclient.data.l[4] = 0;

  XSendEvent(os_gfx_posix_state.xdisplay, os_gfx_posix_state.xroot, False,
             SubstructureRedirectMask | SubstructureNotifyMask, &event);
  XFlush(os_gfx_posix_state.xdisplay);
}

fn void os_window_toggle_borderless(OS_Handle hwindow) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)hwindow.h[0];
  OS_GFX_Posix_MotifWMHint hints = {0};
  hints.flags = 1L << 1;
  hints.decorations = os_window_is_borderless(hwindow);
  XUnmapWindow(os_gfx_posix_state.xdisplay, window->xwindow);
  XChangeProperty(os_gfx_posix_state.xdisplay, window->xwindow,
                  os_gfx_posix_state.xatom_motif_wm_hints,
                  os_gfx_posix_state.xatom_motif_wm_hints, 32,
                  PropModeReplace, (u8 *)&hints, 5);
  XMapWindow(os_gfx_posix_state.xdisplay, window->xwindow);
  XFlush(os_gfx_posix_state.xdisplay);
}

fn bool os_window_is_fullscreen(OS_Handle hwindow) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)hwindow.h[0];
  return os_posix_window_check_xatom(window, os_gfx_posix_state.xatom_state_fullscreen);
}

fn bool os_window_is_minimized(OS_Handle hwindow) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)hwindow.h[0];
  return os_posix_window_check_xatom(window, os_gfx_posix_state.xatom_state_hidden);
}

fn bool os_window_is_maximized(OS_Handle hwindow) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)hwindow.h[0];
  return os_posix_window_check_xatom(window, os_gfx_posix_state.xatom_state_maximized_horz) &&
         os_posix_window_check_xatom(window, os_gfx_posix_state.xatom_state_maximized_vert);
}

fn bool os_window_is_borderless(OS_Handle hwindow) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)hwindow.h[0];

  Atom actual_type;
  i32 actual_format;
  u64 atom_count, bytes_after;
  u8 *prop = NULL;
  if (XGetWindowProperty(os_gfx_posix_state.xdisplay,
                         window->xwindow, os_gfx_posix_state.xatom_motif_wm_hints,
                         0, 5, False, os_gfx_posix_state.xatom_motif_wm_hints,
                         &actual_type, &actual_format,
                         &atom_count, &bytes_after, &prop) != Success) {
    return false;
  }
  if (!prop || atom_count < 5 || actual_format != 32) { return false; }

  bool res = false;
  OS_GFX_Posix_MotifWMHint *hints = (OS_GFX_Posix_MotifWMHint*)prop;
  if (hints->decorations == 0 && hints->flags & (1L << 1)) {
    res = true;
  }
  XFree(prop);
  return res;
}

fn bool os_window_is_focused(OS_Handle hwindow) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)hwindow.h[0];
  Window xwindow_focused;
  i32 revert_to;
  XGetInputFocus(os_gfx_posix_state.xdisplay, &xwindow_focused, &revert_to);
  return xwindow_focused == window->xwindow;
}

fn void os_window_set_title(OS_Handle hwindow, String8 title) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)hwindow.h[0];
  {
    Scratch scratch = ScratchBegin(0, 0);
    XStoreName(os_gfx_posix_state.xdisplay, window->xwindow,
               cstr_from_str8(scratch.arena, title));
    ScratchEnd(scratch);
  }
  XFlush(os_gfx_posix_state.xdisplay);
}

fn void os_window_set_position(OS_Handle hwindow, i32 x, i32 y) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)hwindow.h[0];
  XMoveWindow(os_gfx_posix_state.xdisplay, window->xwindow, x, y);
  XFlush(os_gfx_posix_state.xdisplay);
}

fn void os_window_set_size(OS_Handle hwindow, i32 width, i32 height) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)hwindow.h[0];
  XResizeWindow(os_gfx_posix_state.xdisplay, window->xwindow, (u32)width, (u32)height);
  XFlush(os_gfx_posix_state.xdisplay);
}

fn void os_window_get_size(OS_Handle hwindow, i32 *width, i32 *height) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)hwindow.h[0];
  XWindowAttributes attribs = {0};
  XGetWindowAttributes(os_gfx_posix_state.xdisplay, window->xwindow, &attribs);
  *width = attribs.width;
  *height = attribs.height;
}

fn void os_window_set_monitor(OS_Handle hwindow, OS_Handle hmonitor) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)hwindow.h[0];
  XRRMonitorInfo *xmonitor_target = (XRRMonitorInfo *)hmonitor.h[0];
  XRRMonitorInfo *xmonitor_source = os_gfx_posix_monitor_from_window(window);

  XWindowAttributes window_attributes = {0};
  XGetWindowAttributes(os_gfx_posix_state.xdisplay, window->xwindow, &window_attributes);

  i32 window_relative_x = window_attributes.x - xmonitor_source->x;
  i32 window_relative_y = window_attributes.y - xmonitor_source->y;
  XMoveWindow(os_gfx_posix_state.xdisplay, window->xwindow,
              xmonitor_target->x + window_relative_x,
              xmonitor_target->y + window_relative_y);
}

fn OS_HandleArray os_monitor_array(Arena *arena) {
  OS_HandleArray res = {0};
  res.size = os_gfx_posix_state.xmonitors_count;
  res.handles = arena_push_many(arena, OS_Handle, os_gfx_posix_state.xmonitors_count);
  for (i32 i = 0; i < os_gfx_posix_state.xmonitors_count; ++i) {
    res.handles[i] = (OS_Handle) {{ (u64)&os_gfx_posix_state.xmonitors[i] }};
  }
  return res;
}

fn OS_Handle os_monitor_from_window(OS_Handle hwindow) {
  OS_GFX_Posix_Window *window = (OS_GFX_Posix_Window*)hwindow.h[0];
  OS_Handle res = {{ (u64)os_gfx_posix_monitor_from_window(window) }};
  return res;
}

fn OS_Handle os_monitor_primary(void) {
  XRRMonitorInfo *xmonitor = 0;
  for (i32 i = 0; i < os_gfx_posix_state.xmonitors_count; ++i) {
    if (os_gfx_posix_state.xmonitors[i].primary) {
      xmonitor = &os_gfx_posix_state.xmonitors[i];
    }
  }
  Assert(xmonitor);
  OS_Handle res = {{ (u64)xmonitor }};
  return res;
}

fn String8 os_monitor_name(Arena *arena, OS_Handle hmonitor) {
  XRRMonitorInfo *xmonitor = (XRRMonitorInfo *)hmonitor.h[0];
  char *cname = XGetAtomName(os_gfx_posix_state.xdisplay, xmonitor->name);
  String8 res = {0};
  res.size = str8_len(cname);
  res.str = arena_push_many(arena, u8, res.size);
  memcopy(res.str, cname, res.size);
  XFree(cname);
  return res;
}

fn void os_monitor_size(OS_Handle hmonitor, i32 *width, i32 *height) {
  XRRMonitorInfo *xmonitor = (XRRMonitorInfo *)hmonitor.h[0];
  *width = xmonitor->width;
  *height = xmonitor->height;
}

fn String8 os_key_name_from_event(Arena *arena, OS_Event event) {
  Unused(arena);
  String8 res = {0};
  res.str = (u8*)XKeysymToString(event.key.keycode);
  if (res.str) { res.size = str8_len((char*)res.str); }
  return res;
}

fn bool os_key_is_down(OS_Key key) {
  char bitarray[32] = {0};
  XQueryKeymap(os_gfx_posix_state.xdisplay, bitarray);
  KeySym keysym = os_posix_keysym_from_os_key(key);
  KeyCode keycode = XKeysymToKeycode(os_gfx_posix_state.xdisplay, keysym);
  return bitarray[keycode >> 3] & (1 << (keycode & 7));
}

fn OS_EventList os_get_events(Arena *arena, bool wait) {
  i32 repeated_expose_events = 0;

  OS_EventList res = {0};
  for (; XPending(os_gfx_posix_state.xdisplay) || (wait && res.count == 0); ) {
    XEvent xevent = {0};
    XNextEvent(os_gfx_posix_state.xdisplay, &xevent);
    OS_GFX_Posix_Window *target_window = os_posix_window_from_xwindow(xevent.xany.window);
    if (!target_window) { continue; }

    OS_Event *event = arena_push(arena, OS_Event);
    event->window.h[0] = (u64)target_window;

    switch (xevent.type) {
    case Expose: {
      if (repeated_expose_events) {
        repeated_expose_events -= 1;
      } else {
        XWindowAttributes gwa;
        XGetWindowAttributes(os_gfx_posix_state.xdisplay, xevent.xany.window, &gwa);
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
      event->type = (xevent.type == KeyPress ? OS_EventType_KeyDown : OS_EventType_KeyUp);
      event->key.scancode = xkey->keycode;
      event->key.keycode = (u32)XkbKeycodeToKeysym(os_gfx_posix_state.xdisplay, (KeyCode)xkey->keycode, 0, (xkey->state & ShiftMask ? 1 : 0));
    } break;
    case ClientMessage: {
      if ((Atom)xevent.xclient.data.l[0] == os_gfx_posix_state.xatom_close) {
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
internal void os_gfx_init(void) {
  os_gfx_posix_state.xdisplay = XOpenDisplay(0);
  os_gfx_posix_state.xscreen = DefaultScreen(os_gfx_posix_state.xdisplay);
  os_gfx_posix_state.xroot = RootWindow(os_gfx_posix_state.xdisplay, os_gfx_posix_state.xscreen);
  os_gfx_posix_state.xatom_close = XInternAtom(os_gfx_posix_state.xdisplay, "WM_DELETE_WINDOW", False);
  os_gfx_posix_state.xatom_change_state = XInternAtom(os_gfx_posix_state.xdisplay, "WM_CHANGE_STATE", False);
  os_gfx_posix_state.xatom_active_window = XInternAtom(os_gfx_posix_state.xdisplay, "_NET_ACTIVE_WINDOW", False);
  os_gfx_posix_state.xatom_state = XInternAtom(os_gfx_posix_state.xdisplay, "_NET_WM_STATE", False);
  os_gfx_posix_state.xatom_state_hidden = XInternAtom(os_gfx_posix_state.xdisplay, "_NET_WM_STATE_HIDDEN", False);
  os_gfx_posix_state.xatom_state_fullscreen = XInternAtom(os_gfx_posix_state.xdisplay, "_NET_WM_STATE_FULLSCREEN", False);
  os_gfx_posix_state.xatom_state_maximized_horz = XInternAtom(os_gfx_posix_state.xdisplay, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
  os_gfx_posix_state.xatom_state_maximized_vert = XInternAtom(os_gfx_posix_state.xdisplay, "_NET_WM_STATE_MAXIMIZED_VERT", False);
  os_gfx_posix_state.xatom_motif_wm_hints = XInternAtom(os_gfx_posix_state.xdisplay, "_MOTIF_WM_HINTS", False);
  XSynchronize(os_gfx_posix_state.xdisplay, True);

  XRRMonitorInfo *active_monitors = XRRGetMonitors(os_gfx_posix_state.xdisplay, os_gfx_posix_state.xroot, True,
                                                   &os_gfx_posix_state.xmonitors_count);
  os_gfx_posix_state.xmonitors = arena_push_many(os_posix_state.arena, XRRMonitorInfo,
                                                 os_gfx_posix_state.xmonitors_count);
  memcopy(os_gfx_posix_state.xmonitors, active_monitors,
          sizeof(*active_monitors) * (u32)os_gfx_posix_state.xmonitors_count);
  XRRFreeMonitors(active_monitors);
}

internal void os_gfx_deinit(void) {
  XAutoRepeatOn(os_gfx_posix_state.xdisplay);
  XCloseDisplay(os_gfx_posix_state.xdisplay);
}

internal XRRMonitorInfo* os_gfx_posix_monitor_from_window(OS_GFX_Posix_Window *window) {
  XWindowAttributes window_attributes = {0};
  XGetWindowAttributes(os_gfx_posix_state.xdisplay, window->xwindow, &window_attributes);

  i32 window_center_x = window_attributes.x + window_attributes.width  / 2;
  i32 window_center_y = window_attributes.y + window_attributes.height / 2;

  XRRMonitorInfo *window_monitor = 0;
  for (i32 i = 0; i < os_gfx_posix_state.xmonitors_count; ++i) {
    XRRMonitorInfo *xmonitor = &os_gfx_posix_state.xmonitors[i];
    if (window_center_x >= xmonitor->x &&
        window_center_x < xmonitor->x + xmonitor->width &&
        window_center_y >= xmonitor->y &&
        window_center_y < xmonitor->y + xmonitor->width) {
      window_monitor = xmonitor;
    }
  }
  Assert(window_monitor);
  return window_monitor;
}

internal bool os_posix_window_check_xatom(OS_GFX_Posix_Window *window, Atom target) {
  Atom actual_type;
  i32 actual_format;
  u64 atom_count, bytes_after;
  u8 *prop = NULL;
  if (XGetWindowProperty(os_gfx_posix_state.xdisplay,
                         window->xwindow, os_gfx_posix_state.xatom_state,
                         0, (~0L), False, XA_ATOM, &actual_type, &actual_format,
                         &atom_count, &bytes_after, &prop) != Success) {
    return false;
  }
  if (!prop) { return false; }

  bool res = false;
  Atom *atoms = (Atom*)prop;
  for (u64 i = 0; i < atom_count; ++i) {
    if (atoms[i] == target) {
      res = true;
      break;
    }
  }
  XFree(prop);
  return res;
}

internal OS_GFX_Posix_Window* os_posix_window_from_xwindow(Window xwindow) {
  for (OS_GFX_Posix_Window *curr = os_gfx_posix_state.first_window;
       curr;
       curr = curr->next) {
    if (curr->xwindow == xwindow) { return curr; }
  }
  return 0;
}

internal KeySym os_posix_keysym_from_os_key(OS_Key key) {
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

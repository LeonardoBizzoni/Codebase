global Wl_State waystate = {0};

global struct wl_registry_listener wl_registry_listener = {
#undef global
  .global = wl_registry_event_global,
  .global_remove = 0,
#define global static
};

global struct wl_seat_listener wl_seat_listener = {
  .capabilities = wl_seat_event_capabilities,
  .name = wl_seat_event_name,
};

global struct wl_callback_listener wl_callback_listener = {
  .done = wl_callback_event_done,
};

global struct wl_keyboard_listener wl_keyboard_listener = {
  .keymap = wl_keyboard_event_keymap,
  .enter = wl_keyboard_event_enter,
  .leave = wl_keyboard_event_leave,
  .key = wl_keyboard_event_key,
  .modifiers = wl_keyboard_event_modifiers,
  .repeat_info = wl_keyboard_event_repeat_info,
};

global struct wl_pointer_listener wl_pointer_listener = {
  .enter = wl_pointer_event_enter,
  .leave = wl_pointer_event_leave,
  .motion = wl_pointer_event_motion,
  .button = wl_pointer_event_button,
  .axis = wl_pointer_event_axis,
  .frame = wl_pointer_event_frame,
  .axis_source = wl_pointer_event_axis_source,
  .axis_stop = wl_pointer_event_axis_stop,
  .axis_value120 = wl_pointer_event_axis_value120,
  .axis_relative_direction = wl_pointer_event_axis_relative_direction,
};

global struct xdg_wm_base_listener xdg_wm_base_listener_listener = {
  .ping = xdg_wm_base_even_ping,
};

global struct xdg_surface_listener xdg_surface_listener = {
  .configure = xdg_surface_event_configure,
};

global struct xdg_toplevel_listener xdg_toplevel_listener = {
  .configure = xdg_toplevel_event_configure,
  .close = xdg_toplevel_event_close,
  .configure_bounds = xdg_toplevel_event_configure_bounds,
  .wm_capabilities = xdg_toplevel_event_wm_capabilities,
};

// =============================================================================
// Common Linux definitions
fn void unx_gfx_init(void) {
  waystate.arena = ArenaBuild();
  waystate.events.mutex = os_mutex_alloc();

  waystate.wl_display = wl_display_connect(0);
  Assert(waystate.wl_display);
  waystate.wl_registry = wl_display_get_registry(waystate.wl_display);
  Assert(waystate.wl_registry);
  wl_registry_add_listener(waystate.wl_registry, &wl_registry_listener, 0);

  waystate.xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

  os_gfx_init();

  wl_display_roundtrip(waystate.wl_display);
  waystate.event_dispatcher = os_thread_start(wl_event_dispatcher, 0);
}

fn void unx_gfx_deinit(void) {
  os_thread_cancel(waystate.event_dispatcher);
  wl_compositor_destroy(waystate.wl_compositor);
  wl_registry_destroy(waystate.wl_registry);
  os_gfx_deinit();
  wl_display_disconnect(waystate.wl_display);
}

// =============================================================================
// wayland specific definitions
fn Wl_WindowEvent* wl_alloc_windowevent(void) {
  Wl_WindowEvent *event = 0;
  OS_MutexScope(waystate.events.mutex) {
    event = waystate.events.first;
    if (event) {
      QueuePop(waystate.events.first);
      memzero(event, sizeof(Wl_WindowEvent));
    } else {
      event = New(waystate.arena, Wl_WindowEvent);
    }
  }
  Assert(event);
  return event;
}

fn void wl_event_dispatcher(void *_) {
  for (;;) {
    wl_display_dispatch(waystate.wl_display);
    os_thread_cancelpoint();
  }
}

fn Wl_Window* wl_window_from_surface(struct wl_surface *surface) {
  for (Wl_Window *curr = waystate.window_first; curr; curr = curr->next) {
    if (curr->wl_surface == surface) {
      return curr;
    }
  }
  return 0;
}

fn void wl_registry_event_global(void *data, struct wl_registry *registry,
                                 u32 name, const char *interface, u32 version) {
  /* Info("name: %d, interface: %s, version: %d", name, interface, version); */

  if (cstr_eq(interface, wl_shm_interface.name)) {
    waystate.wl_shm = wl_registry_bind(registry, name, &wl_shm_interface, version);
  } else if (cstr_eq(interface, wl_compositor_interface.name)) {
    waystate.wl_compositor = wl_registry_bind(registry, name, &wl_compositor_interface, version);
  } else if (cstr_eq(interface, wl_seat_interface.name)) {
    waystate.wl_seat = wl_registry_bind(registry, name, &wl_seat_interface, version);
    wl_seat_add_listener(waystate.wl_seat, &wl_seat_listener, 0);
  } else if (cstr_eq(interface, xdg_wm_base_interface.name)) {
    waystate.xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, version);
    xdg_wm_base_add_listener(waystate.xdg_wm_base, &xdg_wm_base_listener_listener, 0);
  }
}

fn void xdg_wm_base_even_ping(void *data, struct xdg_wm_base *xdg_base, u32 serial) {
  xdg_wm_base_pong(xdg_base, serial);
}

fn void wl_seat_event_capabilities(void *data, struct wl_seat *wl_seat,
                                   u32 capabilities) {
  if (capabilities & Wl_Capabilities_Pointer) {
    waystate.wl_pointer = wl_seat_get_pointer(wl_seat);
    Assert(waystate.wl_pointer);
    wl_pointer_add_listener(waystate.wl_pointer, &wl_pointer_listener, 0);
  }
  if (capabilities & Wl_Capabilities_Keyboard) {
    waystate.wl_keyboard = wl_seat_get_keyboard(wl_seat);
    Assert(waystate.wl_keyboard);
    wl_keyboard_add_listener(waystate.wl_keyboard, &wl_keyboard_listener, 0);
  }
  if (capabilities & Wl_Capabilities_Touch) {
    waystate.wl_touch = wl_seat_get_touch(wl_seat);
    Assert(waystate.wl_touch);
  }
}

fn void wl_seat_event_name(void *data, struct wl_seat *wl_seat, const char *name) {}

fn void wl_callback_event_done(void *data, struct wl_callback *wl_callback,
                               u32 callback_data) {
  Wl_Window *window = (Wl_Window*)data;

  Assert(window->wl_callback == wl_callback);
  wl_callback_destroy(wl_callback);
  window->wl_callback = wl_callback = wl_surface_frame(window->wl_surface);
  wl_callback_add_listener(wl_callback, &wl_callback_listener, window);
}


fn void wl_keyboard_event_keymap(void *data, struct wl_keyboard *wl_keyboard,
                                 u32 format, i32 fd, u32 size) {
  char *keymap_shm = (char *)mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
  Assert(keymap_shm != MAP_FAILED);

  switch (format) {
  case Wl_Keyboard_Format_None: {
    // TODO(lb): don't know of a wayland WM that doesn't use XKB_V1
    Assert(false);
  } break;
  case Wl_Keyboard_Format_Xkbv1: {
    waystate.xkb_keymap = xkb_keymap_new_from_string(waystate.xkb_context, keymap_shm,
                                                     XKB_KEYMAP_FORMAT_TEXT_V1,
                                                     XKB_KEYMAP_COMPILE_NO_FLAGS);
    Assert(waystate.xkb_keymap);
    waystate.xkb_state = xkb_state_new(waystate.xkb_keymap);
    Assert(waystate.xkb_state);
    munmap(keymap_shm, size);
  } break;
  }
}

fn void wl_keyboard_event_enter(void *data, struct wl_keyboard *wl_keyboard,
                                u32 serial, struct wl_surface *surface,
                                struct wl_array *keys) {
  waystate.kbd_window_focused = wl_window_from_surface(surface);
}

fn void wl_keyboard_event_leave(void *data, struct wl_keyboard *wl_keyboard,
                                u32 serial, struct wl_surface *surface) {
  waystate.kbd_window_focused = 0;
}

fn void wl_keyboard_event_key(void *data, struct wl_keyboard *wl_keyboard,
                              u32 serial, u32 time, u32 key, u32 state) {
  Assert(waystate.kbd_window_focused);

  u32 scancode = WL_EVDEV_SCANCODE_TO_XKB(key);
  xkb_keysym_t keycode = xkb_state_key_get_one_sym(waystate.xkb_state, scancode);

  Wl_WindowEvent *event = wl_alloc_windowevent();
  event->value.key.scancode = scancode;
  event->value.key.keycode = keycode;
  switch (state) {
  case Wl_Keyboard_Key_State_Released: {
    event->value.type = OS_EventType_KeyUp;
  } break;
  case Wl_Keyboard_Key_State_Pressed:
  case Wl_Keyboard_Key_State_Repeated: {
    event->value.type = OS_EventType_KeyDown;
  } break;
  }

  OS_MutexScope(waystate.kbd_window_focused->events.mutex) {
    QueuePush(waystate.kbd_window_focused->events.first,
              waystate.kbd_window_focused->events.last, event);
    os_cond_signal(waystate.kbd_window_focused->events.condvar);
  }
}

fn void wl_keyboard_event_modifiers(void *data,
                                    struct wl_keyboard *wl_keyboard,
                                    u32 serial, u32 mods_depressed,
                                    u32 mods_latched, u32 mods_locked, u32 group) {

}

fn void wl_keyboard_event_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
                                      int32_t rate, int32_t delay) {

}

fn void wl_pointer_event_enter(void *data, struct wl_pointer *wl_pointer,
                               u32 serial, struct wl_surface *surface,
                               wl_fixed_t surface_x, wl_fixed_t surface_y) {
  waystate.ptr_window_focused = wl_window_from_surface(surface);
}

fn void wl_pointer_event_leave(void *data, struct wl_pointer *wl_pointer,
                               u32 serial, struct wl_surface *surface) {
  waystate.ptr_window_focused = 0;
}

fn void wl_pointer_event_motion(void *data, struct wl_pointer *wl_pointer,
                                u32 time, wl_fixed_t surface_x, wl_fixed_t surface_y) {
  Assert(waystate.ptr_window_focused);

  Wl_WindowEvent *event = wl_alloc_windowevent();
  event->value.type = OS_EventType_PointerMotion;
  os_input_device.pointer.x =
    event->value.pointer.x = ClampBot(WL_F32_FROM_FIXED(surface_x), 0.f);
  os_input_device.pointer.y =
    event->value.pointer.y = ClampBot(WL_F32_FROM_FIXED(surface_y), 0.f);

  OS_MutexScope(waystate.ptr_window_focused->events.mutex) {
    QueuePush(waystate.ptr_window_focused->events.first,
              waystate.ptr_window_focused->events.last, event);
    os_cond_signal(waystate.ptr_window_focused->events.condvar);
  }
}

fn void wl_pointer_event_button(void *data, struct wl_pointer *wl_pointer,
                                u32 serial, u32 time, u32 button, u32 state) {
  Assert(waystate.ptr_window_focused);

  struct BtnPressInfo {
    OS_PtrButton id;
    Vec2f32 position;
    struct BtnPressInfo *next;
    struct BtnPressInfo *prev;
  };

  local usize btninfo_idx = 0;
  local struct BtnPressInfo btninfo_list[Wl_Pointer_Btn_Task - Wl_Pointer_Btn_Left] = {};
  local struct BtnPressInfo *first = 0;
  local struct BtnPressInfo *last = 0;
  local struct BtnPressInfo *freestack = 0;

  Wl_WindowEvent *event = wl_alloc_windowevent();
  Assert(button >= Wl_Pointer_Btn_Left && button <= Wl_Pointer_Btn_Task);
  event->value.btn.id = (u16)(button - Wl_Pointer_Btn_Left + 1);
  memcopy(event->value.btn.position.values, os_input_device.pointer.values, 2 * sizeof(f32));
  switch (state) {
  case Wl_Pointer_Button_State_Released: {
    event->value.type = OS_EventType_BtnUp;

    for (struct BtnPressInfo *curr = first; curr; curr = curr->next) {
      if (curr->id == event->value.btn.id) {
        if (!ApproxEqCustom(curr->position.x, os_input_device.pointer.x, 0.5f) ||
            !ApproxEqCustom(curr->position.y, os_input_device.pointer.y, 0.5f)) {
          event->value.type = OS_EventType_BtnDrag;
          memcopy(event->value.drag.start.values, curr->position.values, 2 * sizeof(f32));
        }
        DLLDelete(first, last, curr);
        curr->prev = curr->next = 0;
        StackPush(freestack, curr);
        break;
      }
    }
  } break;
  case Wl_Pointer_Button_State_Pressed: {
    event->value.type = OS_EventType_BtnDown;

    struct BtnPressInfo *node = freestack;
    if (node) {
      StackPop(freestack);
      node->prev = node->next = 0;
    } else {
      node = &btninfo_list[btninfo_idx++];
      Assert(btninfo_idx <= Wl_Pointer_Btn_Task - Wl_Pointer_Btn_Left);
    }
    node->id = event->value.btn.id;
    memcopy(node->position.values, os_input_device.pointer.values, 2 * sizeof(f32));
    DLLPushBack(first, last, node);
  } break;
  default: Panic("wl_pointer::button unknown state");
  }

  OS_MutexScope(waystate.ptr_window_focused->events.mutex) {
    QueuePush(waystate.ptr_window_focused->events.first,
              waystate.ptr_window_focused->events.last, event);
    os_cond_signal(waystate.ptr_window_focused->events.condvar);
  }
}

fn void wl_pointer_event_axis(void *data, struct wl_pointer *wl_pointer,
                              u32 time, u32 axis, wl_fixed_t value) {
}

fn void wl_pointer_event_frame(void *data, struct wl_pointer *wl_pointer) {
}

fn void wl_pointer_event_axis_source(void *data, struct wl_pointer *wl_pointer,
                                     u32 axis_source) {
}

fn void wl_pointer_event_axis_stop(void *data, struct wl_pointer *wl_pointer,
                                   u32 time, u32 axis) {
}

fn void wl_pointer_event_axis_value120(void *data, struct wl_pointer *wl_pointer,
                                       u32 axis, i32 value120) {
}

fn void wl_pointer_event_axis_relative_direction(void *data,
                                                 struct wl_pointer *wl_pointer,
                                                 u32 axis, u32 direction) {
}


fn void xdg_surface_event_configure(void *data, struct xdg_surface *xdg_surface, u32 serial) {
  atomic_store_explicit(&((Wl_Window*)data)->xdg_surface_acked, true, memory_order_release);
  xdg_surface_ack_configure(xdg_surface, serial);
}

fn void xdg_toplevel_event_configure(void *data, struct xdg_toplevel *xdg_toplevel,
                                     i32 width, i32 height, struct wl_array *states) {
  if (width * height <= 0) { return; }
  Wl_Window *window = (Wl_Window*)data;
  Wl_WindowEvent *event = wl_alloc_windowevent();
  os_gfx_window_resize(window->gfx_context, width, height);
  event->value.type = OS_EventType_Expose;
  window->width = event->value.expose.width = width;
  window->height = event->value.expose.height = height;
  OS_MutexScope(window->events.mutex) {
    QueuePush(window->events.first, window->events.last, event);
    os_cond_signal(window->events.condvar);
  }
}

fn void xdg_toplevel_event_close(void *data, struct xdg_toplevel *xdg_toplevel) {
  Wl_Window *window = (Wl_Window*)data;
  Wl_WindowEvent *event = wl_alloc_windowevent();
  event->value.type = OS_EventType_Kill;
  OS_MutexScope(window->events.mutex) {
    QueuePush(window->events.first, window->events.last, event);
    os_cond_signal(window->events.condvar);
  }
}

fn void xdg_toplevel_event_configure_bounds(void *data, struct xdg_toplevel *xdg_toplevel,
                                            int32_t width, int32_t height) {}
fn void xdg_toplevel_event_wm_capabilities(void *data, struct xdg_toplevel *xdg_toplevel,
                                           struct wl_array *capabilities) {}

// =============================================================================
// OS agnostic definitions
fn OS_Handle os_window_open(String8 name, u32 width, u32 height) {
  Wl_Window *window = waystate.window_freelist;
  if (window) {
    memzero(window, sizeof(Wl_Window));
    StackPop(waystate.window_freelist);
  } else {
    window = New(waystate.arena, Wl_Window);
  }
  DLLPushBack(waystate.window_first, waystate.window_last, window);
  window->events.mutex = os_mutex_alloc();
  window->events.condvar = os_cond_alloc();
  window->width = width;
  window->height = height;

  window->wl_surface = wl_compositor_create_surface(waystate.wl_compositor);
  window->xdg_surface = xdg_wm_base_get_xdg_surface(waystate.xdg_wm_base, window->wl_surface);
  window->xdg_toplevel = xdg_surface_get_toplevel(window->xdg_surface);
  window->wl_callback = wl_surface_frame(window->wl_surface);
  wl_callback_add_listener(window->wl_callback, &wl_callback_listener, window);
  xdg_surface_add_listener(window->xdg_surface, &xdg_surface_listener, window);
  xdg_toplevel_add_listener(window->xdg_toplevel, &xdg_toplevel_listener, window);

  Scratch scratch = ScratchBegin(0, 0);
  xdg_toplevel_set_title(window->xdg_toplevel, cstr_from_str8(scratch.arena, name));
  ScratchEnd(scratch);

  xdg_surface_set_window_geometry(window->xdg_surface, 0, 0, width, height);

  OS_Handle res = {{(u64)window}};
  return res;
}

fn void os_window_show(OS_Handle window_) {
  Wl_Window *window = (Wl_Window*)window_.h[0];
  wl_surface_commit(window->wl_surface);
  wl_display_flush(waystate.wl_display);
  while (!atomic_load_explicit(&window->xdg_surface_acked,
                               memory_order_acquire));
}

fn void os_window_hide(OS_Handle handle) {}

fn void os_window_close(OS_Handle window_) {
  Wl_Window *window = (Wl_Window*)window_.h[0];

  os_gfx_context_window_deinit(window->gfx_context);
  xdg_toplevel_destroy(window->xdg_toplevel);
  xdg_surface_destroy(window->xdg_surface);
  wl_callback_destroy(window->wl_callback);
  wl_surface_destroy(window->wl_surface);
  wl_display_flush(waystate.wl_display);
}

fn OS_Event os_window_get_event(OS_Handle window_) {
  Wl_Window *window = (Wl_Window*)window_.h[0];
  OS_Event res = {0};

  Wl_WindowEvent *event = 0;
  OS_MutexScope(window->events.mutex) {
    event = window->events.first;
    if (window->events.first) {
      memcopy(&res, window->events.first, sizeof(OS_Event));
      QueuePop(window->events.first);
    }
  }
  if (event) {
    OS_MutexScope(waystate.events.mutex) {
      QueuePush(waystate.events.first, waystate.events.last, event);
    }
  }

  return res;
}

fn OS_Event os_window_wait_event(OS_Handle window_) {
  Wl_Window *window = (Wl_Window*)window_.h[0];
  OS_Event res = {0};

  Wl_WindowEvent *event = 0;
  OS_MutexScope(window->events.mutex) {
    for (; !event; event = window->events.first) {
      os_cond_wait(window->events.condvar, window->events.mutex, 0);
    }
    memcopy(&res, window->events.first, sizeof(OS_Event));
    QueuePop(window->events.first);
  }
  OS_MutexScope(waystate.events.mutex) {
    QueuePush(waystate.events.first, waystate.events.last, event);
  }

  return res;
}

fn String8 os_keyname_from_event(Arena *arena, OS_Event event) {
  isize approx_len = 256;
  String8 res = str8(New(arena, u8, approx_len), approx_len);
  isize len = xkb_keysym_get_name(event.key.keycode, (char *)res.str, res.size);
  if (len) {
    arena_pop(arena, approx_len - len);
    res.size = len;
  }
  return res;
}

global W32_GfxState w32_gfxstate = {0};

fn void w32_gfx_init(HINSTANCE instance) {
  timeBeginPeriod(1);
  w32_gfxstate.arena = arena_build();
  w32_gfxstate.instance = instance;
  // TODO(lb): what about non-xinput gamepads like the dualshock
  //           and nintendo controllers?
  w32_xinput_load();

  rhi_init();
}

fn void w32_xinput_load(void) {
  OS_Handle xinput = os_lib_open(Strlit("xinput1_4.dll"));
  if (xinput.h[0]) {
    XInputGetState = (xinput_get_state*)os_lib_lookup(xinput, Strlit("XInputGetState"));
    XInputSetState = (xinput_set_state*)os_lib_lookup(xinput, Strlit("XInputSetState"));
  }
}

fn W32_Window* w32_window_from_handle(HWND target) {
  for (W32_Window *window = w32_gfxstate.first_window;
       window;
       window = window->next) {
    if (window->winhandle == target) { return window; }
  }
  return 0;
}

fn LRESULT CALLBACK w32_message_handler(HWND winhandle, UINT msg_code,
                                        WPARAM wparam, LPARAM lparam) {
  W32_Window *window = w32_window_from_handle(winhandle);
  if (!window || !w32_gfxstate.events.arena) {
    return DefWindowProc(winhandle, msg_code, wparam, lparam);
  }

  switch (msg_code) {
  case WM_INPUTLANGCHANGE: {
    ActivateKeyboardLayout((HKL)lparam, KLF_SETFORPROCESS);
    return 0;
  } break;
  case WM_EXITSIZEMOVE:
  case WM_ENTERSIZEMOVE: {
    return 0;
  } break;
  }

  OS_Event *event = arena_push(w32_gfxstate.events.arena, OS_Event);
  event->window.h[0] = (u64)window;
  switch (msg_code) {
  case WM_QUIT:
  case WM_CLOSE: {
    event->type = OS_EventType_Kill;
  } break;
  case WM_PAINT:
  case WM_SIZE: {
    RECT rect;
    GetClientRect(window->winhandle, &rect);
    event->expose.width = rect.right - rect.left;
    event->expose.height = rect.bottom - rect.top;
    event->type = OS_EventType_Expose;

    PAINTSTRUCT ps = {0};
    BeginPaint(winhandle, &ps);
    update(event->expose.width, event->expose.height);
    EndPaint(winhandle, &ps);
  } break;
  case WM_KEYUP:
  case WM_SYSKEYUP:
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN: {
    event->type = lparam >> 29
                  ? OS_EventType_KeyUp
                  : OS_EventType_KeyDown;
    event->key.keycode = wparam;
    event->key.scancode = (lparam >> 16) & 0xFF;
    if (lparam >> 24 & 0b1) {
      event->key.scancode |= 0xE000;
    } else if (event->key.scancode == 0x45) {
      event->key.scancode = 0xE11D45;
    } else if (event->key.scancode == 0x54) {
      event->key.scancode = 0xE037;
    }
  } break;
  }

  if (event->type) {
    QueuePush(w32_gfxstate.events.list.first,
              w32_gfxstate.events.list.last, event);
    return 0;
  }
  return DefWindowProc(winhandle, msg_code, wparam, lparam);
}

fn void w32_xinput_handle_digitalbtn(OS_BtnState *btn, bool is_down) {
  btn->half_transitions = btn->ended_down ^ is_down;
  btn->ended_down = is_down;
}

#if 0
fn void w32_window_task(void *args_) {
  // TODO(lb): pooling the gamepad state isn't a window specific task
  //           there should be another independent thread that pools the gamepad
  //           periodically for updates. The same goes for the keyboard state but
  //           if there isn't a way to pool the keyboard state then we should use
  //           the key press/release events to update the state ourselves (which i
  //           don't think requires muteces since only the active window can
  //           receive key press/release events)
  for (usize controller_idx = 0;
       controller_idx < MAX_SUPPORTED_GAMEPAD;
       ++controller_idx) {
    XINPUT_STATE controller_state = {0};
    if (XInputGetState(controller_idx, &controller_state) == ERROR_SUCCESS) {
      os_input_device.gamepad[controller_idx].active = 1;
    }
  }

  while (1) {
    for (usize controller_idx = 0;
         controller_idx < MAX_SUPPORTED_GAMEPAD;
         ++controller_idx) {
      XINPUT_STATE controller_state = {0};
      DWORD state = XInputGetState(controller_idx, &controller_state);

      if (state != ERROR_SUCCESS) {
        if (os_input_device.gamepad[controller_idx].active) {
          os_input_device.gamepad[controller_idx].active = 0;
          W32_WindowEvent *event = w32_alloc_windowevent();
          event->value.type = OS_EventType_GamepadDisconnected;
          os_mutex_scope(args->window->eventlist.mutex) {
            QueuePush(args->window->eventlist.first, args->window->eventlist.last, event);
            os_cond_signal(args->window->eventlist.condvar);
          }
        }
        continue;
      }
      if (!os_input_device.gamepad[controller_idx].active) {
        os_input_device.gamepad[controller_idx].active = 1;
        W32_WindowEvent *event = w32_alloc_windowevent();
        event->value.type = OS_EventType_GamepadConnected;
        os_mutex_scope(args->window->eventlist.mutex) {
          QueuePush(args->window->eventlist.first, args->window->eventlist.last, event);
          os_cond_signal(args->window->eventlist.condvar);
        }
      }

      XINPUT_GAMEPAD *pad = &controller_state.Gamepad;
      w32_xinput_handle_digitalbtn(&os_input_device.gamepad[controller_idx].dpad_up, (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0);
      w32_xinput_handle_digitalbtn(&os_input_device.gamepad[controller_idx].dpad_down, (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
      w32_xinput_handle_digitalbtn(&os_input_device.gamepad[controller_idx].dpad_left, (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
      w32_xinput_handle_digitalbtn(&os_input_device.gamepad[controller_idx].dpad_right, (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
      w32_xinput_handle_digitalbtn(&os_input_device.gamepad[controller_idx].start, (pad->wButtons & XINPUT_GAMEPAD_START) != 0);
      w32_xinput_handle_digitalbtn(&os_input_device.gamepad[controller_idx].select, (pad->wButtons & XINPUT_GAMEPAD_BACK) != 0);
      w32_xinput_handle_digitalbtn(&os_input_device.gamepad[controller_idx].l3, (pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
      w32_xinput_handle_digitalbtn(&os_input_device.gamepad[controller_idx].r3, (pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);
      w32_xinput_handle_digitalbtn(&os_input_device.gamepad[controller_idx].l1, (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
      w32_xinput_handle_digitalbtn(&os_input_device.gamepad[controller_idx].r1, (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
      w32_xinput_handle_digitalbtn(&os_input_device.gamepad[controller_idx].a, (pad->wButtons & XINPUT_GAMEPAD_A) != 0);
      w32_xinput_handle_digitalbtn(&os_input_device.gamepad[controller_idx].b, (pad->wButtons & XINPUT_GAMEPAD_B) != 0);
      w32_xinput_handle_digitalbtn(&os_input_device.gamepad[controller_idx].x, (pad->wButtons & XINPUT_GAMEPAD_X) != 0);
      w32_xinput_handle_digitalbtn(&os_input_device.gamepad[controller_idx].y, (pad->wButtons & XINPUT_GAMEPAD_Y) != 0);

      i8 l2_diff = pad->bLeftTrigger - XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
      i8 r2_diff = pad->bRightTrigger - XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
      os_input_device.gamepad[controller_idx].l2 = l2_diff > 0 ? (f32)l2_diff / 255.f : 0.f;
      os_input_device.gamepad[controller_idx].r2 = r2_diff > 0 ? (f32)r2_diff / 255.f : 0.f;

      os_input_device.gamepad[controller_idx].stick_left.x = 0.f;
      os_input_device.gamepad[controller_idx].stick_left.y = 0.f;
      if (pad->sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        os_input_device.gamepad[controller_idx].stick_left.x = (f32)pad->sThumbLX / 32767.f;
      } else if (pad->sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        os_input_device.gamepad[controller_idx].stick_left.x = (f32)pad->sThumbLX / 32768.f;
      }
      if (pad->sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        os_input_device.gamepad[controller_idx].stick_left.y = -(f32)pad->sThumbLY / 32767.f;
      } else if (pad->sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        os_input_device.gamepad[controller_idx].stick_left.y = -(f32)pad->sThumbLY / 32768.f;
      }

      os_input_device.gamepad[controller_idx].stick_right.x = 0.f;
      os_input_device.gamepad[controller_idx].stick_right.y = 0.f;
      if (pad->sThumbRX > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
        os_input_device.gamepad[controller_idx].stick_right.x = (f32)pad->sThumbRX / 32767.f;
      } else if (pad->sThumbRX < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
        os_input_device.gamepad[controller_idx].stick_right.x = (f32)pad->sThumbRX / 32768.f;
      }
      if (pad->sThumbRY > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
        os_input_device.gamepad[controller_idx].stick_right.y = -(f32)pad->sThumbRY / 32767.f;
      } else if (pad->sThumbRY < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
        os_input_device.gamepad[controller_idx].stick_right.y = -(f32)pad->sThumbRY / 32768.f;
      }
    }
  }
}
#endif

fn OS_Handle os_window_open(String8 name, i32 width, i32 height) {
  Assert(width > 0 && height > 0);
  W32_Window *window = w32_gfxstate.freelist_window;
  if (window) {
    memzero(window, sizeof(W32_Window));
    StackPop(w32_gfxstate.freelist_window);
  } else {
    window = arena_push(w32_gfxstate.arena, W32_Window);
  }
  DLLPushBack(w32_gfxstate.first_window, w32_gfxstate.last_window, window);

  {
    Scratch scratch = ScratchBegin(0, 0);
    char *window_name = cstr_from_str8(scratch.arena, name);

    WNDCLASS winclass = {0};
    winclass.lpszClassName = window_name;
    winclass.hInstance = w32_gfxstate.instance;
    winclass.lpfnWndProc = w32_message_handler;
    winclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    RegisterClass(&winclass);
    window->winhandle = CreateWindowA(window_name, window_name,
                                      WS_OVERLAPPEDWINDOW,
                                      0, 0, width, height,
                                      0, 0, w32_gfxstate.instance, 0);
    Assert(window->winhandle);
    ScratchEnd(scratch);
  }

  RECT rect = {0};
  (void)GetWindowRect(window->winhandle, &rect);
  OS_Handle res = {(u64)window};
  return res;
}

fn void os_window_show(OS_Handle window) {
  ShowWindow(((W32_Window *)window.h[0])->winhandle, SW_SHOWDEFAULT);
}

fn void os_window_hide(OS_Handle window) {
  ShowWindow(((W32_Window *)window.h[0])->winhandle, SW_HIDE);
}

fn void os_window_close(OS_Handle window) {
  CloseWindow(((W32_Window *)window.h[0])->winhandle);
}

fn OS_EventList os_get_events(Arena *arena, bool wait) {
  w32_gfxstate.events.arena = arena;
  memzero(&w32_gfxstate.events.list, sizeof w32_gfxstate.events.list);
  for (MSG msg = {0};
       PeekMessage(&msg, 0, 0, 0, PM_REMOVE) ||
       (wait && w32_gfxstate.events.list.count == 0);) {
    DispatchMessage(&msg);
    TranslateMessage(&msg);
  }
  w32_gfxstate.events.arena = 0;
  return w32_gfxstate.events.list;
}

fn String8 os_keyname_from_event(Arena *arena, OS_Event event) {
  u32 lparam = 0, extended = event.key.scancode & 0xFFFF00;
  if (extended == 0xE11D00) {
    lparam = 0x45 << 16;
  } else if (extended) {
    lparam = (0x100 | (event.key.scancode & 0xFF)) << 16;
  } else {
    lparam = event.key.scancode << 16;
    if (event.key.scancode == 0x45) {
      lparam |= 0x1 << 24;
    }
  }

  local isize max_keyname_size = 50;
  String8 res = str8(arena_push_many(arena, u8, 50), max_keyname_size);
  GetKeyNameText(lparam, (char*)res.str, res.size);
  res.size = str8_len((char*)res.str);
  arena_pop(arena, max_keyname_size - res.size);
  return res;
}

global W32_GfxState w32_gfxstate = {0};

fn void w32_gfx_init(HINSTANCE instance) {
  timeBeginPeriod(1);
  w32_gfxstate.arena = ArenaBuild();
  w32_gfxstate.instance = instance;
  // TODO(lb): what about non-xinput gamepads like the dualshock
  //           and nintendo controllers?
  w32_xinput_load();
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
  if (!window) {
    return DefWindowProc(winhandle, msg_code, wparam, lparam);
  }

  W32_WindowEvent *event = 0;
  OS_MutexScope(window->event.mutex) {
    event = window->event.freelist.first;
    if (event) {
      memzero(event, sizeof(W32_WindowEvent));
      QueuePop(window->event.freelist.first);
    } else {
      event = New(w32_gfxstate.arena, W32_WindowEvent);
    }
    QueuePush(window->event.queue.first, window->event.queue.last, event);

    switch (msg_code) {
    case WM_QUIT:
    case WM_CLOSE: {
      event->value.type = OS_EventType_Kill;
    } break;
    case WM_SIZE:
    case WM_PAINT: {
      RECT rect;
      GetClientRect(window->winhandle, &rect);
      event->value.expose.width = rect.right - rect.left;
      event->value.expose.height = rect.bottom - rect.top;
      event->value.type = OS_EventType_Expose;
    } break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
      event->value.type = lparam >> 29 ? OS_EventType_KeyUp
                                       : OS_EventType_KeyDown;
      event->value.key.keycode = wparam;
      event->value.key.scancode = (lparam >> 16) & 0xFF;
      if (lparam >> 24 & 0b1) {
        event->value.key.scancode |= 0xE000;
      } else if (event->value.key.scancode == 0x45) {
        event->value.key.scancode = 0xE11D45;
      } else if (event->value.key.scancode == 0x54) {
        event->value.key.scancode = 0xE037;
      }
    } break;
    case WM_INPUTLANGCHANGE: {
      ActivateKeyboardLayout((HKL)lparam, KLF_SETFORPROCESS);
    } break;
    }
    os_cond_signal(window->event.condvar);
  }

  return DefWindowProc(winhandle, msg_code, wparam, lparam);
}

fn void w32_event_add(W32_Window *window, OS_EventType type) {
  W32_WindowEvent *event = 0;
  OS_MutexScope(window->event.mutex) {
    event = window->event.freelist.first;
    if (event) {
      memzero(event, sizeof(W32_WindowEvent));
      QueuePop(window->event.freelist.first);
    } else {
      event = New(w32_gfxstate.arena, W32_WindowEvent);
    }
    QueuePush(window->event.queue.first, window->event.queue.last, event);
    event->value.type = type;
    os_cond_signal(window->event.condvar);
  }
}

fn void w32_xinput_handle_digitalbtn(OS_BtnState *btn, bool is_down) {
  btn->half_transitions = btn->ended_down ^ is_down;
  btn->ended_down = is_down;
}

fn void w32_window_task(void *window_) {
  W32_Window *window = (W32_Window *)window_;
  WNDCLASS winclass = {0};
  winclass.lpszClassName = (LPCSTR)window->name.str;
  winclass.hInstance = w32_gfxstate.instance;
  winclass.lpfnWndProc = w32_message_handler;
  RegisterClass(&winclass);
  window->winhandle = CreateWindow((LPCSTR)window->name.str, (LPCSTR)window->name.str,
                                 WS_OVERLAPPEDWINDOW, window->x, window->y,
                                 window->width, window->height, 0, 0,
                                 w32_gfxstate.instance, 0);
  Assert(window->winhandle);

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
      os_gamepad[controller_idx].active = 1;
    }
  }

  while (1) {
    for (MSG msg = {0}; PeekMessage(&msg, 0, 0, 0, PM_REMOVE);) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    for (usize controller_idx = 0;
         controller_idx < MAX_SUPPORTED_GAMEPAD;
         ++controller_idx) {
      XINPUT_STATE controller_state = {0};
      DWORD state = XInputGetState(controller_idx, &controller_state);

      if (state != ERROR_SUCCESS) {
        if (os_gamepad[controller_idx].active) {
          os_gamepad[controller_idx].active = 0;
          w32_event_add(window, OS_EventType_GamepadDisconnected);
        }
        continue;
      }
      if (!os_gamepad[controller_idx].active) {
        w32_event_add(window, OS_EventType_GamepadConnected);
        os_gamepad[controller_idx].active = 1;
      }

      XINPUT_GAMEPAD *pad = &controller_state.Gamepad;
      w32_xinput_handle_digitalbtn(&os_gamepad[controller_idx].dpad_up, (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0);
      w32_xinput_handle_digitalbtn(&os_gamepad[controller_idx].dpad_down, (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
      w32_xinput_handle_digitalbtn(&os_gamepad[controller_idx].dpad_left, (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
      w32_xinput_handle_digitalbtn(&os_gamepad[controller_idx].dpad_right, (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
      w32_xinput_handle_digitalbtn(&os_gamepad[controller_idx].start, (pad->wButtons & XINPUT_GAMEPAD_START) != 0);
      w32_xinput_handle_digitalbtn(&os_gamepad[controller_idx].select, (pad->wButtons & XINPUT_GAMEPAD_BACK) != 0);
      w32_xinput_handle_digitalbtn(&os_gamepad[controller_idx].l3, (pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
      w32_xinput_handle_digitalbtn(&os_gamepad[controller_idx].r3, (pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);
      w32_xinput_handle_digitalbtn(&os_gamepad[controller_idx].l1, (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
      w32_xinput_handle_digitalbtn(&os_gamepad[controller_idx].r1, (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
      w32_xinput_handle_digitalbtn(&os_gamepad[controller_idx].a, (pad->wButtons & XINPUT_GAMEPAD_A) != 0);
      w32_xinput_handle_digitalbtn(&os_gamepad[controller_idx].b, (pad->wButtons & XINPUT_GAMEPAD_B) != 0);
      w32_xinput_handle_digitalbtn(&os_gamepad[controller_idx].x, (pad->wButtons & XINPUT_GAMEPAD_X) != 0);
      w32_xinput_handle_digitalbtn(&os_gamepad[controller_idx].y, (pad->wButtons & XINPUT_GAMEPAD_Y) != 0);

      i8 l2_diff = pad->bLeftTrigger - XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
      i8 r2_diff = pad->bRightTrigger - XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
      os_gamepad[controller_idx].l2 = l2_diff > 0 ? (f32)l2_diff / 255.f : 0.f;
      os_gamepad[controller_idx].r2 = r2_diff > 0 ? (f32)r2_diff / 255.f : 0.f;

      os_gamepad[controller_idx].stick_left.x = 0.f;
      os_gamepad[controller_idx].stick_left.y = 0.f;
      if (pad->sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        os_gamepad[controller_idx].stick_left.x = (f32)pad->sThumbLX / 32767.f;
      } else if (pad->sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        os_gamepad[controller_idx].stick_left.x = (f32)pad->sThumbLX / 32768.f;
      }
      if (pad->sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        os_gamepad[controller_idx].stick_left.y = -(f32)pad->sThumbLY / 32767.f;
      } else if (pad->sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        os_gamepad[controller_idx].stick_left.y = -(f32)pad->sThumbLY / 32768.f;
      }

      os_gamepad[controller_idx].stick_right.x = 0.f;
      os_gamepad[controller_idx].stick_right.y = 0.f;
      if (pad->sThumbRX > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
        os_gamepad[controller_idx].stick_right.x = (f32)pad->sThumbRX / 32767.f;
      } else if (pad->sThumbRX < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
        os_gamepad[controller_idx].stick_right.x = (f32)pad->sThumbRX / 32768.f;
      }
      if (pad->sThumbRY > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
        os_gamepad[controller_idx].stick_right.y = -(f32)pad->sThumbRY / 32767.f;
      } else if (pad->sThumbRY < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
        os_gamepad[controller_idx].stick_right.y = -(f32)pad->sThumbRY / 32768.f;
      }
    }
  }
}

fn OS_Window os_window_open(String8 name, u32 x, u32 y, u32 width, u32 height) {
  W32_Window *window = w32_gfxstate.freelist_window;
  if (window) {
    memzero(window, sizeof(W32_Window));
    StackPop(w32_gfxstate.freelist_window);
  } else {
    window = New(w32_gfxstate.arena, W32_Window);
  }
  DLLPushBack(w32_gfxstate.first_window, w32_gfxstate.last_window, window);

  window->name = name;
  window->x = x;
  window->y = y;
  window->width = width;
  window->height = height;
  window->event.mutex = os_mutex_alloc();
  window->event.condvar = os_cond_alloc();
  window->task = os_thread_start(w32_window_task, window);

  while (window->winhandle == 0);
  window->dc = GetDC(window->winhandle);

  RECT rect = {0};
  (void)GetWindowRect(window->winhandle, &rect);
  OS_Window res = {
    .width = rect.right - rect.left,
    .height = rect.bottom - rect.top,
    .handle = {(u64)window}
  };
  return res;
}

fn void os_window_show(OS_Window window) {
  ShowWindow(((W32_Window *)window.handle.h[0])->winhandle, SW_SHOWDEFAULT);
}

fn void os_window_hide(OS_Window window) {
  ShowWindow(((W32_Window *)window.handle.h[0])->winhandle, SW_HIDE);
}

fn void os_window_close(OS_Window window) {
  CloseWindow(((W32_Window *)window.handle.h[0])->winhandle);
}

fn void os_window_swapBuffers(OS_Window window) {
#if USING_OPENGL
  SwapBuffers(((W32_Window *)window.handle.h[0])->dc);
#endif
}

fn OS_Event os_window_get_event(OS_Window window_) {
  W32_Window *window = (W32_Window *)window_.handle.h[0];
  OS_Event res = {0};


  OS_MutexScope(window->event.mutex) {
    W32_WindowEvent *event = window->event.queue.first;
    QueuePop(window->event.queue.first);
    if (event) {
      if (event->value.type) {
        memcopy(&res, &event->value, sizeof(OS_Event));
      }
      QueuePush(window->event.freelist.first, window->event.freelist.last, event);
    }
    QueuePush(window->event.freelist.first, window->event.freelist.last, event);
  }
  return res;
}

fn OS_Event os_window_wait_event(OS_Window window_) {
  W32_Window *window = (W32_Window *)window_.handle.h[0];
  OS_Event res = {0};

  OS_MutexScope(window->event.mutex) {
    W32_WindowEvent *event = window->event.queue.first;
    for (; !event; event = window->event.queue.first) {
      os_cond_wait(window->event.condvar, window->event.mutex, 0);
    }
    QueuePop(window->event.queue.first);
    if (event) {
      if (event->value.type) {
        memcopy(&res, &event->value, sizeof(OS_Event));
      }
      QueuePush(window->event.freelist.first, window->event.freelist.last, event);
    }
    QueuePush(window->event.freelist.first, window->event.freelist.last, event);
  }
  return res;
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
  String8 res = str8(New(arena, u8, 50), max_keyname_size);
  GetKeyNameText(lparam, (char*)res.str, res.size);
  res.size = str8_len((char*)res.str);
  arenaPop(arena, max_keyname_size - res.size);
  return res;
}

// From top to bottom of the window
fn void os_window_render(OS_Window window_, void *mem) {
  W32_Window *window = (W32_Window *)window_.handle.h[0];
  BITMAPINFO bitmap_info = {0};
  bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bitmap_info.bmiHeader.biWidth = window_.width;
  bitmap_info.bmiHeader.biHeight = -(i32)window_.height;
  bitmap_info.bmiHeader.biPlanes = 1;
  bitmap_info.bmiHeader.biBitCount = 32;
  bitmap_info.bmiHeader.biCompression = BI_RGB;
  StretchDIBits(window->dc,
                0, 0, window_.width, window_.height,
                0, 0, window_.width, window_.height,
                mem, &bitmap_info,
                DIB_RGB_COLORS, SRCCOPY);
}

#if USING_OPENGL
fn void opengl_init(OS_Handle handle) {
  W32_Window *window = (W32_Window *)handle.h[0];

  PIXELFORMATDESCRIPTOR pfd = {0};
  PIXELFORMATDESCRIPTOR desired_pfd = {
    .nSize = sizeof(PIXELFORMATDESCRIPTOR),
    .nVersion = 1,
    .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
    .iPixelType = PFD_TYPE_RGBA,
    // NOTE(lb): docs says without alpha but returned pfd has .cColorBits=32
    //           so im not sure if 24 or 32 is the right value
    .cColorBits = 32,
    .cAlphaBits = 8,
    .iLayerType = PFD_MAIN_PLANE,
  };
  i32 pixel_format = ChoosePixelFormat(window->dc, &desired_pfd);
  Assert(pixel_format);
  DescribePixelFormat(window->dc, pixel_format,
                      sizeof(PIXELFORMATDESCRIPTOR), &pfd);
  SetPixelFormat(window->dc, pixel_format, &pfd);

  window->gl_context = wglCreateContext(window->dc);
  Assert(window->gl_context);
  wglMakeCurrent(window->dc, window->gl_context);
}

fn void opengl_deinit(OS_Handle handle) {
  W32_Window *window = (W32_Window *)handle.h[0];
  wglMakeCurrent(0, 0);
  wglDeleteContext(window->gl_context);
}

fn void opengl_make_current(OS_Handle handle) {
  W32_Window *window = (W32_Window *)handle.h[0];
  wglMakeCurrent(window->dc, window->gl_context);
}
#endif

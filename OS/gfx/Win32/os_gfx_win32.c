global W32_GfxState w32_gfxstate = {0};

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

fn bool os_key_is_down(OS_Key key) {
  u8 vkcode = os_vkcode_from_os_key(key);
  SHORT state = GetAsyncKeyState(vkcode);
  return (state & 0x8000) != 0;
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

// =============================================================================
// Platform specific definitions
fn void w32_gfx_init(HINSTANCE instance) {
  timeBeginPeriod(1);
  w32_gfxstate.arena = arena_build();
  w32_gfxstate.instance = instance;
  // TODO(lb): what about non-xinput gamepads like the dualshock
  //           and nintendo controllers?
  w32_xinput_load();

  rhi_init();
}

internal W32_Window* w32_window_from_handle(HWND target) {
  for (W32_Window *window = w32_gfxstate.first_window;
       window;
       window = window->next) {
    if (window->winhandle == target) { return window; }
  }
  return 0;
}

internal void w32_xinput_load(void) {
  OS_Handle xinput = os_lib_open(Strlit("xinput1_4.dll"));
  if (xinput.h[0]) {
    XInputGetState = (xinput_get_state*)os_lib_lookup(xinput, Strlit("XInputGetState"));
    XInputSetState = (xinput_set_state*)os_lib_lookup(xinput, Strlit("XInputSetState"));
  }
}

internal LRESULT CALLBACK w32_message_handler(HWND winhandle, UINT msg_code,
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

internal u8 os_vkcode_from_os_key(OS_Key key) {
  switch (key) {
  case OS_Key_Esc:          return VK_ESCAPE;
  case OS_Key_F1:           return VK_F1;
  case OS_Key_F2:           return VK_F2;
  case OS_Key_F3:           return VK_F3;
  case OS_Key_F4:           return VK_F4;
  case OS_Key_F5:           return VK_F5;
  case OS_Key_F6:           return VK_F6;
  case OS_Key_F7:           return VK_F7;
  case OS_Key_F8:           return VK_F8;
  case OS_Key_F9:           return VK_F9;
  case OS_Key_F10:          return VK_F10;
  case OS_Key_F11:          return VK_F11;
  case OS_Key_F12:          return VK_F12;
  case OS_Key_Tick:         return VK_OEM_3;   // `
  case OS_Key_1:            return '1';
  case OS_Key_2:            return '2';
  case OS_Key_3:            return '3';
  case OS_Key_4:            return '4';
  case OS_Key_5:            return '5';
  case OS_Key_6:            return '6';
  case OS_Key_7:            return '7';
  case OS_Key_8:            return '8';
  case OS_Key_9:            return '9';
  case OS_Key_0:            return '0';
  case OS_Key_Minus:        return VK_OEM_MINUS;
  case OS_Key_Equal:        return VK_OEM_PLUS;
  case OS_Key_Backspace:    return VK_BACK;
  case OS_Key_Tab:          return VK_TAB;
  case OS_Key_Q:            return 'Q';
  case OS_Key_W:            return 'W';
  case OS_Key_E:            return 'E';
  case OS_Key_R:            return 'R';
  case OS_Key_T:            return 'T';
  case OS_Key_Y:            return 'Y';
  case OS_Key_U:            return 'U';
  case OS_Key_I:            return 'I';
  case OS_Key_O:            return 'O';
  case OS_Key_P:            return 'P';
  case OS_Key_LeftBracket:  return VK_OEM_4;   // [
  case OS_Key_RightBracket: return VK_OEM_6;   // ]
  case OS_Key_BackSlash:    return VK_OEM_5;   // '\'
  case OS_Key_CapsLock:     return VK_CAPITAL;
  case OS_Key_A:            return 'A';
  case OS_Key_S:            return 'S';
  case OS_Key_D:            return 'D';
  case OS_Key_F:            return 'F';
  case OS_Key_G:            return 'G';
  case OS_Key_H:            return 'H';
  case OS_Key_J:            return 'J';
  case OS_Key_K:            return 'K';
  case OS_Key_L:            return 'L';
  case OS_Key_Semicolon:    return VK_OEM_1;    // ;
  case OS_Key_Quote:        return VK_OEM_7;    // '
  case OS_Key_Return:       return VK_RETURN;
  case OS_Key_Shift:        return VK_SHIFT;
  case OS_Key_Z:            return 'Z';
  case OS_Key_X:            return 'X';
  case OS_Key_C:            return 'C';
  case OS_Key_V:            return 'V';
  case OS_Key_B:            return 'B';
  case OS_Key_N:            return 'N';
  case OS_Key_M:            return 'M';
  case OS_Key_Comma:        return VK_OEM_COMMA;
  case OS_Key_Period:       return VK_OEM_PERIOD;
  case OS_Key_Slash:        return VK_OEM_2;    // /
  case OS_Key_Ctrl:         return VK_CONTROL;
  case OS_Key_Alt:          return VK_MENU;
  case OS_Key_Space:        return VK_SPACE;
  case OS_Key_ScrollLock:   return VK_SCROLL;
  case OS_Key_Pause:        return VK_PAUSE;
  case OS_Key_Insert:       return VK_INSERT;
  case OS_Key_Home:         return VK_HOME;
  case OS_Key_Delete:       return VK_DELETE;
  case OS_Key_End:          return VK_END;
  case OS_Key_PageUp:       return VK_PRIOR;
  case OS_Key_PageDown:     return VK_NEXT;
  case OS_Key_Up:           return VK_UP;
  case OS_Key_Left:         return VK_LEFT;
  case OS_Key_Down:         return VK_DOWN;
  case OS_Key_Right:        return VK_RIGHT;
  default:                  Unreachable();
  }
  return 0;
}

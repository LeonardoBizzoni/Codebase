global W32_GfxState w32_gfxstate = {0};

fn void w32_gfx_init(HINSTANCE instance) {
  w32_gfxstate.arena = ArenaBuild();
  w32_gfxstate.instance = instance;
}

fn LRESULT CALLBACK w32_message_handler(HWND winhandle, UINT msg_code,
                                        WPARAM wparam, LPARAM lparam) {
  return DefWindowProc(winhandle, msg_code, wparam, lparam);
}

fn OS_Handle os_window_open(String8 name, u32 x, u32 y, u32 width, u32 height) {
  W32_Window *window = w32_gfxstate.freelist_window;
  if (window) {
    memZero(window, sizeof(W32_Window));
    StackPop(w32_gfxstate.freelist_window);
  } else {
    window = New(w32_gfxstate.arena, W32_Window);
  }
  DLLPushBack(w32_gfxstate.first_window, w32_gfxstate.last_window, window);

  String16 classname = UTF16From8(w32_gfxstate.arena, name);
  WNDCLASS winclass = {0};
  winclass.lpszClassName = (PCSTR)classname.str;
  winclass.hInstance = w32_gfxstate.instance;
  winclass.lpfnWndProc = w32_message_handler;

  RegisterClass(&winclass);

  window->winhandle = CreateWindow((PCSTR)classname.str, name.str,
                                   WS_OVERLAPPEDWINDOW, x, y, width, height,
                                   0, 0, w32_gfxstate.instance, 0);
  Assert(window->winhandle);

  OS_Handle res = {{(u64)window}};
  return res;
}

fn void os_window_show(OS_Handle handle) {
  ShowWindow(((W32_Window *)handle.h[0])->winhandle, SW_SHOWDEFAULT);
}

fn void os_window_hide(OS_Handle handle) {
  ShowWindow(((W32_Window *)handle.h[0])->winhandle, SW_HIDE);
}

fn void os_window_close(OS_Handle handle) {
  CloseWindow(((W32_Window *)handle.h[0])->winhandle);
}

fn void os_window_swapBuffers(OS_Handle handle) {

}

fn OS_Event os_window_get_event(OS_Handle handle) {
  W32_Window *window = (W32_Window *)handle.h[0];
  OS_Event res = {0};

  MSG msg;
  if (PeekMessageA(&msg, window->winhandle, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);

    // TODO(lb): not sure if this way of handling messages is correct
    switch (msg.message) {
      case WM_QUIT:
      case WM_CLOSE:
      case WM_DESTROY: {
        res.type = OS_EventType_Kill;
      } break;
      case WM_PAINT:
      case WM_CREATE: {
        res.type = OS_EventType_Expose;
      } break;
    }
  }

  return res;
}

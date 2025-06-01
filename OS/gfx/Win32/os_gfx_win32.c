global W32_GfxState w32_gfxstate = {0};

fn void w32_gfx_init(HINSTANCE instance) {
  w32_gfxstate.arena = ArenaBuild();
  w32_gfxstate.instance = instance;
}

fn LRESULT CALLBACK w32_message_handler(HWND winhandle, UINT msg_code,
                                        WPARAM wparam, LPARAM lparam) {
  switch (msg_code) {
  case WM_CLOSE: {
    PostQuitMessage(0);
    return 0;
  } break;
  }

  return DefWindowProc(winhandle, msg_code, wparam, lparam);
}

fn void w32_window_task(void *args) {
  W32_Window *window = (W32_Window *)args;
  WNDCLASS winclass = {0};
  winclass.lpszClassName = window->name.str;
  winclass.hInstance = w32_gfxstate.instance;
  winclass.lpfnWndProc = w32_message_handler;
  RegisterClass(&winclass);
  window->winhandle = CreateWindow(window->name.str, window->name.str, WS_OVERLAPPEDWINDOW,
                                   window->x, window->y, window->width, window->height, 0, 0,
                                   w32_gfxstate.instance, 0);
  Assert(window->winhandle);

  while (1) {
    MSG msg = {0};
    GetMessage(&msg, 0, 0, 0);
    TranslateMessage(&msg);
    DispatchMessage(&msg);

    W32_WindowEvent *event = 0;
    os_mutex_lock(window->event.mutex);
    DeferLoop(os_mutex_unlock(window->event.mutex)) {
      event = window->event.freelist.first;
      if (event) {
        memZero(event, sizeof(W32_WindowEvent));
        QueuePop(window->event.freelist.first);
      } else {
        event = New(w32_gfxstate.arena, W32_WindowEvent);
      }
      QueuePush(window->event.queue.first, window->event.queue.last, event);

      switch (msg.message) {
      case WM_QUIT: {
        event->value.type = OS_EventType_Kill;
      } break;
      case WM_SIZE:
      case WM_PAINT: {
        event->value.type = OS_EventType_Expose;
      } break;
      }
      os_cond_signal(window->event.condvar);
    }
  }
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

  window->name = name;
  window->x = x;
  window->y = y;
  window->width = width;
  window->height = height;
  window->event.mutex = os_mutex_alloc();
  window->event.condvar = os_cond_alloc();
  window->task = os_thread_start(w32_window_task, window);

  while (window->winhandle == 0);
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

  os_mutex_lock(window->event.mutex);
  DeferLoop(os_mutex_unlock(window->event.mutex)) {
    W32_WindowEvent *event = window->event.queue.first;
    QueuePop(window->event.queue.first);

    if (event) {
      if (event->value.type) {
        memCopy(&res, &event->value, sizeof(OS_Event));
      }
      QueuePush(window->event.freelist.first, window->event.freelist.last, event);
    }
  }
  return res;
}

fn OS_Event os_window_wait_event(OS_Handle handle) {
  W32_Window *window = (W32_Window *)handle.h[0];
  OS_Event res = {0};

  os_mutex_lock(window->event.mutex);
  DeferLoop(os_mutex_unlock(window->event.mutex)) {
    W32_WindowEvent *event = window->event.queue.first;
    for (; !event; event = window->event.queue.first) {
      os_cond_wait(window->event.condvar, window->event.mutex, 0);
    }
    QueuePop(window->event.queue.first);

    if (event) {
      if (event->value.type) {
        memCopy(&res, &event->value, sizeof(OS_Event));
      }
      QueuePush(window->event.freelist.first, window->event.freelist.last, event);
    }
  }
  return res;
}

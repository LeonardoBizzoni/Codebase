#ifndef OS_GFX_WIN32_CORE_H
#define OS_GFX_WIN32_CORE_H

#include <xinput.h>
typedef DWORD WINAPI xinput_get_state(DWORD controller_idx, XINPUT_STATE *state);
DWORD WINAPI XInputGetState_stub(DWORD controller_idx, XINPUT_STATE *state) {return 0;}
global xinput_get_state *_XInputGetState = XInputGetState_stub;
#define XInputGetState _XInputGetState

typedef DWORD WINAPI xinput_set_state(DWORD controller_idx, XINPUT_VIBRATION *vibration);
DWORD WINAPI XInputSetState_stub(DWORD controller_idx, XINPUT_VIBRATION *vibration) {return 0;}
global xinput_set_state *_XInputSetState = XInputSetState_stub;
#define XInputSetState _XInputSetState

typedef struct W32_WindowEvent {
  OS_Event value;
  struct W32_WindowEvent *next;
} W32_WindowEvent;

typedef struct W32_Window {
  OS_Handle task;
  HWND winhandle;
  HBITMAP bitmap_dib;
  HDC mem_dc;
  void *pixels;

  struct {
    OS_Handle mutex, condvar;
    W32_WindowEvent *first;
    W32_WindowEvent *last;
  } eventlist;

  struct W32_Window *next;
  struct W32_Window *prev;
} W32_Window;

typedef struct {
  String8 name;
  i32 x, y, width, height;
  W32_Window *window;
} W32_WindowInitArgs;

typedef struct {
  Arena *arena;
  W32_Window *first_window;
  W32_Window *last_window;
  W32_Window *freelist_window;

  struct {
    OS_Handle mutex;
    W32_WindowEvent *first;
    W32_WindowEvent *last;
  } event_freelist;

  HINSTANCE instance;
} W32_GfxState;

fn void w32_gfx_init(HINSTANCE instance);

fn void w32_window_task(void *args);
fn void w32_xinput_load(void);

fn LRESULT CALLBACK w32_message_handler(HWND winhandle, UINT msg_code,
                                        WPARAM wparam, LPARAM lparam);

fn W32_WindowEvent* w32_alloc_windowevent(void);

#endif

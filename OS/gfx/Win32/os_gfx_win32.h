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

typedef struct W32_Window {
  struct W32_Window *next;
  struct W32_Window *prev;

  OS_Handle task;
  HWND winhandle;
  HBITMAP bitmap_dib;
  HDC mem_dc;
} W32_Window;

typedef struct {
  Arena *arena;
  W32_Window *first_window;
  W32_Window *last_window;
  W32_Window *freelist_window;
  HINSTANCE instance;

  struct {
    Arena *arena;
    OS_EventList list;
  } events;
} W32_GfxState;

fn void w32_gfx_init(HINSTANCE instance);

fn void w32_window_task(void *args);
fn void w32_xinput_load(void);

fn LRESULT CALLBACK w32_message_handler(HWND winhandle, UINT msg_code,
                                        WPARAM wparam, LPARAM lparam);

internal u8 os_vkcode_from_os_key(OS_Key key);

#endif

#if USING_OPENGL
#  include <gl/gl.h>
#endif

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

typedef struct {
  W32_WindowEvent *first;
  W32_WindowEvent *last;
} W32_WindowEventList;

typedef struct W32_Window {
  OS_Handle task;
  HWND winhandle;
  HDC dc;
#if USING_OPENGL
  HGLRC gl_context;
#endif

  struct {
    OS_Handle mutex, condvar;
    W32_WindowEventList queue;
    W32_WindowEventList freelist;
  } event;

  struct W32_Window *next;
  struct W32_Window *prev;
} W32_Window;

typedef struct {
  String8 name;
  u32 x, y, width, height;
  HWND *winhandle;
} W32_WindowArgs;

typedef struct {
  Arena *arena;
  W32_Window *first_window;
  W32_Window *last_window;
  W32_Window *freelist_window;

  HINSTANCE instance;
} W32_GfxState;

fn void w32_gfx_init(HINSTANCE instance);

fn void w32_window_task(void *args);
fn void w32_xinput_load(void);

fn LRESULT CALLBACK w32_message_handler(HWND winhandle, UINT msg_code,
                                        WPARAM wparam, LPARAM lparam);

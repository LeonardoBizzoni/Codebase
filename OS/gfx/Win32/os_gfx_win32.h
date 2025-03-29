typedef struct W32_Window {
  HWND winhandle;

  struct W32_Window *next;
  struct W32_Window *prev;
} W32_Window;

typedef struct {
  Arena *arena;
  W32_Window *first_window;
  W32_Window *last_window;
  W32_Window *freelist_window;

  HINSTANCE instance;
} W32_GfxState;

fn void w32_gfx_init(HINSTANCE instance);
fn LRESULT CALLBACK w32_message_handler(HWND winhandle, UINT msg_code,
                                        WPARAM wparam, LPARAM lparam);

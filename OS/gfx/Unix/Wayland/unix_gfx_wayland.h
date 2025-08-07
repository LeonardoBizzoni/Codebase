#ifndef OS_GFX_UNIX_WL_H
#define OS_GFX_UNIX_WL_H

#include <sys/un.h>
#include <poll.h>

#include <xkbcommon/xkbcommon.h>

#define WL_RINGBUFFER_SIZE 20
#define WL_RINGBUFFER_BYTE_COUNT 256
#define WL_RINGBUFFER_CAPACITY (WL_RINGBUFFER_SIZE * WL_RINGBUFFER_BYTE_COUNT)
#define WL_MAX_FDS 10

#define WL_EVDEV_SCANCODE_TO_XKB(EVDEV_SCANCODE) (EVDEV_SCANCODE + 8)
#define WL_F32_FROM_FIXED(VALUE) ((f32)VALUE / 256.f)

typedef u32 wl_identifier;
typedef i32 wl_display;

typedef u32 wl_capabilities;
enum {
  Wl_Capabilities_Pointer  = 1 << 0,
  Wl_Capabilities_Keyboard = 1 << 1,
  Wl_Capabilities_Touch    = 1 << 2,
};

typedef struct {
  wl_identifier id;
  u16 opcode;
  u16 size;
} Wl_MessageHeader;

typedef struct Wl_WindowEvent {
  OS_Event value;
  struct Wl_WindowEvent *next;
} Wl_WindowEvent;

typedef struct Wl_Window {
  wl_identifier wl_buffer;
  wl_identifier wl_shm_pool;
  wl_identifier wl_surface;
  wl_identifier xdg_surface;
  wl_identifier xdg_toplevel;

  bool xdg_surface_acked;

  SharedMem shm;

  struct {
    OS_Handle mutex;
    OS_Handle condvar;
    Wl_WindowEvent *first;
    Wl_WindowEvent *last;
  } events;

  struct Wl_Window *next;
  struct Wl_Window *prev;
} Wl_Window;

typedef struct {
  Arena *arena;

  struct {
    u8 bytes[WL_RINGBUFFER_SIZE][WL_RINGBUFFER_BYTE_COUNT];
    usize head;
    usize tail;
  } msg_ringbuffer;
  OS_Handle msg_dispatcher;

  u32 curr_id;
  wl_display sockfd;
  wl_identifier registry;
  wl_identifier wl_shm;
  wl_identifier wl_compositor;
  wl_identifier wl_seat;
  wl_identifier wl_pointer;
  wl_identifier wl_keyboard;
  wl_identifier wl_touch;
  wl_identifier xdg_wm_base;

  struct xkb_context *xkb_context;
  struct xkb_keymap *xkb_keymap;
  struct xkb_state *xkb_state;

  struct {
    OS_Handle mutex;
    Wl_WindowEvent *first;
    Wl_WindowEvent *last;
  } events;

  Wl_Window *freelist_window;
  Wl_Window *first_window;
  Wl_Window *last_window;
} Wl_State;

fn wl_identifier wl_allocate_id(void);
fn i32 wl_display_connect(void);
fn wl_identifier wl_display_get_registry(void);

fn void wl_compositor_msg_receiver(void *_);
fn void wl_compositor_msg_dispatcher(void *_);

fn Wl_WindowEvent* wl_alloc_windowevent(void);

#define WL_DISPLAY_OBJECT_ID 1
#define WL_REGISTRY_EVENT_GLOBAL 0
#define WL_REGISTRY_EVENT_GLOBAL_REMOVE 1
#define WL_SHM_POOL_EVENT_FORMAT 0
#define WL_BUFFER_EVENT_RELEASE 0
#define WL_BUFFER_DESTROY_OPCODE 0
#define WL_XDG_WM_BASE_EVENT_PING 0
#define WL_XDG_TOPLEVEL_EVENT_CONFIGURE 0
#define WL_XDG_TOPLEVEL_EVENT_CLOSE 1
#define WL_XDG_SURFACE_EVENT_CONFIGURE 0
#define WL_DISPLAY_GET_REGISTRY_OPCODE 1
#define WL_DISPLAY_EVENT_ERROR 0
#define WL_REGISTRY_BIND_OPCODE 0
#define WL_COMPOSITOR_CREATE_SURFACE_OPCODE 0
#define WL_XDG_WM_BASE_PONG_OPCODE 3
#define WL_XDG_SURFACE_ACK_CONFIGURE_OPCODE 4
#define WL_SHM_CREATE_POOL_OPCODE 0
#define WL_XDG_WM_BASE_GET_XDG_SURFACE_OPCODE 2
#define WL_SHM_POOL_CREATE_BUFFER_OPCODE 0
#define WL_SHM_POOL_DESTROY_OPCODE 1
#define WL_SHM_POOL_RESIZE_OPCODE 2
#define WL_SURFACE_ATTACH_OPCODE 1
#define WL_XDG_SURFACE_GET_TOPLEVEL_OPCODE 1
#define WL_SURFACE_COMMIT_OPCODE 6
#define WL_SEAT_GET_POINTER_OPCODE 0
#define WL_SEAT_GET_KEYBOARD_OPCODE 1
#define WL_SEAT_GET_TOUCH_OPCODE 2
#define WL_SEAT_RELEASE_OPCODE 3
#define WL_SEAT_CAPABILITIES_EVENT 0
#define WL_KEYBOARD_FORMAT_NO_KEYMAP 0
#define WL_KEYBOARD_FORMAT_XKB_V1 1
#define WL_KEYBOARD_KEYMAP_EVENT 0
#define WL_KEYBOARD_ENTER_EVENT 1
#define WL_KEYBOARD_LEAVE_EVENT 2
#define WL_KEYBOARD_KEY_EVENT 3
#define WL_KEYBOARD_KEY_STATE_RELEASED 0
#define WL_KEYBOARD_KEY_STATE_PRESSED 1
#define WL_KEYBOARD_KEY_STATE_REPEATED 2
#define WL_KEYBOARD_MODIFIERS_EVENT 4
#define WL_KEYBOARD_REPEAT_INFO_EVENT 5
#define WL_POINTER_ENTER_EVENT 0
#define WL_POINTER_LEAVE_EVENT 1
#define WL_POINTER_MOTION_EVENT 2
#define WL_POINTER_BUTTON_EVENT 3
#define WL_POINTER_BUTTON_STATE_RELEASED 0
#define WL_POINTER_BUTTON_STATE_PRESSED 1
#define WL_POINTER_AXIS_EVENT 4
#define WL_POINTER_FRAME_EVENT 5
#define WL_POINTER_AXIS_SOURCE_EVENT 6
#define WL_POINTER_AXIS_VALUE120_EVENT 9
#define WL_POINTER_AXIS_RELATIVE_DIRECTION_EVENT 10
#define WL_DISPLAY_ERROR_EVENT 0
#define WL_FORMAT_XRGB8888 1

#define WL_POINTER_BTN_LEFT    0x110
#define WL_POINTER_BTN_RIGHT   0x111
#define WL_POINTER_BTN_MIDDLE  0x112
#define WL_POINTER_BTN_SIDE    0x113
#define WL_POINTER_BTN_EXTRA   0x114
#define WL_POINTER_BTN_FORWARD 0x115
#define WL_POINTER_BTN_BACK    0x116
#define WL_POINTER_BTN_TASK    0x117

#endif

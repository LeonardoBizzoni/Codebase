#ifndef OS_GFX_UNIX_WL_H
#define OS_GFX_UNIX_WL_H

#include <sys/un.h>
#include <poll.h>

// Wl protocol numeric values
#define WL_DISPLAY_OBJECT_ID 1
#define WL_REGISTRY_EVENT_GLOBAL 0
#define WL_REGISTRY_EVENT_GLOBAL_REMOVE 1
#define WL_SHM_POOL_EVENT_FORMAT 0
#define WL_BUFFER_EVENT_RELEASE 0
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
#define WL_SURFACE_ATTACH_OPCODE 1
#define WL_XDG_SURFACE_GET_TOPLEVEL_OPCODE 1
#define WL_SURFACE_COMMIT_OPCODE 6
#define WL_DISPLAY_ERROR_EVENT 0
#define WL_FORMAT_XRGB8888 1
#define WL_RINGBUFFER_SIZE 20
#define WL_RINGBUFFER_BYTE_COUNT 256
#define WL_RINGBUFFER_CAPACITY (WL_RINGBUFFER_SIZE * WL_RINGBUFFER_BYTE_COUNT)

typedef struct {
  u32 id;
  u16 opcode;
  u16 size;
} Wl_MessageHeader;

typedef struct Wl_WindowEvent {
  OS_Event value;
  struct Wl_WindowEvent *next;
} Wl_WindowEvent;

typedef struct {
  Wl_WindowEvent *first;
  Wl_WindowEvent *last;
} Wl_WindowEventList;

typedef struct Wl_Window {
  u32 wl_buffer;
  u32 wl_shm_pool;
  u32 wl_surface;
  u32 xdg_surface;
  u32 xdg_toplevel;

  bool xdg_surface_acked;

  SharedMem shm;

  struct {
    OS_Handle lock;
    OS_Handle condvar;
    Wl_WindowEventList list;
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
  u32 display;
  u32 registry;
  u32 wl_shm;
  u32 wl_compositor;
  u32 xdg_wm_base;

  struct {
    OS_Handle lock;
    Wl_WindowEventList freelist;
  } events;

  Wl_Window *freelist_window;
  Wl_Window *first_window;
  Wl_Window *last_window;
} Wl_State;

fn u32 wl_allocate_id(void);
fn i32 wl_display_connect(void);
fn u32 wl_display_get_registry(void);

fn void wl_compositor_msg_receiver(void *_);
fn void wl_compositor_msg_dispatcher(void *_);

fn Wl_WindowEvent* wl_alloc_windowevent(void);

#endif
